#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <target_pid>\n", argv[0]);
        return 1;
    }
    int pid = atoi(argv[1]);
    if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1)
        return 1;
    waitpid(pid, NULL, 0);
    printf("Attached to %d. Peeking...\n", pid);
    // cheat
    ptrace(PTRACE_DETACH, pid, 0, 0);
    printf("Detached.\n");
    return 0;
}