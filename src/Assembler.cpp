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
    ts.limpar();
    bufferCodigo.clear();
    bufferDados.clear();
    tamanhoCodigo = 0;
    tamanhoDados = 0;

    LeitorFonte leitor(erros);
    if (!leitor.abrir(caminhoEntrada)) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel abrir o arquivo '" + caminhoEntrada + "'");
        return false;
    }

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

    return escreverObjeto(caminhoSaida);
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

        std::string op = normalizar(linha.operacao);

        // SECAO (ou SECTION) e tratada aqui, nao pelo ProcessadorDiretivas,
        // porque o papel dela e trocar o estado de secao, nao gerar
        // nada. A troca so vale pra linha seguinte: a propria linha
        // SECAO carrega a secao anterior em linha.secao.
        if (op == "SECAO" || op == "SECTION") {
            if (!linha.operandos.empty()) {
                std::string s = normalizar(linha.operandos[0]);
                if (s == "TEXT" || s == "TEXTO") {
                    temSecaoTexto = true;
                }
            }
            continue;
        }

        // Diretivas conhecidas (CONST, SPACE, EQU, IF). O
        // ProcessadorDiretivas cuida da validacao sintatica e do
        // contexto (EQU so antes de secao, IF so em TEXT) e, no caso
        // do EQU, ja insere o simbolo na TS.
        if (diretivas.ehDiretiva(linha.operacao)) {
            ResultadoDiretiva r = diretivas.processar(linha, ts, true);

            // CONST e SPACE nao sao inseridos pelo ProcessadorDiretivas
            // porque o valor do simbolo e o endereco na secao de dados,
            // e so o Assembler conhece o contador. Usamos o offset
            // dentro de DADOS; a segunda passagem soma tamanhoCodigo.
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
    // e nao pode ser executado. O erro vai pra linha 0 porque nao ha
    // linha especifica pra apontar.
    if (!temSecaoTexto) {
        erros.registrar(0, TipoErro::SEMANTICO, "secao de texto ausente");
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

        std::string op = normalizar(linha.operacao);

        if (op == "SECAO" || op == "SECTION") {
            continue;
        }

        // EQU e IF nao geram codigo. IF ainda pode pedir pra pular
        // a proxima linha, entao precisa ser reprocessado na segunda
        // passagem (o estado do simbolo ja esta na TS).
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

            for (const std::string& opnd : linha.operandos) {
                int valor = 0;
                if (converterInteiro(opnd, valor)) {
                    bufferCodigo.push_back(std::to_string(valor));
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
                    continue;
                }

                // Rotulos que apontam pra dados estao guardados na TS
                // como offset dentro da secao DADOS. Como os dados
                // vao pro fim do .OBJ, o endereco real e
                // tamanhoCodigo + offset.
                int endereco = e.valor;
                if (e.tipo == TipoSimbolo::CONSTANTE) {
                    endereco += tamanhoCodigo;
                }
                bufferCodigo.push_back(std::to_string(endereco));
            }
            continue;
        }

        // CONST e SPACE alimentam o buffer de dados na ordem em que
        // aparecem no fonte. Cada palavra e uma entrada propria; SPACE
        // com N vem como N entradas "0".
        if (op == "CONST" || op == "SPACE") {
            ResultadoDiretiva r = diretivas.processar(linha, ts, false);
            for (const std::string& v : r.valoresParaObjeto) {
                bufferDados.push_back(v);
            }
            continue;
        }

        // Operacao desconhecida ja foi reportada na primeira
        // passagem; se chegamos aqui, e sinal de bug entre as
        // passagens, mas nao vamos duplicar o erro.
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

std::string Assembler::normalizar(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
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
        if (consumido != s.size()) {
            return false;
        }
        saida = valor;
        return true;
    } catch (...) {
        return false;
    }
}