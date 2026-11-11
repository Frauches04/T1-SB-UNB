#include "Instrucao.h"

#include <cctype>

TabelaInstrucoes::TabelaInstrucoes() {
    // A ordem dos valorez segue a dos slides: Opcode, quantidade de operandos, tamanho em palavras.
    instrucoes.emplace("ADD",    DadosInstrucao{1,  1, 2});
    instrucoes.emplace("SUB",    DadosInstrucao{2,  1, 2});
    instrucoes.emplace("MUL",    DadosInstrucao{3,  1, 2});
    instrucoes.emplace("DIV",    DadosInstrucao{4,  1, 2});
    instrucoes.emplace("JMP",    DadosInstrucao{5,  1, 2});
    instrucoes.emplace("JMPN",   DadosInstrucao{6,  1, 2});
    instrucoes.emplace("JMPP",   DadosInstrucao{7,  1, 2});
    instrucoes.emplace("JMPZ",   DadosInstrucao{8,  1, 2});
    instrucoes.emplace("COPY",   DadosInstrucao{9,  2, 3});
    instrucoes.emplace("LOAD",   DadosInstrucao{10, 1, 2});
    instrucoes.emplace("STORE",  DadosInstrucao{11, 1, 2});
    instrucoes.emplace("INPUT",  DadosInstrucao{12, 1, 2});
    instrucoes.emplace("OUTPUT", DadosInstrucao{13, 1, 2});
    instrucoes.emplace("STOP",   DadosInstrucao{14, 0, 1});
}

bool TabelaInstrucoes::buscar(const std::string& nome, DadosInstrucao& saida) const {
    auto it = instrucoes.find(normalizar(nome));
    if (it == instrucoes.end()) {
        return false; // Caso o mnemonico nao seja encontrado.
    }
    saida = it->second;
    return true;
}

bool TabelaInstrucoes::contem(const std::string& nome) const {
    return instrucoes.find(normalizar(nome)) != instrucoes.end();
}

std::string TabelaInstrucoes::normalizar(const std::string& nome) {
    std::string r;
    r.reserve(nome.size());
    for (char c : nome) {
        
        r.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return r;
}