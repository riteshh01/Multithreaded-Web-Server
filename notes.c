// In this I'm going to write all the notes about the topics which I'm not aware of (in depth).

/*

⁡⁢⁣⁣Semaphore⁡⁡ => Semaphore ek variable ya counter hota hai jo multiple threads/processes ke access ko control krta hai, jab wo shared resource (like file, memory, printer, etc.) use krte hai.

⁡⁣⁣⁢It is of two tyes => ⁡
1) Binary Semaphore (Mutex type)         value 0 or 1           that means ek time pe ek hi thread access kare
2) Counting Semaphore                    value is N(N>1)        ek time pe mulitple threads allowed hai


⁡⁣⁣⁢Now we are going to know about Semaphore functions in C(POSIX)⁡

sem_init()                          Initialize Semaphore
sem_wait()                          Wait (decrease count, block if 0)
sem_post()                          Signal (increase count, unblock if anything)
sem_destroy()                       Clean up semaphore



⁡⁢⁣⁣Thread⁡ => A thread is the smallest unit of execution within a process that can be managed independently by the scheduler. Each thread within a process has its own program counter, stack, and set of CPU registers, but shares code, data, and operating system resources (like open files) with other threads in the same process. Threads are sometimes called "lightweight processes" because they're more efficient to create and manage than full processes and enable concurrent execution of tasks within the same application.

⁡⁣⁣⁢It is of two types =>⁡
User-level threads: Managed by user libraries; the operating system doesn't know about them directly.
Kernel-level threads: Managed directly by the operating system kernel; each thread is scheduled by the OS and can run on different processors for true parallelism.

pthread_create()                                          Naya thread create karta hai
pthread_join()                                            Thread ke complete hone ka wait karta hai
pthread_exit()                                            Current thread ko terminate karta hai
pthread_self()                                            Current thread ka ID return karta hai
pthread_detach()                                          Thread ko detach karta hai (auto cleanup)
pthread_mutex_init()                                      Mutex (lock) initialize karta hai
pthread_mutex_lock()                                      Lock acquire karta hai
pthread_mutex_unlock()                                    Lock release karta hai
pthread_mutex_destroy()                                   Lock destroy karta hai


*/