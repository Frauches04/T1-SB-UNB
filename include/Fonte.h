#pragma once

#include <fstream>
#include <string>

#include "Erro.h"
#include "Tipos.h"

// LeitorFonte cuida da leitura do arquivo .asm e da tokenizacao de cada
// linha util. Ele e usado duas vezes pelo Assembler: uma na primeira
// passagem (para montar a tabela de simbolos e contar palavras) e outra
// na segunda (para resolver operandos e gerar o objeto). Por isso tem o
// metodo reiniciar(), que rebobina o arquivo e zera o estado interno.
//
// O leitor nao sabe o que e uma instrucao nem o que e uma diretiva: ele
// so separa rotulo, operacao e operandos. Quem decide o que cada coisa
// significa e o Assembler.
class LeitorFonte {
public:
    // O gerenciador de erros vem por referencia porque e compartilhado com
    // o Assembler. O leitor reporta aqui os erros lexicos (caractere
    // invalido, identificador malformado) que encontra durante a leitura.
    explicit LeitorFonte(GerenciadorErros& erros);

    // Abre o arquivo para leitura. Devolve false se nao conseguir abrir
    // (caminho errado, sem permissao). O chamador e responsavel por
    // mostrar a mensagem e abortar.
    bool abrir(const std::string& caminho);

    // Le a proxima linha "util" e devolve ja tokenizada. Pula linhas
    // vazias e linhas que so tem comentario. Trata o caso de rotulo
    // isolado, guardando o rotulo para aplicar na proxima linha com
    // operacao (o enunciado diz "o rotulo seria equivalente a linha
    // seguinte"). Devolve false quando o arquivo acaba.
    bool proximaLinha(LinhaTokenizada& saida);

    // Rebobina o arquivo e zera o estado interno (contador de linhas,
    // rotulo pendente, secao atual). Usado entre a primeira e a segunda
    // passagem. Os erros ja registrados permanecem, porque nao faz
    // sentido apagar o que ja foi reportado.
    void reiniciar();

    // Fecha o arquivo. Chamado no destrutor e pode ser chamado antes
    // de reabrir para outro caminho.
    void fechar();

private:
    // Varre o texto (sem comentario) atras de caractere invalido. So
    // aceita letra, digito, underscore, dois-pontos, mais, menos e
    // espaco/tab. Qualquer outra coisa dispara um erro LEXICO na linha
    // corrente. Reporta no maximo um erro por linha, o que ja basta
    // para orientar a correcao.
    void checarCaracteres(const std::string& texto, int linha);

    GerenciadorErros& erros;

    std::ifstream arquivo;
    std::string caminho;

    int linhaAtual = 0;

    // Rotulo que apareceu sozinho numa linha e vai ser aplicado a
    // proxima linha util. Fica vazio quando nao ha pendencia.
    std::string rotuloPendente;
    int linhaRotuloPendente = 0;

    // Secao em que as linhas seguintes estao. Atualizada quando o
    // leitor encontra a diretiva SECAO. A linha da propria SECAO tem
    // secao = a anterior (a troca vale para as linhas seguintes).
    TipoSecao secaoAtual = TipoSecao::NENHUMA;
};