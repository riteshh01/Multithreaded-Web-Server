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


#define MAX_BYTES 4096    //max allowed size of request/response
#define MAX_CLIENTS 400     //max number of client requests served at a time
#define MAX_SIZE 200*(1<<20)     //size of the cache
#define MAX_ELEMENT_SIZE 10*(1<<20)     //max size of an element in cache


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


int sendErrorMessage(int socket, int status_code)
{
	char str[1024];
	char currentTime[50];
	time_t now = time(0);

	struct tm data = *gmtime(&now);
	strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

	switch(status_code)
	{
		case 400: snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
				  printf("400 Bad Request\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 403: snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
				  printf("403 Forbidden\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 404: snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
				  printf("404 Not Found\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 500: snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
				  //printf("500 Internal Server Error\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 501: snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
				  printf("501 Not Implemented\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 505: snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
				  printf("505 HTTP Version Not Supported\n");
				  send(socket, str, strlen(str), 0);
				  break;

		default:  return -1;

	}
	return 1;
}

int connectRemoteServer(char* host_addr, int port_num)
{
	// Creating Socket for remote server

	int remoteSocket = socket(AF_INET, SOCK_STREAM, 0);

	if( remoteSocket < 0)
	{
		printf("Error in Creating Socket.\n");
		return -1;
	}
	
	// Get host by the name or ip address provided

	struct hostent *host = gethostbyname(host_addr);	
	if(host == NULL)
	{
		fprintf(stderr, "No such host exists.\n");	
		return -1;
	}

	// inserts ip address and port number of host in struct `server_addr`
	struct sockaddr_in server_addr;

	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);

	bcopy((char *)host->h_addr,(char *)&server_addr.sin_addr.s_addr,host->h_length);

	// Connect to Remote server

	if( connect(remoteSocket, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr)) < 0 )
	{
		fprintf(stderr, "Error in connecting !\n"); 
		return -1;
	}
	// free(host_addr);
	return remoteSocket;
}


int handle_request(int clientSocket, ParsedRequest *request, char *tempReq)
{
	char *buf = (char*)malloc(sizeof(char)*MAX_BYTES);
	strcpy(buf, "GET ");
	strcat(buf, request->path);
	strcat(buf, " ");
	strcat(buf, request->version);
	strcat(buf, "\r\n");

	size_t len = strlen(buf);

	if (ParsedHeader_set(request, "Connection", "close") < 0){
		printf("set header key not work\n");
	}

	if(ParsedHeader_get(request, "Host") == NULL)
	{
		if(ParsedHeader_set(request, "Host", request->host) < 0){
			printf("Set \"Host\" header key not working\n");
		}
	}

	if (ParsedRequest_unparse_headers(request, buf + len, (size_t)MAX_BYTES - len) < 0) {
		printf("unparse failed\n");
	}

	int server_port = 80;
	if(request->port != NULL)
		server_port = atoi(request->port);

	int remoteSocketID = connectRemoteServer(request->host, server_port);

	if(remoteSocketID < 0)
		return -1;

	int bytes_send = send(remoteSocketID, buf, strlen(buf), 0);

	bzero(buf, MAX_BYTES);

	bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);
	char *temp_buffer = (char*)malloc(sizeof(char)*MAX_BYTES); //temp buffer
	int temp_buffer_size = MAX_BYTES;
	int temp_buffer_index = 0;

	while(bytes_send > 0)
	{
		bytes_send = send(clientSocket, buf, bytes_send, 0);
		
		for(int i=0;i<bytes_send/sizeof(char);i++){
			temp_buffer[temp_buffer_index] = buf[i];
			// printf("%c",buf[i]); // Response Printing
			temp_buffer_index++;
		}
		temp_buffer_size += MAX_BYTES;
		temp_buffer=(char*)realloc(temp_buffer,temp_buffer_size);

		if(bytes_send < 0)
		{
			perror("Error in sending data to client socket.\n");
			break;
		}
		bzero(buf, MAX_BYTES);

		bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);

	} 
	temp_buffer[temp_buffer_index]='\0';
	free(buf);
	add_cache_element(temp_buffer, strlen(temp_buffer), tempReq);
	printf("Done\n");
	free(temp_buffer);
	
	
 	close(remoteSocketID);
	return 0;
}


int checkHTTPversion(char *msg)
{
	int version = -1;

	if(strncmp(msg, "HTTP/1.1", 8) == 0){
		version = 1;
	}
	else if(strncmp(msg, "HTTP/1.0", 8) == 0){
		version = 1;										// Handling this similar to version 1.1
	}
	else
		version = -1;

        
	return version;
}


// thread function ham yaha pe likhenge
void* thread_fn(void* SocketNew){

    // semaphore hadling (client limiting)
    sem_wait(&semaphore);
    int p;
    sem_getvalue(&semaphore, &p);
    printf("semaphore value is: %d\n", p);

    // socket setup
    int *t = (int*) SocketNew;
    int socket = *t; // thread ke paas apna client ka socket descriptor aata hai.
    int bytes_send_client, len;

    // data recieve karna
    char *buffer = (char*)calloc(MAX_BYTES, sizeof(char));
    bzero(buffer, MAX_BYTES);
    bytes_send_client = recv(socket, buffer, MAX_BYTES, 0); // // recv() function se client ka raw HTTP request receive hota hai


    // Complete request receive hone tak loop
    while(bytes_send_client > 0){
        len  = strlen(buffer);
        if(strstr(buffer, "\r\n\r\n") == NULL){
            bytes_send_client = recv(socket, buffer+len, MAX_BYTES-len, 0);  
        }
        else{
            break;
        }
    }

    // Copy request to another buffer
    char *tempReq = (char *)malloc(strlen(buffer)*sizeof(char)+1);
    for(int i = 0; i < strlen(buffer); i++){
        tempReq[i] = buffer[i];
    }
    
    // cache check
    struct cache_element* temp = find(tempReq); // cache list me URL ko search karega

    
    // Cache hit (data mil gaya)
    if(temp != NULL){
          int size = temp->len/sizeof(char); 
          int pos = 0;
          char response[MAX_BYTES];

          while(pos < size){
            bzero(response, MAX_BYTES);
            for(int i = 0; i < MAX_BYTES; i++){
                 response[i] = temp->data[i];
                 pos++;
            }
            send(socket, response, MAX_BYTES, 0);
          }
        printf("Data retrieved from the cache\n");
        printf("%s\n\n", response);
    }
    else if(bytes_send_client > 0){

		len = strlen(buffer); 
		//Parsing the request
		ParsedRequest* request = ParsedRequest_create();
		
        //ParsedRequest_parse returns 0 on success and -1 on failure.On success it stores parsed request in
        // the request
		if (ParsedRequest_parse(request, buffer, len) < 0) 
		{
		   	printf("Parsing failed\n");
		}
		else
		{	
			bzero(buffer, MAX_BYTES);
			if(!strcmp(request->method,"GET"))							
			{
                
				if( request->host && request->path && (checkHTTPversion(request->version) == 1) )
				{
					bytes_send_client = handle_request(socket, request, tempReq);		// Handle GET request
					if(bytes_send_client == -1)
					{	
						sendErrorMessage(socket, 500);
					}

				}
				else
					sendErrorMessage(socket, 500);			// 500 Internal Error

			}
            else
            {
                printf("This code doesn't support any method other than GET\n");
            }
    
		}
        //freeing up the request pointer
		ParsedRequest_destroy(request);
        }
    
        else if( bytes_send_client < 0){
            perror("Error in receiving from client.\n");
        }
        else if(bytes_send_client == 0){
            printf("Client disconnected!\n");
        }

        shutdown(socket, SHUT_RDWR);
        close(socket);
        free(buffer);
        sem_post(&semaphore);	
        
        sem_getvalue(&semaphore,&p);
        printf("Semaphore post value:%d\n",p);
        free(tempReq);
        return NULL;

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
    if (bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("Port is not available\n");
    exit(1);
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



cache_element* find(char* url){

// Checks for url in the cache if found returns pointer to the respective cache element or else returns NULL
    cache_element* site=NULL;
	//sem_wait(&cache_lock);
    int temp_lock_val = pthread_mutex_lock(&lock);
	printf("Remove Cache Lock Acquired %d\n",temp_lock_val); 
    if(head!=NULL){
        site = head;
        while (site!=NULL)
        {
            if(!strcmp(site->url,url)){
				printf("LRU Time Track Before : %ld", site->lru_time_track);
                printf("\nurl found\n");
				// Updating the time_track
				site->lru_time_track = time(NULL);
				printf("LRU Time Track After : %ld", site->lru_time_track);
				break;
            }
            site=site->next;
        }       
    }
	else {
    printf("\nurl not found\n");
	}
	//sem_post(&cache_lock);
    temp_lock_val = pthread_mutex_unlock(&lock);
	printf("Remove Cache Lock Unlocked %d\n",temp_lock_val); 
    return site;
}

void remove_cache_element(){
    // If cache is not empty searches for the node which has the least lru_time_track and deletes it
    cache_element * p ;  	// Cache_element Pointer (Prev. Pointer)
	cache_element * q ;		// Cache_element Pointer (Next Pointer)
	cache_element * temp;	// Cache element to remove
    //sem_wait(&cache_lock);
    int temp_lock_val = pthread_mutex_lock(&lock);
	printf("Remove Cache Lock Acquired %d\n",temp_lock_val); 
	if( head != NULL) { // Cache != empty
		for (q = head, p = head, temp =head ; q -> next != NULL; 
			q = q -> next) { // Iterate through entire cache and search for oldest time track
			if(( (q -> next) -> lru_time_track) < (temp -> lru_time_track)) {
				temp = q -> next;
				p = q;
			}
		}
		if(temp == head) { 
			head = head -> next; /*Handle the base case*/
		} else {
			p->next = temp->next;	
		}
		cache_size = cache_size - (temp -> len) - sizeof(cache_element) - 
		strlen(temp -> url) - 1;     //updating the cache size
		free(temp->data);     		
		free(temp->url); // Free the removed element 
		free(temp);
	} 
	//sem_post(&cache_lock);
    temp_lock_val = pthread_mutex_unlock(&lock);
	printf("Remove Cache Lock Unlocked %d\n",temp_lock_val); 
}

int add_cache_element(char* data,int size,char* url){
    // Adds element to the cache
	// sem_wait(&cache_lock);
    int temp_lock_val = pthread_mutex_lock(&lock);
	printf("Add Cache Lock Acquired %d\n", temp_lock_val);
    int element_size=size+1+strlen(url)+sizeof(cache_element); // Size of the new element which will be added to the cache
    if(element_size > MAX_ELEMENT_SIZE){
		//sem_post(&cache_lock);
        // If element size is greater than MAX_ELEMENT_SIZE we don't add the element to the cache
        temp_lock_val = pthread_mutex_unlock(&lock);
		printf("Add Cache Lock Unlocked %d\n", temp_lock_val);
		// free(data);
		// printf("--\n");
		// free(url);
        return 0;
    }
    else
    {   while(cache_size+element_size > MAX_SIZE){
            // We keep removing elements from cache until we get enough space to add the element
            remove_cache_element();
        }
        cache_element* element = (cache_element*) malloc(sizeof(cache_element)); // Allocating memory for the new cache element
        element->data= (char*)malloc(size+1); // Allocating memory for the response to be stored in the cache element
		strcpy(element->data,data); 
        element -> url = (char*)malloc(1+( strlen( url )*sizeof(char)  )); // Allocating memory for the request to be stored in the cache element (as a key)
		strcpy( element -> url, url );
		element->lru_time_track=time(NULL);    // Updating the time_track
        element->next=head; 
        element->len=size;
        head=element;
        cache_size+=element_size;
        temp_lock_val = pthread_mutex_unlock(&lock);
		printf("Add Cache Lock Unlocked %d\n", temp_lock_val);
		//sem_post(&cache_lock);
		// free(data);
		// printf("--\n");
		// free(url);
        return 1;
    }
    return 0;
}