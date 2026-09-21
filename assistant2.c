/* ChatGPT
 * Copy a file without replacing an existing destination.
 * The implementation deliberately uses only open, read, write, and close.
 */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

static int write_all(int fd, const char *buffer, size_t length)
{
	size_t offset = 0;

	while (offset < length) {
		ssize_t written = write(fd, buffer + offset, length - offset);

		if (written > 0) {
			offset += (size_t)written;
		} else if (written == -1 && errno == EINTR) {
			continue;
		} else {
			return -1;
		}
	}

	return 0;
}

static void report(const char *message, size_t length)
{
	const char newline = '\n';

	(void)write_all(STDERR_FILENO, message, length);
	(void)write_all(STDERR_FILENO, &newline, 1);
}

static int open_retry(const char *path, int flags, mode_t mode)
{
	int fd;

	do {
		fd = open(path, flags, mode);
	} while (fd == -1 && errno == EINTR);

	return fd;
}

int main(int argc, char *argv[])
{
	int source_fd = -1;
	int destination_fd = -1;
	char buffer[BUFFER_SIZE];
	int status = 1;

	if (argc != 3 || argv[1] == 0 || argv[2] == 0) {
		report("usage: filecopy SOURCE DESTINATION",
			   sizeof("usage: filecopy SOURCE DESTINATION") - 1);
		return 1;
	}

	source_fd = open_retry(argv[1], O_RDONLY, 0);
	if (source_fd == -1) {
		report("could not open source file", sizeof("could not open source file") - 1);
		return 1;
	}

	destination_fd = open_retry(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (destination_fd == -1) {
		report("could not create destination file",
			   sizeof("could not create destination file") - 1);
		(void)close(source_fd);
		return 1;
	}

	for (;;) {
		ssize_t bytes_read;

		do {
			bytes_read = read(source_fd, buffer, sizeof(buffer));
		} while (bytes_read == -1 && errno == EINTR);

		if (bytes_read == 0) {
			status = 0;
			break;
		}
		if (bytes_read == -1) {
			report("could not read source file", sizeof("could not read source file") - 1);
			break;
		}
		if (write_all(destination_fd, buffer, (size_t)bytes_read) == -1) {
			report("could not write destination file",
				   sizeof("could not write destination file") - 1);
			break;
		}
	}

	if (close(source_fd) == -1) {
		report("could not close source file", sizeof("could not close source file") - 1);
		status = 1;
	}
	if (close(destination_fd) == -1) {
		report("could not close destination file",
			   sizeof("could not close destination file") - 1);
		status = 1;
	}

	return status;
}
