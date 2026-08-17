#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int source_fd, destination_fd;
    char source[100], destination[100];
    char buffer[1024];
    ssize_t bytes_read, bytes_written;

    printf("Enter source file name: ");
    scanf("%99s", source);

    printf("Enter destination file name: ");
    scanf("%99s", destination);

    source_fd = open(source, O_RDONLY);
    if (source_fd == -1)
    {
        perror("Error opening source file");
        return 1;
    }

    destination_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination_fd == -1)
    {
        perror("Error opening/creating destination file");
        close(source_fd);
        return 1;
    }

    while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t total_written = 0;

        while (total_written < bytes_read)
        {
            bytes_written = write(destination_fd,
                                  buffer + total_written,
                                  bytes_read - total_written);

            if (bytes_written == -1)
            {
                perror("Error writing to destination file");
                close(source_fd);
                close(destination_fd);
                return 1;
            }

            total_written += bytes_written;
        }
    }

    if (bytes_read == -1)
    {
        perror("Error reading source file");
        close(source_fd);
        close(destination_fd);
        return 1;
    }

    if (close(source_fd) == -1)
    {
        perror("Error closing source file");
        close(destination_fd);
        return 1;
    }

    if (close(destination_fd) == -1)
    {
        perror("Error closing destination file");
        return 1;
    }

    printf("File copied successfully.\n");
    return 0;
}
