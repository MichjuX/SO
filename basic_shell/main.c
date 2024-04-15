#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 64
#define MAX_COMMAND_LEN 1024
#define HISTORY_FILE "history.txt"
#define MAX_HISTORY_COMMANDS 20

// Zmienna globalna przechowująca historię poleceń
char command_history[MAX_HISTORY_COMMANDS][MAX_COMMAND_LEN];
int command_count = 0;

void execute_command(char *command, char *args[], int *output_redirected) {
    // Dodaj polecenie do historii
    if (command_count < MAX_HISTORY_COMMANDS) {
        strcpy(command_history[command_count], command);
        command_count++;
    } else {
        // Przesuń starsze polecenia w historii
        for (int i = 0; i < MAX_HISTORY_COMMANDS - 1; ++i) {
            strcpy(command_history[i], command_history[i + 1]);
        }
        strcpy(command_history[MAX_HISTORY_COMMANDS - 1], command);
    }

    // Wykonaj polecenie
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Otwórz plik docelowy w trybie dołączania tylko jeśli przekierowanie wyjścia jest włączone
        if (*output_redirected) {
            int output_fd = open("output.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            // Przekieruj wyjście standardowe do pliku
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        execvp(command, args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

// Obsługa sygnału SIGQUIT - wyświetlenie historii poleceń
void sigquit_handler(int sig) {
    printf("Command History:\n");
    for (int i = 0; i < command_count; ++i) {
        printf("%d: %s\n", i + 1, command_history[i]);
    }
}

int main() {
    // Ustaw obsługę sygnału SIGQUIT
    signal(SIGQUIT, sigquit_handler);

    char command_line[MAX_COMMAND_LEN];
    char *args[MAX_ARGS];
    char *token;
    const char delim[] = " \n\t";
    const char background_delim[] = "&";
    const char output_redirect_delim[] = ">>"; // przekierowanie
    const char pipe_delim[] = "|"; // potoki

    int output_redirected = 0;

    while (1) {
        printf("$ ");
        fflush(stdout);

        if (!fgets(command_line, sizeof(command_line), stdin)) {
            break;
        }

        // Sprawdź, czy ostatnim słowem jest '&'
        int run_in_background = 0;
        if ((token = strtok(command_line, background_delim)) != NULL) {
            char *last_word = NULL;
            while ((token = strtok(NULL, background_delim)) != NULL) {
                last_word = token;
            }
            if (last_word && strcmp(last_word, "") == 0) {
                run_in_background = 1;
            } else {
                strtok(command_line, "\n"); // Usuń znak nowej linii z ostatniego słowa
            }
        }

        // Sprawdź, czy występuje przekierowanie wyjścia
        char *output_redirect = strstr(command_line, output_redirect_delim);
        if (output_redirect != NULL) {
            *output_redirect = '\0'; // Zakończ argument przed znakiem przekierowania
            output_redirect += strlen(output_redirect_delim); // Przesuń wskaźnik za znak przekierowania
            output_redirected = 1;
        } else {
            output_redirected = 0;
        }

        // Podziel polecenie na komendy za pomocą potoku
        int arg_count = 0;
        token = strtok(command_line, pipe_delim);
        while (token != NULL) {
            args[arg_count++] = token;
            token = strtok(NULL, pipe_delim);
        }
        args[arg_count] = NULL;

        // Utwórz potok dla każdej komendy, z wyjątkiem ostatniej
        int pipefd[2];
        int prev_read_end = STDIN_FILENO;
        for (int i = 0; i < arg_count; ++i) {
            if (i < arg_count - 1) {
                pipe(pipefd);
            }

            char *cmd_args[MAX_ARGS];
            char *cmd_token = strtok(args[i], delim);
            int cmd_arg_count = 0;
            while (cmd_token != NULL) {
                cmd_args[cmd_arg_count++] = cmd_token;
                cmd_token = strtok(NULL, delim);
            }
            cmd_args[cmd_arg_count] = NULL;

            execute_command(cmd_args[0], cmd_args, &output_redirected);

            if (i < arg_count - 1) {
                close(pipefd[1]); // Zamykaj końcówkę piszącą
                prev_read_end = pipefd[0]; // Ustaw końcówkę czytającą dla następnej komendy
            }
        }

        // Czekaj na zakończenie wszystkich procesów potomnych
        for (int i = 0; i < arg_count; ++i) {
            wait(NULL);
        }
    }

    return 0;
}
