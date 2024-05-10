#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE_LENGTH 1024

void execute_command(char *args[], int background);
void execute_piped_commands(char *commands[], int num_commands);
void append_command_to_history(const char *command);
void sigquit_handler(int sig);
int check_cd_command(char *args[]);
int check_touch_command(char *args[]);

int main() {
    char *line = NULL;
    size_t line_buf_size = 0;
    ssize_t line_size;
    char *args[MAX_LINE_LENGTH / 2 + 1]; // Maksymalna liczba argumentów
    int background;

    signal(SIGQUIT, sigquit_handler);

    while (1) {
        printf("> ");
        line_size = getline(&line, &line_buf_size, stdin);

        if (line_size <= 0) {
            break; // Koniec pliku (EOF)
        }

        if (line[line_size - 1] == '\n') {
            line[line_size - 1] = '\0';
        }

        append_command_to_history(line); // Zapisz polecenie do historii

        // Sprawdź, czy linia zawiera znak `|`
        if (strchr(line, '|') != NULL) {
            // Podziel linię na polecenia
            char *commands[MAX_LINE_LENGTH / 2 + 1];
            int num_commands = 0;
            char *token = strtok(line, "|");
            while (token != NULL) {
                commands[num_commands++] = token;
                token = strtok(NULL, "|");
            }

            // Wykonaj potok poleceń
            execute_piped_commands(commands, num_commands);
        } else {
            // Podziel linię na słowa (argumenty)
            int arg_count = 0;
            char *token = strtok(line, " ");
            while (token != NULL) {
                args[arg_count++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_count] = NULL; // Ostatni element musi być NULL dla execvp

            if (arg_count == 0) {
                continue; // Pusta linia
            }

            // Sprawdź, czy ostatni argument to '&'
            background = 0;
            if (strcmp(args[arg_count - 1], "&") == 0) {
                background = 1;
                args[--arg_count] = NULL;
            }

            // Najpierw sprawdź `touch`, potem `cd`
            if (!check_touch_command(args) && !check_cd_command(args)) {
                execute_command(args, background);
            }
        }
    }

    free(line); // Zwolnienie pamięci zaalokowanej przez getline
    return 0;
}


int check_cd_command(char *args[]) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: argument expected\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }
    return 0;
}

void execute_command(char *args[], int background) {
    int redirect_output = 0;
    char *output_file = NULL;

    // Znajdź znak przekierowania i oddziel nazwę pliku
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            args[i] = NULL; // Usuń '>' z argumentów
            output_file = args[i + 1]; // Nazwa pliku jest następnym argumentem
            redirect_output = 1;
            break;
        }
    }

    pid_t pid = fork();
    if (pid == 0) { // Proces dziecka
        // Przekierowanie wyjścia, jeśli wymagane
        if (redirect_output) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Wykonanie polecenia
        if (execvp(args[0], args) < 0) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { // Proces rodzica
        int status;
        if (!background) {
            waitpid(pid, &status, 0); // Czekaj na zakończenie procesu dziecka, jeśli nie jest w tle
            if (WIFEXITED(status)) {
                printf("Kod powrotu: %d\n", WEXITSTATUS(status));
            }
        }
    } else {
        perror("fork"); // Nie udało się utworzyć procesu
    }
}


void execute_piped_commands(char *commands[], int num_commands) {
    int num_pipes = num_commands - 1;
    int pipe_fd[2 * num_pipes];

    // Tworzenie wszystkich potoków
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fd + 2 * i) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Proces dziecka
            if (i > 0) { // Nie pierwszy proces: podłącz do potoku z poprzednim procesem
                dup2(pipe_fd[2 * (i - 1)], STDIN_FILENO);
            }
            if (i < num_pipes) { // Nie ostatni proces: podłącz do potoku z następnym procesem
                dup2(pipe_fd[2 * i + 1], STDOUT_FILENO);
            }

            // Zamknij wszystkie deskryptory w potoku
            for (int j = 0; j < 2 * num_pipes; j++) {
                close(pipe_fd[j]);
            }

            // Podziel polecenie na argumenty i wykonaj je
            char *args[MAX_LINE_LENGTH / 2 + 1];
            int arg_count = 0;
            char *token = strtok(commands[i], " ");
            while (token != NULL) {
                args[arg_count++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_count] = NULL;

            if (execvp(args[0], args) < 0) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Zamknij wszystkie deskryptory w procesie rodzica
    for (int i = 0; i < 2 * num_pipes; i++) {
        close(pipe_fd[i]);
    }

    // Czekaj na zakończenie wszystkich procesów
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

void append_command_to_history(const char *command) {
    const char *home_directory = getenv("HOME"); // Pobierz ścieżkę do katalogu domowego
    if (home_directory == NULL) {
        perror("getenv");
        exit(EXIT_FAILURE);
    }

    char history_file_path[256];
    snprintf(history_file_path, sizeof(history_file_path), "%s/.my_shell_history", home_directory);

    FILE *file = fopen(history_file_path, "a"); // Otwórz plik historii do dopisywania
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s\n", command); // Dopisz polecenie do pliku historii
    fclose(file);
}

void sigquit_handler(int sig) {
    const char *home_directory = getenv("HOME");
    if (home_directory == NULL) {
        perror("getenv");
        exit(EXIT_FAILURE);
    }

    char history_file_path[256];
    snprintf(history_file_path, sizeof(history_file_path), "%s/.my_shell_history", home_directory);

    printf("Historia poleceń:\n");
    FILE *file = fopen(history_file_path, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        printf("%s", line);
    }
    printf("> ");

    free(line);
    fclose(file);
}

int check_touch_command(char *args[]) {
    if (strcmp(args[0], "touch") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "touch: argument expected\n");
        } else {
            for (int i = 1; args[i] != NULL; i++) {
                int fd = open(args[i], O_WRONLY | O_CREAT, 0644);
                if (fd < 0) {
                    perror("touch");
                } else {
                    close(fd);
                }
            }
        }
        return 1; // Potwierdzenie, że polecenie zostało obsłużone
    }
    return 0;
}

