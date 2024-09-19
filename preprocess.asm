section .data
    TOK_TOTAL db 86
    TOK_HASH db 79
    TOK_DEFINE db 0 ; add to tokenTypes
    TOK_IDENTIFER db 83
    TOK_LITERAL db 84
    TOK_IMMEDIATE db 85
    TOK_SKIP db 87
    DEFINE_STR db 'define', 0
    DEFINE_STR_LEN equ $ - DEFINE_STR
    noMemoryStr db 'No memory...', 0

section .bss
    preProcessHt resq 1 
    SrcTokenStreamTokens resq 1
    SrcTokenStreamAttrTokens resq 1
    currPreProcessToken resq 1 ; points to the curr #define ID IMM / LITERAL <- (the imm / literal token) 
    

section .text
    global preProcess
    extern malloc ; amount of bytes
    extern printf
    extern ht_set ; pointer to hash table, pointer to char, pointer to val
    extern ht_get ; pointer to hash table, pointer to string
    extern ht_create ; default entries to initalize with -1 will set a defautl value
    extern keywordTokenConverter
    extern SrcTokenStream 

    ; TokenStream
    ; 0-7   - Tokens
    ; 8-15  - basicTokenCapacity
    ; 16-23 - basicTokenIndex
    ; 24-31 - attrTokens
    ; 32-39 - attrTokenCapacity
    ; 40-47 - attrTokenIndex

    ; Token
    ; 0-3   - line
    ; 4-7   - col
    ; 8-11  - TokenType
    ; 12-15 - tokenNum

    ; AttrToken
    ; 0-7   - data
    ; 8-11  - TokenType
    ; 12-15 - tokenNum

preProcess:
    push rbp
    mov rbp, rsp

    push rbx ; callee saved register

    mov rbx, SrcTokenStream
    mov qword [SrcTokenStreamTokens], SrcTokenStream ; same address
    add rbx, 24 ; SrcTokenStream.attrTokens
    mov [SrcTokenStreamAttrTokens], rbx

    ; iterate over tokens, find a # with an define keyword following it , store the id and val following the #define into the preProcess hash table
    mov rdi, -1 ; default size for intial value
    call ht_create
    cmp rax, 0
    jne null_check_1

    ; null returned
    mov rdi, noMemoryStr
    call printf
    mov rax, 0 ; error val
    pop rbx
    pop rbp
    ret

null_check_1:

    mov [preProcessHt], rax ; move the preProcess hash table into this pointer
    mov rcx, 0 ; first Token in tokenStream

    ; Tokens[rcx].TokenType != TOK_TOTAL

token_while_loop:
    mov rax, rcx ; get index 
    shl rax, 4 ; correct the size to 16
    add rax, SrcTokenStream ; get curr token
    add rax, 8 ; get tokentype
    mov rdi, rax ; save address (currently pointing to Tokens[rcx].TokenType)
    mov rax, [rax] ; mov the actual val at tokentype address into rax
    cmp [TOK_TOTAL], rax ; cmp TOT_TOTAL to current token.tokentype
    je token_while_loop_end ; break;

    cmp [TOK_HASH], rax ; see if hash
    jne token_while_loop_next_iter ; continue 

    add rdi, 16 ; get next index
    mov rax, [rdi] ; mov next index token type into rax
    cmp [TOK_DEFINE], rax ; see if token define
    jne token_while_loop_next_iter ; / or handle inncorrect usage of #

    add rdi, 16 ; get next index
    mov rax, [rdi] ; mov next index token type into rax
    cmp [TOK_IDENTIFER], rax ; see if next token is identifer
    jne token_while_loop_next_iter ; / or handle inncorrect usage of #define

    add rdi, 16 ; get next index
    mov rax, [rdi] ; mov next index token type into rax
    cmp [TOK_IMMEDIATE], rax ; see if next token is immediate
    je immediate_or_literal
    cmp [TOK_LITERAL], rax ; see if token is literal
    je immediate_or_literal
    jmp token_while_loop_next_iter
immediate_or_literal:
    ; found valid #define IDENTIFER IMM or #define IDENTIFER LITERAL
    ; store in table then set all these tokens to TOK_SKIP

    sub rdi, 12 ; setup pointer
    mov [currPreProcessToken], rdi
    add rdi, 12 ; restore index

    push rcx ; save rcx (caller saved register)
    push rdi ; save rdi (address of Tokens[?].TokenType) <-- right now the token of IMM / LITERAL

    mov rdi, 4 ; allocate 4 bytes 
    call malloc
    cmp rax, 0 ; see if null
    jne null_check_2

    ; null returned
    mov rdi, noMemoryStr
    call printf
    mov rax, 0 ; error val
    pop rdi
    pop rcx
    pop rbx
    pop rbp
    ret    

null_check_2:

    pop rdi ; curr token count Tokens[?].TokenType
    add rdi, 4 ; move to index of imm / literal in Tokens[?].index
    push rdi ; save again
    mov rdi, [rdi] ; get the acutal index here
    mov rbx, SrcTokenStream 
    add rbx, 24 ; get SrcTokenStream.attrTokens
    shl rdi, 4 ; mutiple the index * sizeof(attrToken)
    add rdi, rbx ; SrcTokenStream.attrTokens[?].data
    mov [rax], rdi ; mov the curr token index into malloced memory
    mov rcx, rax ; move into 3rd arg for ht_set

    pop rdi
    sub rdi, 16 ; go back to identifer <-- now of IDENTIFER but index of Identifer
    mov rdi, [rdi] ; get actual index
    mov rbx, SrcTokenStream
    add rbx, 24 ; get SrcTokenStream.attrTokens
    shl rdi, 4 ; mutiple the index * sizeof(attrToken)
    add rdi, rbx ; SrcTokenStream.attrTokens[?].data
    mov rsi, rdi ; move into 2nd arg for ht_set

    mov rdi, preProcessHt ; pointer to ht 1st arg for ht_set 

    call ht_set ; set this #define IDENTIFER LITERAL into hash table

    ; set #, define, id, val -> all to tokenType = TOK_SKIP
    xor rcx, rcx ; 0 out
    mov rdx, currPreProcessToken ; get address of this back
    add rdx, 12 ; get to tokenType part
    mov rax, [TOK_SKIP] ; thing to fill
set_token_skip:
    cmp rcx, 3 ; amount to do (0 , 1 , 2 , 3 (4 times))
    je set_token_skip_out
    mov [rdx], rax ; move TOK_SKIP into curr token.tokentype
    sub rdx, 16 ; move to before token
    INC rcx ; rcx++
    jmp set_token_skip
set_token_skip_out:

    pop rcx ; doing this to offset push and pops, we are about to set it manually
    add rcx, 3 ; only 3 not 4 because we are about to increment anyways
token_while_loop_next_iter:
    inc rcx ; increment to next index
    jmp token_while_loop
token_while_loop_end:

    pop rbx ; callee saved
    pop rbp
    ret
