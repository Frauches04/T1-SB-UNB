#pragma once

#include <map>
#include <string>
#include <vector>

#include "Erro.h"

// SimboloPublico representa um simbolo que o modulo exporta pra
// outros modulos enxergarem. O endereco e LOCAL ao modulo (offset
// dentro do proprio .OBJ). O Ligador soma o offset do modulo na
// hora de construir a tabela global.
struct SimboloPublico {
    std::string nome;
    int enderecoLocal = 0;
};

// UsoExterno marca uma posicao do .OBJ onde aparece um simbolo
// importado de outro modulo. O placeholder ja vai no .OBJ como 0;
// o Ligador substitui pelo endereco global.
struct UsoExterno {
    std::string nome;
    int posicao = 0;  // indice da palavra no .OBJ onde o simbolo aparece
};

// MetaModulo e o espelho do arquivo .meta gerado pelo montador. Tem
// o ponto de entrada (se houver BEGIN), a lista de simbolos
// publicos, a lista de usos externos e o bitmap de relocacao (um
// valor 0/1 por palavra do .OBJ, indicando "esta palavra contem um
// endereco que precisa de ajuste?").
struct MetaModulo {
    int entry = 0;
    bool temEntry = false;
    std::vector<SimboloPublico> publicos;
    std::vector<UsoExterno> externos;
    std::vector<int> relocacao;
};

// Modulo e o que o Ligador monta na memoria para cada .asm: o
// caminho original, o conteudo do .OBJ (tokens, na ordem), os
// metadados e o offset calculado na hora do layout.
struct Modulo {
    std::string caminhoAsm;
    std::vector<std::string> objeto;
    MetaModulo meta;
    int offset = 0;
};

// Ligador combina de 1 a 3 modulos .asm em um unico .LIG, no mesmo
// formato do .OBJ (uma linha de numeros). O fluxo e: montar cada
// .asm via executavel montador, ler .OBJ + .meta, calcular offsets
// cumulativos, construir a tabela global de simbolos, resolver os
// externos e relocar os internos, concatenar e escrever.
class Ligador {
public:
    explicit Ligador(GerenciadorErros& erros);

    // Ponto de entrada. Devolve true se ligou tudo sem erro e o
    // .LIG foi escrito. Os erros ficam no GerenciadorErros.
    bool ligar(const std::vector<std::string>& arquivosAsm,
               const std::string& caminhoSaida);

private:
    // Chama o executavel montador.exe sobre o .asm. Devolve false
    // se o montador nao existir no PATH ou no diretorio atual, ou
    // se ele sair com codigo diferente de 0 (fonte com erro).
    bool montarModulo(const std::string& caminhoAsm);

    // Le o .OBJ e o .meta gerados pelo montador para o .asm
    // informado. Se o .meta nao existir, trata como modulo sem
    // simbolos e sem relocacao (so funciona se o modulo nao tiver
    // enderecos internos, caso de programa de um modulo so com
    // codigo puro).
    bool lerModulo(const std::string& caminhoAsm, Modulo& saida);

    // Parse do .meta no formato chave + valores por linha:
    //   ENTRY <n>
    //   D <nome> <addr>
    //   U <nome> <pos>
    //   R <bit> <bit> ...
    bool lerMeta(const std::string& caminhoMeta, MetaModulo& saida);

    // Preenche mod.offset de cada modulo com o tamanho acumulado
    // dos anteriores. O primeiro modulo sempre comeca em 0.
    void calcularOffsets(std::vector<Modulo>& modulos);

    // Junta todos os D de todos os modulos numa unica tabela,
    // somando o offset do modulo ao endereco local. Detecta
    // definicao duplicada (mesmo nome publico em dois modulos).
    bool construirTabelaGlobal(const std::vector<Modulo>& modulos,
                               std::map<std::string, int>& global);

    // Para cada modulo, percorre o bitmap de relocacao. Onde o bit
    // e 1:
    //   - se a posicao esta em U, substitui pelo endereco global
    //     do simbolo publico correspondente (erro se nao achar);
    //   - se nao, soma mod.offset ao valor atual (endereco
    //     interno relocado).
    // O resultado e concatenado em 'resultado'.
    bool resolver(const std::vector<Modulo>& modulos,
                  const std::map<std::string, int>& global,
                  std::vector<std::string>& resultado);

    // Escreve 'resultado' em uma unica linha no .LIG, valores
    // separados por espaco.
    bool escreverLig(const std::string& caminho,
                     const std::vector<std::string>& resultado);

    // Troca a extensao do caminho (ex: "tests/mod1.asm" -> "tests/mod1.OBJ"
    // ou "tests/mod1.meta"). Nao usa regex; so corta no ultimo ponto.
    static std::string derivarCaminho(const std::string& caminho,
                                      const std::string& novaExt);

    // Normalizacao pra maiusculo (ASCII), mesma usada nos outros
    // modulos. Centralizada aqui pra nao criar dependencia cruzada.
    static std::string normalizar(const std::string& s);

    // Tenta converter a string inteira em int. Rejeita sobras.
    static bool converterInteiro(const std::string& s, int& saida);

    GerenciadorErros& erros;
};