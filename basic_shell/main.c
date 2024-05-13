#include <stdio.h> //Zapewnia funkcje do operacji wejścia/wyjścia.
#include <stdlib.h> //Zapewnia funkcje ogólne, takie jak alokacja pamięci (malloc, free) i kontrola procesu (exit)
#include <unistd.h> //Zapewnia dostęp do funkcji systemowych, takich jak fork, execvp, dup2, itp.
#include <string.h> //Zapewnia funkcje do manipulacji łańcuchami znaków, takie jak strcpy, strtok, itp.
#include <sys/types.h> //Zawiera definicje typów danych używanych w systemowych wywołaniach funkcji.
#include <sys/wait.h> //Zapewnia funkcje związane z oczekiwaniem na zakończenie procesów potomnych.
#include <fcntl.h> //Zapewnia funkcje do manipulacji deskryptorami plików.
#include <signal.h> //Zapewnia obsługę sygnałów.
#include <errno.h> //Zapewnia dostęp do zmiennych globalnych errno i funkcji perror do obsługi błędów.

#define MAX_ARGS 64
#define MAX_COMMAND_LEN 1024

char current_directory[MAX_COMMAND_LEN];

//służy do wykonania skryptu shellowego, który jest przekazany jako argument funkcji.
void execute_script(char *script_path) {
    char *interpreter = "./main"; //Tworzy wskaźnik interpreter (odczytuje i wykonuje polecenia zawarte w skrypcie), który wskazuje na ścieżkę do interpretera naszej powłoki.
    char *script_args[] = {interpreter, script_path, NULL};
    //Tworzy tablicę script_args, która zawiera argumenty przekazane do interpretera. Pierwszy argument to ścieżka do 
    //interpretera, drugi to ścieżka do skryptu, a trzeci to NULL, co jest wymagane przez funkcję execvp, aby zakończyć listę argumentów.
    execvp(script_args[0], script_args);
    //Wywołuje funkcję execvp, która zastępuje obecny proces nowym procesem, uruchamiając interpreter bash z podanymi argumentami. 
    //Funkcja execvp szuka interpretera bash w podanej ścieżce (/bin/bash) i wykonuje skrypt shellowy, którego ścieżka została przekazana.

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

    // Otwarcie pliku do odczytu
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

//funkcja obsluguje sygnal SIGQUIT ktory jesy wysylany po wcisniecuy CTRL + \ 
//Po odebraniu sygnału SIGQUIT, funkcja drukuje nową linię, wyświetla historię poleceń 
//(prawdopodobnie wczytaną z pliku history.txt) za pomocą funkcji display_history, a następnie drukuje znak dolara $ jako znak zachęty do wprowadzenia nowego polecenia.
void handle_sigquit(int sig) {
    printf("\n");
    display_history();
    printf("$ ");
    fflush(stdout); //upewnienie sie ze dane wyjsciowe sa natychmiast zapisywane na ekranie
    signal(SIGQUIT, handle_sigquit); // Ponowne ustawienie obsługi SIGQUIT
}

//obsługująca sygnał SIGCHLD, który jest wysyłany, gdy dziecko procesu zakończy swoje działanie.
void handle_sigchld(int sig) {
    int saved_errno = errno; //Po odebraniu sygnału SIGCHLD, funkcja przechwytuje zmienną errno i zapisuje jej aktualną wartość.
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0); //czeka na zakończenie wszystkich procesów potomnych, ale bez blokowania wykonywania programu w przypadku braku dostępnych procesów do zakończenia.
    errno = saved_errno; //ponowne ustawienie errno na stara wartosc
}

int main(int argc, char *argv[]) {
    // Sprawdza, czy został podany argument (np. ścieżka do skryptu)
    if (argc > 1){
        execute_script(argv[1]);
        return 0;
    }
    
    //Ustawienie obsługi sygnałów SIGQUIT (Ctrl+\) i SIGCHLD (zakończenie dziecka)
    signal(SIGQUIT, handle_sigquit);
    signal(SIGCHLD, handle_sigchld);

    // Pobranie aktualnego katalogu roboczego
    getcwd(current_directory, sizeof(current_directory));

    char command_line[MAX_COMMAND_LEN];
    char *args[MAX_ARGS];
    const char background_delim[] = "&";
    const char output_redirect_delim[] = ">>";
    const char pipe_delim[] = "|";
    const char delim[] = " \n\t";

    while (1) {
        printf("$ ");
        fflush(stdout); //Wyczyszczenie bufora wyjścia, aby zapewnić natychmiastowe wyświetlenie znaku zachęty

         // Wczytanie polecenia z linii poleceń
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

        // Otwieranie pliku do zapisu historii
        FILE *history_file_ptr = fopen(history_file, "a");
        if (history_file_ptr) {
            fprintf(history_file_ptr, "%s", command_line);
            fclose(history_file_ptr);
        } else {
            perror("Unable to open history file");
        }

        int output_redirected = 0;
        char *output_file = NULL; //Wskaźnik na ciąg znaków, który będzie zawierał nazwę pliku wyjściowego, jeśli wyjście ma być przekierowane
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

        // Zmiana katalogu
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

        // Przekierowanie wyjścia
        char *output_redirect = strstr(command_line, output_redirect_delim);
        if (output_redirect != NULL) {
            *output_redirect = '\0';
            output_redirect += strlen(output_redirect_delim);
            output_redirected = 1;
            output_file = strtok(output_redirect, delim);
        }

        // printf("flaga_out: %d\n", output_redirected);

        // Dzielenie na argumenty
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
