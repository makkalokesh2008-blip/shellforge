#define _POSIX_C_SOURCE 200809L
#include "token.h"
#include <stdlib.h>
#include <string.h>

Token create_token(TokenType type, const char *value) {
    Token token;
    token.type = type;
    if (value != NULL) {
        token.value = strdup(value);
    } else {
        token.value = NULL;
    }
    return token;
}

void free_token(Token *token) {
    if (token->value != NULL) {
        free(token->value);
        token->value = NULL;
    }
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_WORD:            return "WORD";
        case TOKEN_PIPE:            return "PIPE";
        case TOKEN_REDIRECT_IN:     return "REDIRECT_IN";
        case TOKEN_REDIRECT_OUT:    return "REDIRECT_OUT";
        case TOKEN_REDIRECT_APPEND: return "REDIRECT_APPEND";
        case TOKEN_AMPERSAND:       return "AMPERSAND";
        case TOKEN_END:             return "END";
        case TOKEN_ERROR:           return "ERROR";
        default:                    return "UNKNOWN";
    }
}
