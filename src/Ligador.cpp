#include "Ligador.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
// No Windows, std::system devolve o exit code do comando direto.
// Nao precisamos de wait.h.
#else
#include <sys/wait.h>
#endif

namespace {

// Interpreta o valor devolvido por std::system. No Windows, e o
// proprio exit code. No POSIX, precisa passar por WEXITSTATUS.
// A checagem de WIFEXITED evita que um sinal (segfault, por
// exemplo) seja confundido com sucesso.
bool comandoTerminouBem(int rc) {
#ifdef _WIN32
    return rc == 0;
#else
    return rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) == 0;
#endif
}

}  // namespace

Ligador::Ligador(GerenciadorErros& e) : erros(e) {
}

bool Ligador::ligar(const std::vector<std::string>& arquivosAsm,
                    const std::string& caminhoSaida) {
    // A especificacao fixa de 1 a 3 arquivos. Menos que 1 nao faz
    // sentido (nada pra ligar); mais que 3 tambem nao, porque o
    // professor quer comparar contra um gabarito de 3 modulos.
    if (arquivosAsm.empty() || arquivosAsm.size() > 3) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "o ligador aceita de 1 a 3 arquivos .asm");
        return false;
    }

    // Primeiro, montar cada .asm. Se qualquer um falhar, paramos:
    // nao adianta tentar ligar modulos incompletos.
    for (const std::string& arquivoAsm : arquivosAsm) {
        if (!montarModulo(arquivoAsm)) {
            return false;
        }
    }

    // Depois de montar, ler .OBJ e .meta de cada um. Se algum
    // arquivo nao existir (por exemplo, o montador saiu bem mas nao
    // escreveu o .meta por bug), reportamos e paramos.
    std::vector<Modulo> modulos;
    modulos.reserve(arquivosAsm.size());
    for (const std::string& arquivoAsm : arquivosAsm) {
        Modulo mod;
        if (!lerModulo(arquivoAsm, mod)) {
            return false;
        }
        modulos.push_back(mod);
    }

    // Layout: cada modulo ganha um offset cumulativo. O primeiro
    // comeca em 0; o segundo em tamanho(modulo[0]); e assim por
    // diante. Usamos o tamanho total do .OBJ (codigo + dados), nao
    // so o codigo, porque o .OBJ que o montador entrega ja tem os
    // dados no final e queremos que o proximo modulo comece depois
    // deles.
    calcularOffsets(modulos);

    // Tabela global: nome -> endereco absoluto. Construida a partir
    // dos D de todos os .meta, somando o offset do modulo. Se
    // aparecer o mesmo nome em dois modulos, e erro (duas
    // definicoes do mesmo publico).
    std::map<std::string, int> global;
    if (!construirTabelaGlobal(modulos, global)) {
        return false;
    }

    // Resolucao: percorre o bitmap de relocacao de cada modulo,
    // trocando uso externo por endereco global e ajustando os
    // internos. O resultado concatenado e o corpo do .LIG.
    std::vector<std::string> resultado;
    if (!resolver(modulos, global, resultado)) {
        return false;
    }

    return escreverLig(caminhoSaida, resultado);
}

bool Ligador::montarModulo(const std::string& caminhoAsm) {
    // O comando tem o caminho entre aspas pra suportar diretorios
    // com espaco. O montador esta no diretorio atual (junto do
    // ligador) ou no PATH; std::system procura nos dois.
    std::string comando = "montador.exe \"" + caminhoAsm + "\"";

    int rc = std::system(comando.c_str());
    if (!comandoTerminouBem(rc)) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "falha ao montar '" + caminhoAsm +
                        "' (verifique o .asm e se montador.exe esta no PATH)");
        return false;
    }
    return true;
}

bool Ligador::lerModulo(const std::string& caminhoAsm, Modulo& saida) {
    saida = Modulo{};
    saida.caminhoAsm = caminhoAsm;

    // Ler o .OBJ (uma linha de numeros). O operador >> do ifstream
    // separa por qualquer espaco/tab/quebra, entao o formato "uma
    // linha com espacos" e "varias linhas" dao no mesmo.
    std::string caminhoObj = derivarCaminho(caminhoAsm, "OBJ");
    std::ifstream arquivoObj(caminhoObj);
    if (!arquivoObj.is_open()) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel abrir o objeto '" + caminhoObj + "'");
        return false;
    }
    std::string token;
    while (arquivoObj >> token) {
        saida.objeto.push_back(token);
    }

    // Ler o .meta se existir. Se nao existir, o modulo nao tem
    // simbolos nem relocacao - isso e valido para programa de um
    // modulo so, sem PUBLIC/EXTERN/BEGIN, em que nenhum endereco
    // precisa de ajuste. Nesse caso, tudo bem.
    std::string caminhoMeta = derivarCaminho(caminhoAsm, "meta");
    std::ifstream arquivoMeta(caminhoMeta);
    if (!arquivoMeta.is_open()) {
        // Nao e erro: modulo sem metadados.
        return true;
    }
    // Delegamos o parse a lerMeta; aqui ja temos o stream aberto,
    // mas pra manter a funcao simples, fechamos e chamamos de novo
    // pelo caminho. Fica um open a mais, custo desprezivel.
    arquivoMeta.close();
    return lerMeta(caminhoMeta, saida.meta);
}

bool Ligador::lerMeta(const std::string& caminhoMeta, MetaModulo& saida) {
    std::ifstream arquivo(caminhoMeta);
    if (!arquivo.is_open()) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel abrir o meta '" + caminhoMeta + "'");
        return false;
    }

    std::string linha;
    while (std::getline(arquivo, linha)) {
        // Remove o \r de arquivos gerados no Windows.
        if (!linha.empty() && linha.back() == '\r') {
            linha.pop_back();
        }
        // Pula linhas em branco (separadores que o montador pode
        // emitir entre blocos).
        if (linha.empty()) {
            continue;
        }

        std::istringstream iss(linha);
        std::string tipo;
        iss >> tipo;
        if (tipo == "ENTRY") {
            int valor = 0;
            if (iss >> valor) {
                saida.entry = valor;
                saida.temEntry = true;
            }
        } else if (tipo == "D") {
            SimboloPublico sp;
            if (iss >> sp.nome >> sp.enderecoLocal) {
                saida.publicos.push_back(sp);
            }
        } else if (tipo == "U") {
            UsoExterno ue;
            if (iss >> ue.nome >> ue.posicao) {
                saida.externos.push_back(ue);
            }
        } else if (tipo == "R") {
            int bit = 0;
            while (iss >> bit) {
                saida.relocacao.push_back(bit);
            }
        }
        // Linha desconhecida: ignora em vez de falhar, pra
        // permitir que o formato do .meta evolua sem quebrar a
        // leitura (comentarios futuros, por exemplo).
    }
    return true;
}

void Ligador::calcularOffsets(std::vector<Modulo>& modulos) {
    int acumulado = 0;
    for (Modulo& mod : modulos) {
        mod.offset = acumulado;
        acumulado += static_cast<int>(mod.objeto.size());
    }
}

bool Ligador::construirTabelaGlobal(const std::vector<Modulo>& modulos,
                                    std::map<std::string, int>& global) {
    for (const Modulo& mod : modulos) {
        for (const SimboloPublico& sp : mod.meta.publicos) {
            std::string chave = normalizar(sp.nome);
            int endereco = mod.offset + sp.enderecoLocal;

            auto it = global.find(chave);
            if (it != global.end()) {
                // Mesmo nome publico em dois modulos: a especificacao
                // nao preve o caso, entao tratamos como erro semantico
                // pra o usuario perceber e corrigir.
                erros.registrar(0, TipoErro::SEMANTICO,
                                "simbolo publico definido mais de uma vez: '" +
                                sp.nome + "'");
                return false;
            }
            global[chave] = endereco;
        }
    }
    return true;
}

bool Ligador::resolver(const std::vector<Modulo>& modulos,
                       const std::map<std::string, int>& global,
                       std::vector<std::string>& resultado) {
    for (const Modulo& mod : modulos) {
        // Copia pra poder modificar sem sujar o original (util se
        // um dia quisermos processar duas vezes).
        std::vector<std::string> objeto = mod.objeto;

        // Indice posicao -> nome do simbolo externo. Permite, ao
        // percorrer o bitmap, saber se uma posicao relocavel e
        // interna (soma offset) ou externa (consulta global).
        std::map<int, std::string> externoPorPosicao;
        for (const UsoExterno& ue : mod.meta.externos) {
            externoPorPosicao[ue.posicao] = normalizar(ue.nome);
        }

        for (std::size_t p = 0; p < mod.meta.relocacao.size() &&
                                p < objeto.size(); ++p) {
            if (mod.meta.relocacao[p] != 1) {
                continue;
            }

            auto itExt = externoPorPosicao.find(static_cast<int>(p));
            if (itExt != externoPorPosicao.end()) {
                auto itGlobal = global.find(itExt->second);
                if (itGlobal == global.end()) {
                    erros.registrar(0, TipoErro::SEMANTICO,
                                    "simbolo externo nao resolvido: '" +
                                    itExt->second + "' em '" +
                                    mod.caminhoAsm + "'");
                    return false;
                }
                objeto[p] = std::to_string(itGlobal->second);
            } else {
                int valorAtual = 0;
                if (!converterInteiro(objeto[p], valorAtual)) {
                    // Posicao marcada como relocavel mas com valor
                    // nao-numerico. So acontece se o montador gerar
                    // .OBJ malformado; reporta e segue (o proximo
                    // teste vai pegar de qualquer jeito).
                    erros.registrar(0, TipoErro::SINTATICO,
                                    "valor nao numerico em posicao relocavel do modulo '" +
                                    mod.caminhoAsm + "'");
                    return false;
                }
                objeto[p] = std::to_string(valorAtual + mod.offset);
            }
        }

        for (const std::string& s : objeto) {
            resultado.push_back(s);
        }
    }
    return true;
}

bool Ligador::escreverLig(const std::string& caminho,
                          const std::vector<std::string>& resultado) {
    std::ofstream arquivo(caminho);
    if (!arquivo.is_open()) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "nao foi possivel criar '" + caminho + "'");
        return false;
    }
    for (std::size_t i = 0; i < resultado.size(); ++i) {
        if (i > 0) {
            arquivo << ' ';
        }
        arquivo << resultado[i];
    }
    arquivo << '\n';
    return true;
}

std::string Ligador::derivarCaminho(const std::string& caminho,
                                    const std::string& novaExt) {
    std::size_t pos = caminho.find_last_of('.');
    if (pos == std::string::npos) {
        // Sem extensao: so anexa. Acontece se o usuario passar
        // "mod1" em vez de "mod1.asm".
        return caminho + "." + novaExt;
    }
    // O find_last_of respeita diretorios com ponto no nome
    // ("meu.projeto/mod1.asm" corta no ponto de .asm, nao no de
    // "meu.projeto"). O caminho completo (com diretorio) e
    // preservado; so a extensao muda.
    return caminho.substr(0, pos) + "." + novaExt;
}

std::string Ligador::normalizar(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        r.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(c))));
    }
    return r;
}

bool Ligador::converterInteiro(const std::string& s, int& saida) {
    if (s.empty()) {
        return false;
    }
    try {
        std::size_t consumido = 0;
        int valor = std::stoi(s, &consumido);
        if (consumido != s.size()) {
            return false;
        }
        saida = valor;
        return true;
    } catch (...) {
        return false;
    }
}