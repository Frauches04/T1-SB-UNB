#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstddef>

#include "Instruction.h"
#include "Lexer.h"
#include "SymbolTable.h"

class Assembler {
public:
    Assembler();

    bool assemble(const std::string& arquivoEntrada,
                  const std::string& arquivoSaida);

private:
    // ============================================================
    // Componentes
    // ============================================================
    Lexer analisadorLexico;
    SymbolTable tabelaSimbolos;
    std::vector<Instruction> instrucoes;

    // ============================================================
    // Tabelas internas
    // ============================================================
    // tabelaInstrucoes: mnemônico -> {opcode, tamanho em palavras}
    //   Ex: "ADD" -> {1, 2}, "COPY" -> {9, 3}, "STOP" -> {14, 1}
    std::unordered_map<std::string, std::pair<int, int>> tabelaInstrucoes;

    // tabelaEqu: símbolos definidos via EQU (antes de qualquer seção)
    //   Ex: "TAM" -> 10
    std::unordered_map<std::string, int> tabelaEqu;

    // ============================================================
    // Estado do processo de montagem
    // ============================================================
    int contadorPosicao = 0;
    bool temSecaoTexto = false;

    // ============================================================
    // Constantes do projeto
    // ============================================================
    static constexpr std::size_t TAMANHO_MAXIMO_SIMBOLO = 30;

    // ============================================================
    // Inicialização
    // ============================================================
    void inicializarTabelaInstrucoes();

    // ============================================================
    // Passagens
    // ============================================================
    bool primeiraPassagem(const std::string& caminhoEntrada);
    bool segundaPassagem(const std::string& caminhoSaida);

    // ============================================================
    // Processamento de diretivas especiais
    // ============================================================
    bool processarEqu(const std::string& rotulo,
                      const std::string& valor,
                      int numeroLinha);

    bool avaliarIf(const std::string& expressao,
                   bool& deveIncluir,
                   int numeroLinha) const;

    // ============================================================
    // Resolução de operandos
    // ============================================================
    bool resolverOperando(const std::string& operando,
                          int& valorSaida,
                          int numeroLinha) const;

    bool resolverExpressao(const std::string& expressao,
                           int& valorSaida,
                           int numeroLinha) const;

    // ============================================================
    // Validação
    // ============================================================
    bool ehSimboloValido(const std::string& s) const;
    bool ehNumero(const std::string& s) const;

    // ============================================================
    // Utilidades de string
    // ============================================================
    static std::string aparar(const std::string& s);
    static std::string paraMaiusculo(const std::string& s);

    // ============================================================
    // Diagnóstico
    // ============================================================
    void imprimirErro(const std::string& mensagem, int numeroLinha) const;
};

#endif // ASSEMBLER_H