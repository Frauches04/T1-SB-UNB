#pragma once

#include <string>
#include <vector>

#include "Erro.h"
#include "TabelaSimbolos.h"
#include "Tipos.h"

// O que cada chamada a processar() precisa devolver pro Assembler:
//  - se a linha era mesmo uma diretiva conhecida;
//  - se houve erro (registrado no GerenciadorErros);
//  - quantas palavras a diretiva ocupa no objeto (0 para EQU e IF);
//  - se a proxima linha deve ser pulada (apenas IF com condicao falsa);
//  - o que efetivamente vai pro arquivo objeto (CONST e SPACE).
//
// Devolver tudo numa struct evita que o Assembler dependa de estado
// interno do ProcessadorDiretivas. A classe fica "sem memoria".
struct ResultadoDiretiva {
    bool reconhecida = false;
    bool erro = false;
    int palavrasAlocadas = 0;
    bool pularProximaLinha = false;
    std::vector<std::string> valoresParaObjeto;
};

// ProcessadorDiretivas valida a sintaxe e o contexto de cada diretiva
// (CONST, SPACE, EQU, IF) e diz ao Assembler o que fazer com ela.
// Nao le arquivo, nao mexe em buffer de codigo: so responde perguntas
// pontuais. Quem percorre o fonte e orquestra as duas passagens e o
// Assembler.
class ProcessadorDiretivas {
public:
    explicit ProcessadorDiretivas(GerenciadorErros& erros);

    // Diz se o mnemônico e uma diretiva conhecida. Usado pelo Assembler
    // pra decidir se tenta a tabela de instrucoes primeiro ou a de
    // diretivas, e pela segunda passagem pra nao tratar diretiva como
    // simbolo indefinido.
    bool ehDiretiva(const std::string& nome) const;

    // Ponto de entrada principal. Retorna o resultado descrito acima.
    // O parametro 'primeiraPassagem' e usado pela EQU: na primeira
    // passagem ela insere o simbolo na TS; na segunda, so confere que
    // ele continua la (checagem defensiva contra bug interno).
    ResultadoDiretiva processar(const LinhaTokenizada& linha,
                                TabelaSimbolos& ts,
                                bool primeiraPassagem);

private:
    ResultadoDiretiva processarConst(const LinhaTokenizada& linha);
    ResultadoDiretiva processarSpace(const LinhaTokenizada& linha);
    ResultadoDiretiva processarEqu(const LinhaTokenizada& linha,
                                   TabelaSimbolos& ts,
                                   bool primeiraPassagem);
    ResultadoDiretiva processarIf(const LinhaTokenizada& linha,
                                  const TabelaSimbolos& ts);

    // Tenta converter a string inteira em int. Rejeita "0x10", "3.5",
    // "abc" e strings com sobra apos o numero ("2x"). Aceita "+2" e
    // "-2" porque o enunciado cobre positivos e negativos.
    static bool converterInteiro(const std::string& s, int& saida);

    // Mesma normalizacao usada em TabelaInstrucoes e TabelaSimbolos.
    // Mantida aqui tambem pra nao criar dependencia cruzada entre
    // modulos que nao precisam se conhecer.
    static std::string normalizar(const std::string& s);

    GerenciadorErros& erros;
};