#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buffer[512];

int main(int argc, char *argv[])
{
    int fd[2]; // file descriptors for the pipe
    int pid, nbytes;
    char input[] = "Hello, world!";
    char output[sizeof(input)];

    // Create a pipe
    if (pipe(fd) < 0) {
        printf("Error: could not create pipe\n");
        exit(1);
    }

    // Write some data to the pipe
    write(fd[1], input, sizeof(input));

    // Create a child process
    pid = fork();

    if (pid < 0) {
        printf("Error: fork failed\n");
        exit(1);
    }
    else if (pid == 0) {
        // Child process reads data from the pipe
        nbytes = read(fd[0], output, sizeof(output));
        printf("Child read %d bytes from pipe: %s\n", nbytes, output);

        // Write some data to a file
        int fd_file = open("test.txt", O_CREATE | O_WRONLY);
        write(fd_file, input, sizeof(input));
        close(fd_file);

        exit(1);
    }
    else {
        // Parent process waits for the child to exit
        wait(0);

        // Read data from the file
        int fd_file = open("test.txt", O_RDONLY);
        read(fd_file, buffer, sizeof(buffer));
        close(fd_file);

        printf("Parent read from file: %s\n", buffer);
    }

    exit(1);
}
