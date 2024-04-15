#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MAX_LENGTH 1024
#define MAX_ARGS 100

void execute_command(char **args, int background) {
    pid_t pid = fork();
    if (pid == 0) { // Proces potomny
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { // Proces rodzicielski
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void change_directory(char *path) {
    if (chdir(path) != 0) {
        perror("chdir");
    }
}

int main(int argc, char *argv[]) {
    char line[MAX_LENGTH];
    char *args[MAX_ARGS];
    int background;
    FILE *input = stdin;

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    while (true) {
        background = 0;
        if (input == stdin) {
            printf("> ");
        }
        if (!fgets(line, sizeof(line), input)) break; // Koniec pliku (EOF) lub błąd

        if (line[0] == '#' || line[0] == '\n') continue; // Ignorowanie komentarzy i pustych linii

        int arg_count = 0;
        args[arg_count] = strtok(line, " \n");
        while (args[arg_count] != NULL) {
            arg_count++;
            args[arg_count] = strtok(NULL, " \n");
        }

        if (args[0] == NULL) continue; // Pusta linia

        if (strcmp(args[0], "cd") == 0) {
            change_directory(args[1]);
        } else {
            if (args[arg_count-1] && strcmp(args[arg_count-1], "&") == 0) {
                background = 1;
                args[arg_count-1] = NULL;
            }
            execute_command(args, background);
        }
    }

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}
