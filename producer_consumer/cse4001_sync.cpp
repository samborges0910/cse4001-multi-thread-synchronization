//
// Example from: http://www.amparo.net/ce155/sem-ex.c
//
// Adapted using some code from Downey's book on semaphores
//
// Compilation:
//
//       g++ main.cpp -lpthread -o main -lm
// or 
//      make
//

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <iostream>
using namespace std;

/*
 This wrapper class for semaphore.h functions is from:
 http://stackoverflow.com/questions/2899604/using-sem-t-in-a-qt-project
 */
class Semaphore {
public:
    // Default constructor (for arrays)
    Semaphore() { 
        sem_init(&mSemaphore, 0, 1);
    }

    // Constructor with initial value
    Semaphore(int initialValue) { 
        sem_init(&mSemaphore, 0, initialValue); 
    }

    // Destructor
    ~Semaphore() { 
        sem_destroy(&mSemaphore); 
    }

    void wait() { sem_wait(&mSemaphore); }
    void signal() { sem_post(&mSemaphore); }

private:
    sem_t mSemaphore;
};




/* global vars */
const int bufferSize = 5;
const int numConsumers = 5; 
const int numProducers = 5; 

/* semaphores are declared global so they can be accessed
 in main() and in thread routine. */
Semaphore Mutex(1);
Semaphore Spaces(bufferSize);
Semaphore Items(0);             



/*
    Producer function 
*/
void *Producer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;

    while( 1 )
    {
        sleep(3); // Slow the thread down a bit so we can see what is going on
        Spaces.wait();
        Mutex.wait();
            printf("Producer %d adding item to buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Items.signal();
    }

}

/*
    Consumer function 
*/
void *Consumer ( void *threadID )
{
    // Thread number 
    int x = (long)threadID;
    
    while( 1 )
    {
        Items.wait();
        Mutex.wait();
            printf("Consumer %d removing item from buffer \n", x);
            fflush(stdout);
        Mutex.signal();
        Spaces.signal();
        sleep(5);   // Slow the thread down a bit so we can see what is going on
    }

}



//Problem 1 - No-starve readers-writers

class Lightswitch {
private:
    int count;
    Semaphore mutex;
public:
    Lightswitch() : count(0), mutex(1) {}

    void lock(Semaphore &semaphore) {
        mutex.wait();
        count++;
        if (count == 1) semaphore.wait(); // first thread locks
        mutex.signal();
    }

    void unlock(Semaphore &semaphore) {
        mutex.wait();
        count--;
        if (count == 0) semaphore.signal(); // last thread unlocks
        mutex.signal();
    }
};

Lightswitch readSwitch1;
Semaphore roomEmpty(1);
Semaphore turnstile(1);

void *Reader1(void *threadID) {
    long id = (long)threadID;

    while (1) {
        turnstile.wait();
        turnstile.signal();

        readSwitch1.lock(roomEmpty);

        // critical section
        printf("Reader %ld: Reading\n", id);
        fflush(stdout);
        sleep(1);

        readSwitch1.unlock(roomEmpty);
        sleep(2);
    }
}

// Writer function
void *Writer1(void *threadID) {
    long id = (long)threadID;

    while (1) {
        turnstile.wait();
        roomEmpty.wait();

        // critical section
        printf("Writer %ld: Writing\n", id);
        fflush(stdout);
        sleep(1);

        turnstile.signal();
        roomEmpty.signal();
        sleep(2);
    }
}


void Problem1() {
    const int numReaders = 5;
    const int numWriters = 5;
    pthread_t readers[numReaders];
    pthread_t writers[numWriters];

    for (long i = 0; i < numReaders; i++)
        pthread_create(&readers[i], NULL, Reader1, (void*)(i + 1));

    for (long i = 0; i < numWriters; i++)
        pthread_create(&writers[i], NULL, Writer1, (void*)(i + 1));

    pthread_exit(NULL);
}



// Problem 2 - Writer-priority readers-writers



Lightswitch readSwitch;
Lightswitch writeSwitch;

Semaphore noReaders(1);
Semaphore noWriters(1);

// Reader function
void *Reader2(void *threadID) {
    long id = (long)threadID;
    unsigned int seed = time(NULL) ^ id;

    while (1) {
        noReaders.wait();
        readSwitch.lock(noWriters);
        noReaders.signal();

        printf("Reader %ld: Reading\n", id);
        fflush(stdout);
        sleep(1);

        readSwitch.unlock(noWriters);

        usleep(100000);
    }
}

// Writer function
void *Writer2(void *threadID) {
    long id = (long)threadID;
    unsigned int seed = time(NULL) ^ id;

    while (1) {
        writeSwitch.lock(noReaders);
        noWriters.wait();

        printf("Writer %ld: Writing\n", id);
        fflush(stdout);
        sleep(1);

        noWriters.signal();
        writeSwitch.unlock(noReaders);

        usleep(500000 + rand_r(&seed) % 500000);
    }
}


void Problem2() {
    const int numReaders = 5;
    const int numWriters = 5;
    pthread_t readers[numReaders];
    pthread_t writers[numWriters];

    for (long i = 0; i < numReaders; i++)
        pthread_create(&readers[i], NULL, Reader2, (void*)(i + 1));

    for (long i = 0; i < numWriters; i++)
        pthread_create(&writers[i], NULL, Writer2, (void*)(i + 1));

    pthread_exit(NULL);
}




// Problem 3 - Dining philosophers solution #1



const int problem3_numPhilosophers = 4;
Semaphore footman(problem3_numPhilosophers-1);
Semaphore forkSem[problem3_numPhilosophers];

void *Philosopher(void *threadID) {
    long id = (long)threadID;

    while(1) {
        printf("Philosopher %ld: Thinking\n", id+1);
        fflush(stdout);
        sleep(1 + rand() % 2);

        footman.wait();
        forkSem[id].wait();
        forkSem[(id + 1) % problem3_numPhilosophers].wait();

        printf("Philosopher %ld: Eating\n", id+1);
        fflush(stdout);
        sleep(1 + rand() % 2);

        forkSem[id].signal();
        forkSem[(id + 1) % problem3_numPhilosophers].signal();
        footman.signal();
    }
    return NULL;
}

void Problem3() {
    pthread_t philosophers[problem3_numPhilosophers];

    for (long i = 0; i < problem3_numPhilosophers; i++)
        pthread_create(&philosophers[i], NULL, Philosopher, (void*)i);

    pthread_exit(NULL);
}



// Problem 4 - Dining philosopher’s solution #2



const int problem4_numPhilosophers = 5;
Semaphore footman4(problem4_numPhilosophers - 1);
Semaphore forks4[problem4_numPhilosophers];
bool rightHanded[problem4_numPhilosophers] = {true, true, true, true, false};

void *Philosopher4(void *threadID) {
    long id = (long)threadID;
    while (1) {
        printf("Philosopher %ld: Thinking\n", id+1);
        fflush(stdout);
        sleep(1 + rand() % 2);

        footman4.wait();
        if (rightHanded[id]) {
            forks4[id].wait();
            forks4[(id + 1) % problem4_numPhilosophers].wait();
        } else {
            forks4[(id + 1) % problem4_numPhilosophers].wait();
            forks4[id].wait();
        }

        printf("Philosopher %ld: Eating\n", id+1);
        fflush(stdout);
        sleep(1 + rand() % 2);

        forks4[id].signal();
        forks4[(id + 1) % problem4_numPhilosophers].signal();
        footman4.signal();
    }
    return NULL;
}

void Problem4() {
    pthread_t philosophers[problem4_numPhilosophers];
    for (int i = 0; i < problem4_numPhilosophers; i++)
        forks4[i] = Semaphore(1);

    for (long i = 0; i < problem4_numPhilosophers; i++)
        pthread_create(&philosophers[i], NULL, Philosopher4, (void*)i);

    pthread_exit(NULL);
}



int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <problem_number>\n", argv[0]);
        return 1;
    }

    int problemNum = atoi(argv[1]);
    srand(time(NULL));

    switch (problemNum) {
        case 1:
            Problem1();
            break;
        case 2:
            Problem2();
            break;
        case 3:
            Problem3();
            break;
        case 4:
            Problem4();
            break;
        default:
            printf("Invalid problem number: %d\n", problemNum);
            return 1;
    }

    return 0;
}
