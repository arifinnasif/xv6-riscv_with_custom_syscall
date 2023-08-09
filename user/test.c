#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char** argv) {
    printf("start\n");
    int count = atoi(argv[1]);
    char * page[count];
    for(int i = 0; i < count; i++) {
        printf("allocating a page from %d\n", getpid());
        page[i] = sbrk(4096);
        sysinfo();
        // sbrk(4096);
    }
    printf("starting write\n");
    for(int i = 0; i < count; i++) {
        *page[i] = 'a';
    }
    printf("write done\n");
    return 0;
}