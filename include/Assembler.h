#pragma once

#include <string>
#include <vector>

#include "Diretivas.h"
#include "Erro.h"
#include "Fonte.h"
#include "Instrucao.h"
#include "TabelaSimbolos.h"
#include "Tipos.h"

// Assembler e o orquestrador: recebe um caminho de .asm, executa a
// primeira e a segunda passagem e escreve o .OBJ numa unica linha.
//
// O estado interno (tabela de simbolos, buffers e tamanho do codigo)
// e mantido como membro porque precisa sobreviver entre as passagens:
// a segunda passagem le o que a primeira preencheu. Como o objeto e
// de uso unico, montar() limpa tudo no inicio.
class Assembler {
public:
    // O GerenciadorErros e compartilhado com o LeitorFonte e o
    // ProcessadorDiretivas. Passar por referencia garante que todos
    // enxergam os mesmos erros.
    explicit Assembler(GerenciadorErros& erros);

    // Ponto de entrada. Devolve true se a montagem terminou sem
    // erros e o arquivo de saida foi escrito. Devolve false se
    // houve qualquer erro (os erros ficam no GerenciadorErros pro
    // chamador imprimir).
    bool montar(const std::string& caminhoEntrada,
                const std::string& caminhoSaida);

private:
    // Primeira passagem: monta a tabela de simbolos, calcula o
    // tamanho do codigo e do bloco de dados e detecta rotulo
    // repetido, instrucao invalida, secao de texto ausente etc.
    void primeiraPassagem(LeitorFonte& leitor);

    // Segunda passagem: resolve operandos, gera os buffers de
    // codigo e de dados e detecta simbolo indefinido. Usa o
    // tamanhoCodigo calculado na primeira passagem para ajustar o
    // endereco dos labels que apontam pra dados.
    void segundaPassagem(LeitorFonte& leitor);

    // Concatena bufferCodigo + bufferDados em uma linha so e grava
    // no caminho de saida. Devolve false se nao conseguir abrir o
    // arquivo.
    bool escreverObjeto(const std::string& caminhoSaida);

    // Normaliza uma string pra maiusculo (so ASCII). Mesma logica
    // usada em outros modulos, mantida local pra nao criar
    // dependencia cruzada.
    static std::string normalizar(const std::string& s);

    // Tenta converter a string inteira em int, rejeitando sobras
    // ("3.5", "2x"). Aceita sinal "+" e "-".
    static bool converterInteiro(const std::string& s, int& saida);

    GerenciadorErros& erros;

    // Nao poderiam ser locais a cada chamada porque a segunda
    // passagem depende do que a primeira preencheu.
    TabelaInstrucoes tabela;
    ProcessadorDiretivas diretivas;
    TabelaSimbolos ts;

    // Guardados no fim da primeira passagem. Usados na segunda pra
    // ajustar o endereco de labels de dados (data offset + tamanho
    // do codigo) e para nada mais.
    int tamanhoCodigo = 0;
    int tamanhoDados = 0;

    // Saida da segunda passagem. O buffer de codigo recebe as
    // instrucoes na ordem em que aparecem no fonte; o buffer de
    // dados recebe CONST e SPACE na ordem em que aparecem. No
    // arquivo final, codigo vem primeiro e dados depois,
    // independente da ordem das secoes no fonte.
    std::vector<std::string> bufferCodigo;
    std::vector<std::string> bufferDados;
};