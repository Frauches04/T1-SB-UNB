#include "Ligador.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
// No Windows, std::system ja devolve o exit code.
#else
#include <sys/wait.h>
#endif

namespace {

// Interpreta o retorno de std::system (exit code no Windows, WEXITSTATUS no POSIX).
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
    // A especificacao aceita de 1 a 3 arquivos.
    if (arquivosAsm.empty() || arquivosAsm.size() > 3) {
        erros.registrar(0, TipoErro::SINTATICO,
                        "o ligador aceita de 1 a 3 arquivos .asm");
        return false;
    }

    // Monta cada .asm; aborta no primeiro erro.
    for (const std::string& arquivoAsm : arquivosAsm) {
        if (!montarModulo(arquivoAsm)) {
            return false;
        }
    }

    // Le .OBJ e .meta de cada modulo.
    std::vector<Modulo> modulos;
    modulos.reserve(arquivosAsm.size());
    for (const std::string& arquivoAsm : arquivosAsm) {
        Modulo mod;
        if (!lerModulo(arquivoAsm, mod)) {
            return false;
        }
        modulos.push_back(mod);
    }

    // Layout: offset cumulativo por modulo (tamanho total do .OBJ).
    calcularOffsets(modulos);

    // Tabela global nome -> endereco absoluto.
    std::map<std::string, int> global;
    if (!construirTabelaGlobal(modulos, global)) {
        return false;
    }

    // Resolve externos e reloca internos.
    std::vector<std::string> resultado;
    if (!resolver(modulos, global, resultado)) {
        return false;
    }

    return escreverLig(caminhoSaida, resultado);
}

bool Ligador::montarModulo(const std::string& caminhoAsm) {
    // Aspas para suportar diretorios com espaco.
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

    // Le o .OBJ (o >> do ifstream ja separa por espaco/tab/quebra).
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

    // Le o .meta se existir; modulo sem .meta nao tem simbolos nem relocacao.
    std::string caminhoMeta = derivarCaminho(caminhoAsm, "meta");
    std::ifstream arquivoMeta(caminhoMeta);
    if (!arquivoMeta.is_open()) {
        return true;
    }
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
        // Pula linhas em branco.
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
        // Linha desconhecida: ignora para permitir evolucao do formato.
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
                // Nome publico duplicado entre modulos.
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
        // Copia para nao alterar o original.
        std::vector<std::string> objeto = mod.objeto;

        // Mapa posicao -> nome externo, para distinguir interno de externo.
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
                // Uso externo: substitui pelo endereco global.
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
                // Endereco interno: soma offset do modulo.
                int valorAtual = 0;
                if (!converterInteiro(objeto[p], valorAtual)) {
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
        // Sem extensao: so anexa.
        return caminho + "." + novaExt;
    }
    // find_last_of respeita diretorios com ponto no nome.
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