#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

void *banker(void *ptr);
void *watcher();

int account = 0;
pthread_mutex_t transfer_mutex    = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  transfer_var  = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;


int  main() {
    int iret1, iret2, iret3;
    int bCount = 16;
    pthread_t threads[bCount], watcherT;
    int tret[10];

    long i;
    for(i=0; i<bCount; i++) {
        tret[i] = pthread_create( &threads[i], NULL, banker,(void*)i);
        if (tret[i]) {
          fprintf(stderr, "ERROR; return code from banker  pthread_create() is %d\n", tret[i]);
          exit(EXIT_FAILURE);
       }
    }

    int wret = pthread_create( &watcherT, NULL, (void*)watcher,NULL);
    if (wret) {
      fprintf(stderr, "ERROR; return code from watcher pthread_create() is %d\n", wret);
      exit(EXIT_FAILURE);
    }

    for(i=0; i<bCount; i++) {
        pthread_join( threads[i], NULL);
    }

    pthread_join( watcherT, NULL);

    exit(EXIT_SUCCESS);
}

void *banker(void* ptr) {
    long nr = (long) ptr;

    while(1) {
        pthread_mutex_lock( &transfer_mutex );
        int value = 100;
        account += value;
        printf("banker nr %ld added %d, account: %d\n", nr, value, account);

        if (account % 1000 == 0) {
            pthread_cond_signal(&transfer_var);
              printf("  Watcher signaled\n");
        }

        pthread_mutex_unlock( &transfer_mutex );

        usleep(500*1000);

    }

}

  void *watcher() {
    printf("Account watcher start\n");
    while(1) {
        pthread_mutex_lock(&transfer_mutex);

        while (account % 1000 != 0)
            pthread_cond_wait(&transfer_var, &transfer_mutex);

        printf("\tAccount watcher: %d $$\n", account);
        //account += 100; // !!!
        pthread_mutex_unlock(&transfer_mutex);
        sleep(1);
      }
}