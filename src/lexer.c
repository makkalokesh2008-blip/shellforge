#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void lexer_init(Lexer *lexer, const char *input) {
    lexer->input = input;
    lexer->pos = 0;
    lexer->length = input ? strlen(input) : 0;
}

static char lexer_peek(Lexer *lexer) {
    if (lexer->pos >= lexer->length) {
        return '\0';
    }
    return lexer->input[lexer->pos];
}

static char lexer_advance(Lexer *lexer) {
    char c = lexer_peek(lexer);
    if (c != '\0') {
        lexer->pos++;
    }
    return c;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->length && isspace((unsigned char)lexer->input[lexer->pos])) {
        lexer->pos++;
    }
}

Token lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);

    char c = lexer_peek(lexer);
    if (c == '\0') {
        return create_token(TOKEN_END, "END");
    }

    if (c == '|') {
        lexer_advance(lexer);
        return create_token(TOKEN_PIPE, "|");
    }

    if (c == '&') {
        lexer_advance(lexer);
        return create_token(TOKEN_AMPERSAND, "&");
    }

    if (c == '<') {
        lexer_advance(lexer);
        return create_token(TOKEN_REDIRECT_IN, "<");
    }

    if (c == '>') {
        lexer_advance(lexer);
        if (lexer_peek(lexer) == '>') {
            lexer_advance(lexer);
            return create_token(TOKEN_REDIRECT_APPEND, ">>");
        }
        return create_token(TOKEN_REDIRECT_OUT, ">");
    }

    // Handle quoted strings
    if (c == '"' || c == '\'') {
        char quote = c;
        lexer_advance(lexer); // Consume opening quote
        size_t start = lexer->pos;
        while (lexer->pos < lexer->length && lexer->input[lexer->pos] != quote) {
            lexer->pos++;
        }
        size_t len = lexer->pos - start;
        char *val = malloc(len + 1);
        if (val) {
            memcpy(val, lexer->input + start, len);
            val[len] = '\0';
        }
        if (lexer_peek(lexer) == quote) {
            lexer_advance(lexer); // Consume closing quote
        }
        Token t = create_token(TOKEN_WORD, val);
        free(val);
        return t;
    }

    // Handle regular words
    size_t start = lexer->pos;
    while (lexer->pos < lexer->length) {
        char next = lexer->input[lexer->pos];
        if (isspace((unsigned char)next) || next == '|' || next == '&' || next == '<' || next == '>' || next == '"' || next == '\'') {
            break;
        }
        lexer->pos++;
    }
    size_t len = lexer->pos - start;
    char *val = malloc(len + 1);
    if (val) {
        memcpy(val, lexer->input + start, len);
        val[len] = '\0';
    }
    Token t = create_token(TOKEN_WORD, val);
    free(val);
    return t;
}
