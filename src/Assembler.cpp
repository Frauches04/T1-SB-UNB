#include "Assembler.h"

#include <cctype>
#include <fstream>

Assembler::Assembler(GerenciadorErros& e)
    : erros(e), diretivas(e) {
}

bool Assembler::montar(const std::string& caminhoEntrada,
                       const std::string& caminhoSaida) {
    // Reaproveitamento defensivo: se o mesmo objeto for usado pra
    // montar dois arquivos, nao queremos estado da execucao anterior.
    // Os membros novos (publicosNomes, usosExternos, bitmapCodigo,
    // entryLabelName, entryPos, temEntry) tambem precisam ser
    // limpos aqui, senao o segundo modulo herdaria metadados do
    // primeiro.
    ts.limpar();
    bufferCodigo.clear();
    bufferDados.clear();
    publicosNomes.clear();
    usosExternos.clear();
    bitmapCodigo.clear();
    tamanhoCodigo = 0;
    tamanhoDados = 0;
    entryLabelName.clear();
    entryPos = 0;
    temEntry = false;

    LeitorFonte leitor(erros);
    if (!leitor.abrir(caminhoEntrada)) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel abrir o arquivo '" + caminhoEntrada + "'");
        return false;
    }

    // Se a primeira passagem ja encontrou erro, nao adianta tentar a
    // segunda: ela assumiria TS incompleta e geraria .OBJ corrompido.
    primeiraPassagem(leitor);
    if (erros.temErros()) {
        leitor.fechar();
        return false;
    }

    leitor.reiniciar();
    segundaPassagem(leitor);
    leitor.fechar();

    if (erros.temErros()) {
        return false;
    }

    // Escreve o .OBJ (que ja era gerado antes) e, se houver
    // metadados, o .meta ao lado. O .meta e o que o ligador le.
    if (!escreverObjeto(caminhoSaida)) {
        return false;
    }
    return escreverMeta(caminhoEntrada);
}

void Assembler::primeiraPassagem(LeitorFonte& leitor) {
    LinhaTokenizada linha;
    bool pularProxima = false;
    bool temSecaoTexto = false;

    while (leitor.proximaLinha(linha)) {
        // O IF da linha anterior pediu pra pular esta. Pular aqui
        // tambem mantem o contador coerente com o que a segunda
        // passagem vai de fato emitir.
        if (pularProxima) {
            pularProxima = false;
            continue;
        }

        // Antes de olhar a operacao, ajusta o formato de "BEGIN: X".
        // O LeitorFonte corta no ':' e entrega (rotulo=BEGIN,
        // operacao=X); aqui viramos (op=BEGIN, operandos=[X]) pra
        // que o resto do codigo trate BEGIN como qualquer diretiva.
        ajustarBegin(linha);
        std::string op = normalizar(linha.operacao);

        // SECAO (ou SECTION) e tratada aqui, nao pelo
        // ProcessadorDiretivas, porque o papel dela e trocar o
        // estado de secao, nao gerar nada. A troca so vale pra
        // linha seguinte: a propria linha SECAO carrega a secao
        // anterior em linha.secao.
        if (op == "SECAO" || op == "SECTION") {
            if (!linha.operandos.empty()) {
                std::string s = normalizar(linha.operandos[0]);
                if (s == "TEXT" || s == "TEXTO") {
                    temSecaoTexto = true;
                }
            }
            continue;
        }

        // BEGIN: registra o nome do entry point como referencia
        // futura. A resolucao (achar o endereco) vem no fim da
        // passagem, quando o label ja tiver sido declarado por
        // "MAIN:" ou similar. Se a gente tentasse inserir MAIN na
        // TS agora, esbarraria na declaracao "MAIN:" que vem
        // depois, e o programa seria recusado com "simbolo ja
        // declarado". Por isso e referencia, nao insercao.
        if (op == "BEGIN") {
            if (linha.operandos.size() != 1) {
                erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                                "BEGIN espera exatamente um operando");
            } else {
                entryLabelName = linha.operandos[0];
                temEntry = true;
            }
            continue;
        }

        // END marca o fim do modulo. Nada a fazer alem de ignorar.
        if (op == "END") {
            continue;
        }

        // PUBLIC: so registra o nome; a resolucao do endereco vem
        // no fim da primeira passagem, quando todos os labels ja
        // estao na TS. Nao inserimos nada na TS aqui: PUBLIC so
        // exporta um simbolo que ja foi (ou vai ser) declarado no
        // modulo. Se o nome nao existir no fim, vira erro.
        if (op == "PUBLIC") {
            if (linha.operandos.size() != 1) {
                erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                                "PUBLIC espera exatamente um operando");
            } else {
                publicosNomes.push_back(linha.operandos[0]);
            }
            continue;
        }

        // EXTERN: o nome (que veio no rotulo, do lado esquerdo dos
        // dois-pontos) e importado de outro modulo. Inserimos na TS
        // com eExterno=true pra a segunda passagem saber que deve
        // usar placeholder em vez de tentar resolver.
        if (op == "EXTERN") {
            if (linha.rotulo.empty()) {
                erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                                "EXTERN exige um rotulo do lado esquerdo");
            } else {
                EntradaTS e;
                e.nome = linha.rotulo;
                e.valor = 0;
                e.tipo = TipoSimbolo::ROTULO;
                e.eExterno = true;
                ts.inserir(e);
            }
            continue;
        }

        // Diretivas conhecidas (CONST, SPACE, EQU, IF). O
        // ProcessadorDiretivas cuida da validacao sintatica e do
        // contexto (EQU so antes de secao, IF so em TEXT) e, no
        // caso do EQU, ja insere o simbolo na TS.
        if (diretivas.ehDiretiva(linha.operacao)) {
            ResultadoDiretiva r = diretivas.processar(linha, ts, true);

            // CONST e SPACE nao sao inseridos pelo
            // ProcessadorDiretivas porque o valor do simbolo e o
            // endereco na secao de dados, e so o Assembler conhece
            // o contador. Usamos o offset dentro de DADOS; a
            // segunda passagem soma tamanhoCodigo.
            if (!linha.rotulo.empty() && (op == "CONST" || op == "SPACE")) {
                EntradaTS e;
                e.nome = linha.rotulo;
                e.valor = tamanhoDados;
                e.tipo = TipoSimbolo::CONSTANTE;
                if (!ts.inserir(e)) {
                    erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                                    "simbolo '" + linha.rotulo + "' ja declarado");
                }
            }

            tamanhoDados += r.palavrasAlocadas;
            if (r.pularProximaLinha) {
                pularProxima = true;
            }
            continue;
        }

        // Instrucoes da maquina hipotetica.
        DadosInstrucao dados;
        if (tabela.buscar(linha.operacao, dados)) {
            if (linha.secao != TipoSecao::TEXTO) {
                erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                                "instrucao '" + linha.operacao +
                                "' fora da secao de texto");
            }

            if (!linha.rotulo.empty()) {
                EntradaTS e;
                e.nome = linha.rotulo;
                e.valor = tamanhoCodigo;
                e.tipo = TipoSimbolo::ROTULO;
                if (!ts.inserir(e)) {
                    erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                                    "simbolo '" + linha.rotulo + "' ja declarado");
                }
            }

            if (linha.operandos.size() !=
                static_cast<std::size_t>(dados.qtdOperandos)) {
                erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                                "instrucao '" + linha.operacao + "' espera " +
                                std::to_string(dados.qtdOperandos) +
                                " operando(s), recebeu " +
                                std::to_string(linha.operandos.size()));
            }

            tamanhoCodigo += dados.tamanhoPalavras;
            continue;
        }

        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "instrucao ou diretiva desconhecida: '" +
                        linha.operacao + "'");
    }

    // Se nenhuma linha foi de SECAO TEXT, o programa nao tem codigo
    // e nao pode ser executado. O erro vai pra linha 0 porque nao
    // ha linha especifica pra apontar.
    if (!temSecaoTexto) {
        erros.registrar(0, TipoErro::SEMANTICO, "secao de texto ausente");
    }

    // Agora que todos os labels foram declarados, resolve o entry
    // point e checa se cada PUBLIC existe no modulo.
    if (temEntry) {
        EntradaTS e;
        if (!ts.buscar(entryLabelName, e)) {
            erros.registrar(0, TipoErro::SEMANTICO,
                            "label de entrada '" + entryLabelName + "' nao definido");
        } else {
            entryPos = e.valor;
        }
    }

    for (const std::string& nome : publicosNomes) {
        EntradaTS e;
        if (!ts.buscar(nome, e) || e.eExterno) {
            erros.registrar(0, TipoErro::SEMANTICO,
                            "PUBLIC '" + nome + "' nao definido no modulo");
        }
    }
}

void Assembler::segundaPassagem(LeitorFonte& leitor) {
    LinhaTokenizada linha;
    bool pularProxima = false;

    while (leitor.proximaLinha(linha)) {
        if (pularProxima) {
            pularProxima = false;
            continue;
        }

        // Mesma reorganizacao da primeira passagem: sem ela, "BEGIN:
        // X" chegaria como op="X" e cairia no ramo de instrucao
        // desconhecida.
        ajustarBegin(linha);
        std::string op = normalizar(linha.operacao);

        // SECAO nao gera codigo. BEGIN/END/PUBLIC/EXTERN tambem nao:
        // so existem pra alimentar o .meta, que ja foi coletado na
        // primeira passagem. Pular aqui evita tratar esses nomes
        // como instrucao.
        if (op == "SECAO" || op == "SECTION") {
            continue;
        }
        if (op == "BEGIN" || op == "END" || op == "PUBLIC" || op == "EXTERN") {
            continue;
        }

        // EQU nao gera codigo (o valor ja foi pra TS na primeira
        // passagem). IF ainda pode pedir pra pular a proxima linha,
        // entao precisa ser reprocessado; o estado do simbolo ja
        // esta na TS.
        if (op == "EQU") {
            continue;
        }
        if (op == "IF") {
            ResultadoDiretiva r = diretivas.processar(linha, ts, false);
            if (r.pularProximaLinha) {
                pularProxima = true;
            }
            continue;
        }

        // Instrucoes.
        DadosInstrucao dados;
        if (tabela.buscar(linha.operacao, dados)) {
            bufferCodigo.push_back(std::to_string(dados.opcode));
            // Opcode nao e endereco: o ligador nunca reloca essa
            // posicao. Bit 0 no bitmap.
            bitmapCodigo.push_back(0);

            for (const std::string& opnd : linha.operandos) {
                int valor = 0;
                if (converterInteiro(opnd, valor)) {
                    // Operando literal (ex: LOAD 5). O valor nao
                    // depende de offset de modulo; so entra no
                    // bitmap pra o ligador saber que existe ali uma
                    // palavra de operando (consistencia do formato).
                    bufferCodigo.push_back(std::to_string(valor));
                    bitmapCodigo.push_back(1);
                    continue;
                }

                EntradaTS e;
                if (!ts.buscar(opnd, e)) {
                    erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                                    "simbolo '" + opnd + "' indefinido");
                    // Empurra um 0 so pra manter a contagem de
                    // palavras coerente. Como ha erro, o .OBJ nem
                    // sera gerado.
                    bufferCodigo.push_back("0");
                    bitmapCodigo.push_back(1);
                    continue;
                }

                // Simbolo externo: o montador nao sabe o endereco
                // real (so o ligador sabe). Emite 0 como placeholder
                // e anota a posicao pra o ligador substituir pelo
                // endereco global do simbolo publico correspondente.
                if (e.eExterno) {
                    usosExternos[static_cast<int>(bufferCodigo.size())] = opnd;
                    bufferCodigo.push_back("0");
                    bitmapCodigo.push_back(1);
                    continue;
                }

                // Rotulos que apontam pra dados estao guardados na
                // TS como offset dentro da secao DADOS. Como os
                // dados vao pro fim do .OBJ, o endereco real e
                // tamanhoCodigo + offset.
                int endereco = e.valor;
                if (e.tipo == TipoSimbolo::CONSTANTE) {
                    endereco += tamanhoCodigo;
                }
                bufferCodigo.push_back(std::to_string(endereco));
                bitmapCodigo.push_back(1);
            }
            continue;
        }

        // CONST e SPACE alimentam o buffer de dados na ordem em que
        // aparecem no fonte. Cada palavra e uma entrada propria;
        // SPACE com N vem como N entradas "0". Essas palavras nao
        // entram no bitmapCodigo porque o bitmap e so do codigo; o
        // ligador trata dados como bloco opaco.
        if (op == "CONST" || op == "SPACE") {
            ResultadoDiretiva r = diretivas.processar(linha, ts, false);
            for (const std::string& v : r.valoresParaObjeto) {
                bufferDados.push_back(v);
            }
            continue;
        }
    }
}

bool Assembler::escreverObjeto(const std::string& caminhoSaida) {
    std::ofstream saida(caminhoSaida);
    if (!saida.is_open()) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel abrir o arquivo de saida '" +
                        caminhoSaida + "'");
        return false;
    }

    // Tudo numa linha so, separado por espaco. Codigo primeiro,
    // dados depois, independente da ordem das secoes no fonte.
    bool primeiro = true;
    auto escrever = [&](const std::string& v) {
        if (!primeiro) {
            saida << ' ';
        }
        saida << v;
        primeiro = false;
    };

    for (const std::string& s : bufferCodigo) {
        escrever(s);
    }
    for (const std::string& s : bufferDados) {
        escrever(s);
    }
    saida << '\n';
    return true;
}

bool Assembler::escreverMeta(const std::string& caminhoEntrada) {
    // Modulo sem nenhum metadado: nao gera .meta. O ligador trata
    // como modulo sem relocacao nem simbolos. Util pra programa de
    // um modulo so, com so instrucoes e sem PUBLIC/EXTERN/BEGIN.
    if (!temEntry && publicosNomes.empty() && usosExternos.empty()
        && bitmapCodigo.empty()) {
        return true;
    }

    // Deriva o caminho do .meta a partir do .asm (troca extensao).
    std::string caminhoMeta;
    std::size_t pos = caminhoEntrada.find_last_of('.');
    if (pos == std::string::npos) {
        caminhoMeta = caminhoEntrada + ".meta";
    } else {
        caminhoMeta = caminhoEntrada.substr(0, pos) + ".meta";
    }

    std::ofstream arquivo(caminhoMeta);
    if (!arquivo.is_open()) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel criar '" + caminhoMeta + "'");
        return false;
    }

    // ENTRY e opcional. Se nao houver BEGIN, o ligador usa 0 por
    // convencao (posicao do primeiro modulo).
    if (temEntry) {
        arquivo << "ENTRY " << entryPos << "\n";
    }

    // Cada D <nome> <enderecoLocal> diz que o simbolo nome esta
    // definido no endereco local indicado. O ligador soma o offset
    // do modulo em cima desse valor pra obter o endereco global.
    for (const std::string& nome : publicosNomes) {
        EntradaTS e;
        if (!ts.buscar(nome, e)) {
            continue;
        }
        // CONSTANTE guarda o offset dentro da secao DADOS; no .OBJ,
        // o dado fica depois do codigo, entao o endereco local e
        // offset + tamanhoCodigo. ROTULO ja guarda o endereco local
        // direto (codigo).
        int local = e.valor;
        if (e.tipo == TipoSimbolo::CONSTANTE) {
            local += tamanhoCodigo;
        }
        arquivo << "D " << normalizar(nome) << " " << local << "\n";
    }

    // Cada U <nome> <posicao> diz que o simbolo nome e importado de
    // outro modulo e aparece na posicao indicada do .OBJ. O ligador
    // usa isso junto com o bitmap pra substituir o placeholder 0
    // pelo endereco global do PUBLIC correspondente.
    for (const auto& par : usosExternos) {
        arquivo << "U " << normalizar(par.second) << " " << par.first << "\n";
    }

    // Bitmap de relocacao: um bit por palavra de codigo. 0 = opcode,
    // 1 = operando/endereco. O ligador percorre o bitmap e decide o
    // que ajustar.
    arquivo << "R";
    for (int bit : bitmapCodigo) {
        arquivo << " " << bit;
    }
    arquivo << "\n";
    return true;
}

std::string Assembler::normalizar(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        // O cast pra unsigned char evita comportamento indefinido do
        // toupper quando o char tem o bit de sinal ligado.
        r.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(c))));
    }
    return r;
}

bool Assembler::converterInteiro(const std::string& s, int& saida) {
    if (s.empty()) {
        return false;
    }
    try {
        std::size_t consumido = 0;
        int valor = std::stoi(s, &consumido);
        // stoi para no primeiro caractere nao numerico. Se sobrou
        // algo, nao era um inteiro puro (ex.: "3.5", "2x").
        if (consumido != s.size()) {
            return false;
        }
        saida = valor;
        return true;
    } catch (...) {
        return false;
    }
}

void Assembler::ajustarBegin(LinhaTokenizada& linha) {
    // "BEGIN: MAIN" chega do LeitorFonte como rotulo="BEGIN",
    // operacao="MAIN". Reorganiza pra operacao="BEGIN",
    // operandos=["MAIN"], rotulo="". Assim o resto do codigo trata
    // BEGIN como qualquer outra diretiva. Chamada nas duas
    // passagens, logo no inicio do laco.
    if (normalizar(linha.rotulo) == "BEGIN" && !linha.operacao.empty()) {
        linha.operandos.insert(linha.operandos.begin(), linha.operacao);
        linha.operacao = "BEGIN";
        linha.rotulo.clear();
    }
}