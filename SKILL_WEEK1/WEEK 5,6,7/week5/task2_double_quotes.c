#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char name[100];
    char input[200];

    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    setenv("NAME", name, 1);

    printf("Enter a double-quoted string containing spaces and $NAME: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';

    printf("\nOriginal input : \"%s\"\n", input);

    char *pos = strstr(input, "$NAME");

    if (pos != NULL) {
        char result[200];
        size_t prefix_length = pos - input;

        snprintf(result, sizeof(result),
                 "%.*s%s%s",
                 (int)prefix_length,
                 input,
                 getenv("NAME"),
                 pos + 5);

        printf("Expanded output: \"%s\"\n", result);
    } else {
        printf("Expanded output: \"%s\"\n", input);
    }

    printf("\nDouble quotes preserve spaces while allowing variable expansion.\n");

    return 0;
}
