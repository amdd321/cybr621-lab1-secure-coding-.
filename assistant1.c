// copilot

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 8192

static int close_fd(int fd)
{
    if (fd >= 0) {
        if (close(fd) == -1) {
            perror("close");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int src_fd = -1;
    int dst_fd = -1;
    char buffer[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* Open source file for reading */
    do {
        src_fd = open(argv[1], O_RDONLY);
    } while (src_fd == -1 && errno == EINTR);

    if (src_fd == -1) {
        perror("open source");
        return EXIT_FAILURE;
    }

    /*
     * Create destination file:
     * - O_WRONLY: write only
     * - O_CREAT: create if missing
     * - O_EXCL : fail if file already exists
     * Permissions: 0600
     */
    do {
        dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0600);
    } while (dst_fd == -1 && errno == EINTR);

    if (dst_fd == -1) {
        perror("open destination");

        close_fd(src_fd);
        return EXIT_FAILURE;
    }

    for (;;) {
        ssize_t bytes_read;

        /* Retry interrupted reads */
        do {
            bytes_read = read(src_fd, buffer, sizeof(buffer));
        } while (bytes_read == -1 && errno == EINTR);

        if (bytes_read == -1) {
            perror("read");
            close_fd(src_fd);
            close_fd(dst_fd);
            return EXIT_FAILURE;
        }

        if (bytes_read == 0) {
            /* EOF */
            break;
        }

        ssize_t total_written = 0;

        while (total_written < bytes_read) {
            ssize_t bytes_written;

            do {
                bytes_written = write(
                    dst_fd,
                    buffer + total_written,
                    (size_t)(bytes_read - total_written)
                );
            } while (bytes_written == -1 && errno == EINTR);

            if (bytes_written == -1) {
                perror("write");
                close_fd(src_fd);
                close_fd(dst_fd);
                return EXIT_FAILURE;
            }

            total_written += bytes_written;
        }
    }

    if (close_fd(src_fd) == -1) {
        close_fd(dst_fd);
        return EXIT_FAILURE;
    }

    if (close_fd(dst_fd) == -1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}