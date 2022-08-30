#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

static int readline(int fd, char *buf, int buflen) {
    int ch;
    char *p = buf;
    while ((ch = getc(fd)) >= 0) {
        if (p - buf >= buflen) {
            printf("grep: fail to read a line: the line bigger than the buffer "
                   "length %d.\n",
                   buflen);
            exit(1);
        }
        if (ch == 0 || ch == '\n' || ch == '\r') {
            break;
        }
        *p++ = (char) ch;
    }
    *p = 0;
    if (ch <= 0) {
        return -1;
    }
    return p - buf;
}

static bool matches(char *str, const char *pattern) {
    return strstr(str, pattern);
}

static int grep(int fd, int pn, const char **patterns) {
    char buf[1024];
    bool found = false;
    for (;;) {
        int r = readline(fd, buf, sizeof buf);
        bool matched = false;
        for (int i = 0; i < pn; i++) {
            const char *pattern = patterns[i];
            if (matches(buf, pattern)) {
                matched = true;
                break;
            }
        }
        if (matched) {
            found = true;
            printf("%s\n", buf);
        }
        if (r < 0) {
            return found ? 0 : 1;
        }
    }
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("grep: PATTERNS...\n");
        return 2;
    }
    return grep(stdin, argc - 1, (const char **) (argv + 1));
}