
#ifndef MYTHREADS_H 
#define MYTHREADS_H

#include <ucontext.h>
#include <time.h>

typedef struct mythread {
    ucontext_t context;
    enum {
        MYTHREAD_READY,
        MYTHREAD_RUNNING,
        MYTHREAD_SLEEPING,
        MYTHREAD_TERMINATED,
        MYTHREAD_BLOCKED
    } status;
    int id;
    time_t sleepingtime;
} mythr_t;

typedef struct thread_node {
    mythr_t *thread;
    struct thread_node *next;
} thread_node;

typedef struct thread_queue {
    thread_node *head;
    thread_node *tail;
    int size;
} thread_queue;

typedef struct {
    int value; // Semaphore value
    int initialised;
    thread_queue blocked; // Waiting threads
} mysem_t;

// Thread functions
int mythreads_init();
int mythreads_create(mythr_t *thr, void (*body)(void *), void *arg);
int mythreads_yield();
int mythreads_sleep(int secs);
int mythreads_join(mythr_t *thr);
int mythreads_destroy(mythr_t *thr);
int mythreads_gettid();


void print_thread_queue();
void print_ucontext(ucontext_t *context);
// Semaphore functions
int mythreads_sem_create(mysem_t *s, int val);
int mythreads_sem_down(mysem_t *s);
int mythreads_sem_up(mysem_t *s);
int mythreads_sem_destroy(mysem_t *s);

#endif
