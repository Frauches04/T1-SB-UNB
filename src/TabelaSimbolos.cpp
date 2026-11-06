#include "TabelaSimbolos.h"

#include <cctype>

TabelaSimbolos::TabelaSimbolos() {
}

bool TabelaSimbolos::inserir(const EntradaTS& entrada) {
    // Copia antes de mexer pra nao alterar o objeto do chamador. O nome da
    // copia vai normalizado pra ficar coerente com a chave do map; assim,
    // quando alguem chamar listar(), o nome que vem junto com o valor ja
    // esta no formato usado nas comparacoes.
    EntradaTS copia = entrada;
    copia.nome = normalizar(entrada.nome);

    auto resultado = simbolos.emplace(copia.nome, copia);
    // emplace devolve um par<iterator, bool>. O bool e false quando ja havia
    // uma chave igual, exatamente o caso de rotulo repetido.
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