#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>

int main(void){
    if (unshare(CLONE_NEWPID))
        perror("Create pid ns failed.\n");

    int pid = fork();
    int pidp;
    if (pid < 0)
        perror("Fork failed.\n");
    if (pid == 0){
        pidp = getpid();
        printf("This is the child process, pid:%d\n", pidp);
    }
    if (pid > 0){
        pidp = getpid();
        printf("This is the parent process, pid:%d, child pid:%d\n", pidp, pid);
    }
    return 0;
}
