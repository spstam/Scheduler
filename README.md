# User-Level Thread Scheduler

This project implements a user-level thread scheduler in C.

## Features

* **User-Level Threads:**  Creates and manages threads entirely in user space.
* **Scheduling Algorithm:** Implements a scheduling algorithm to manage thread execution (round-robin).
* **Synchronization:** Provides synchronization mechanisms like semaphores to control access to shared resources and prevent race conditions.
* **Sleep Functionality:** Allows threads to yield the processor and sleep for a specified duration.

## Getting Started

### Build and Run

* **Clean:** `make clean`
* **Build:** `make`
* **Run:** `./ex2` 
* **Run with test script:** `./script.sh` (This script cleans, builds, and runs the program with printf statements to track thread execution.)

### Testing

* `ex2.c` provides a test case where multiple threads enter a critical section using semaphores. This tests thread synchronization, the sleep function, and semaphore functionality.
* Modify the number of runs of ex2 in `script.sh` or the number of threads in `ex2.c` to experiment with different scenarios.

## Developers

* Spyridon Stamoulis
* Emmanouil Raftopoulos
* Panagiotis Mpalamotis

## License: MIT License
