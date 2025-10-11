#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char input[1024];

    while (1) {
        printf("myshell> ");

        // Read input from the user
        if (!fgets(input, sizeof(input), stdin)) {
            // Handle Ctrl+D (EOF) to exit the shell
            printf("\n");
            break;
        }

        // For now, just print the input back to the user
        printf("You entered: %s", input);
    }

    return 0;
}
