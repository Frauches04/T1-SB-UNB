#pragma once

#include <fstream>
#include <string>

#include "Erro.h"
#include "Tipos.h"

// LeitorFonte cuida da leitura do arquivo .asm e da tokenizacao de cada linha util
class LeitorFonte {
public:
    // Usado para criar o leitor e passar a referencia do gerenciador de erros, que sera usado para registrar erros lexicos e sintaticos.
    explicit LeitorFonte(GerenciadorErros& erros);

    // Abre o arquivo para leitura e depois disso a gente chama proximaLinha() ate o arquivo acabar. Retorna false se nao conseguiu abrir.
    bool abrir(const std::string& caminho);

    // Ele ira fornecer um metodo para ler a proxima linha do arquivo, que sera tokenizada e retornada em uma struct LinhaTokenizada. Caso o arquivo tenha acabado, ele retorna false. 
    bool proximaLinha(LinhaTokenizada& saida);

    // Rebobina o arquivo e zera o estado interno Os erros ja registrados permanecem, porque nao faz sentido apagar o que ja foi reportado.
    void reiniciar();

    // Fecha o arquivo. Chamado no destrutor e pode ser chamado antes de reabrir para outro caminho.
    void fechar();

private:
    // Varre o texto atras de caractere invalido. 
    void checarCaracteres(const std::string& texto, int linha);

    GerenciadorErros& erros;

    std::ifstream arquivo;
    std::string caminho;

    int linhaAtual = 0;

    // Rotulo que apareceu sozinho numa linha e vai ser aplicado a proxima linha util. Fica vazio quando nao ha pendencia.
    std::string rotuloPendente;
    int linhaRotuloPendente = 0;

    // Secao em que as linhas seguintes estao. Atualizada quando o leitor encontra a diretiva SECAO. 
    TipoSecao secaoAtual = TipoSecao::NENHUMA;
};