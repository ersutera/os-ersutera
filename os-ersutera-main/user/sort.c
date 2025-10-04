#include "kernel/types.h"
#include "user/user.h"

#define MAXLINES 256
#define MAXLEN   128

char lines[MAXLINES][MAXLEN];

int main(int argc, char *argv[]) {
    int n = 0;
    char buf[MAXLEN];
    int unique = 0;
    int reverse = 0;
    char *filename = 0;

    // parse flags
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-u") == 0) {
                unique = 1;
            } else if (strcmp(argv[i], "-r") == 0) {
                reverse = 1;
            } else {
                fprintf(2, "sort: unknown flag %s\n", argv[i]);
                exit(1);
            }
        } else {
            filename = argv[i]; // first non-flag argument
        }
    }

    // read lines
    if (!filename) {
        while (gets(buf, sizeof(buf)) != 0 && n < MAXLINES) {
            strcpy(lines[n++], buf);
        }
    } else {
        int fd = open(filename, 0);
        if (fd < 0) {
            fprintf(2, "sort: cannot open %s\n", filename);
            exit(1);
        }
        int m;
        while ((m = read(fd, buf, sizeof(buf)-1)) > 0) {
            buf[m] = '\0';
            char *p = buf;
            while (*p && n < MAXLINES) {
                char *q = strchr(p, '\n');
                if (q) *q = '\0';
                strcpy(lines[n++], p);
                if (!q) break;
                p = q + 1;
            }
        }
        close(fd);
    }

    // simple bubble sort
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            int cmp = strcmp(lines[i], lines[j]);
            if ((reverse && cmp < 0) || (!reverse && cmp > 0)) {
                char tmp[MAXLEN];
                strcpy(tmp, lines[i]);
                strcpy(lines[i], lines[j]);
                strcpy(lines[j], tmp);
            }
        }
    }

    // print lines with optional unique filter
    for (int i = 0; i < n; i++) {
        if (unique && i > 0 && strcmp(lines[i], lines[i-1]) == 0) {
            continue;
        }
        printf("%s\n", lines[i]);
    }

    exit(0);
}

