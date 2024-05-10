#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_THREADS 100
#define MAX_ITERATIONS 1000000

int counter = 0;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

void *thread_function(void *arg) {
    int ITER = *((int *) arg);
    for (int i = 0; i < ITER; i++) {
        pthread_mutex_lock(&mymutex);
        counter++;
        pthread_mutex_unlock(&mymutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <number_of_threads> <iterations_per_thread>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    int ITER = atoi(argv[2]);

    if (N <= 0 || ITER <= 0 || N > MAX_THREADS) {
        printf("Invalid arguments\n");
        return EXIT_FAILURE;
    }

    pthread_t threads[N];
    int thread_args[N];

    for (int i = 0; i < N; i++) {
        thread_args[i] = ITER;
        if (pthread_create(&threads[i], NULL, thread_function, &thread_args[i]) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < N; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }
    }

    printf("Final value of counter: %d\n", counter);

    return EXIT_SUCCESS;
}
