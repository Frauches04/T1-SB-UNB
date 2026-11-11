#include "Erro.h"
#include "Ligador.h"

#include <iostream>
#include <string>
#include <vector>

namespace {


std::string derivarNomeLig(const std::string& caminhoAsm) {
    std::size_t pos = caminhoAsm.find_last_of('.');
    if (pos == std::string::npos) {
        return caminhoAsm + ".LIG";
    }
    return caminhoAsm.substr(0, pos) + ".LIG";
}

}  

int main(int argc, char* argv[]) {

    if (argc < 2 || argc > 4) {
        std::cerr << "uso: " << argv[0]
                  << " prog1.asm [prog2.asm] [prog3.asm]\n";
        return 1;
    }


    std::vector<std::string> arquivosAsm;
    arquivosAsm.reserve(static_cast<std::size_t>(argc - 1));
    for (int i = 1; i < argc; ++i) {
        arquivosAsm.push_back(argv[i]);
    }


    const std::string caminhoSaida = derivarNomeLig(arquivosAsm[0]);

    GerenciadorErros erros;

    Ligador ligador(erros);

    const bool ligou = ligador.ligar(arquivosAsm, caminhoSaida);


    if (erros.temErros()) {
        erros.imprimir(std::cerr);
        return 1;
    }


    if (!ligou) {
        std::cerr << "erro ao ligar os modulos\n";
        return 1;
    }


    return 0;
}