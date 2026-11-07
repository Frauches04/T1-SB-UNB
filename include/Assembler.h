#pragma once

#include <map>
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
// Desde a Etapa 2 tambem reconhece BEGIN/END/PUBLIC/EXTERN e gera um
// .meta ao lado do .OBJ, com os simbolos publicos, os usos externos
// e o bitmap de relocacao que o ligador consome.
//
// O estado interno (tabela de simbolos, buffers, tamanho do codigo)
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
    // erros e os arquivos de saida foram escritos. Devolve false se
    // houve qualquer erro (os erros ficam no GerenciadorErros pro
    // chamador imprimir).
    bool montar(const std::string& caminhoEntrada,
                const std::string& caminhoSaida);

private:
    // Primeira passagem: monta a tabela de simbolos, calcula o
    // tamanho do codigo e do bloco de dados e detecta rotulo
    // repetido, instrucao invalida, secao de texto ausente, PUBLIC
    // que nao existe no modulo e BEGIN que aponta pra label
    // inexistente.
    void primeiraPassagem(LeitorFonte& leitor);

    // Segunda passagem: resolve operandos, gera os buffers de
    // codigo e de dados, e detecta simbolo indefinido. Usa o
    // tamanhoCodigo calculado na primeira passagem pra ajustar o
    // endereco dos labels que apontam pra dados. Tambem preenche o
    // bitmapCodigo e a lista de usosExternos, que o ligador vai
    // consultar.
    void segundaPassagem(LeitorFonte& leitor);

    // Concatena bufferCodigo + bufferDados em uma linha so e grava
    // no caminho de saida. Devolve false se nao conseguir abrir o
    // arquivo.
    bool escreverObjeto(const std::string& caminhoSaida);

    // Grava o .meta (ENTRY / D / U / R) no caminho derivado do .asm.
    // Se o modulo nao tiver nenhum metadado (sem BEGIN, sem PUBLIC,
    // sem EXTERN e sem bitmap), nao cria o arquivo — o ligador
    // trata esse caso como "modulo sem relocacao nem simbolos".
    bool escreverMeta(const std::string& caminhoEntrada);

    // Normaliza uma string pra maiusculo (so ASCII). Mesma logica
    // usada em outros modulos, mantida local pra nao criar
    // dependencia cruzada.
    static std::string normalizar(const std::string& s);

    // Tenta converter a string inteira em int, rejeitando sobras
    // ("3.5", "2x"). Aceita sinal "+" e "-".
    static bool converterInteiro(const std::string& s, int& saida);

    // "BEGIN: MAIN" chega do LeitorFonte como (rotulo="BEGIN",
    // operacao="MAIN", operandos=[]). Esta funcao reorganiza pra
    // (rotulo="", operacao="BEGIN", operandos=["MAIN"]) antes de
    // qualquer processamento, nas duas passagens. Sem isso, o
    // BEGIN nao seria reconhecido como diretiva.
    static void ajustarBegin(LinhaTokenizada& linha);

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

    // Metadados do .meta pro ligador.
    // - entryLabelName: nome declarado em BEGIN. A posicao final
    //   (entryPos) so e resolvida no fim da primeira passagem,
    //   quando o label ja foi declarado por "MAIN:" ou similar.
    // - entryPos: endereco local do label de entrada no modulo.
    // - temEntry: true se houve BEGIN (para saber se escreve ENTRY).
    // - publicosNomes: nomes declarados por PUBLIC, na ordem em que
    //   aparecem no fonte. A resolucao do endereco de cada um fica
    //   pra escreverMeta, quando a TS ja esta completa.
    // - usosExternos: posicao no .OBJ -> nome do simbolo EXTERN
    //   usado ali. O montador emite 0 como placeholder e o ligador
    //   substitui pelo endereco global.
    // - bitmapCodigo: um bit por palavra de codigo (0 = opcode,
    //   1 = operando/endereco que o ligador precisa ajustar). O
    //   ligador percorre esse bitmap e decide o que relocar.
    std::string entryLabelName;
    int entryPos = 0;
    bool temEntry = false;
    std::vector<std::string> publicosNomes;
    std::map<int, std::string> usosExternos;
    std::vector<int> bitmapCodigo;
};