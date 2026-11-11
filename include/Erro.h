#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "Tipos.h"

// Struct que guarda a linha, tipo e mensagem de cada erro. A linha e o tipo sao usados na hora de imprimir.
struct Erro {
    int linha = 0;
    TipoErro tipo = TipoErro::SINTATICO;
    std::string mensagem;
};
class GerenciadorErros {
public:
    // Registra o erro apartir de sua linha, tipo e mensagem. 
    void registrar(int linha, TipoErro tipo, const std::string& mensagem);

    // Caso haja erro irá retornar true e assim o programa não irá gerar o arquivo .OBJ, mas sim imprimir os erros na tela.
    bool temErros() const;

    // Quantidade total de erros
    std::size_t quantidade() const;

    // Imprime todos os erros
    void imprimir(std::ostream& saida) const;

    // Ele forncece um metodo para limpar a lista de erros, caso seja necessario reiniciar o processo de montagem.
    void limpar();

    // Ele irá fornecer um método para obter o nome do tipo de erro, que será útil para exibir mensagens de erro mais informativas.
    static std::string nomeDoTipo(TipoErro tipo);

private:
    // Ele organiza os erros em um vetor de objetos Erro, permitindo que múltiplos erros sejam armazenados e acessados de forma eficiente.
    std::vector<Erro> erros;
};