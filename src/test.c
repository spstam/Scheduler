#include <stdio.h>
#include <stdlib.h>
#include "mythreads.h"

#define NUM_THREADS 5
mysem_t semaphore;
mysem_t semaphore1;

mythr_t threads[NUM_THREADS];
int j=0;

void thread_function(void* arg){
    int *id =(int *)arg;
    printf("enter thread %d\n", *id);
    mythreads_sem_down(&semaphore);
    printf("Thread %d: Entering critical section\n", *id);
    for (int i = 0; i < 100; i++) {
        printf("Thread %d: Iteration %d\n", *id, i + 1);
        //mythreads_sleep(1); // Sleep for 1 second
        j++;
    }
    // mythreads_sleep(1);
    mythreads_sem_up(&semaphore);
    printf("Thread %d: Exiting critical section\n", *id);
    return;
}

int main() {
    if (mythreads_init() == -1) {
        fprintf(stderr, "Failed to initialize threading library.\n");
        return EXIT_FAILURE;
    }

    if (mythreads_sem_create(&semaphore, 1) == -1) {
        fprintf(stderr, "Failed to create semaphore.\n");
        return EXIT_FAILURE;
    }
    // mythreads_yield();
    for (int i = 0; i < NUM_THREADS; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        printf("going\n");
        if (mythreads_create(&threads[i], thread_function, id) == -1) {
            fprintf(stderr, "Failed to create thread %d.\n", i + 1);
            return EXIT_FAILURE;
        }
    }
    //printf("1\n");
    // print_thread_queue();
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("trying2join %d\n",i);
        mythreads_join(&threads[i]);
    }
    printf("%d\n", j);
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("destroying %d\n",i);
        mythreads_destroy(&threads[i]);
    }
    print_thread_queue();
    // mythreads_sleep(7);
    // mythreads_sem_destroy(&semaphore1);
    mythreads_sem_destroy(&semaphore);
    printf("All threads completed.\n");
    return EXIT_SUCCESS;
}