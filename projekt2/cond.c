#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h> // Dodanie nagłówka dla strcmp

#define NUM_CHAIRS 10 // Liczba krzeseł w poczekalni
#define NUM_CUSTOMERS 20 // Liczba klientów
#define HAIRCUT_TIME 3 // Czas strzyżenia

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex do synchronizacji
pthread_cond_t cond_barber = PTHREAD_COND_INITIALIZER; // Zmienna warunkowa dla fryzjera
pthread_cond_t cond_customer = PTHREAD_COND_INITIALIZER; // Zmienna warunkowa dla klientów

int allDone = 0; // Flaga zakończenia
int customersInWaitingRoom = 0; // Liczba klientów w poczekalni
int rejections = 0; // Liczba klientów, którzy nie dostali się do poczekalni
int currentCustomer = -1; // Numer aktualnie strzyżonego klienta
int waitingQueue[NUM_CHAIRS]; // Kolejka klientów w poczekalni
int rejectedQueue[NUM_CUSTOMERS]; // Lista klientów, którzy nie dostali się do poczekalni
int rejectedCount = 0; // Licznik odrzuconych klientów

void printStatus() {
    printf("Rezygnacja: %d Poczekalnia: %d/%d [Fotel: %d]\n",
           rejections, customersInWaitingRoom, NUM_CHAIRS, currentCustomer);
}

void printInfo() {
    printf("Kolejka oczekujących: ");
    for (int i = 0; i < customersInWaitingRoom; i++) {
        printf("%d ", waitingQueue[i]);
    }
    printf("\nLista odrzuconych: ");
    for (int i = 0; i < rejectedCount; i++) {
        printf("%d ", rejectedQueue[i]);
    }
    printf("\n");
}

void *barber(void *arg) {
    while (!allDone) {
        pthread_mutex_lock(&mutex);
        
        // Jeśli nie ma klientów, fryzjer czeka
        while (customersInWaitingRoom == 0 && !allDone) {
            pthread_cond_wait(&cond_barber, &mutex);
        }
        
        // Jeśli koniec strzyżenia, wyjdź z pętli
        if (allDone) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Strzyżenie klienta (czas strzyżenia)
        currentCustomer = waitingQueue[0];
        for (int i = 0; i < customersInWaitingRoom - 1; i++) {
            waitingQueue[i] = waitingQueue[i + 1];
        }
        customersInWaitingRoom--;
        printStatus();
        pthread_cond_signal(&cond_customer);
        pthread_mutex_unlock(&mutex);

        printf("Fryzjer strzyże klienta %d...\n", currentCustomer);
        sleep(HAIRCUT_TIME);
        printf("Fryzjer skończył strzyżenie klienta %d.\n", currentCustomer);

        currentCustomer = -1;
        printStatus();
    }
    pthread_exit(NULL);
}

void *customer(void *num) {
    int id = *(int *)num;
    printf("Klient %d przychodzi.\n", id);

    pthread_mutex_lock(&mutex);

    // Sprawdza, czy jest miejsce w poczekalni
    if (customersInWaitingRoom < NUM_CHAIRS) {
        waitingQueue[customersInWaitingRoom++] = id;
        printStatus();

        // Budzi fryzjera, jeśli jest potrzebny
        pthread_cond_signal(&cond_barber);

        // Czeka na dostęp do fotela fryzjerskiego
        while (currentCustomer != id) {
            pthread_cond_wait(&cond_customer, &mutex);
        }

        pthread_mutex_unlock(&mutex);

        // Klient jest strzyżony
        printf("Klient %d siada na fotelu fryzjerskim.\n", id);
        printStatus();

        pthread_mutex_lock(&mutex);
    } else {
        // Brak miejsca w poczekalni, klient odchodzi
        rejectedQueue[rejectedCount++] = id;
        rejections++;
        printStatus();
        printf("Klient %d rezygnuje z wizyty (brak miejsca).\n", id);
    }

    pthread_mutex_unlock(&mutex);

    free(num);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t btid;
    pthread_t tid[NUM_CUSTOMERS];
    int infoFlag = (argc > 1 && strcmp(argv[1], "-info") == 0);

    // Tworzenie wątku fryzjera
    pthread_create(&btid, NULL, barber, NULL);

    srand(time(NULL));
    // Tworzenie wątków klientów
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        int *num = malloc(sizeof(int));
        *num = i + 1;
        pthread_create(&tid[i], NULL, customer, num);
        sleep(rand() % (HAIRCUT_TIME * 2)); // Nowy klient przychodzi w losowym czasie
    }

    // Oczekiwanie na zakończenie wątków klientów
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(tid[i], NULL);
    }

    // Zakończenie pracy fryzjera
    allDone = 1;
    pthread_cond_signal(&cond_barber); // Budzenie fryzjera, aby mógł się zakończyć
    pthread_join(btid, NULL);

    // Usuwanie zmiennych warunkowych i mutexu
    pthread_cond_destroy(&cond_barber);
    pthread_cond_destroy(&cond_customer);
    pthread_mutex_destroy(&mutex);

    if (infoFlag) {
        printInfo();
    }

    return 0;
}
