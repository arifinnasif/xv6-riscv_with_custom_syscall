#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int pg_count = 5;
    char *p = sbrk(pg_count*4096);
    char ch= 'u';
    printf("writing to pages in unforked\n");
    for(char *a = p; a < p+pg_count*4096; a += 4096) {
        *a = ch;
    }

    printf("before fork\n");

    int pid = fork();
    if(pid < 0) {
        printf("Fork failed\n");
        exit(-1);
    } else if(pid == 0) {
        printf("in child process\n");
        
        printf("Writing in child process\n");
        for(char *a = p; a < p+pg_count*4096; a += 4096) {
            printf("child found: %c\n", *a);
            *a = 'c';
            printf("child wrote: %c\n", *a);
        }

        
        printf("child process exiting\n");
        exit(0);
    } else {
        wait(0);
        printf("in parent process again\n");
        for(char *a = p; a < p+pg_count*4096; a += 4096) {
            printf("parent found: %c\n", *a);
        }

    }
    
    printf("parent process exiting\n");
    exit(0);
}