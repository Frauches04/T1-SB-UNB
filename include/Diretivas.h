#pragma once

#include <string>
#include <vector>

#include "Erro.h"
#include "TabelaSimbolos.h"
#include "Tipos.h"

// Sua função é processar cada diretiva (CONST, SPACE, EQU, IF) e dizer ao Assembler o que fazer com ela. 
//Nao le arquivo, nao mexe em buffer de codigo: so responde perguntas pontuais. Quem percorre o fonte e orquestra as duas passagens e o Assembler.
struct ResultadoDiretiva {
    bool reconhecida = false;
    bool erro = false;
    int palavrasAlocadas = 0;
    bool pularProximaLinha = false;
    std::vector<std::string> valoresParaObjeto;
};

class ProcessadorDiretivas {
public:
    explicit ProcessadorDiretivas(GerenciadorErros& erros);

    // Diz se o mnemônico e uma diretiva conhecida.

    bool ehDiretiva(const std::string& nome) const;

    // Ponto de entrada principal. na primeira passagem ela insere o simbolo na TS; na segunda, so confere 
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

    // Tenta converter a string inteira em int usando std::stoi. Devolve true se conseguiu, false se nao. Se conseguiu, o valor convertido vai pra 'saida'.
    static bool converterInteiro(const std::string& s, int& saida);

    // Mesma normalizacao usada antes
    static std::string normalizar(const std::string& s);

    // Referencia pro gerenciador de erros, pra registrar erros sem precisar passar ele em cada chamada.
    GerenciadorErros& erros;
};