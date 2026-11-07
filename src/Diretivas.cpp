#include "Diretivas.h"

#include <cctype>

namespace {

// Representacao textual do zero que vai pro objeto em cada palavra
// alocada por SPACE. O enunciado aceita "0" ou "00"; escolhi "0"
// porque e o formato que aparece nos arquivos de teste do professor
// (por exemplo, o final de bin.obj: "14 2 0 0 0", onde CONST 2 e
// seguido de tres SPACE).
const char* PALAVRA_ZERO = "0";

}  // namespace

ProcessadorDiretivas::ProcessadorDiretivas(GerenciadorErros& g) : erros(g) {
}

bool ProcessadorDiretivas::ehDiretiva(const std::string& nome) const {
    std::string op = normalizar(nome);
    return op == "CONST" || op == "SPACE" || op == "EQU" || op == "IF";
}

ResultadoDiretiva ProcessadorDiretivas::processar(const LinhaTokenizada& linha,
                                                  TabelaSimbolos& ts,
                                                  bool primeiraPassagem) {
    std::string op = normalizar(linha.operacao);
    if (op == "CONST") return processarConst(linha);
    if (op == "SPACE") return processarSpace(linha);
    if (op == "EQU")   return processarEqu(linha, ts, primeiraPassagem);
    if (op == "IF")    return processarIf(linha, ts);
    return {};
}

ResultadoDiretiva ProcessadorDiretivas::processarConst(const LinhaTokenizada& linha) {
    ResultadoDiretiva r;
    r.reconhecida = true;

    // Sem operando, nao ha valor a escrever. Dois ou mais, a diretiva
    // nao sabe o que fazer com eles. So 1 operando e valido.
    if (linha.operandos.size() != 1) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "CONST espera exatamente um operando");
        r.erro = true;
        return r;
    }

    int valor = 0;
    if (!converterInteiro(linha.operandos[0], valor)) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "CONST espera um inteiro decimal, recebeu '" +
                        linha.operandos[0] + "'");
        r.erro = true;
        return r;
    }

    // CONST ocupa uma palavra. Normalizamos o texto do valor pra
    // descartar zeros a esquerda e o sinal "+" opcional: o objeto
    // final fica sempre com a forma canonica ("2", "-48").
    r.palavrasAlocadas = 1;
    r.valoresParaObjeto.push_back(std::to_string(valor));
    return r;
}

ResultadoDiretiva ProcessadorDiretivas::processarSpace(const LinhaTokenizada& linha) {
    ResultadoDiretiva r;
    r.reconhecida = true;

    // SPACE aceita 0 ou 1 operando. 0 significa "uma palavra". 1
    // significa "N palavras", com N > 0.
    if (linha.operandos.size() > 1) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "SPACE aceita no maximo um operando");
        r.erro = true;
        return r;
    }

    int quantidade = 1;
    if (linha.operandos.size() == 1) {
        if (!converterInteiro(linha.operandos[0], quantidade) || quantidade < 1) {
            erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                            "SPACE espera um inteiro positivo, recebeu '" +
                            linha.operandos[0] + "'");
            r.erro = true;
            return r;
        }
    }

    r.palavrasAlocadas = quantidade;
    r.valoresParaObjeto.assign(static_cast<std::size_t>(quantidade), PALAVRA_ZERO);
    return r;
}

ResultadoDiretiva ProcessadorDiretivas::processarEqu(const LinhaTokenizada& linha,
                                                     TabelaSimbolos& ts,
                                                     bool primeiraPassagem) {
    ResultadoDiretiva r;
    r.reconhecida = true;

    // EQU precisa de um rotulo do lado esquerdo (o nome do simbolo)
    // e de um valor do lado direito.
    if (linha.rotulo.empty()) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "EQU exige um rotulo do lado esquerdo");
        r.erro = true;
        return r;
    }
    if (linha.operandos.size() != 1) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "EQU espera exatamente um operando");
        r.erro = true;
        return r;
    }

    // O enunciado e explicito: EQU so pode vir antes de qualquer secao.
    // Se o leitor ja entrou em DADOS ou TEXTO, a diretiva esta no lugar
    // errado.
    if (linha.secao != TipoSecao::NENHUMA) {
        erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                        "EQU deve ser declarado antes de qualquer secao");
        r.erro = true;
        return r;
    }

    int valor = 0;
    if (!converterInteiro(linha.operandos[0], valor)) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "EQU espera um inteiro decimal, recebeu '" +
                        linha.operandos[0] + "'");
        r.erro = true;
        return r;
    }

    if (primeiraPassagem) {
        EntradaTS e;
        e.nome = linha.rotulo;
        e.valor = valor;
        e.tipo = TipoSimbolo::EQU;
        if (!ts.inserir(e)) {
            erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                            "simbolo '" + linha.rotulo + "' redefinido por EQU");
            r.erro = true;
        }
    } else {
        // Segunda passagem: o simbolo ja foi inserido na primeira.
        // Checagem defensiva; se nao estiver la, algum modulo saiu do
        // esperado entre as passagens e queremos saber.
        EntradaTS e;
        if (!ts.buscar(linha.rotulo, e)) {
            erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                            "simbolo '" + linha.rotulo + "' desapareceu entre as passagens");
            r.erro = true;
        }
    }
    return r;
}

ResultadoDiretiva ProcessadorDiretivas::processarIf(const LinhaTokenizada& linha,
                                                    const TabelaSimbolos& ts) {
    ResultadoDiretiva r;
    r.reconhecida = true;

    // O enunciado restringe IF a secao de texto. Se aparecer em DADOS
    // ou antes de qualquer secao, e erro semantico.
    if (linha.secao != TipoSecao::TEXTO) {
        erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                        "IF so e permitido dentro da secao de texto");
        r.erro = true;
        return r;
    }

    if (linha.operandos.size() != 1) {
        erros.registrar(linha.numeroLinha, TipoErro::SINTATICO,
                        "IF espera exatamente um operando");
        r.erro = true;
        return r;
    }

    // O operando tipico e um simbolo definido por EQU. Aceitamos tambem
    // um inteiro literal, porque a avaliacao e a mesma e evita surpresa
    // se o usuario escrever algo como "IF 0" pra desligar um trecho.
    int valor = 0;
    if (!converterInteiro(linha.operandos[0], valor)) {
        EntradaTS e;
        if (!ts.buscar(linha.operandos[0], e)) {
            erros.registrar(linha.numeroLinha, TipoErro::SEMANTICO,
                            "simbolo '" + linha.operandos[0] + "' indefinido em IF");
            r.erro = true;
            return r;
        }
        valor = e.valor;
    }

    // Convencao classica: valor zero pula o que vem na sequencia; valor
    // diferente de zero mantem. Como nao ha ENDIF na especificacao,
    // "a sequencia" e a proxima linha util.
    if (valor == 0) {
        r.pularProximaLinha = true;
    }
    return r;
}

bool ProcessadorDiretivas::converterInteiro(const std::string& s, int& saida) {
    if (s.empty()) {
        return false;
    }
    try {
        std::size_t consumido = 0;
        int valor = std::stoi(s, &consumido);
        // stoi para no primeiro caractere nao numerico. Se sobrou algo,
        // nao era um inteiro puro (ex.: "3.5", "2x", "12abc").
        if (consumido != s.size()) {
            return false;
        }
        saida = valor;
        return true;
    } catch (...) {
        return false;
    }
}

std::string ProcessadorDiretivas::normalizar(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        r.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return r;
}