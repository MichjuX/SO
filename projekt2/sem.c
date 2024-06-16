#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define HAIRCUT_TIME 3 // Czas trwania strzyżenia

sem_t waitingRoom;       // Semafor dla krzeseł w poczekalni
sem_t barberChair;       // Semafor dla fotela fryzjerskiego
sem_t barberPillow;      // Semafor do informowania fryzjera o obecności klienta
sem_t seatBelt;          // Semafor do synchronizacji zakończenia strzyżenia
sem_t nextCustomer;      // Semafor do informowania o następnym kliencie
pthread_mutex_t printLock; // Mutex do synchronizacji drukowania
pthread_mutex_t queueLock; // Mutex do synchronizacji kolejki

int allDone = 0; // Flaga informująca o zakończeniu pracy fryzjera
int customersInWaitingRoom = 0; // Liczba klientów w poczekalni
int rejections = 0; // Liczba odrzuconych klientów
int currentCustomer = -1; // Identyfikator bieżącego klienta
int *waitingQueue; // Kolejka klientów w poczekalni
int *rejectedQueue; // Kolejka odrzuconych klientów
int rejectedCount = 0; // Liczba odrzuconych klientów
int servicedCustomers = 0; // Liczba obsłużonych klientów
int infoFlag = 0; // Flaga określająca, czy program został uruchomiony z parametrem -info

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
    if (infoFlag) { // Wyświetl listy tylko jeśli infoFlag jest ustawione na 1
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
    while (!allDone) { // Dopóki fryzjer nie zakończy pracy
        if (customersInWaitingRoom == 0) { // Jeśli nie ma klientów
            printf("Fryzjer śpi.\n");
            sem_wait(&nextCustomer); // Fryzjer czeka na klienta
        }

        sem_wait(&barberPillow); // Fryzjer czeka, aż klient będzie gotowy

        if (allDone) break; // Jeśli fryzjer skończył pracę, koniec pętli
	
        //Jeśli muteks jest już zablokowany przez inny wątek, obecny wątek będzie czekać, aż muteks zostanie odblokowany.
        pthread_mutex_lock(&queueLock);
        
        // Pobranie pierwszego klienta z kolejki
        currentCustomer = waitingQueue[0];
        for (int i = 0; i < customersInWaitingRoom - 1; i++) {
            waitingQueue[i] = waitingQueue[i + 1]; // Przesunięcie reszty kolejki do przodu
        }
        customersInWaitingRoom--; // Zmniejszenie liczby klientów w poczekalni
        
        //Po zakończeniu dostępu do chronionego zasobu wątek odblokowuje muteks, pozwalając innym wątkom na uzyskanie dostępu do tego zasobu.
        pthread_mutex_unlock(&queueLock);

        printf("Fryzjer strzyże klienta %d...\n", currentCustomer);
        printStatus();
        printInfo();
        sleep(HAIRCUT_TIME);
        printf("Fryzjer skończył strzyżenie klienta %d.\n", currentCustomer);
        currentCustomer = -1; // Zaktualizowanie stanu fotela
        printStatus(); // Aktualizacja statusu przed wypuszczeniem klienta z salonu

        sem_post(&seatBelt); // Sygnalizowanie zakończenia strzyżenia
        sem_post(&nextCustomer); // Informowanie następnego klienta
    }

    pthread_exit(NULL);
}

void *customer(void *num) {
    int id = *(int *)num; // Pobranie ID klienta
    free(num); // Zwolnienie pamięci

    int arrivalTime = rand() % (HAIRCUT_TIME * 3000000); // Losowy czas przybycia klienta w mikrosekundach
    usleep(arrivalTime);

    printf("Klient %d przychodzi.\n", id);

    if (sem_trywait(&waitingRoom) == 0) { // Jeśli jest miejsce w poczekalni
        pthread_mutex_lock(&queueLock); // Blokada kolejki
        waitingQueue[customersInWaitingRoom++] = id; // Dodanie klienta do kolejki
        pthread_mutex_unlock(&queueLock); // Odblokowanie kolejki

        printf("Klient %d siada w poczekalni.\n", id);
        printStatus();
        printInfo(); // Dodane wywołanie funkcji printInfo()

        sem_post(&nextCustomer); // Informowanie fryzjera, że klient jest gotowy

        sem_wait(&barberChair); // Oczekiwanie na fotel fryzjerski

        sem_post(&waitingRoom); // Zwolnienie miejsca w poczekalni

        sem_post(&barberPillow); // Informowanie fryzjera, że klient jest gotowy na strzyżenie

        sem_wait(&seatBelt); // Oczekiwanie na zakończenie strzyżenia

        printf("Klient %d opuszcza salon.\n", id);
        servicedCustomers++;

        if (servicedCustomers >= NUM_CUSTOMERS) {
            allDone = 1;
        }

        sem_post(&barberChair);

    } else {
        pthread_mutex_lock(&queueLock); // Blokada kolejki
        rejectedQueue[rejectedCount++] = id; // Dodanie klienta do kolejki odrzuconych
        pthread_mutex_unlock(&queueLock); // Odblokowanie kolejki
        rejections++; // Aktualizacja liczby odrzuconych klientów
        printf("Klient %d rezygnuje z wizyty (brak miejsca).\n", id);
        printStatus();
        printInfo(); // Dodane wywołanie funkcji printInfo()
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_chairs> <num_customers> [-info]\n", argv[0]);
        return 1;
    }

    NUM_CHAIRS = atoi(argv[1]);
    NUM_CUSTOMERS = atoi(argv[2]);

    // Przekształcenie argv na zmienne numeryczne
    waitingQueue = (int *)malloc(sizeof(int) * NUM_CHAIRS);
    rejectedQueue = (int *)malloc(sizeof(int) * NUM_CUSTOMERS);

    pthread_t btid;
    pthread_t tid[NUM_CUSTOMERS];

	//strcmp służy do porównywania dwóch ciągów znaków (łańcuchów) podanych jako argumenty
	//Warunek argc > 1 sprawdza, czy program został uruchomiony z przynajmniej jednym argumentem (oprócz nazwy programu)
    if (argc > 3 && strcmp(argv[3], "-info") == 0) {
	//porównuje pierwszy argument (argv[1]), który jest przekazywany z linii poleceń, z napisem -info. Funkcja strcmp zwraca 0, jeśli napisy są identyczne.
        infoFlag = 1; // Ustawienie flagi infoFlag na 1, jeśli program został uruchomiony z parametrem -info
    }

    sem_init(&waitingRoom, 0, NUM_CHAIRS);
    sem_init(&barberChair, 0, 1);
    sem_init(&barberPillow, 0, 0);
    sem_init(&seatBelt, 0, 0);
    sem_init(&nextCustomer, 0, 0); // Inicjalizacja nowego semafora
    pthread_mutex_init(&printLock, NULL);
    pthread_mutex_init(&queueLock, NULL);

    pthread_create(&btid, NULL, barber, NULL);

    srand(time(NULL));

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        int *num = malloc(sizeof(int));
        *num = i + 1;
        pthread_create(&tid[i], NULL, customer, num);
        usleep(rand() % (HAIRCUT_TIME * 1000000) + 1); // Skrócenie czasu pomiędzy pojawieniem się nowych klientów
		//Rozkładamy w czasie tworzenie nowych wątków klientów, poniewaz jezeli wszystkie wątki byłyby tworzone jednocześnie, mogłoby to 
		//obciążyć system lub prowadzić do nieprzewidywalnych wyników.
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(tid[i], NULL); // Oczekiwanie na zakończenie wątków klientów
    }

    allDone = 1; // Zakończenie pracy fryzjera
    sem_post(&barberPillow); // Sygnalizowanie fryzjerowi, że ma zakończyć pracę
    pthread_join(btid, NULL); // Oczekiwanie na zakończenie wątku fryzjera

    // Zniszczenie semaforów i mutexów
    sem_destroy(&waitingRoom);
    sem_destroy(&barberChair);
    sem_destroy(&barberPillow);
    sem_destroy(&seatBelt);
    sem_destroy(&nextCustomer); 
    pthread_mutex_destroy(&printLock);
    pthread_mutex_destroy(&queueLock);

    free(waitingQueue);
    free(rejectedQueue);

    return 0;
}
