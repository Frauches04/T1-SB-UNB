#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Tipos.h"

// TabelaSimbolos guarda as entradas (rotulos, constantes de CONST e valores
// de EQU) que o montador vai acumulando ao longo da primeira passagem. A
// consulta e case-insensitive: "Rot", "rot" e "ROT" caem na mesma entrada.
//
// A tabela nao guarda o contador de posicao; quem cuida disso e o Assembler.
// Ela so responde "existe?" e "qual o valor?" e, por isso, serve tanto para
// a primeira passagem (insercao) quanto para a segunda (resolucao).
class TabelaSimbolos {
public:
    TabelaSimbolos();

    // Tenta inserir a entrada. Devolve true se inseriu, false se ja havia um
    // simbolo com o mesmo nome (normalizado). Nao sobrescreve a entrada antiga
    // quando da duplicata: quem chama usa esse false para registrar o erro
    // semantico de rotulo repetido.
    bool inserir(const EntradaTS& entrada);

    // Procura pelo nome e copia a entrada achada pra 'saida'. Devolve false
    // se nao existir. Usado na segunda passagem para resolver operandos e,
    // depois, pelo ligador para achar os simbolos publicos.
    bool buscar(const std::string& nome, EntradaTS& saida) const;

    // Atalho pra quando so interessa saber da existencia, sem precisar de uma
    // EntradaTS de saida. Serve pra checar operandos e pra distinguir rotulo
    // de instrucao na segunda passagem.
    bool existe(const std::string& nome) const;

    // Devolve copias de todas as entradas. Como o map interno ordena por chave,
    // a saida sai em ordem alfabetica e deterministica; isso ajuda na hora de
    // gerar o metadado do ligador (PUBLIC/EXTERN) e nos testes.
    std::vector<EntradaTS> listar() const;

    // Quantidade de entradas. Util pra debug e pra teste.
    std::size_t tamanho() const;

    // Esvazia a tabela. Nao e usada no fluxo normal, mas ajuda a reaproveitar
    // o mesmo objeto em testes unitarios.
    void limpar();

private:
    // Chave em maiusculo, valor com os dados originais. Guardar o nome dentro
    // da EntradaTS tambem e proposital: o ligador vai querer o nome junto do
    // valor na hora de exportar PUBLIC.
    std::map<std::string, EntradaTS> simbolos;

    // Passa a string pra maiuscula (so ASCII). Mesma ideia da normalizacao
    // usada em TabelaInstrucoes; centraliza o case-insensitive num lugar so.
    static std::string normalizar(const std::string& nome);
};