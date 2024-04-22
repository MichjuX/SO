#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define MAX_COMMAND_LEN 1024
#define HISTORY_FILE "./history.txt"

void execute_command(char *command, char *args[], int output_redirected, char *output_file) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (output_redirected) {
            int output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        execvp(command, args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status)) {
            fprintf(stderr, "Komenda \"%s\" nie została poprawnie wykonana.\n", command);
        }
    }
}

void execute_script(char *script_path) {
    char *interpreter = "/bin/bash";
    char *script_args[] = {interpreter, script_path, NULL};
    execvp(interpreter, script_args);
    perror("execvp");
    exit(EXIT_FAILURE);
}

void display_history() {
    FILE *file = fopen(HISTORY_FILE, "r");
    if (file) {
        char line[MAX_COMMAND_LEN];
        while (fgets(line, sizeof(line), file)) {
            printf("%s", line);
        }
        fclose(file);
    }
}

void handle_sigquit(int sig) {
    display_history();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        execute_script(argv[1]);
        return 0;
    }

    signal(SIGQUIT, handle_sigquit);

    char command_line[MAX_COMMAND_LEN];
    char *args[MAX_ARGS];
    const char background_delim[] = "&";
    const char output_redirect_delim[] = ">>";
    const char pipe_delim[] = "|";
    const char delim[] = " \n\t";

    while (1) {
        printf("$ ");
        fflush(stdout);

        if (!fgets(command_line, sizeof(command_line), stdin)) {
            break;
        }

        FILE *history_file = fopen(HISTORY_FILE, "a");
        if (history_file) {
            fprintf(history_file, "%s", command_line);
            fclose(history_file);
        } else {
            perror("Unable to open history file");
        }

        int output_redirected = 0;
        int run_in_background = 0;
        char *output_file = NULL;

        char *token = strtok(command_line, background_delim);
        if (token != NULL) {
            char *last_word = NULL;
            while ((token = strtok(NULL, background_delim)) != NULL) {
                last_word = token;
            }
            if (last_word && strcmp(last_word, "") == 0) {
                run_in_background = 1;
            } else {
                strtok(command_line, "\n");
            }
        }

        char *output_redirect = strstr(command_line, output_redirect_delim);
        if (output_redirect != NULL) {
            *output_redirect = '\0';
            output_redirect += strlen(output_redirect_delim);
            output_redirected = 1;
            output_file = strtok(output_redirect, delim);
        }

        int arg_count = 0;
        token = strtok(command_line, pipe_delim);
        while (token != NULL) {
            args[arg_count++] = token;
            token = strtok(NULL, pipe_delim);
        }
        args[arg_count] = NULL;

        int prev_read_end = STDIN_FILENO;
        for (int i = 0; i < arg_count; ++i) {
            int pipefd[2];
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

            pid_t pid = fork();
            if (pid == 0) {
                dup2(prev_read_end, STDIN_FILENO);
                if (i < arg_count - 1) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]);
                } else {
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
                execvp(cmd_args[0], cmd_args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (i < arg_count - 1) {
                close(pipefd[1]);
                prev_read_end = pipefd[0];
            }
        }

        for (int i = 0; i < arg_count; ++i) {
            wait(NULL);
        }
    }

    return 0;
}
