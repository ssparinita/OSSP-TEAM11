#include <stdio.h>
#include <string.h>

int main() {
    char input[200];

    printf("Enter text inside single quotes: ");
    fgets(input, sizeof(input), stdin);

    input[strcspn(input, "\n")] = '\0';

    printf("\nOriginal input: %s\n", input);
    printf("Parsed output : %s\n", input);

    printf("\nSingle quotes preserve the content literally.\n");
    printf("Variable expansion and special-character interpretation are ignored.\n");

    return 0;
}

