#include "common.h"
#include "sysnum.h"

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

static int starts(const char *s, const char *p)
{
    int i = 0;
    while (p[i]) {
        if (s[i] != p[i]) return 0;
        ++i;
    }
    return 1;
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

static void run_command(char *cmd)
{
    if (streq(cmd, "")) return;
    if (streq(cmd, "help")) { help(); return; }
    if (streq(cmd, "getpid")) {
        print("pid="); print_num(sys_getpid()); print("\n"); return;
    }
    if (streq(cmd, "ps")) { sys_ps(); return; }
    if (starts(cmd, "echo ")) { print(cmd + 5); print("\n"); return; }
    if (starts(cmd, "sleep ")) {
        unsigned long n = parse_num(cmd + 6);
        print("sleeping...\n");
        sys_sleep(n);
        print("awake\n");
        return;
    }
    if (streq(cmd, "forktest")) { forktest(); return; }
    if (streq(cmd, "ls")) { sys_ls(); return; }
    if (starts(cmd, "cat ")) {
        sys_cat(cmd + 4);
        return;
    }
    if (streq(cmd, "isotest")) {
        long rc = sys_write((const void *)0x80200000ULL, 1);
        if (rc < 0) print("isolation PASS: kernel address rejected\n");
        else print("isolation FAIL: kernel address accepted\n");
        return;
    }
    if (streq(cmd, "exit")) { print("shell exiting\n"); sys_exit(0); }
    print("unknown command; type 'help'\n");
}

void main(void)
{
    char line[128];
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
        run_command(line);
    }
}

void _start(void) __attribute__((section(".text.start"), used, noreturn));
void _start(void)
{
    main();
    sys_exit(0);
    for (;;) { }
}
