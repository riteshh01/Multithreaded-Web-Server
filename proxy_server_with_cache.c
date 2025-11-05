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
#define MAX_BYTES 4096

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


// thread function ham yaha pe likhenge
void* thread_fn(void* SocketNew){
    sem_wait(&semaphore);
    int p;
    sem_getvalue(&semaphore, p);
    printf("semaphore value is: %d\n", p);
    int *t = (int*) SocketNew;

    int socket = *t;
    int bytes_send_client, len;

    char *buffer = (char*)calloc(MAX_BYTES, sizeof(char));
    bzero(buffer, MAX_BYTES);
    bytes_send_client = recv(socket, buffer, MAX_BYTES, 0);

    while(bytes_send_client > 0){
        len  = strlen(buffer);
        if(substr(buffer, "\r\n\r\n") == NULL){
            bytes_send_client = recv(socket, buffer+len, MAX_BYTES-len, 0);
        }
        else{
            break;
        }
    }

    char *tempReq = (char *)malloc(strlen(buffer)*sizeof(char)+1);

    for(int i = 0; i < strlen(buffer); i++){
        tempReq[i] = buffer[i];
    }

    struct cache_element* temp = find(tempReq);


    
}

// IMP => Whenever we run a C program O.S gives two things to the program (argc) argument count and (argv[]) argument vector

// ⁡⁣⁣⁢1) Initialization and Argument Handling⁡
int main(int argc, char* argv[]){
    int client_socketId, client_len;  // new client ke connection ko represent karega aur client address structure ki size ko store karega
    struct sockaddr_in server_addr, client_addr;  // server_addr me server ka IP aur port store hoga and client_addr me client ka address store hoga,(sockaddr_in is specific for IPv4)
    sem_init(&semaphore, 0 , MAX_CLIENTS); // concurrency control ke liye semaphore ko initialize kiya gaya hai

    pthread_mutex_init(&lock, NULL); // Shared resources (jaise Cache) ko thread-safe access dene ke liye lock ko initialize kiya gaya hai.

    if(argc == 2){ // argv[0]: program ka name, argv[1]: pehla argument that is port number
        port_number = atoi(argv[1]);
    }
    else{
        printf("Too few arguments\n");
        exit(1);
    }

    printf("Starting the Proxy Server at the Port Number: %d\n", port_number);


// ⁡⁣⁣⁡⁣⁣⁢2) Scoket Creation and Configuration⁡
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0); // Socket Creation: Ek naya socket (listening endpoint) create hota hai. Parameters hain: AF_INET (IPv4), SOCK_STREAM (TCP/Connection-Oriented), aur 0 (default protocol).
    // AF_INET => IPv4 protocol use karna hai and SOCK_STREAM => TCP connection banana hai and agar socket create nhi hota to error print hota hai aur program exit kr jata hai

    if(proxy_socketId < 0){
        perror("Failder to create a socket\n");
        exit(1);
    }

// SO_REUSEADDR option set kiya gaya hai. Iska matlab hai ki agar server crash ho jaye ya restart ho, toh yeh turant hi same port number ko dobara use kar sakta hai. Isse "Address already in use" error nahi aata
    int reuse = 1;

    if(setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))){
        // setsockopt() ye ek ek system call (function) hai jo socket ke internal settings ko modify karta hai.
        perror("setSockOpt failed\n");
    }


// ⁡⁣⁣⁢3) Binding (Address assign karna)⁡

    bzero((char*)&server_addr, sizeof(server_addr)); // server_addr structure ki poori memory ko 0 se fill kar diya gaya hai. Yeh best practice hai taaki koi garbage value na rahe
    server_addr.sin_family = AF_INET; // Bata diya ki yeh IPv4 address use karega.
    server_addr.sin_port = htons(port_number); // (Host To Network Short) function port number ko network byte order (Big-Endian) mein convert karta hai. Yeh cross-platform compatibility ke liye zaroori hai.
    server_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY ka matlab hai ki server us machine ke kisi bhi network interface (e.g., WiFi IP, LAN IP, Localhost) par aane wale connection ko accept karega.


    // Listening socket (proxy_socketId) ko upar define kiye gaye IP/Port se jodta hai. Agar port already use ho raha hai ya koi aur error hai, toh fail ho jata hai. (struct sockaddr*) generic typecasting karna zaroori hai.
    if(bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr) < 0)){
        perror("Port in not available\n");
    }
    printf("Binding on port %d\n", port_number);



// ⁡⁣⁣⁢4) Listening aur Accepting connections⁡

    // Server ko passive mode mein daalta hai, matlab ab woh incoming connections ko sunega. MAX_CLIENTS (10) yahan backlog queue size set karta hai, matlab kitne pending connections ko server wait karwa sakta hai.
    int listen_status = listen(proxy_socketId, MAX_CLIENTS);

    if(listen_status < 0){ // Agar listening fail ho jaaye, toh program exit ho jaata hai.
        perror("Error in listening\n");
        exit(1);
    }

    int i = 0;
    int Connected_socketId[MAX_CLIENTS]; // Connected_socketId array naye connected clients ke sockets ko temporarily store karega.

    while(1){
        bzero((char *)&client_addr, sizeof(client_addr)); // simply yaha pe bhi hum memory clean kar rahe hai
        client_len = sizeof(client_addr); // accept() function ko batane ke liye ki address structure ka size kya hai.
        client_socketId = accept(proxy_socketId, (struct sockaddr *)&client_addr, (socklen_t*)&client_len);
        // Yeh function ruk jaata hai (blocking) aur intezaar karta hai jab tak koi naya client connect nahi ho jaata. Jab client connect hota hai, toh yeh naya unique socket ID (client_socketId) return karta hai jiska use communication (send/recv) ke liye kiya jaayega.


            if(client_socketId < 0){
                    printf("Not able to connect\n");
                    exit(1);
            }
            else{
                Connected_socketId[i] = client_socketId; 
            }


            struct sockaddr_in *client_pt = (struct sockaddr_in *)&client_addr;
            struct in_addr ip_addr = client_pt -> sin_addr;
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &ip_addr, str, INET6_ADDRSTRLEN);

            printf("Client is connected with port number %d and ip address is %s\n", ntohs(client_addr.sin_port), str);

            pthread_create(&tid[i], NULL, thread_fn, (void *)&Connected_socketId[i]);
            i++;
    }

    close(proxy_socketId);
    return 0;

}

