#include "Erro.h"
#include "Ligador.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

// Troca a extensao do caminho por .LIG. A especificacao nao diz o
// nome exato do arquivo de saida, mas a convencao do professor
// (confirmada pelo LIG.OBJ.Txt que ele distribuiu) e pegar o nome
// do primeiro .asm e trocar a extensao. Se nao houver ponto no
// caminho, anexa .LIG (caso raro, ja que a entrada vem com .asm).
//
// "LIG" em maiusculo porque o enunciado escreve .LIG assim. No
// Windows a caixa nao aparece no sistema de arquivos, mas no Linux
// saida.lig e saida.LIG sao arquivos diferentes. Seguimos a grafia
// do professor.
std::string derivarNomeLig(const std::string& caminhoAsm) {
    std::size_t pos = caminhoAsm.find_last_of('.');
    if (pos == std::string::npos) {
        return caminhoAsm + ".LIG";
    }
    return caminhoAsm.substr(0, pos) + ".LIG";
}

}  // namespace

int main(int argc, char* argv[]) {
    // O enunciado fixa o uso: ./ligador prog1.asm prog2.asm prog3.asm
    // (no maximo 3 programas). argc inclui o nome do programa, entao
    // 2..4 significa 1..3 arquivos. Menos que 1 nao ha o que ligar;
    // mais que 3 viola a especificacao.
    if (argc < 2 || argc > 4) {
        std::cerr << "uso: " << argv[0]
                  << " prog1.asm [prog2.asm] [prog3.asm]\n";
        return 1;
    }

    // Monta a lista de argumentos que o Ligador vai receber. Isso
    // evita que o Ligador tenha que saber que existe argc/argv (ele
    // so recebe um vector<string>).
    std::vector<std::string> arquivosAsm;
    arquivosAsm.reserve(static_cast<std::size_t>(argc - 1));
    for (int i = 1; i < argc; ++i) {
        arquivosAsm.push_back(argv[i]);
    }

    // Nome do .LIG vem do PRIMEIRO .asm, nao do ultimo. A escolha
    // e arbitraria, mas casar com o primeiro e o que deixa
    // "ligador mod1.asm mod2.asm" produzir mod1.LIG, que e o que
    // o professor espera comparar com o gabarito do modulo 1.
    const std::string caminhoSaida = derivarNomeLig(arquivosAsm[0]);

    // O GerenciadorErros e compartilhado com o Ligador e, por
    // consequencia, com os processos montadores chamados via
    // system(). Cada montador imprime seus proprios erros no
    // stderr quando falha (o main_montador faz isso), mas o
    // ligador so sabe que falhou pelo exit code. Os erros que o
    // proprio Ligador detecta (externo nao resolvido, publico
    // duplicado, arquivo nao abrir) ficam neste gerenciador.
    GerenciadorErros erros;

    Ligador ligador(erros);

    const bool ligou = ligador.ligar(arquivosAsm, caminhoSaida);

    // Se o Ligador registrou algum erro, imprime todos no stderr.
    // Isso cobre externo nao resolvido, publico duplicado, falha
    // ao abrir .OBJ/.meta e falha ao criar o .LIG. O codigo de
    // saida e != 0 pro chamador (script, Makefile) saber que falhou.
    if (erros.temErros()) {
        erros.imprimir(std::cerr);
        return 1;
    }

    // Caso raro: ligar() devolveu false sem registrar erro. Isso
    // nao deveria acontecer, mas se acontecer, saimos com falha
    // pra nao mentir pro chamador.
    if (!ligou) {
        std::cerr << "erro ao ligar os modulos\n";
        return 1;
    }

    // Sucesso silencioso. O .LIG foi escrito em caminhoSaida. Se
    // o professor quiser uma mensagem, e uma linha antes do
    // return; mas o silencio e o que permite rodar em script
    // sem poluir a saida.
    return 0;
}