#include "Erro.h"

void GerenciadorErros::registrar(int linha, TipoErro tipo, const std::string& mensagem) {
    Erro e;
    e.linha = linha;
    e.tipo = tipo;
    e.mensagem = mensagem;
    erros.push_back(e);
}

bool GerenciadorErros::temErros() const {
    return !erros.empty();
}

std::size_t GerenciadorErros::quantidade() const {
    return erros.size();
}

void GerenciadorErros::imprimir(std::ostream& saida) const {
    for (const Erro& e : erros) {
        saida << "Linha " << e.linha << ": "
              << nomeDoTipo(e.tipo) << " - "
              << e.mensagem << "\n";
    }
}

void GerenciadorErros::limpar() {
    erros.clear();
}

std::string GerenciadorErros::nomeDoTipo(TipoErro tipo) {
    switch (tipo) {
        case TipoErro::LEXICO:    return "LEXICO";
        case TipoErro::SINTATICO: return "SINTATICO";
        case TipoErro::SEMANTICO: return "SEMANTICO";
    }
    return "DESCONHECIDO";
}