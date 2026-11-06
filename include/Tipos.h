#pragma once

#include <string>
#include <vector>

/*
 * Tipos.h
 *
 * Aqui mora so o "vocabulario comum" do montador: os enums de tipo de erro
 * e de secao, a struct que representa uma linha ja tokenizada e a entrada
 * da tabela de simbolos. Nada de logica, nada de implementacao. A ideia e
 * que LeitorFonte, TabelaInstrucoes, TabelaSimbolos, Diretivas, GerenciadorErros
 * e o proprio Assembler incluam esse header e falem a mesma lingua.
 *
 * A tabela de instrucoes (opcode / qtd de operandos / tamanho) nao entra
 * aqui de proposito: ela vive em Instrucao.h, junto com a classe que a
 * encapsula.
 */

// Classificacao do erro. Usada pelo GerenciadorErros pra montar a mensagem
// no formato "Linha X: TIPO - descricao".
enum class TipoErro {
    LEXICO,     // forma invalida: caractere estranho, numero malformado, etc
    SINTATICO,  // estrutura errada: instrucao inexistente, operando faltando/sobrando
    SEMANTICO   // estrutura ok, mas o sentido nao fecha: rotulo repetido, simbolo indefinido
};

// Estado de secao durante a leitura do fonte. O montador comeca em NENHUMA
// e troca quando aparece a diretiva SECAO. Os valores internos sao DADOS e
// TEXTO pra nao confundir com as palavras-chave DATA/TEXT que o usuario
// escreve no .asm (e que o LeitorFonte normaliza).
enum class TipoSecao {
    NENHUMA,
    DADOS,
    TEXTO
};

// Diferencia o que cada entrada da TS representa. ROTULO e o caso normal
// (endereco de codigo ou de dado); CONSTANTE vem de CONST; EQU e o valor
// fixo declarado pela diretiva EQU antes das secoes.
enum class TipoSimbolo {
    ROTULO,
    CONSTANTE,
    EQU
};

// Uma linha do fonte ja "mastigada" pelo LeitorFonte: comentario separado,
// rotulo/operacao/operandos isolados e a secao vigente anotada. O resto do
// montador so consome essa struct, nao mexe em string crua.
struct LinhaTokenizada {
    int numeroLinha = 0;                 // linha original do .asm, pra mensagem de erro
    std::string rotulo;                  // vazio quando a linha nao tem rotulo
    std::string operacao;                // mnemonico ou diretiva (ex: "ADD", "SPACE")
    std::vector<std::string> operandos;  // ja separados; caso "X + 2" vem como um operando so
    std::string comentario;              // guardado pra debug, nao entra no .OBJ
    TipoSecao secao = TipoSecao::NENHUMA; // secao em que a linha caiu
};

// Entrada da Tabela de Simbolos. Na primeira passagem a gente insere com o
// contador de posicao do momento; na segunda so consulta pra resolver
// operandos e escrever o .OBJ.
struct EntradaTS {
    std::string nome;
    int valor = 0;                       // endereco (rotulo) ou valor numerico (CONST/EQU)
    TipoSimbolo tipo = TipoSimbolo::ROTULO;

    // Deixei esses dois aqui desde ja pra nao ter que mexer na struct quando
    // o ligador entrar em cena. O montador standalone deixa os dois em false;
    // quando ele for chamado pelo ligador (etapa 3.2) esses campos passam a
    // ser preenchidos a partir de PUBLIC/EXTERN.
    bool ePublico = false;
    bool eExterno = false;
};