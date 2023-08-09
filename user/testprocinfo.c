#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    struct pstat p;
    getpinfo(&p);
    printf("PID | In Use | Original Tickets | Current Tickets | Time Slices\n");
    for(int i = 0; i < NPROC; i++) {
        if(p.inuse[i] == 1) {
            printf("%d\t%d\t%d\t\t  %d\t\t    %d\n", p.pid[i], p.inuse[i], p.tickets_original[i], p.tickets_current[i], p.time_slices[i]);
        }
    }

    return 0;
}