; +===========+
; ERROS BÁSICOS
; +===========+

; erros léxicos
;         STORE   -DATA
;         STORE   DATA-
; ISSO:   ISSO&
; DOIS:   +OPA
; TRES:   OPA+    2

; acertos léxicos
; OPA:    _OPA_           ; 13813 98#*($!&*$!*$*#&$ !$ !)
; LABEL:  STORE   ISSO + 2
;         begin
;         COPY    AAA     BBB
;         Output  LABEL


; erros sintáticos
;         STORE
; L1:     OUTPUT
; L2:     Y       + 2 + 2
; L3:     BEGIN   TOP
;         STORE   Y EQU 2
;         COPY    ONE
;         COPY    ONE     ONE  ONE
;         COPY    2 +     ONE  ONE
;         STORE   2 +     ONE
;         PRINT   ONE                   ; instrução/diretiva iválida
;         CONNECT_TO_THE_WIRED          ; instrução/diretiva iválida

; acertos sintáticos
;         COPY    ONE     ONE  + 2
;         COPY    ONE + 2 ONE
; L1:     OUTPUT  ONE
;         STOP


; erros semânticos

; 1 diretiva de alocação (SPACE, CONST) em seção de dado
; SECTION TEXT
; L1:     OUTPUT ONE
;         STOP

; ONE:    CONST 1

; 2 rótulo/label ausente
; SECTION TEXT
; L1:     OUTPUT ONE
;         STOP
;
; SECTION DATA
;         CONST 1

; 3 rótulo/label repetido
; SECTION TEXT
; L1:     OUTPUT ONE
; L1:     STOP
;
; SECTION DATA
; ONE:    CONST 1
; ONE:    CONST 2

; 4 seção de texto ausente (poderia ser sintático? acho que não)
;         STOP

; 5 declaração de sinônimo com EQU sem ser antes das duas seções
; SECTION TEXT
;         lain    EQU     1
;         OUTPUT  ONE
;         STOP

; 6 módulos com definições faltando
; arquivo mod_a.asm
; SECTION TEXT
; MOD_A:  BEGIN
; MOD_B:  EXTERN
;         PUBLIC  L1
;         LOAD    ZERO
;         ADD     ONE
;         JUMP    MOD_B
; L1:     STOP
;         END
; SECTION DATA
; ZERO:   CONST   0

; arquivo mod_b.asm
; SECTION TEXT
; MOD_B:  BEGIN
; L1:     EXTERN
;         ; falta PUBLIC MOD_B
;         ; falta PUBLIC ONE
;         ADD     ONE
;         JMPP    L1
;         END
; SECTION DATA
; ONE:    CONST   1

; +=============+
; ERROS COMPLEXOS
; +=============+

; ! atenção !   dependendo pode ser que esse não seja um erro usar ":" assim
;               acho que ser léxico ou semântico depende da implementação
; UM :    DOIS


; erros sintáticos complexos
;         COPY    DATA    TEXT
; STORE:  DATA          ; se vc considerar que DATA e TEXT são palavras reservadas

; acertos semânticos complexos (bem exagerado)
; arquivo mod_a.asm
; DEF     EQU     1
; SECTION TEXT
; MOD_A:  BEGIN
; MOD_B:  EXTERN
;         PUBLIC  L1
;         INPUT   X
;         LOAD    X
;         IF      DEF   ; se DEF != 0, ACC = input() + 1
;         ADD     ONE
;         IF      NDEF  ; se NDEF != 0, ACC = input() - 1
;         SUB     ONE
;         END
; SECTION DATA
; X:      SPACE   2
; ONE:    CONST   1

; arquivo mod_b.asm
; NDEF    EQU     0
; SECTION TEXT
; MOD_B:  BEGIN
; MOD_A:  EXTERN
; X:      EXTERN
;         PUBLIC  MOD_B
;         STORE   X
;         JMP     MOD_A
;         END
