#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char** argv) {
    if(argc != 2) {
        printf("failed\n");
        exit(-1);
    }
    printf("start (pid %d)\n", getpid());
    sysinfo();
    int count = atoi(argv[1]);
    char * page[count];
    for(int i = 0; i < count; i++) {
        printf("allocating a page from %d\n", getpid());
        page[i] = sbrk(4096);
        sysinfo();
        // sbrk(4096);
    }

    printf("starting write (pid %d)\n", getpid());
    for(int i = 0; i < count; i++) {
        printf("writing to page %p\n", page[i]);
        *page[i] = 'a';
        sysinfo();
    }
    printf("write done (pid %d)\n", getpid());
    printf("end (pid %d)\n", getpid());

    return 0;
}