#include "Fonte.h"

#include <cctype>
#include <vector>

namespace {

// Limite da especificacao para nomes de rotulo. O enunciado garante
// que ninguem vai passar disso, mas o leitor checa para dar mensagem
// decente caso aconteca.
const std::size_t TAMANHO_MAX_IDENTIFICADOR = 30;

// Remove espacos e tabs das duas pontas. Usada em varios pontos, por
// isso ficou no anonimo em vez de dentro da classe.
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

// Separa a linha em texto util e comentario. O ';' marca o inicio
// do comentario; o resto e descartado pela montagem, mas guardamos
// por conveniencia (debug, testes).
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

// Verifica se o nome segue a regra do enunciado: comeca com letra ou
// underscore, depois so letra, digito ou underscore, ate 30 chars.
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

// Tenta extrair um rotulo no inicio do texto. O rotulo e a parte antes
// do primeiro ':' e precisa ser um token unico (sem espaco). Devolve
// true e preenche 'rotulo' e 'resto' quando consegue.
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
    // Se tem espaco antes dos dois-pontos, nao e rotulo, e um operando
    // qualquer com ':' dentro (por exemplo, algo malformado).
    if (candidato.find_first_of(" \t") != std::string::npos) {
        return false;
    }
    rotulo = candidato;
    resto = texto.substr(pos + 1);
    return true;
}

// Quebra o texto por espaco/tab, descartando tokens vazios. Nao usa
// stringstream para deixar explicito o que consideramos separador.
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

// Junta tokens que formam uma expressao com '+': "X", "+", "2" vira
// "X+2". Sem isso, "ADD X + 2" seria interpretado como tres operandos,
// quando o enunciado quer um so (endereco X mais 2). A juncao respeita
// a ordem: sempre pega o token anterior mais o '+' mais o proximo.
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

// Passa para maiusculo (so ASCII). Usado para comparar nomes de secao
// sem depender da caixa que o usuario digitou.
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
        // seekg(0) volta ao inicio sem fechar o arquivo. O clear()
        // antes e necessario porque apos getline falhar no fim do
        // arquivo o stream fica com a flag eofbit setada, e seekg
        // sozinho nao limpa isso.
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

        // Tira o \r do fim caso o arquivo tenha vindo do Windows. Sem
        // isso, o ultimo token de cada linha fica com um \r invisivel.
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

        // Dois rotulos seguidos: o da linha anterior ficou pendente e
        // esta linha tambem traz um. Nao ha operacao para o pendente.
        if (tinhaRotulo && !rotuloPendente.empty()) {
            erros.registrar(linhaRotuloPendente, TipoErro::SINTATICO,
                            "rotulo '" + rotuloPendente + "' sem operacao");
            rotuloPendente.clear();
        }

        // Se a linha nao trouxe rotulo mas havia um pendente, aplica.
        if (!tinhaRotulo && !rotuloPendente.empty()) {
            rotulo = rotuloPendente;
            rotuloPendente.clear();
        }

        resto = aparar(resto);

        if (resto.empty()) {
            if (tinhaRotulo) {
                // Linha so com rotulo. Guarda para a proxima.
                rotuloPendente = rotulo;
                linhaRotuloPendente = linhaAtual;
            }
            // Se era so ":", nao faz sentido nenhum; ignora.
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

        // Atualiza a secao corrente depois de preencher saida.secao.
        // Assim a linha da SECAO fica com a secao anterior, e a troca
        // vale a partir da linha seguinte.
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

    // Fim do arquivo. Se sobrou rotulo pendente, foi um rotulo que
    // ficou sem operacao; reporta e nao devolve linha.
    if (!rotuloPendente.empty()) {
        erros.registrar(linhaRotuloPendente, TipoErro::SINTATICO,
                        "rotulo '" + rotuloPendente + "' sem operacao");
        rotuloPendente.clear();
    }
    return false;
}