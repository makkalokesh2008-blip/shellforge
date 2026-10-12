#include "common.h"
#include "sysnum.h"

#define MAX_LINE        128
#define MAX_TOKENS      32
#define MAX_TOKEN_LEN   64
#define MAX_ARGS        16

long sys_read(void *buf, unsigned long len);
long sys_write(const void *buf, unsigned long len);
long sys_fork(void);
void sys_exit(int status);
long sys_wait(void);
long sys_getpid(void);
long sys_sleep(unsigned long ticks);
long sys_ps(void);
long sys_ls(void);
long sys_cat(const char *name);

typedef enum {
    TOK_END = 0,
    TOK_WORD,
    TOK_PIPE,         /* | */
    TOK_REDIR_IN,     /* < */
    TOK_REDIR_OUT,    /* > */
    TOK_REDIR_APPEND, /* >> */
    TOK_AMPERSAND,    /* & */
    TOK_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    char val[MAX_TOKEN_LEN];
} token_t;

typedef struct {
    int argc;
    char *argv[MAX_ARGS];
    char redir_in[MAX_TOKEN_LEN];
    char redir_out[MAX_TOKEN_LEN];
    int append;
    int background;
    int has_pipe;
} command_t;

static void print(const char *s)
{
    unsigned long n = 0;
    while (s[n]) ++n;
    sys_write(s, n);
}

static void print_num(long x)
{
    char b[32];
    int i = 0;
    if (x == 0) { print("0"); return; }
    if (x < 0) { print("-"); x = -x; }
    while (x && i < 31) { b[i++] = (char)('0' + (x % 10)); x /= 10; }
    while (i) {
        char c = b[--i];
        sys_write(&c, 1);
    }
}

static int streq(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == 0 && b[i] == 0;
}

static void str_copy(char *dst, const char *src, int max)
{
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static unsigned long parse_num(const char *s)
{
    unsigned long x = 0;
    int i = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        x = x * 10 + (unsigned long)(s[i] - '0');
        ++i;
    }
    return x;
}

/*
 * LEXER / TOKENIZER
 * Breaks input line into discrete tokens.
 * Handles single quotes ('...'), double quotes ("..."), operators, and whitespace.
 */
static int tokenize(const char *line, token_t *tokens, int max_tokens, char *err_msg, int err_max)
{
    const char *p = line;
    int ntok = 0;

    while (*p) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;

        if (ntok >= max_tokens - 1) {
            str_copy(err_msg, "error: command exceeds maximum token limit", err_max);
            return -1;
        }

        /* Operator: pipe '|' */
        if (*p == '|') {
            tokens[ntok].type = TOK_PIPE;
            tokens[ntok].val[0] = '|';
            tokens[ntok].val[1] = '\0';
            ntok++;
            p++;
            continue;
        }

        /* Operator: input redirection '<' */
        if (*p == '<') {
            tokens[ntok].type = TOK_REDIR_IN;
            tokens[ntok].val[0] = '<';
            tokens[ntok].val[1] = '\0';
            ntok++;
            p++;
            continue;
        }

        /* Operator: output redirection '>' or '>>' */
        if (*p == '>') {
            if (*(p + 1) == '>') {
                tokens[ntok].type = TOK_REDIR_APPEND;
                tokens[ntok].val[0] = '>';
                tokens[ntok].val[1] = '>';
                tokens[ntok].val[2] = '\0';
                p += 2;
            } else {
                tokens[ntok].type = TOK_REDIR_OUT;
                tokens[ntok].val[0] = '>';
                tokens[ntok].val[1] = '\0';
                p++;
            }
            ntok++;
            continue;
        }

        /* Operator: background '&' */
        if (*p == '&') {
            tokens[ntok].type = TOK_AMPERSAND;
            tokens[ntok].val[0] = '&';
            tokens[ntok].val[1] = '\0';
            ntok++;
            p++;
            continue;
        }

        /* Word or quoted string */
        tokens[ntok].type = TOK_WORD;
        int vlen = 0;
        char in_quote = 0;

        while (*p) {
            if (!in_quote) {
                if (*p == '\'' || *p == '\"') {
                    in_quote = *p;
                    p++;
                    continue;
                }
                if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ||
                    *p == '|' || *p == '<' || *p == '>' || *p == '&') {
                    break;
                }
                if (vlen < MAX_TOKEN_LEN - 1) {
                    tokens[ntok].val[vlen++] = *p;
                }
                p++;
            } else {
                if (*p == in_quote) {
                    in_quote = 0;
                    p++;
                    continue;
                }
                if (vlen < MAX_TOKEN_LEN - 1) {
                    tokens[ntok].val[vlen++] = *p;
                }
                p++;
            }
        }

        if (in_quote) {
            str_copy(err_msg, "syntax error: unclosed quote", err_max);
            return -1;
        }

        tokens[ntok].val[vlen] = '\0';
        ntok++;
    }

    tokens[ntok].type = TOK_END;
    tokens[ntok].val[0] = '\0';
    return ntok;
}

/*
 * PARSER
 * Converts token sequence into structured command_t.
 * Validates argument counts, missing operands, and redirection syntax.
 */
static int parse_command(const token_t *tokens, int ntok, command_t *cmd, char *err_msg, int err_max)
{
    cmd->argc = 0;
    cmd->redir_in[0] = '\0';
    cmd->redir_out[0] = '\0';
    cmd->append = 0;
    cmd->background = 0;
    cmd->has_pipe = 0;

    for (int i = 0; i < ntok; ++i) {
        if (tokens[i].type == TOK_END) break;

        if (tokens[i].type == TOK_WORD) {
            if (cmd->argc >= MAX_ARGS - 1) {
                str_copy(err_msg, "error: too many arguments", err_max);
                return -1;
            }
            cmd->argv[cmd->argc++] = (char *)tokens[i].val;
            continue;
        }

        if (tokens[i].type == TOK_REDIR_IN) {
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                str_copy(err_msg, "syntax error: missing file for '<'", err_max);
                return -1;
            }
            str_copy(cmd->redir_in, tokens[i + 1].val, sizeof(cmd->redir_in));
            i++;
            continue;
        }

        if (tokens[i].type == TOK_REDIR_OUT || tokens[i].type == TOK_REDIR_APPEND) {
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                str_copy(err_msg, "syntax error: missing file for redirection", err_max);
                return -1;
            }
            str_copy(cmd->redir_out, tokens[i + 1].val, sizeof(cmd->redir_out));
            cmd->append = (tokens[i].type == TOK_REDIR_APPEND);
            i++;
            continue;
        }

        if (tokens[i].type == TOK_PIPE) {
            cmd->has_pipe = 1;
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                str_copy(err_msg, "syntax error: missing command after '|'", err_max);
                return -1;
            }
            continue;
        }

        if (tokens[i].type == TOK_AMPERSAND) {
            cmd->background = 1;
            if (i + 1 < ntok && tokens[i + 1].type != TOK_END) {
                str_copy(err_msg, "syntax error: '&' must be at end of command", err_max);
                return -1;
            }
            continue;
        }
    }

    cmd->argv[cmd->argc] = (char *)0;

    if (cmd->argc == 0) {
        if (cmd->has_pipe || cmd->redir_in[0] || cmd->redir_out[0]) {
            str_copy(err_msg, "syntax error: missing command", err_max);
            return -1;
        }
    }

    return 0;
}

static void help(void)
{
    print("Commands:\n");
    print("  help                 show this help\n");
    print("  getpid               show current process id\n");
    print("  ps                   show process table\n");
    print("  echo TEXT            print TEXT\n");
    print("  sleep N              sleep for N timer ticks\n");
    print("  forktest             demonstrate fork/wait/exit\n");
    print("  ls                   list RAMFS files\n");
    print("  cat FILE             print RAMFS file\n");
    print("  isotest              verify kernel/user memory isolation\n");
    print("  exit                 terminate this shell\n");
}

static void forktest(void)
{
    long pid = sys_fork();
    if (pid < 0) {
        print("fork failed\n");
        return;
    }
    if (pid == 0) {
        print("forktest: child running, pid=");
        print_num(sys_getpid());
        print("\n");
        sys_sleep(2);
        print("forktest: child exiting\n");
        sys_exit(42);
    }
    print("forktest: parent created child pid=");
    print_num(pid);
    print(", waiting...\n");
    long got = sys_wait();
    print("forktest: wait returned pid=");
    print_num(got);
    print("\n");
}

/*
 * COMMAND EXECUTOR & BUILT-INS
 * Dispatches parsed commands using structured argv.
 */
static void execute_command(command_t *cmd)
{
    if (cmd->argc == 0) return;

    /* Architectural limitation checks */
    if (cmd->has_pipe) {
        print("pipe: not supported by NanoBoot architecture (no pipe syscall)\n");
        return;
    }
    if (cmd->redir_in[0] || cmd->redir_out[0]) {
        print("redirection: not supported by NanoBoot architecture (read-only RAMFS)\n");
        return;
    }
    if (cmd->background) {
        print("background: not supported by NanoBoot architecture\n");
        return;
    }

    const char *prog = cmd->argv[0];

    if (streq(prog, "help")) {
        help();
        return;
    }
    if (streq(prog, "getpid")) {
        print("pid=");
        print_num(sys_getpid());
        print("\n");
        return;
    }
    if (streq(prog, "ps")) {
        sys_ps();
        return;
    }
    if (streq(prog, "echo")) {
        for (int i = 1; i < cmd->argc; ++i) {
            print(cmd->argv[i]);
            if (i + 1 < cmd->argc) print(" ");
        }
        print("\n");
        return;
    }
    if (streq(prog, "sleep")) {
        if (cmd->argc < 2) {
            print("sleep: missing operand\n");
            return;
        }
        unsigned long n = parse_num(cmd->argv[1]);
        print("sleeping...\n");
        sys_sleep(n);
        print("awake\n");
        return;
    }
    if (streq(prog, "forktest")) {
        forktest();
        return;
    }
    if (streq(prog, "ls")) {
        sys_ls();
        return;
    }
    if (streq(prog, "cat")) {
        if (cmd->argc < 2) {
            print("cat: missing file operand\n");
            return;
        }
        sys_cat(cmd->argv[1]);
        return;
    }
    if (streq(prog, "isotest")) {
        long rc = sys_write((const void *)0x80200000ULL, 1);
        if (rc < 0) print("isolation PASS: kernel address rejected\n");
        else print("isolation FAIL: kernel address accepted\n");
        return;
    }
    if (streq(prog, "exit")) {
        print("shell exiting\n");
        sys_exit(0);
    }

    print("unknown command; type 'help'\n");
}

void main(void)
{
    char line[MAX_LINE];
    token_t tokens[MAX_TOKENS];
    char err_msg[64];
    command_t cmd;

    print("\nWelcome to NanoBoot user shell.\n");
    print("Type 'help' for commands.\n\n");

    for (;;) {
        int n = 0;
        print("nb$ ");
        while (n < (int)sizeof(line) - 1) {
            char c = 0;
            long rc = sys_read(&c, 1);
            if (rc <= 0) continue;
            if (c == '\r') c = '\n';
            if (c == '\n') {
                print("\n");
                break;
            }
            if (c == 8 || c == 127) {
                if (n > 0) {
                    --n;
                    print("\b \b");
                }
                continue;
            }
            line[n++] = c;
            sys_write(&c, 1);
        }
        line[n] = 0;

        err_msg[0] = '\0';
        int ntok = tokenize(line, tokens, MAX_TOKENS, err_msg, sizeof(err_msg));
        if (ntok < 0) {
            print(err_msg);
            print("\n");
            continue;
        }

        if (parse_command(tokens, ntok, &cmd, err_msg, sizeof(err_msg)) < 0) {
            print(err_msg);
            print("\n");
            continue;
        }

        execute_command(&cmd);
    }
}

void _start(void) __attribute__((section(".text.start"), used, noreturn));
void _start(void)
{
    main();
    sys_exit(0);
    for (;;) { }
}
