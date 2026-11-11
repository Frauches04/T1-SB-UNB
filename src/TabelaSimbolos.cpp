#include "TabelaSimbolos.h"

#include <cctype>

TabelaSimbolos::TabelaSimbolos() {
}

bool TabelaSimbolos::inserir(const EntradaTS& entrada) {
    // Esse trecho irá normalizar o nome do simbolo para maiusculo, garantindo que a busca seja case-insensitive.
    EntradaTS copia = entrada;
    copia.nome = normalizar(entrada.nome);

    // Aqui e inserido o par (copia.nome, copia) na tabela de simbolos. Se já existir uma entrada com o mesmo nome, emplace não insere e retorna um par cujo segundo elemento é false. 
    auto resultado = simbolos.emplace(copia.nome, copia);
    
    // irá retornar true se a inserção foi bem-sucedida (ou seja, não havia um símbolo com o mesmo nome) e false caso contrário.
    return resultado.second;
}

bool TabelaSimbolos::buscar(const std::string& nome, EntradaTS& saida) const {
    auto it = simbolos.find(normalizar(nome));
    if (it == simbolos.end()) {
        return false;
    }
    saida = it->second;
    return true;
}

bool TabelaSimbolos::existe(const std::string& nome) const {
    return simbolos.find(normalizar(nome)) != simbolos.end();
}

std::vector<EntradaTS> TabelaSimbolos::listar() const {
    std::vector<EntradaTS> resultado;
    resultado.reserve(simbolos.size());
    for (const auto& par : simbolos) {
        resultado.push_back(par.second);
    }
    return resultado;
}

std::size_t TabelaSimbolos::tamanho() const {
    return simbolos.size();
}

void TabelaSimbolos::limpar() {
    simbolos.clear();
}

std::string TabelaSimbolos::normalizar(const std::string& nome) {
    std::string r;
    r.reserve(nome.size());
    for (char c : nome) {
        r.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return r;
}