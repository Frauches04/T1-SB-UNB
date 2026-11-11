#pragma once

#include <string>
#include <vector>

 //Identifica o tipo de erro
enum class TipoErro {
    LEXICO,     
    SINTATICO,  
    SEMANTICO   
};

//Valor do tipo de secao em que a linha caiu. OBS: NENHUMA significa que a linha
//nao esta dentro de nenhuma secao (ex: antes de "SECTION TEXT" ou depois de "SECTION DATA").
enum class TipoSecao {
    NENHUMA,
    DADOS,
    TEXTO 
};

// Diferencia o que cada entrada da TS representa entre rotulo(padrão), constante ou EQU.
enum class TipoSimbolo {
    ROTULO,
    CONSTANTE,
    EQU
};

// Revisão dos componentes de uma linha de codigo, para que seja possivel separar cada parte da linha e tratar de forma mais organizada.
struct LinhaTokenizada {
    int numeroLinha = 0;                
    std::string rotulo;                  
    std::string operacao;                
    std::vector<std::string> operandos; 
    std::string comentario;              
    TipoSecao secao = TipoSecao::NENHUMA; 
};

// Entrada da Tabela de Simbolos para as duas passagens do montador. O valor e o tipo sao preenchidos na primeira passagem, e
// o resto das flags na segunda. O valor e o tipo sao usados para gerar o .OBJ.
struct EntradaTS {
    std::string nome;
    int valor = 0;                       
    TipoSimbolo tipo = TipoSimbolo::ROTULO;

    
    bool ePublico = false;
    bool eExterno = false;
};