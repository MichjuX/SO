#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CHAIRS 10
#define HAIRCUT_TIME 2

int waiting_clients = 0;
int chair_count = MAX_CHAIRS;
int rejections = 0;
int current_client = -1;
pthread_mutex_t mutex;
pthread_cond_t cond_barber;
pthread_cond_t cond_client;
pthread_mutex_t print_lock;

void print_status() {
    pthread_mutex_lock(&print_lock);
    printf("Rezygnacja:%d Poczekalnia: %d/%d [Fotel: %d]\n", rejections, waiting_clients, chair_count, current_client);
    pthread_mutex_unlock(&print_lock);
}

void *client(void *num) {
    int id = *(int *)num;
    usleep(rand() % (HAIRCUT_TIME * 1000000)); // Random arrival time (in microseconds)
    pthread_mutex_lock(&print_lock);
    printf("Client %d arrived.\n", id);
    pthread_mutex_unlock(&print_lock);

    pthread_mutex_lock(&mutex);
    if (waiting_clients < chair_count) {
        waiting_clients++;
        print_status();

        pthread_cond_signal(&cond_barber);
        pthread_cond_wait(&cond_client, &mutex);

        waiting_clients--;
        current_client = id;
        print_status();

        pthread_mutex_unlock(&mutex);
        usleep(HAIRCUT_TIME * 1000000); // Simulate haircut time

        pthread_mutex_lock(&print_lock);
        printf("Client %d is done with the haircut.\n", id);
        pthread_mutex_unlock(&print_lock);

        pthread_mutex_lock(&mutex);
        current_client = -1;
        pthread_mutex_unlock(&mutex);

        pthread_cond_signal(&cond_barber);
    } else {
        rejections++;
        print_status();
        pthread_mutex_unlock(&mutex);
    }
    free(num);
    return NULL;
}

void *barber(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (waiting_clients == 0) {
            current_client = -1; // Set to -1 to indicate no client being served
            print_status();
            pthread_cond_wait(&cond_barber, &mutex);
        }
        pthread_cond_signal(&cond_client);
        pthread_mutex_unlock(&mutex);

        usleep(HAIRCUT_TIME * 1000000); // Simulate time between finishing a haircut and starting a new one
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    pthread_t barber_thread;
    pthread_t client_threads[MAX_CHAIRS * 2];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_barber, NULL);
    pthread_cond_init(&cond_client, NULL);
    pthread_mutex_init(&print_lock, NULL);

    pthread_create(&barber_thread, NULL, barber, NULL);

    for (int i = 0; i < MAX_CHAIRS * 2; i++) {
        int *num = malloc(sizeof(int));
        *num = i;
        pthread_create(&client_threads[i], NULL, client, num);
        usleep(rand() % (HAIRCUT_TIME * 1000000)); // Random client arrival time
    }

    for (int i = 0; i < MAX_CHAIRS * 2; i++) {
        pthread_join(client_threads[i], NULL);
    }

    pthread_cancel(barber_thread);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_barber);
    pthread_cond_destroy(&cond_client);
    pthread_mutex_destroy(&print_lock);

    return 0;
}
