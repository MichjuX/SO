#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_COMMAND_LEN 1024

char current_directory[MAX_COMMAND_LEN];

void execute_script(char *script_path) {
    char *interpreter = "./main";
    char *script_args[] = {interpreter, script_path, NULL};
    // execvp(interpreter, script_args);
    execvp(script_args[0], script_args);
    perror("execvp");
    exit(EXIT_FAILURE);
}

void display_history() {
    char *home_directory = getenv("HOME");
    if (home_directory == NULL) {
        fprintf(stderr, "Error: HOME environment variable is not set.\n");
        return;
    }

    char history_file[MAX_COMMAND_LEN];
    snprintf(history_file, sizeof(history_file), "%s/history.txt", home_directory);

    FILE *file = fopen(history_file, "r");
    if (file) {
        char line[MAX_COMMAND_LEN];
        while (fgets(line, sizeof(line), file)) {
            printf("%s", line);
        }
        fclose(file);
    } else {
        perror("Unable to open history file");
    }
}

void handle_sigquit(int sig) {
    printf("\n");
    display_history();
    printf("$ ");
    fflush(stdout);
    signal(SIGQUIT, handle_sigquit); // Ponowne ustawienie obsługi SIGQUIT
}

void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0);
    errno = saved_errno;
}

int main(int argc, char *argv[]) {
    if (argc > 1){
        execute_script(argv[1]);
        return 0;
    }
    
    signal(SIGQUIT, handle_sigquit);
    signal(SIGCHLD, handle_sigchld);

    getcwd(current_directory, sizeof(current_directory));

    char command_line[MAX_COMMAND_LEN];
    char *args[MAX_ARGS];
    const char background_delim[] = "&";
    const char output_redirect_delim[] = ">>";
    const char pipe_delim[] = "|";
    const char delim[] = " \n\t";

    while (1) {
        printf("$ ");
        fflush(stdout);

        if (fgets(command_line, sizeof(command_line), stdin) == NULL) {
            if (feof(stdin)) {
                break;
            } else {
                perror("fgets");
                exit(EXIT_FAILURE);
            }
        }

        char *home_directory = getenv("HOME");
        if (home_directory == NULL) {
            fprintf(stderr, "Error: HOME environment variable is not set.\n");
            continue;
        }

        char history_file[MAX_COMMAND_LEN];
        snprintf(history_file, sizeof(history_file), "%s/history.txt", home_directory); 

        FILE *history_file_ptr = fopen(history_file, "a");
        if (history_file_ptr) {
            fprintf(history_file_ptr, "%s", command_line);
            fclose(history_file_ptr);
        } else {
            perror("Unable to open history file");
        }

        int output_redirected = 0;
        char *output_file = NULL;
        int run_in_background = 0;

        // Check if the command is to be run in background
        // char *token = strtok(command_line, background_delim);
        // Output redirection
        char *rn_background = strstr(command_line, background_delim);
        if (rn_background != NULL) {
            *rn_background = '\0';
            rn_background += strlen(background_delim);
            run_in_background = 1;
            printf("Running command in background.\n");
        }


        // printf("flaga_bg: %d\n", run_in_background);

        // Change directory command
        if (strncmp(command_line, "cd", 2) == 0) {
            char *new_dir = strtok(command_line + 2, delim);
            if (new_dir == NULL) {
                chdir(getenv("HOME"));
            } else {
                if (chdir(new_dir) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        // Output redirection
        char *output_redirect = strstr(command_line, output_redirect_delim);
        if (output_redirect != NULL) {
            *output_redirect = '\0';
            output_redirect += strlen(output_redirect_delim);
            output_redirected = 1;
            output_file = strtok(output_redirect, delim);
        }

        // printf("flaga_out: %d\n", output_redirected);

        // Parse command into arguments
        int arg_count = 0;
        char * token = strtok(command_line, pipe_delim);
        while (token != NULL) {
            args[arg_count++] = token;
            token = strtok(NULL, pipe_delim);
        }
        args[arg_count] = NULL;

        int prev_read_end = STDIN_FILENO; // Inicjalizacja zmiennej przechowującej końcówkę czytania, początkowo ustawiona na STDIN_FILENO (czytanie z wejścia standardowego)
        for (int i = 0; i < arg_count; ++i) { // Pętla przechodzi przez wszystkie elementy (komendy) zawarte w tablicy args[]
            int pipefd[2]; // Tablica dla deskryptorów plików potoku

            // Tworzenie potoku, jeśli nie jest to ostatnia komenda w potoku
            if (i < arg_count - 1) {
                pipe(pipefd); // Tworzenie potoku
            }

            char *cmd_args[MAX_ARGS]; // Tablica dla argumentów aktualnej komendy
            char *cmd_token = strtok(args[i], delim); // Dzielenie komendy na argumenty
            int cmd_arg_count = 0; // Licznik argumentów

            // Wypełnianie tablicy argumentów aktualnej komendy
            while (cmd_token != NULL) {
                cmd_args[cmd_arg_count++] = cmd_token;
                cmd_token = strtok(NULL, delim);
            }
            cmd_args[cmd_arg_count] = NULL; // Zakończenie tablicy NULL-em

            pid_t pid = fork(); // Tworzenie nowego procesu
            if (pid == 0) { // Kod dla procesu potomnego
                dup2(prev_read_end, STDIN_FILENO); // Przekierowanie wejścia z poprzedniego procesu
                if (i < arg_count - 1) {
                    dup2(pipefd[1], STDOUT_FILENO); // Przekierowanie wyjścia do potoku (jeśli to nie jest ostatnia komenda)
                    close(pipefd[0]); // Zamknięcie deskryptora czytania potoku
                } else {
                    // Jeśli przekierowanie wyjścia jest włączone, otwórz plik wyjściowy i przekieruj wyjście
                    if (output_redirected) {
                        int output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                        if (output_fd == -1) {
                            perror("open");
                            exit(EXIT_FAILURE);
                        }
                        dup2(output_fd, STDOUT_FILENO);
                        close(output_fd);
                    }
                }

                // Wykonanie komendy
                if (execvp(cmd_args[0], cmd_args) < 0){
                    perror("execvp"); // Wyświetlenie błędu, jeśli execvp się nie powiedzie
                    exit(EXIT_FAILURE);
                }
                
            } else if (pid > 0) {
                int status;
                // printf("%d", run_in_background);
                if (!run_in_background){
                    waitpid(pid, &status, 0);
                }
            } else if (pid < 0) { // Obsługa błędu podczas tworzenia procesu potomnego
                perror("fork");
                exit(EXIT_FAILURE);
            }

            // Zamknięcie deskryptora pisania potoku w procesie rodzicielskim
            if (i < arg_count - 1) {
                close(pipefd[1]);
                prev_read_end = pipefd[0]; // Ustawienie końca czytania na deskryptor czytania potoku dla następnej komendy
            }
        }

    }

    return 0;
}
