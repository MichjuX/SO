#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_PHILOSOPHERS 5

pthread_mutex_t forks[NUM_PHILOSOPHERS];

void *philosopher(void *arg) {
    int phil_id = *((int *)arg);
    int left_fork = phil_id;
    int right_fork = (phil_id + 1) % NUM_PHILOSOPHERS;

    while (1) {
        printf("Philosopher %d is thinking.\n", phil_id);
        sleep(1);

        pthread_mutex_lock(&forks[left_fork]);
        printf("Philosopher %d picked up left fork.\n", phil_id);
        sleep(1);

        pthread_mutex_lock(&forks[right_fork]);
        printf("Philosopher %d picked up right fork.\n", phil_id);
        sleep(1);

        printf("Philosopher %d is eating.\n", phil_id);
        sleep(2);

        pthread_mutex_unlock(&forks[right_fork]);
        printf("Philosopher %d put down right fork.\n", phil_id);

        pthread_mutex_unlock(&forks[left_fork]);
        printf("Philosopher %d put down left fork.\n", phil_id);
    }

    return NULL;
}

int main() {
    pthread_t philosophers[NUM_PHILOSOPHERS];
    int phil_ids[NUM_PHILOSOPHERS];

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        phil_ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher, &phil_ids[i]);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&forks[i]);
    }

    return 0;
}
