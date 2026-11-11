#pragma once

#include <map>
#include <string>
#include <vector>

#include "Diretivas.h"
#include "Erro.h"
#include "Fonte.h"
#include "Instrucao.h"
#include "TabelaSimbolos.h"
#include "Tipos.h"

// Orquestrador do montador: recebe um .asm, executa as duas passagens
// e escreve o .OBJ (uma linha) e, se houver metadados, o .meta ao lado.
class Assembler {
public:
    // Vai compartilhar o GerenciadorErros com os demais modulos.
    explicit Assembler(GerenciadorErros& erros);

    // Ponto de entrada. Devolve true se montou sem erros e escreveu as saidas.
    bool montar(const std::string& caminhoEntrada,
                const std::string& caminhoSaida);

private:
    // Primeira passagem: preenche a tabela de simbolos, calcula os
    // tamanhos de codigo e de dados e detecta os erros iniciais.
    void primeiraPassagem(LeitorFonte& leitor);

    // Segunda passagem: resolve operandos, gera os buffers de codigo
    // e de dados e preenche o bitmap de relocacao e os usos externos.
    void segundaPassagem(LeitorFonte& leitor);

    // Grava o .OBJ (bufferCodigo + bufferDados numa linha).
    bool escreverObjeto(const std::string& caminhoSaida);

    // Grava o .meta (ENTRY / D / U / R), se houver metadados.
    bool escreverMeta(const std::string& caminhoEntrada);

    // Passa a string para maiusculo (ASCII).
    static std::string normalizar(const std::string& s);

    // Converte a string inteira em int; rejeita sobras e nao-numeros.
    static bool converterInteiro(const std::string& s, int& saida);

    // Reorganiza "BEGIN: X" para (op=BEGIN, operandos=[X]).
    static void ajustarBegin(LinhaTokenizada& linha);

    GerenciadorErros& erros;

    TabelaInstrucoes tabela;
    ProcessadorDiretivas diretivas;
    TabelaSimbolos ts;

    // Preenchidos no fim da 1a passagem; usados na 2a.
    int tamanhoCodigo = 0;
    int tamanhoDados = 0;

    // Saida da 2a passagem. Codigo primeiro, dados depois.
    std::vector<std::string> bufferCodigo;
    std::vector<std::string> bufferDados;

    // Metadados do .meta (consumidos pelo ligador).
    std::string entryLabelName;
    int entryPos = 0;
    bool temEntry = false;
    std::vector<std::string> publicosNomes;
    std::map<int, std::string> usosExternos;
    std::vector<int> bitmapCodigo;
};