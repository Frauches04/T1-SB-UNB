#include "Assembler.h"

#include <cctype>
#include <fstream>

Assembler::Assembler(GerenciadorErros& e)
    : erros(e), diretivas(e) {
}

bool Assembler::montar(const std::string& caminhoEntrada,
                       const std::string& caminhoSaida) {
    // Zera o estado para permitir reuso do mesmo objeto.
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

    // Aborta se a 1a passagem ja encontrou erro.
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

    // Escreve o .OBJ e, se houver metadados, o .meta.
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
        // IF anterior pediu para pular esta linha.
        if (pularProxima) {
            pularProxima = false;
            continue;
        }

        // Normaliza o formato "BEGIN: X".
        ajustarBegin(linha);
        std::string op = normalizar(linha.operacao);

        // Troca de secao; nao gera codigo.
        if (op == "SECAO" || op == "SECTION") {
            if (!linha.operandos.empty()) {
                std::string s = normalizar(linha.operandos[0]);
                if (s == "TEXT" || s == "TEXTO") {
                    temSecaoTexto = true;
                }
            }
            continue;
        }

        // BEGIN: guarda o nome do ponto de entrada; a posicao e resolvida no fim.
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

        // END: fim do modulo, sem efeito.
        if (op == "END") {
            continue;
        }

        // PUBLIC: registra o nome; o endereco e resolvido no fim.
        if (op == "PUBLIC") {
            if (linha.operandos.size() != 1) {
                erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                                "PUBLIC espera exatamente um operando");
            } else {
                publicosNomes.push_back(linha.operandos[0]);
            }
            continue;
        }

        // EXTERN: insere o simbolo como externo para a 2a passagem.
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

        // Diretivas CONST/SPACE/EQU/IF: delegadas ao ProcessadorDiretivas.
        if (diretivas.ehDiretiva(linha.operacao)) {
            ResultadoDiretiva r = diretivas.processar(linha, ts, true);

            // CONST/SPACE: o endereco do simbolo e o offset dentro de DADOS.
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

        // Instrucoes da maquina.
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

    // Se nenhuma linha marcou SECAO TEXT, o programa nao tem codigo.
    if (!temSecaoTexto) {
        erros.registrar(0, TipoErro::SEMANTICO, "secao de texto ausente");
    }

    // Com todos os labels declarados, resolve o entry point e checa os PUBLICs.
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

        // Mesma normalizacao da 1a passagem.
        ajustarBegin(linha);
        std::string op = normalizar(linha.operacao);

        // Diretivas que nao geram codigo na 2a passagem.
        if (op == "SECAO" || op == "SECTION") {
            continue;
        }
        if (op == "BEGIN" || op == "END" || op == "PUBLIC" || op == "EXTERN") {
            continue;
        }

        // EQU nao gera codigo; IF precisa reavaliar para decidir o pulo.
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

        // Instrucoes: emite opcode e resolve operandos.
        DadosInstrucao dados;
        if (tabela.buscar(linha.operacao, dados)) {
            bufferCodigo.push_back(std::to_string(dados.opcode));
            bitmapCodigo.push_back(0);

            for (const std::string& opnd : linha.operandos) {
                int valor = 0;
                if (converterInteiro(opnd, valor)) {
                    // Operando numerico literal.
                    bufferCodigo.push_back(std::to_string(valor));
                    bitmapCodigo.push_back(1);
                    continue;
                }

                EntradaTS e;
                if (!ts.buscar(opnd, e)) {
                    erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                                    "simbolo '" + opnd + "' indefinido");
                    bufferCodigo.push_back("0");
                    bitmapCodigo.push_back(1);
                    continue;
                }

                // Simbolo externo: emite placeholder e registra o uso.
                if (e.eExterno) {
                    usosExternos[static_cast<int>(bufferCodigo.size())] = opnd;
                    bufferCodigo.push_back("0");
                    bitmapCodigo.push_back(1);
                    continue;
                }

                // Rotulos de dados: soma tamanhoCodigo ao offset de DADOS.
                int endereco = e.valor;
                if (e.tipo == TipoSimbolo::CONSTANTE) {
                    endereco += tamanhoCodigo;
                }
                bufferCodigo.push_back(std::to_string(endereco));
                bitmapCodigo.push_back(1);
            }
            continue;
        }

        // CONST/SPACE: alimentam o buffer de dados.
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

    // Codigo primeiro, dados depois, tudo em uma linha.
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
    // Sem metadados: nao cria .meta.
    if (!temEntry && publicosNomes.empty() && usosExternos.empty()
        && bitmapCodigo.empty()) {
        return true;
    }

    // Deriva o caminho do .meta a partir do .asm.
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

    // ENTRY opcional: so escreve se houve BEGIN.
    if (temEntry) {
        arquivo << "ENTRY " << entryPos << "\n";
    }

    // D <nome> <enderecoLocal>: simbolo publico definido no modulo.
    for (const std::string& nome : publicosNomes) {
        EntradaTS e;
        if (!ts.buscar(nome, e)) {
            continue;
        }
        // Dado tem endereco local = offset de DADOS + tamanhoCodigo; rotulo ja tem o endereco.
        int local = e.valor;
        if (e.tipo == TipoSimbolo::CONSTANTE) {
            local += tamanhoCodigo;
        }
        arquivo << "D " << normalizar(nome) << " " << local << "\n";
    }

    // U <nome> <posicao>: simbolo externo usado na posicao indicada.
    for (const auto& par : usosExternos) {
        arquivo << "U " << normalizar(par.second) << " " << par.first << "\n";
    }

    // R: bitmap de relocacao (0 = opcode, 1 = operando/endereco).
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
        // Cast para unsigned char evita UB no toupper.
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
        // Se sobrou caractere, nao era inteiro puro.
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
    // "BEGIN: X" vira (op=BEGIN, operandos=[X], rotulo="").
    if (normalizar(linha.rotulo) == "BEGIN" && !linha.operacao.empty()) {
        linha.operandos.insert(linha.operandos.begin(), linha.operacao);
        linha.operacao = "BEGIN";
        linha.rotulo.clear();
    }
}