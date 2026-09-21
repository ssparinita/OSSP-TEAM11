#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

int main() {
    char *path = getenv("PATH");

    if (path == NULL) {
        printf("PATH variable is not set.\n");
        return 1;
    }

    char command[100];

    printf("Enter executable name: ");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = '\0';

    char path_copy[8192];

    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *directory = strtok(path_copy, ":");

    while (directory != NULL) {

        char full_path[PATH_MAX];

        snprintf(full_path, sizeof(full_path),
                 "%s/%s", directory, command);

        if (access(full_path, X_OK) == 0) {
            printf("\nExecutable found: %s\n", full_path);
            printf("Permission check: executable\n");

            return 0;
        }

        directory = strtok(NULL, ":");
    }

    printf("\nCommand '%s' was not found in PATH or is not executable.\n",
           command);

    return 1;
}

