#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Tipos.h"

class TabelaSimbolos {
public:
    TabelaSimbolos();

    // Sua função é inserir a entrada na tabela. Se ja existir uma entrada com o mesmo nome, nao insere e devolve false; caso contrario, insere e devolve true.
    bool inserir(const EntradaTS& entrada);

    // Busca achar a entrada com o nome dado. Se achar, devolve true e copia a entrada pra saida; caso contrario, devolve false e saida nao e alterada.
    bool buscar(const std::string& nome, EntradaTS& saida) const;

    // Verifica se existe uma entrada com o nome dado. Devolve true se existir, false caso contrario.
    bool existe(const std::string& nome) const;

    // Seu dever e devolver todas as entradas da tabela, em qualquer ordem. A ideia e que o ligador possa percorrer a tabela de simbolos do montador e gerar o .OBJ.
    std::vector<EntradaTS> listar() const;

   
    std::size_t tamanho() const;

   
    void limpar();

private:
    // Ela guarda as entradas da tabela de simbolos, indexadas pelo nome do simbolo. A chave e o nome normalizado (maiusculo) do simbolo, e o valor e a entrada correspondente.
    std::map<std::string, EntradaTS> simbolos;

   
    static std::string normalizar(const std::string& nome);
};