#pragma once

#include <map>
#include <string>

struct DadosInstrucao {
    int opcode = 0;
    int qtdOperandos = 0;
    int tamanhoPalavras = 0;
};

class TabelaInstrucoes {
public:
    // Monta a tabela com as 14 instrucoes da especificacao.
    TabelaInstrucoes();

    // Procura o mnemonico para ver se ele existe e, caso exista, retorna os dados da instrucao. Retorna true se achou, false caso contrario.
    bool buscar(const std::string& nome, DadosInstrucao& saida) const;

    // Atalho pra quando so interessa saber se o mnemonico existe.
    bool contem(const std::string& nome) const;

private:
    // Chave e o mnemonico em maiusculo; valor e o pacote de dados da instrucao.
    std::map<std::string, DadosInstrucao> instrucoes;


    static std::string normalizar(const std::string& nome);
};