#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stddef.h>

typedef struct {
    const char *input;
    size_t pos;
    size_t length;
} Lexer;

void lexer_init(Lexer *lexer, const char *input);
Token lexer_next_token(Lexer *lexer);

#endif // LEXER_H
