#include "proxy_parse.h" // HTTP Request Parsing ka matlab hai ki jab bhi koi client server ko data bhejta hai, toh us raw data ko samajhne wale tukdon mein todna
#include <stdio.h> // it is used for the standard I/O like (printf, scanf, fopen, fprintf, fgets, perror, etc.)
#include <stdlib.h> // in this general functions are there which is used to complete common tasks, malloc(), free(), exit(), atoi(), rand() etc.
#include <string.h>
#include <sys/types.h> // yeh aapke program ko portable banane mein help karta hai, matlab ki aapka code alag-alag operating systems aur architectures par theek se run ho sake.
#include <sys/socket.h> // aap apne C program ko Client ya Server se baat karne ki ability de dete hain, inshort ham log intercommunication krte hai.
#include <netinet/in.h> // type of header file hai jo Internet Protocols (IP) se related definitions provide karta hai.
#include <netdb.h> // kaam hai Internet Hostnames (jaise google.com) ko unke IP addresses mein badalna (resolve karna). Is process ko hum DNS (Domain Name System) lookup kehte hain.
#include <arpa/inet.h> // main purpose hai human-readable IP address (jaise "192.168.1.1") ko network-friendly binary format mein, aur iska ulta (vice versa), convert karna.
#include <unistd.h> // Yeh file POSIX standard (Portable Operating System Interface) se related symbolic constants aur declarations provide karti hai. In simple words, yeh file aapke C program ko Operating System (OS) ki core functionalities ko directly control karne ki shakti deti hai.
// Iska Main Focus: Process Management (jaise naye programs chalana), File I/O (low-level file handling), aur System Configuration.
#include <fcntl.h> // File Descriptors ko control aur manage karne ke liye use hoti hai.
#include <sys/wait.h> // ye Process Management ke liye zaroori hai (parent process ko child process ke khatam hone ka wait karwana).
#include <errno.h> // error reporting ke liye use hoti hai.
#include <time.h>
#include <pthread.h>
#include <semaphore.h>


#define MAX_CLIENTS 10 // it means it can handle at max 10 clients

typedef struct cache_element cache_element;

struct cache_element{
    char* data;
    int len;
    char* url;
    time_t lru_time_track; // it is used to track LRU (Least Recently Used) cache policy mein purane items ko hatane ke liye use hota hai.
    cache_element* next; // Cache ko ek Linked List ke form mein maintain karne ke liye next node ka pointer.
};

cache_element* find(char* url); // it is a function which is made for, to search the URL firstly in the cache
int add_cache_element(char* data, int size, char* url); // It is used to store the new data in the cache
void remove_cache_element(); // It is used to remove the new data in the cache


int port_number = 8080; 
int proxy_socketId;     // Server ka Listening Socket store karne ke liye variable.
pthread_t tid[MAX_CLIENTS];   // 10 threads ko store karne ke liye array. Har thread ek client connection ko handle karega.
sem_t semaphore;                // Yeh semaphore client limit (10) ko enforce karega. Jab koi client connect hoga, value kam hogi, jab disconnect hoga, toh zyada.
pthread_mutex_t lock;           // Yeh Mutex Lock Caching jaise shared resources (shared memory) ko safe tareeke se access karne ke liye use hoga.

cache_element* head;    
int cache_size;

// IMP => Whenever we run a C program O.S gives two things to the program (argc) argument count and (argv[]) argument vector

int main(int argc, char* argv[]){
    int client_socketId, client_len;  // it is a variable for the client socket functionality
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore, 0 , MAX_CLIENTS);

    pthread_mutex_init(&lock, NULL);

    if(argc == 2){ // argv[0]: program ka name, argv[1]: pehla argument that is port number
        port_number = atoi(argv[1]);
    }
    else{
        printf("Too few arguments\n");
        exit(1);
    }

    printf("Starting the Proxy Server at the Port Number: %d\n", port_number);


    // creation of socket
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);

    // AF_INET => IPv4 protocol use karna hai and SOCK_STREAM => TCP connection banana hai and agar socket create nhi hota to error print hota hai aur program exit kr jata hai
    if(proxy_socketId < 0){
        perror("Failder to create a socket\n");
        exit(1);
    }

    // Ye allow karta hai ki agar tu server ko restart kare, to same port number turant dobara use kar sake bina “Address already in use” error ke.
    int reuse = 1;
    if(setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))){
        perror("setSockOpt failed\n");
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Ye pura block server ke address structure ko fill karta hai.
	// bzero() → memory ko clean (0) kar deta hai
	// AF_INET → IPv4
	// htons() → port number ko network byte order me convert karta hai (endianness fix karta hai)
	// INADDR_ANY → iska matlab hai server kisi bhi IP address se connection accept karega


    if(bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr) < 0)){
        perror("Port in not available\n");
    }
    printf("Binding on port %d\n", port_number);

    int listen_status = listen(proxy_socketId, MAX_CLIENTS);
    if(listen_status < 0){
        perror("Error in listening\n");
        exit(1);
    }

    int i = 0;
    int Connected_socketId[MAX_CLIENTS];

    while(1){
        bzero((char *)&client_addr, size_of(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_socketId, (struct sockaddr *)&client_addr, (socklen_t*)&client_len);
    }

    if(client_socketId < 0){
            printf("Not able to connect\n");
            exit(1);
    }
    else{
        Connected_socketId[i] = client_socketId; 
    }

}

