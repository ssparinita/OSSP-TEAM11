#include <stdio.h>
#include <string.h>

int main() {
    char input[200];
    char output[200];
    int j = 0;

    printf("Enter text with escape characters: ");
    fgets(input, sizeof(input), stdin);

    input[strcspn(input, "\n")] = '\0';

    for (int i = 0; input[i] != '\0'; i++) {

        if (input[i] == '\\' && input[i + 1] != '\0') {
            i++;
            output[j++] = input[i];
        } else {
            output[j++] = input[i];
        }
    }

    output[j] = '\0';

    printf("\nOriginal input: %s\n", input);
    printf("Parsed output : %s\n", output);

    printf("\nEscape characters are processed and the escaped characters are preserved.\n");

    return 0;
}
