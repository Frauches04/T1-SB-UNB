#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "Tipos.h"

// Um erro registrado guarda de qual linha do arquivo original ele veio,
// a que categoria pertence (lexico, sintatico ou semantico) e um texto
// curto pra ajudar a localizar o problema.
struct Erro {
    int linha = 0;
    TipoErro tipo = TipoErro::SINTATICO;
    std::string mensagem;
};

// GerenciadorErros junta os erros que forem aparecendo durante a montagem
// e imprime todos de uma vez no final. A ideia e nao abortar na primeira
// falha: a especificacao pede que o programa inteiro seja percorrido e que
// todas as linhas com problema sejam mostradas.
class GerenciadorErros {
public:
    // Registra um erro. O numero da linha vem do leitor do fonte; o tipo
    // e a mensagem ficam a cargo de quem detectou o problema.
    void registrar(int linha, TipoErro tipo, const std::string& mensagem);

    // Retorna true se pelo menos um erro foi registrado. Usado pelo
    // Assembler pra decidir se gera ou nao o arquivo .OBJ.
    bool temErros() const;

    // Quantidade total de erros, util pra testes e pra log.
    std::size_t quantidade() const;

    // Imprime todos os erros no formato "Linha N: TIPO - mensagem",
    // um por linha, na ordem em que foram registrados.
    void imprimir(std::ostream& saida) const;

    // Limpa a lista. Nao e usado no fluxo normal, mas ajuda em testes
    // quando o mesmo gerenciador e reaproveitado.
    void limpar();

    // Traduz o enum pro nome que aparece na tela (LEXICO, SINTATICO,
    // SEMANTICO). Deixei publico pra poder ser usado em logs de teste.
    static std::string nomeDoTipo(TipoErro tipo);

private:
    // Os erros ficam na ordem de registro; nao ha necessidade de ordenar
    // por linha porque a leitura do fonte ja acontece em ordem crescente.
    std::vector<Erro> erros;
};