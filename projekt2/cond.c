#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define HAIRCUT_TIME 3 // Czas trwania strzyżenia

pthread_mutex_t waitingRoomLock; // Mutex do synchronizacji dostępu do poczekalni
pthread_cond_t barberReady; // Zmienna warunkowa sygnalizująca gotowość fryzjera
pthread_cond_t customerReady; // Zmienna warunkowa sygnalizująca gotowość klienta
pthread_cond_t haircutDone; // Zmienna warunkowa sygnalizująca zakończenie strzyżenia
pthread_mutex_t printLock; // Mutex do synchronizacji drukowania
pthread_mutex_t queueLock; // Mutex do synchronizacji kolejki

int allDone = 0; // Flaga informująca, że wszyscy klienci są obsłużeni
int customersInWaitingRoom = 0; // Liczba klientów w poczekalni
int rejections = 0; // Liczba odrzuconych klientów
int currentCustomer = -1; // ID aktualnie obsługiwanego klienta
int *waitingQueue; // Kolejka klientów oczekujących
int *rejectedQueue; // Kolejka odrzuconych klientów
int rejectedCount = 0; // Liczba odrzuconych klientów
int servicedCustomers = 0; // Liczba obsłużonych klientów
int infoFlag = 0; // Flaga wskazująca, czy program został uruchomiony z parametrem -info

int NUM_CHAIRS; // Liczba krzeseł w poczekalni
int NUM_CUSTOMERS; // Liczba klientów

void printStatus() {
    pthread_mutex_lock(&printLock);
    int barberChairStatus = (currentCustomer == -1) ? 0 : currentCustomer;
    printf("Rezygnacja:%d Poczekalnia:%d/%d [Fotel:%d]\n",
           rejections, customersInWaitingRoom, NUM_CHAIRS, barberChairStatus);
    pthread_mutex_unlock(&printLock);
}

void printInfo() {
    if (infoFlag) { 
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
}

void *barber(void *arg) {
    while (!allDone) {
        pthread_mutex_lock(&waitingRoomLock);
        
        while (customersInWaitingRoom == 0 && !allDone) {
            printf("Fryzjer śpi.\n");
            pthread_cond_wait(&customerReady, &waitingRoomLock);
        }
        
        if (allDone) {
            pthread_mutex_unlock(&waitingRoomLock);
            break;
        }
        
        currentCustomer = waitingQueue[0];
        for (int i = 0; i < customersInWaitingRoom - 1; i++) {
            waitingQueue[i] = waitingQueue[i + 1];
        }
        customersInWaitingRoom--;
        
        printf("Fryzjer strzyże klienta %d...\n", currentCustomer);
        printStatus();
        printInfo();
        
        pthread_mutex_unlock(&waitingRoomLock);
        sleep(HAIRCUT_TIME);

        pthread_mutex_lock(&waitingRoomLock);
        printf("Fryzjer skończył strzyżenie klienta %d.\n", currentCustomer);
        currentCustomer = -1;
        printStatus();

        pthread_cond_signal(&haircutDone);
        pthread_mutex_unlock(&waitingRoomLock);
    }
    pthread_exit(NULL);
}

void *customer(void *num) {
    int id = *(int *)num;
    free(num);

    int arrivalTime = rand() % (HAIRCUT_TIME * 3000000);
    usleep(arrivalTime);

    printf("Klient %d przychodzi.\n", id);

    pthread_mutex_lock(&waitingRoomLock);
    if (customersInWaitingRoom < NUM_CHAIRS) {
        waitingQueue[customersInWaitingRoom++] = id;
        printf("Klient %d siada w poczekalni.\n", id);
        printStatus();
        printInfo();
        
        pthread_cond_signal(&customerReady);

        while (currentCustomer != id) {
            pthread_cond_wait(&barberReady, &waitingRoomLock);
        }

        printf("Klient %d siada na fotelu fryzjerskim.\n", id);
        printStatus();

        while (currentCustomer != -1) {
            pthread_cond_wait(&haircutDone, &waitingRoomLock);
        }
        printf("Klient %d opuszcza salon.\n", id);
        servicedCustomers++;

        if (servicedCustomers >= NUM_CUSTOMERS) {
            allDone = 1;
            pthread_cond_signal(&customerReady);
        }
    } else {
        rejectedQueue[rejectedCount++] = id;
        rejections++;
        printf("Klient %d rezygnuje z wizyty (brak miejsca).\n", id);
        printStatus();
        printInfo();
    }
    pthread_mutex_unlock(&waitingRoomLock);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_chairs> <num_customers> [-info]\n", argv[0]);
        return 1;
    }

    NUM_CHAIRS = atoi(argv[1]);
    NUM_CUSTOMERS = atoi(argv[2]);

    waitingQueue = (int *)malloc(sizeof(int) * NUM_CHAIRS);
    rejectedQueue = (int *)malloc(sizeof(int) * NUM_CUSTOMERS);

    pthread_t btid;
    pthread_t tid[NUM_CUSTOMERS];

    if (argc > 3 && strcmp(argv[3], "-info") == 0) {
        infoFlag = 1;
    }

    pthread_mutex_init(&waitingRoomLock, NULL);
    pthread_cond_init(&barberReady, NULL);
    pthread_cond_init(&customerReady, NULL);
    pthread_cond_init(&haircutDone, NULL);
    pthread_mutex_init(&printLock, NULL);
    pthread_mutex_init(&queueLock, NULL);

    pthread_create(&btid, NULL, barber, NULL);

    srand(time(NULL));

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        int *num = malloc(sizeof(int));
        *num = i + 1;
        pthread_create(&tid[i], NULL, customer, num);
        usleep(rand() % (HAIRCUT_TIME * 1000000) + 1);
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(tid[i], NULL);
    }

    allDone = 1;
    pthread_cond_signal(&customerReady);
    pthread_join(btid, NULL);

    pthread_mutex_destroy(&waitingRoomLock);
    pthread_cond_destroy(&barberReady);
    pthread_cond_destroy(&customerReady);
    pthread_cond_destroy(&haircutDone);
    pthread_mutex_destroy(&printLock);
    pthread_mutex_destroy(&queueLock);

    free(waitingQueue);
    free(rejectedQueue);

    return 0;
}
