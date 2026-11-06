#pragma once

#include <map>
#include <string>

// Dados de uma instrucao da maquina hipotetica: o numero do opcode que vai
// pro arquivo objeto, quantos operandos ela espera e quantas palavras ela
// ocupa na memoria (na pratica, opcode + operandos).
struct DadosInstrucao {
    int opcode = 0;
    int qtdOperandos = 0;
    int tamanhoPalavras = 0;
};

// TabelaInstrucoes guarda as 14 instrucoes da especificacao e permite
// consultar pelo mnemonico. A busca e case-insensitive, entao tanto faz o
// usuario escrever "add" ou "ADD" no .asm.
class TabelaInstrucoes {
public:
    // Monta a tabela com as 14 instrucoes da especificacao.
    TabelaInstrucoes();

    // Procura o mnemonico e, se achar, copia os dados pra 'saida' e devolve
    // true. Se nao achar, devolve false sem mexer em 'saida'. Quem chama usa
    // esse false pra registrar erro sintatico de instrucao invalida.
    bool buscar(const std::string& nome, DadosInstrucao& saida) const;

    // Atalho pra quando so interessa saber se o mnemonico existe.
    bool contem(const std::string& nome) const;

private:
    // Chave e o mnemonico em maiusculo; valor e o pacote de dados da instrucao.
    std::map<std::string, DadosInstrucao> instrucoes;

    // Passa a string pra maiuscula (so ASCII). Usado nas consultas pra deixar
    // a tabela com chaves em maiusculo e comparar sempre no mesmo formato.
    static std::string normalizar(const std::string& nome);
};