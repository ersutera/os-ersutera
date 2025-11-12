#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define COLOR_RESET  "\033[0m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED    "\033[31m"

#define MAX_ALIASES 32
#define MAX_HISTORY 100
#define MAX_ARGS 32
#define BUF_SIZE 256

struct alias {
    char name[32];
    char value[128];
};

struct history_entry {
    char cmd[BUF_SIZE];
    uint duration_ms;
};

struct alias aliases[MAX_ALIASES];
int alias_count = 0;

struct history_entry history[MAX_HISTORY];
int history_count = 0;

// emulate Linux dup2 using xv6 dup()
void dup2(int oldfd, int newfd) {
    if (oldfd == newfd) return;
    close(newfd);
    int fd = dup(oldfd);
    if (fd != newfd) {
        fprintf(2, "dup2 failed\n");
        exit(1);
    }
}

// Safe string copy
int safestrcpy(char *s, const char *t, int n) {
    int i;
    for (i = 0; i < n-1 && t[i]; i++)
        s[i] = t[i];
    s[i] = '\0';
    return i;
}

// History
void add_history(const char *cmd, uint duration_ms) {
    if (strlen(cmd) == 0) return;
    int idx = history_count % MAX_HISTORY;
    safestrcpy(history[idx].cmd, cmd, sizeof(history[idx].cmd));
    history[idx].duration_ms = duration_ms;
    history_count++;
}

void show_history(int with_time) {
    int start = history_count > MAX_HISTORY ? history_count - MAX_HISTORY : 0;
    for (int i = start; i < history_count; i++) {
        struct history_entry *he = &history[i % MAX_HISTORY];
        if (with_time)
            printf("[%d|%dms] %s\n", i, he->duration_ms, he->cmd);
        else
            printf(" %d %s\n", i, he->cmd);
    }
}

// Tokenizer
char *next_token(char **str_ptr, const char *delim) {
    if (*str_ptr == 0) return 0;
    char *start = *str_ptr;
    while (*start && strchr(delim, *start)) start++;
    if (*start == 0) { *str_ptr = 0; return 0; }
    char *end = start;
    while (*end && !strchr(delim, *end)) end++;
    if (*end) { *end = 0; *str_ptr = end+1; }
    else *str_ptr = 0;
    return start;
}

// I/O redirection
void apply_redirection(char **args) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i+1], O_WRONLY|O_CREATE|O_TRUNC);
            if (fd < 0) { fprintf(2,"cannot open %s\n", args[i+1]); exit(1);}
            dup2(fd, 1); close(fd);
            args[i] = 0;
        } else if (strcmp(args[i], ">>") == 0) {
            int fd = open(args[i+1], O_WRONLY|O_CREATE|O_APPEND);
            if (fd < 0) { fprintf(2,"cannot open %s\n", args[i+1]); exit(1);}
            dup2(fd, 1); close(fd);
            args[i] = 0;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i+1], O_RDONLY);
            if (fd < 0) { fprintf(2,"cannot open %s\n", args[i+1]); exit(1);}
            dup2(fd, 0); close(fd);
            args[i] = 0;
        }
    }
}

// Run pipeline (with background)
void run_pipeline(char **args, int background) {
    int pipes[2][2];
    int last_in = 0;
    int cmd_idx = 0;

    int start_idx = 0;
    while (args[start_idx]) {
        char *cmd[MAX_ARGS];
        int j = 0;
        int i = start_idx;
        while (args[i] && strcmp(args[i], "|") != 0) cmd[j++] = args[i++];
        cmd[j] = 0;
        if (args[i]) i++; // skip '|'

        if (args[i]) { if (pipe(pipes[cmd_idx%2]) < 0) { fprintf(2,"pipe failed\n"); return; } }

        int pid = fork();
        if (pid == 0) {
            if (last_in != 0) { dup2(last_in, 0); close(last_in); }
            if (args[i]) { dup2(pipes[cmd_idx%2][1], 1); close(pipes[cmd_idx%2][0]); close(pipes[cmd_idx%2][1]); }
            apply_redirection(cmd);
            exec(cmd[0], cmd);
            fprintf(2,"exec %s failed\n", cmd[0]);
            exit(1);
        } else {
            if (last_in != 0) close(last_in);
            if (args[i]) { close(pipes[cmd_idx%2][1]); last_in = pipes[cmd_idx%2][0]; }
        }

        start_idx = i;
        cmd_idx++;
    }

    if (!background) {
        int wstatus;
        while (wait(&wstatus) > 0);
    }
}

// Check if s starts with prefix (like strncmp(s,prefix,len)==0)
int str_startswith(const char *s, const char *prefix, int len) {
    for(int i=0; i<len; i++){
        if(s[i]==0 || s[i]!=prefix[i]) {return 0;}
    }
    return 1;
}

// History expansion (!num, !prefix, !!)
int expand_history(char *buf) {
    if (buf[0] != '!') return 0;

    if (buf[1] == '!') { // !!
        if (history_count == 0) return -1;
        safestrcpy(buf, history[(history_count-1)%MAX_HISTORY].cmd, BUF_SIZE);
        return 1;
    } else if (buf[1]>='0' && buf[1]<='9') { // !num
        int num = atoi(buf+1);
        if (num < 0 || num >= history_count) return -1;
        safestrcpy(buf, history[num%MAX_HISTORY].cmd, BUF_SIZE);
        return 1;
    } else { // !prefix
        char *prefix = buf+1;
        int plen = strlen(prefix);
        for (int i = history_count-1; i >= 0; i--) {
            struct history_entry *he = &history[i%MAX_HISTORY];
            if (str_startswith(he->cmd, prefix, plen)) {
                safestrcpy(buf, he->cmd, BUF_SIZE);
                return 1;
            }
        }
        return -1;
    }
}

int main(void) {
    int cmd_num = 1;
    int last_status = 0;
    char cwd[128];
    char buf[BUF_SIZE];

    while (1) {
        getcwd(cwd, sizeof(cwd));
        printf(COLOR_CYAN "[%d]" COLOR_RESET
               COLOR_YELLOW "-[%d]" COLOR_RESET
               "─" COLOR_GREEN "[%s]" COLOR_RESET
               "$ ", last_status, cmd_num, cwd);

        if (gets(buf, sizeof(buf)) == 0) break;
        if (strlen(buf) == 0) continue;

        // Expand history (!)
        if (buf[0]=='!') {
            if (expand_history(buf) < 0) {
                printf("No such history command: %s\n", buf);
                continue;
            } else {
                printf("%s\n", buf);
            }
        }

        char cmd_buf[BUF_SIZE];
        safestrcpy(cmd_buf, buf, sizeof(cmd_buf));

        // Tokenize
        char *args[MAX_ARGS];
        int tokens = 0;
        char *next_tok = buf;
        char *tok;
        while ((tok = next_token(&next_tok, " \t\n")) != 0)
            args[tokens++] = tok;
        args[tokens] = 0;
        if (tokens == 0) continue;

        // Apply alias expansion
        int max_exp = 10, ex = 0;
        while (ex < max_exp) {
            int found = 0;
            for (int i = 0; i < alias_count; i++) {
                if (strcmp(args[0], aliases[i].name)==0) {
                    char tmp[BUF_SIZE];
                    safestrcpy(tmp, aliases[i].value, BUF_SIZE);
                    int ntok = 0;
                    char *tptr = tmp;
                    char *t;
                    while ((t = next_token(&tptr, " \t\n"))!=0 && ntok<MAX_ARGS-1)
                        args[ntok++] = t;
                    args[ntok]=0;
                    found=1;
                    break;
                }
            }
            if (!found) break;
            ex++;
        }

        // Built-ins
        if (args[0][0]=='#') continue;
        if (strcmp(args[0],"exit")==0) exit(0);
        if (strcmp(args[0],"cd")==0) {
            if (args[1]==0 || chdir(args[1])<0)
                fprintf(2,"chdir: no such file or directory: %s\n", args[1]);
            continue;
        }
        if (strcmp(args[0],"alias")==0) {
            if (!args[1]) {
                for (int i=0;i<alias_count;i++)
                    printf("%s='%s'\n", aliases[i].name, aliases[i].value);
            } else {
                char *eq = strchr(args[1],'=');
                if (!eq) { fprintf(2,"usage: alias name=\"value\"\n"); continue; }
                *eq=0;
                char *name=args[1],*value=eq+1;
                if (*value=='"'||*value=='\'') value++;
                if (value[strlen(value)-1]=='"'||value[strlen(value)-1]=='\'') value[strlen(value)-1]=0;
                int found=0;
                for(int i=0;i<alias_count;i++){
                    if(strcmp(aliases[i].name,name)==0){
                        safestrcpy(aliases[i].value,value,sizeof(aliases[i].value));
                        found=1;
                        break;
                    }
                }
                if(!found && alias_count<MAX_ALIASES){
                    safestrcpy(aliases[alias_count].name,name,sizeof(aliases[alias_count].name));
                    safestrcpy(aliases[alias_count].value,value,sizeof(aliases[alias_count].value));
                    alias_count++;
                }
            }
            continue;
        }
        if (strcmp(args[0],"history")==0) {
            int with_time = args[1] && strcmp(args[1],"-t")==0;
            show_history(with_time);
            continue;
        }

        // Background
        int background=0;
        if (tokens>0 && strcmp(args[tokens-1],"&")==0) { background=1; args[tokens-1]=0; }

        // Time measurement
        uint start = uptime();
        run_pipeline(args, background);
        uint end = uptime();
        add_history(cmd_buf,end-start);

        cmd_num++;
    }

    return 0;
}

