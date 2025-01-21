#include "mythreads.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <bits/sigstack.h>
#include <errno.h>
#include <bits/sigaction.h>

// Global variables
thread_node *running_thread=NULL;
thread_node *blocked_thread=NULL;
thread_queue *t_queue=NULL;
static volatile sig_atomic_t fromdown = 0;
time_t last_time;
int sleepingThreads = 0, countthreads=0;
sigset_t new_set, old_set;

// Function to print the contents of the thread queue
void print_thread_queue() {
    printf("Thread queue contents:\n");
    thread_node *current_thread = t_queue->head;
    while(current_thread->next != t_queue->head) {
        printf("Thread ID: %d\n", current_thread->thread->id);
        current_thread=current_thread->next;
    }
    printf("exvos\n");
    printf("Thread ID: %d\n", current_thread->thread->id);
}

// Helper functions
int mythreads_gettid(){
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    int id = running_thread->thread->id;
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return id;
}

void thread_wrapper(void (*body)(void *), void *arg) {
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    // printf("im gonna execute %d\n", running_thread->thread->id);
    body(arg);
    
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    // Mark the thread as finished
    // printf("thread %d finished\n", running_thread->thread->id);
    running_thread->thread->status = MYTHREAD_TERMINATED;
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    // Yield control to the scheduler
    mythreads_yield();
}


int schedule() {
    sigprocmask(SIG_BLOCK, &new_set, &old_set);


    thread_node *temp = running_thread;
    thread_node *currthread = running_thread;

    // Check if the thread queue is empty
    if (t_queue == NULL) {sigprocmask(SIG_SETMASK, &old_set, NULL); return -1;}
    // print_thread_queue();
    //checks if called from blocked and got ready
    if (running_thread->thread->status == MYTHREAD_READY){
        // printf("ready %d", running_thread->thread->id);
        running_thread->thread->status = MYTHREAD_RUNNING;  
        fromdown = 0;
        // printf("swapping to %d\n", running_thread->thread->id);
        swapcontext(&blocked_thread->thread->context, &running_thread->thread->context);
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        return 0;
    }
    //finds the next available thread
    currthread = currthread->next;
    while (currthread->thread->status != MYTHREAD_READY && currthread->thread->status != MYTHREAD_SLEEPING){
        if (currthread == running_thread) {/*printf("cyrcled back\n");*/ break;}
        currthread = currthread->next;
    }
    
    //if running thread was running
    if (running_thread->thread->status == MYTHREAD_RUNNING){
        if (currthread == running_thread){
            //keep up the good work
            //you are the only one available
            // printf("only one available\n");
            sigprocmask(SIG_SETMASK, &old_set, NULL);
            return 0;
        }
        else {
            //change with the next available
            running_thread->thread->status = MYTHREAD_READY;
        }
    }
    //if running thread finished 
    else if (running_thread->thread->status == MYTHREAD_TERMINATED){
        if (currthread == running_thread){
            //deadlock
            // printf("deadlock\n");
            sigprocmask(SIG_SETMASK, &old_set, NULL);
            return -2;
        }
    }
    //thread now must be sleeping
    else if (running_thread->thread->status == MYTHREAD_SLEEPING){
        time_t curr_time = time(NULL);
        if (curr_time - last_time >= running_thread->thread->sleepingtime){
            last_time = curr_time;
            //time to wake up, so i change to my last self with sleepingtime = 0
            //or i save the blocked thread context and go back
            // printf("woke up %d\n", running_thread->thread->id);
            running_thread->thread->status = MYTHREAD_RUNNING;
            running_thread->thread->sleepingtime = 0;
            // flag = 0;
            if (fromdown == 1){
                fromdown = 0;
                // printf("swapping3 to %d\n", running_thread->thread->id);
                swapcontext(&blocked_thread->thread->context, &running_thread->thread->context);
            }
            sigprocmask(SIG_SETMASK, &old_set, NULL);
            return 0;
            
        }
        else {
            //not time to wake need to change back to a new thread
            running_thread->thread->sleepingtime -= (curr_time - last_time);
            last_time = curr_time;
            if (fromdown == 1){
                fromdown=0;
                // printf("swapping3 to %d\n", running_thread->thread->id);
                swapcontext(&blocked_thread->thread->context, &running_thread->thread->context);
                // printf("just swapped\n")
                sigprocmask(SIG_SETMASK, &old_set, NULL);
                return 0;
            }
        }
    }
    //swap with the next available 
    running_thread = currthread;
    if (currthread->thread->status != MYTHREAD_SLEEPING){
        currthread->thread->status = MYTHREAD_RUNNING;  
    }
    // printf("swapping to %d\n", currthread->thread->id);
    swapcontext(&temp->thread->context, &currthread->thread->context);
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return 0;
}


void alarm_handler(int signum) {
    if (signum == SIGALRM && t_queue->head != t_queue->head->next) {
        // write(STDOUT_FILENO, msg, 10);  // Safe alternative to printf
        schedule();
    }
}

int mythreads_init() {
    struct sigaction sa;
    struct itimerval timer;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGALRM);

    // Configure the timer to expire every 10 milliseconds
    timer.it_value.tv_sec = 0;        // First expiration
    timer.it_value.tv_usec = 100;   // 10 milliseconds
    timer.it_interval.tv_sec = 0;     // Subsequent expiration
    timer.it_interval.tv_usec = 100; // 10 milliseconds

    thread_node *main_thread = malloc(sizeof(thread_node));
    main_thread->thread = malloc(sizeof(mythr_t));
    if (main_thread == NULL) {
        return -1;
    }
    getcontext(&main_thread->thread->context);
    main_thread->thread->status = MYTHREAD_RUNNING;
    main_thread->thread->sleepingtime = 0;
    if (t_queue == NULL) {
        main_thread->thread->id = 0;
        t_queue = malloc(sizeof(thread_queue));
        t_queue->head = main_thread;
        t_queue->head->next = t_queue->head;
        t_queue->size = 1;
        t_queue->tail=main_thread;
        running_thread = main_thread; 
        // Start the timer
        setitimer(ITIMER_REAL, &timer, NULL);
        return 0;
    } else {
        // printf("Dont initialize twice\n");
        return -1;
    }
}

int mythreads_create(mythr_t *thr, void (*body)(void *), void *arg) {
    
    if (thr == NULL) {
        return -1;
    }
    sigprocmask(SIG_BLOCK, &new_set, &old_set);

    getcontext(&thr->context);
    thr->context.uc_stack.ss_sp = calloc(1, SIGSTKSZ);
    thr->context.uc_stack.ss_size = SIGSTKSZ;
    thr->context.uc_stack.ss_flags = 0;
    thr->context.uc_link = NULL;
    
    thr->status = MYTHREAD_READY;
    makecontext(&(thr->context), (void (*)())thread_wrapper, 2, body, arg);
    countthreads++;
    thr->id = countthreads;
    thr->sleepingtime = 0;
    t_queue->size += 1;

    thread_node *temp = malloc(sizeof(thread_node));
    temp->thread = thr;
    temp->next = t_queue->head;
    
    t_queue->tail->next=temp;
    t_queue->tail=temp;

    printf("creating thread %d\n", thr->id);
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return 0;

}

int mythreads_yield() {
    
    int res = schedule();
    if (res == -2) {
        printf("only one left\n");
        return -1;
    }
    return 0;   
}


int mythreads_sleep(int secs) {
    if (secs <= 0){
        return -1;
    }
    sigprocmask(SIG_BLOCK, &new_set, &old_set);

    last_time = time(NULL);
    running_thread->thread->status = MYTHREAD_SLEEPING;
    running_thread->thread->sleepingtime = secs;
    while (running_thread->thread->sleepingtime != 0){
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        mythreads_yield();
        sigprocmask(SIG_BLOCK, &new_set, &old_set);

    }
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return 0;
}


int mythreads_join(mythr_t *thr) {
    //checks that all threads are finished in order to continue
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    while (thr->status != MYTHREAD_TERMINATED){
        // printf("going in\n");
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        mythreads_yield();
    }
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return 0;
}

int mythreads_destroy(mythr_t *thr) {
    // printf("destroying\n");
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    thread_node *currthread = t_queue->head;
    while(currthread->next != t_queue->head && currthread->next->thread->id != thr->id) {
        // printf("%d\n",currthread->next->thread->id);
        currthread = currthread->next;
    }
    thread_node *temp = currthread->next;
    currthread->next = currthread->next->next;
    countthreads --;
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    free(temp->thread->context.uc_stack.ss_sp);
    // printf("out of loop temp=%p\n",temp);
    free(temp);
    return 0;
}

int mythreads_sem_create(mysem_t *s, int val) {
    if (s->initialised==1){
        printf("Already initialiazed\n");
        return -2;
    }
    if (s == NULL) return -1;
    
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    s->value = val;
    s->blocked.head = NULL;
    s->blocked.size = 0;
    s->initialised = 1;

    // printf("sema created\n");
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return 0;

}

int mythreads_sem_down(mysem_t *s) {
    if (s->initialised!=1){
        printf("Didnt initialiaze\n");
        return -2;
    }
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    // print_thread_queue();
    if(s->value == 0){
        thread_node *oldrunning  = running_thread;
        printf("going down %d\n", oldrunning->thread->id);
        blocked_thread = oldrunning;
        fromdown = 1;
        s->blocked.size += 1;
        running_thread->thread->status = MYTHREAD_BLOCKED;
        //finds the next ready or sleeping thread
        thread_node *temp1 = running_thread->next;
        while (temp1->thread->status == MYTHREAD_TERMINATED){
            if (temp1->next == running_thread) break;
            temp1 = temp1->next;
        }
        //updates the running thread
        running_thread = temp1;
        if (s->blocked.head == NULL){
            s->blocked.head = oldrunning;
            thread_node *temp = t_queue->head;
            while (temp->next != oldrunning){
                temp = temp->next;
            }
            //if what im going to delete is the tail
            if (temp->next == t_queue->tail){
                t_queue->tail = temp;
            }
            //if what im about to delete is the head
            if (temp->next == t_queue->head){
                printf("head\n");

                t_queue->head = t_queue->head->next;
                printf("new head %d\n", t_queue->head->thread->id);

            }
            temp->next = temp->next->next;
            oldrunning->next = s->blocked.head;
            s->blocked.tail=oldrunning;
        }
        else {
            thread_node *temp = s->blocked.tail;
            thread_node *temp1 = t_queue->head;
            while (temp1->next != oldrunning){
                temp1 = temp1->next;
            }
            if (temp1->next== t_queue->tail){
                t_queue->tail = temp1;
            }
            if (temp1->next == t_queue->head){
                printf("head\n");
                t_queue->head = t_queue->head->next;
                printf("new head %d\n", t_queue->head->thread->id);
            }
            temp1->next = temp1->next->next;
            oldrunning->next = s->blocked.head;
            temp->next = oldrunning;//ccc
            s->blocked.tail=oldrunning;
        }
        // print_thread_queue();
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        mythreads_yield();
    }else {
        s->value -= 1;
    }
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    return 0;
}

int mythreads_sem_up(mysem_t *s) {
    if (s->initialised!=1){
        printf("Didnt initialiaze\n");
        return -2;
    }
    int last = 0;
    sigprocmask(SIG_BLOCK, &new_set, &old_set);
    if (s->value == 0){
        // printf("blocked size %d\n", s->blocked.size);
        if (s->blocked.head != NULL){ 
            thread_node *temp = s->blocked.head;
            printf("going up %d\n", temp->thread->id);
            if (s->blocked.head->next == s->blocked.head){
                // printf("last\n");
                last = 1;
            }
            else{
                s->blocked.head = s->blocked.head->next;
                s->blocked.tail->next = s->blocked.head;

            }

            if (temp->thread->id==0){
                temp->next = t_queue->head;
                t_queue->head = temp;
                t_queue->tail->next = t_queue->head;
            }
            else{
                temp->next = t_queue->head->next;
                t_queue->head->next = temp;
            }
            if (t_queue->head == t_queue->tail){
                t_queue->tail = temp;
            }
            temp->thread->status = MYTHREAD_READY;
            s->blocked.size -= 1;
            if (last == 1){
                s->blocked.head = NULL;
            }
        }
        else {
            s->value ++;
        }

    }else {
        sigprocmask(SIG_SETMASK, &old_set, NULL);
        return -1;
    }
   
    // print_thread_queue();
    sigprocmask(SIG_SETMASK, &old_set, NULL);
    // printf("sema up\n");
    
    return 0;
}

//added tail
int mythreads_sem_destroy(mysem_t *s) {
    if (s->initialised!=1){
        printf("Didnt initialiaze\n");
        return -2;
    }
    if (s->blocked.size != 0){
        printf("eisai vlakas, RTFM!\n");
        return -1;
    }
    s->initialised=0;
    // printf("sema desstroed\n");

    return 0;
}