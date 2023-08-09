#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char** argv) {
    printf("start (pid %d)\n", getpid());
    sysinfo();
    int count = 51;
    char * page[count];
    for(int i = 0; i < count; i++) {
        printf("allocating a page from %d\n", getpid());
        page[i] = sbrk(4096);
        sysinfo();
        // sbrk(4096);
    }
    printf("fork...\n");
    int pid = fork();
    if(pid < 0) {
        printf("Fork failed\n");
        exit(-1);
    } else if(pid == 0) {
        printf("in child process\n");
        sysinfo();
        printf("Writing in child process\n");
        for(int i = 0; i < count; i++) {
            printf("writing to page address %p started\n", page[i]);
            *page[i] = 'c';
            printf("writing to page address %p ended\n", page[i]);
            sysinfo();
        }

        printf("child process exiting\n");
    exit(0);
    } else {
        wait(0);
        printf("in parent process again\n");
        sysinfo();
        printf("parent process exiting\n");
        exit(0);
    }
}