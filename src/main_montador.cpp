#include "Assembler.h"
#include "Erro.h"

#include <iostream>
#include <string>

namespace {

std::string derivarNomeObjeto(const std::string& caminhoEntrada) {
    std::size_t pos = caminhoEntrada.find_last_of('.');
    if (pos == std::string::npos) {
        return caminhoEntrada + ".OBJ";
    }
    return caminhoEntrada.substr(0, pos) + ".OBJ";
}

} 

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "uso: " << argv[0] << " arquivo.asm\n";
        return 1;
    }

    const std::string caminhoEntrada = argv[1];
    const std::string caminhoSaida = derivarNomeObjeto(caminhoEntrada);


    GerenciadorErros erros;


    Assembler montador(erros);


    const bool montou = montador.montar(caminhoEntrada, caminhoSaida);


    if (erros.temErros()) {
        erros.imprimir(std::cerr);
        return 1;
    }


    if (!montou) {
        std::cerr << "erro ao gerar o arquivo de saida\n";
        return 1;
    }


    return 0;
}