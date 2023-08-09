#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uint64 number_of_tickets = atoi(argv[1]);
    settickets(number_of_tickets);
    if(argc == 3 && atoi(argv[2]) > 0) {
        int f = fork();
        if(f == 0) {
            printf("Child process with %d tickets is running\n", number_of_tickets);
        } else if(f > 0) {
            printf("Parent process with %d tickets is running\n", number_of_tickets);
        } else {
            printf("Fork failed\n");
        }
    }

    while(1) {
    }

    return 0;
}