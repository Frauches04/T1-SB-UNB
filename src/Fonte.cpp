#include "Fonte.h"

#include <cctype>
#include <vector>

namespace {

// Limite de 30 caracteres para identificadores.
const std::size_t TAMANHO_MAX_IDENTIFICADOR = 30;

// Remove espacos e tabs das duas pontas.
std::string aparar(const std::string& s) {
    std::size_t i = 0;
    std::size_t j = s.size();
    while (i < j && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
    while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1]))) {
        --j;
    }
    return s.substr(i, j - i);
}

// Separa a linha em texto util e comentario (a partir do ';').
void separarComentario(const std::string& linha,
                       std::string& semComentario,
                       std::string& comentario) {
    std::size_t pos = linha.find(';');
    if (pos == std::string::npos) {
        semComentario = linha;
        comentario.clear();
    } else {
        semComentario = linha.substr(0, pos);
        comentario = linha.substr(pos + 1);
    }
}

// Confere se o nome segue a regra C: letra/_ inicial, depois letra/digito/_, ate 30.
bool identificadorValido(const std::string& nome) {
    if (nome.empty() || nome.size() > TAMANHO_MAX_IDENTIFICADOR) {
        return false;
    }
    char primeiro = nome[0];
    if (!(std::isalpha(static_cast<unsigned char>(primeiro)) || primeiro == '_')) {
        return false;
    }
    for (std::size_t i = 1; i < nome.size(); ++i) {
        char c = nome[i];
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) {
            return false;
        }
    }
    return true;
}

// Extrai o rotulo antes do primeiro ':' e devolve o resto da linha.
bool extrairRotulo(const std::string& texto,
                   std::string& rotulo,
                   std::string& resto) {
    std::size_t pos = texto.find(':');
    if (pos == std::string::npos) {
        return false;
    }
    std::string candidato = aparar(texto.substr(0, pos));
    if (candidato.empty()) {
        return false;
    }
    // Se tem espaco antes dos dois-pontos, nao e rotulo.
    if (candidato.find_first_of(" \t") != std::string::npos) {
        return false;
    }
    rotulo = candidato;
    resto = texto.substr(pos + 1);
    return true;
}

// Quebra o texto por espaco/tab, descartando tokens vazios.
std::vector<std::string> dividirPorEspaco(const std::string& texto) {
    std::vector<std::string> tokens;
    std::string atual;
    for (char c : texto) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!atual.empty()) {
                tokens.push_back(atual);
                atual.clear();
            }
        } else {
            atual.push_back(c);
        }
    }
    if (!atual.empty()) {
        tokens.push_back(atual);
    }
    return tokens;
}

// Junta "X", "+", "2" em "X+2" para o caso ADD X + 2.
std::vector<std::string> juntarSomas(const std::vector<std::string>& tokens) {
    std::vector<std::string> resultado;
    std::size_t i = 0;
    while (i < tokens.size()) {
        if (tokens[i] == "+" && !resultado.empty() && i + 1 < tokens.size()) {
            resultado.back() += "+" + tokens[i + 1];
            i += 2;
        } else {
            resultado.push_back(tokens[i]);
            ++i;
        }
    }
    return resultado;
}

// Passa para maiusculo (so ASCII).
std::string normalizar(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        r.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return r;
}

}  // namespace

LeitorFonte::LeitorFonte(GerenciadorErros& g) : erros(g) {
}

bool LeitorFonte::abrir(const std::string& c) {
    fechar();
    caminho = c;
    arquivo.open(c);
    if (!arquivo.is_open()) {
        return false;
    }
    linhaAtual = 0;
    rotuloPendente.clear();
    linhaRotuloPendente = 0;
    secaoAtual = TipoSecao::NENHUMA;
    return true;
}

void LeitorFonte::reiniciar() {
    if (!arquivo.is_open() && !caminho.empty()) {
        arquivo.open(caminho);
    } else {
        // clear() limpa o eofbit antes do seekg(0).
        arquivo.clear();
        arquivo.seekg(0);
    }
    linhaAtual = 0;
    rotuloPendente.clear();
    linhaRotuloPendente = 0;
    secaoAtual = TipoSecao::NENHUMA;
}

void LeitorFonte::fechar() {
    if (arquivo.is_open()) {
        arquivo.close();
    }
}

void LeitorFonte::checarCaracteres(const std::string& texto, int linha) {
    for (std::size_t i = 0; i < texto.size(); ++i) {
        char c = texto[i];
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || std::isspace(uc)) {
            continue;
        }
        if (c == '_' || c == ':' || c == '+' || c == '-') {
            continue;
        }
        std::string msg = "caractere invalido '";
        msg += c;
        msg += "'";
        erros.registrar(linha, TipoErro::LEXICO, msg);
        return;
    }
}

bool LeitorFonte::proximaLinha(LinhaTokenizada& saida) {
    std::string linhaBruta;
    while (std::getline(arquivo, linhaBruta)) {
        ++linhaAtual;

        // Remove o \r de fim de linha vindo do Windows.
        if (!linhaBruta.empty() && linhaBruta.back() == '\r') {
            linhaBruta.pop_back();
        }

        std::string semComentario, comentario;
        separarComentario(linhaBruta, semComentario, comentario);
        std::string texto = aparar(semComentario);

        if (texto.empty()) {
            continue;
        }

        checarCaracteres(texto, linhaAtual);

        std::string rotulo;
        std::string resto;
        bool tinhaRotulo = extrairRotulo(texto, rotulo, resto);
        if (!tinhaRotulo) {
            resto = texto;
        }

        if (tinhaRotulo && !identificadorValido(rotulo)) {
            erros.registrar(linhaAtual, TipoErro::LEXICO,
                            "identificador de rotulo invalido: '" + rotulo + "'");
            continue;
        }

        // Dois rotulos seguidos: o pendente anterior fica sem operacao.
        if (tinhaRotulo && !rotuloPendente.empty()) {
            erros.registrar(linhaRotuloPendente, TipoErro::SINTATICO,
                            "rotulo '" + rotuloPendente + "' sem operacao");
            rotuloPendente.clear();
        }

        // Aplica rotulo pendente se a linha atual nao trouxe um.
        if (!tinhaRotulo && !rotuloPendente.empty()) {
            rotulo = rotuloPendente;
            rotuloPendente.clear();
        }

        resto = aparar(resto);

        if (resto.empty()) {
            if (tinhaRotulo) {
                // Linha so com rotulo: guarda para a proxima.
                rotuloPendente = rotulo;
                linhaRotuloPendente = linhaAtual;
            }
            continue;
        }

        std::vector<std::string> tokens = dividirPorEspaco(resto);
        if (tokens.empty()) {
            continue;
        }

        saida.numeroLinha = linhaAtual;
        saida.rotulo = rotulo;
        saida.operacao = tokens[0];
        if (tokens.size() > 1) {
            std::vector<std::string> opTokens(tokens.begin() + 1, tokens.end());
            saida.operandos = juntarSomas(opTokens);
        } else {
            saida.operandos.clear();
        }
        saida.comentario = aparar(comentario);
        saida.secao = secaoAtual;

        // A troca de secao so vale para a linha seguinte.
        std::string opUpper = normalizar(saida.operacao);
        if ((opUpper == "SECAO" || opUpper == "SECTION") && !saida.operandos.empty()) {
            std::string secaoOp = normalizar(saida.operandos[0]);
            if (secaoOp == "DATA" || secaoOp == "DADOS") {
                secaoAtual = TipoSecao::DADOS;
            } else if (secaoOp == "TEXT" || secaoOp == "TEXTO") {
                secaoAtual = TipoSecao::TEXTO;
            }
        }

        return true;
    }

    // EOF: rotulo pendente sem operacao e erro.
    if (!rotuloPendente.empty()) {
        erros.registrar(linhaRotuloPendente, TipoErro::SINTATICO,
                        "rotulo '" + rotuloPendente + "' sem operacao");
        rotuloPendente.clear();
    }
    return false;
}