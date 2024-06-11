#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h> // Dodanie nagłówka dla strcmp

#define NUM_CHAIRS 10 // Liczba krzeseł w poczekalni
#define NUM_CUSTOMERS 20 // Liczba klientów
#define HAIRCUT_TIME 3 // Czas strzyżenia

sem_t waitingRoom; // Semafor dla poczekalni
sem_t barberChair; // Semafor dla fotela fryzjerskiego
sem_t barberPillow; // Semafor do budzenia fryzjera
sem_t seatBelt; // Semafor do synchronizacji zakończenia strzyżenia
pthread_mutex_t printLock; // Mutex do synchronizacji wypisywania stanu
pthread_mutex_t queueLock; // Mutex do synchronizacji kolejki

int allDone = 0; // Flaga zakończenia
int customersInWaitingRoom = 0; // Liczba klientów w poczekalni
int rejections = 0; // Liczba klientów, którzy nie dostali się do poczekalni
int currentCustomer = -1; // Numer aktualnie strzyżonego klienta
int waitingQueue[NUM_CHAIRS]; // Kolejka klientów w poczekalni
int rejectedQueue[NUM_CUSTOMERS]; // Lista klientów, którzy nie dostali się do poczekalni
int rejectedCount = 0; // Licznik odrzuconych klientów

void printStatus() {
    pthread_mutex_lock(&printLock);
    printf("Rezygnacja: %d Poczekalnia: %d/%d [Fotel: %d]\n",
           rejections, customersInWaitingRoom, NUM_CHAIRS, currentCustomer);
    pthread_mutex_unlock(&printLock);
}

void printInfo() {
    pthread_mutex_lock(&printLock);
    printf("Kolejka oczekujących: ");
    for (int i = 0; i < customersInWaitingRoom; i++) {
        printf("%d ", waitingQueue[i]);
    }
    printf("\nLista odrzuconych: ");
    for (int i = 0; i < rejectedCount; i++) {
        printf("%d ", rejectedQueue[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&printLock);
}

void *barber(void *arg) {
    while (!allDone) {
        // Czeka, aż zostanie obudzony przez klienta
        sem_wait(&barberPillow);

        // Jeśli koniec strzyżenia, wyjdź z pętli
        if (allDone) break;

        // Strzyżenie klienta (czas strzyżenia)
        printf("Fryzjer strzyże klienta %d...\n", currentCustomer);
        sleep(HAIRCUT_TIME);
        printf("Fryzjer skończył strzyżenie klienta %d.\n", currentCustomer);

        // Zwalnia fotel fryzjerski
        sem_post(&seatBelt);
        currentCustomer = -1;
        printStatus();
    }
    pthread_exit(NULL);
}

void *customer(void *num) {
    int id = *(int *)num;
    printf("Klient %d przychodzi.\n", id);

    // Sprawdza, czy jest miejsce w poczekalni
    if (sem_trywait(&waitingRoom) == 0) {
        pthread_mutex_lock(&queueLock);
        waitingQueue[customersInWaitingRoom++] = id;
        pthread_mutex_unlock(&queueLock);

        printf("Klient %d siada w poczekalni.\n", id);
        printStatus();

        // Czeka na dostęp do fotela fryzjerskiego
        sem_wait(&barberChair);

        pthread_mutex_lock(&queueLock);
        for (int i = 0; i < customersInWaitingRoom - 1; i++) {
            waitingQueue[i] = waitingQueue[i + 1];
        }
        customersInWaitingRoom--;
        pthread_mutex_unlock(&queueLock);

        // Zwalnia miejsce w poczekalni
        sem_post(&waitingRoom);

        // Budzi fryzjera
        currentCustomer = id;
        sem_post(&barberPillow);

        // Zajmuje fotel fryzjerski
        printf("Klient %d siada na fotelu fryzjerskim.\n", id);
        printStatus();

        // Czeka na zakończenie strzyżenia
        sem_wait(&seatBelt);

        // Zwalnia fotel fryzjerski
        sem_post(&barberChair);

        printf("Klient %d opuszcza salon.\n", id);
    } else {
        // Brak miejsca w poczekalni, klient odchodzi
        pthread_mutex_lock(&queueLock);
        rejectedQueue[rejectedCount++] = id;
        pthread_mutex_unlock(&queueLock);
        rejections++;
        printf("Klient %d rezygnuje z wizyty (brak miejsca).\n", id);
        printStatus();
    }

    free(num);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t btid;
    pthread_t tid[NUM_CUSTOMERS];
    int infoFlag = (argc > 1 && strcmp(argv[1], "-info") == 0);

    // Inicjalizacja semaforów i mutexów
    sem_init(&waitingRoom, 0, NUM_CHAIRS);
    sem_init(&barberChair, 0, 1);
    sem_init(&barberPillow, 0, 0);
    sem_init(&seatBelt, 0, 0);
    pthread_mutex_init(&printLock, NULL);
    pthread_mutex_init(&queueLock, NULL);

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
    sem_post(&barberPillow); // Budzenie fryzjera, aby mógł się zakończyć
    pthread_join(btid, NULL);

    // Usuwanie semaforów i mutexów
    sem_destroy(&waitingRoom);
    sem_destroy(&barberChair);
    sem_destroy(&barberPillow);
    sem_destroy(&seatBelt);
    pthread_mutex_destroy(&printLock);
    pthread_mutex_destroy(&queueLock);

    if (infoFlag) {
        printInfo();
    }

    return 0;
}
