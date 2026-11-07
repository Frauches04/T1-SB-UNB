#include "Assembler.h"
#include "Erro.h"

#include <iostream>
#include <string>

namespace {

// Troca a extensao do caminho por .OBJ, respeitando o nome do arquivo
// original. A especificacao diz "mesmo nome MUDANDO a extensao para
// .OBJ", entao procuramos o ultimo ponto do caminho e substituimos
// tudo o que vem depois por "OBJ". Se nao houver ponto, anexamos .OBJ
// (caso raro, ja que a entrada vem com extensao).
//
// O sufixo e "OBJ" em maiusculo porque o enunciado escreve assim. No
// Windows a diferenca de caixa nao aparece no sistema de arquivos, mas
// no Linux um arquivo chamado saida.obj e outro saida.OBJ sao coisas
// diferentes. Como o professor pode olhar o nome exato, seguimos a
// grafia dele.
std::string derivarNomeObjeto(const std::string& caminhoEntrada) {
    std::size_t pos = caminhoEntrada.find_last_of('.');
    if (pos == std::string::npos) {
        return caminhoEntrada + ".OBJ";
    }
    return caminhoEntrada.substr(0, pos) + ".OBJ";
}

}  // namespace

int main(int argc, char* argv[]) {
    // O enunciado fixa o uso: ./montador arquivo.asm. Sem argumento
    // ou com argumento a mais, nao ha o que montar. Mensagem de uso
    // no stderr pra nao misturar com a saida normal do programa.
    if (argc != 2) {
        std::cerr << "uso: " << argv[0] << " arquivo.asm\n";
        return 1;
    }

    const std::string caminhoEntrada = argv[1];
    const std::string caminhoSaida = derivarNomeObjeto(caminhoEntrada);

    // O GerenciadorErros e criado aqui e passado por referencia pro
    // Assembler. Ele acumula o que o LeitorFonte e o ProcessadorDiretivas
    // reportarem, e so no fim a gente imprime tudo de uma vez. Isso
    // atende ao "montado sempre o programa inteiro" da especificacao:
    // nao paramos no primeiro erro, percorremos o fonte todo.
    GerenciadorErros erros;

    // O Assembler guarda a tabela de simbolos internamente; e de uso
    // unico (montar() limpa o estado no inicio), entao criar na pilha
    // e chamar uma vez esta correto.
    Assembler montador(erros);

    // A montagem pode falhar por tres motivos: arquivo de entrada nao
    // abre, o fonte tem erros, ou o arquivo de saida nao pode ser
    // criado. Em todos os casos, o erro detalhado esta em 'erros' ou
    // o montar() devolveu false. Tratamos os dois separadamente abaixo.
    const bool montou = montador.montar(caminhoEntrada, caminhoSaida);

    // Se o leitor ou o assembler registraram qualquer erro (lexico,
    // sintatico ou semantico), imprimimos todos no stderr. O .OBJ
    // NAO e gerado quando ha erro (o montar() ja garante isso), entao
    // sair com codigo != 0 aqui e o sinal correto para scripts e para
    // o professor ver que a execucao nao foi bem-sucedida.
    if (erros.temErros()) {
        erros.imprimir(std::cerr);
        return 1;
    }

    // Caso raro: montar() devolveu false sem registrar erro. Isso
    // acontece so se o arquivo de saida nao puder ser aberto (o
    // montar() registra isso, mas colocamos aqui de reserva). Sem
    // erro na lista e sem .OBJ no disco, saimos com falha.
    if (!montou) {
        std::cerr << "erro ao gerar o arquivo de saida\n";
        return 1;
    }

    // Sucesso silencioso, para nao poluir a saida caso o professor
    // rode o montador dentro de um script que compara stdout. Se
    // quiser uma mensagem, e uma linha aqui.
    return 0;
}