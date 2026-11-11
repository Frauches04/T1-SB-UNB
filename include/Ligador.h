#pragma once

#include <map>
#include <string>
#include <vector>

#include "Erro.h"

// SimboloPublico: simbolo exportado pelo modulo; enderecoLocal e o offset dentro do .OBJ.
struct SimboloPublico {
    std::string nome;
    int enderecoLocal = 0;
};

// UsoExterno: posicao do .OBJ onde aparece um simbolo importado; o ligador substitui pelo global.
struct UsoExterno {
    std::string nome;
    int posicao = 0;
};

// MetaModulo: espelho do .meta (entry, publicos, externos, bitmap de relocacao).
struct MetaModulo {
    int entry = 0;
    bool temEntry = false;
    std::vector<SimboloPublico> publicos;
    std::vector<UsoExterno> externos;
    std::vector<int> relocacao;
};

// Modulo: dados de um .asm ja lido (.OBJ, .meta e offset calculado no layout).
struct Modulo {
    std::string caminhoAsm;
    std::vector<std::string> objeto;
    MetaModulo meta;
    int offset = 0;
};

// Ligador: combina de 1 a 3 .asm em um unico .LIG (uma linha de numeros).
class Ligador {
public:
    explicit Ligador(GerenciadorErros& erros);

    // Ponto de entrada. Devolve true se ligou e escreveu o .LIG.
    bool ligar(const std::vector<std::string>& arquivosAsm,
               const std::string& caminhoSaida);

private:
    // Chama montador.exe sobre o .asm e verifica o exit code.
    bool montarModulo(const std::string& caminhoAsm);

    // Le o .OBJ e o .meta do .asm (o .meta e opcional).
    bool lerModulo(const std::string& caminhoAsm, Modulo& saida);

    // Parse do .meta (linhas ENTRY / D / U / R).
    bool lerMeta(const std::string& caminhoMeta, MetaModulo& saida);

    // Preenche mod.offset de cada modulo com o tamanho acumulado dos anteriores.
    void calcularOffsets(std::vector<Modulo>& modulos);

    // Constroi nome -> endereco global a partir dos D de todos os modulos.
    bool construirTabelaGlobal(const std::vector<Modulo>& modulos,
                               std::map<std::string, int>& global);

    // Resolve externos e reloca internos, concatenando em 'resultado'.
    bool resolver(const std::vector<Modulo>& modulos,
                  const std::map<std::string, int>& global,
                  std::vector<std::string>& resultado);

    // Escreve 'resultado' em uma linha no .LIG.
    bool escreverLig(const std::string& caminho,
                     const std::vector<std::string>& resultado);

    // Troca a extensao do caminho (ex.: .asm -> .OBJ).
    static std::string derivarCaminho(const std::string& caminho,
                                      const std::string& novaExt);


    static std::string normalizar(const std::string& s);


    static bool converterInteiro(const std::string& s, int& saida);

    GerenciadorErros& erros;
};