/*
 * client.c
 * 15 January 2018
 * Prepared by: James Arias
 * Acts as multi-threaded client.  The client1, hag and client2 addresses need to be configured
 */

#include<stdio.h> 
#include<string.h>    
#include<sys/socket.h>    
#include<arpa/inet.h> 
#include<pthread.h> 
#include<stdbool.h>
#include<stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <linux/fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>

#define clear() printf("\033[H\033[J");
#define FILENAME "test_receive.MP4"  //file to be sent, should be stored in local directory
#define FILENAME2 "test_receive2.MP4"  //file to be sent, should be stored in local directory
#define LENGTH 1024

typedef struct ThreadArg{
  char *c_server_send_buf;
  int prty;
  int *k_flag;
  int sock_arr_T;
  
}ThreadArg_T;


pthread_mutex_t lock;

void* client_threads_1(struct ThreadArg *t);
void* client_threads_2(struct ThreadArg *t);



int file_write(char *buffer, ssize_t bs)
{
  FILE *received_file_1;
  received_file_1=fopen(FILENAME,"a");
  
  if((fwrite(buffer, sizeof(char), bs, received_file_1)<0))
    {	    
      perror("error during write to flie on client 2");
    }
fclose(received_file_1);

}

int main(int argc , char *argv[])
{

  if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

  system("clear");
  printf("--------Client 1--------\n");

  int n_threads = 2;
  int i;
  int priority = 0;
  pthread_t sniffer_thread;
  ThreadArg_T *arg = malloc(sizeof(ThreadArg_T) * n_threads);
  int k_flag_main = 0;
  
  int sock_array[2]; //in order to pass socket file handles to pthreads
  
  int sock_client1, sock_client2;
  struct sockaddr_in client_1, client_2, server_1,server_2;
 
  char server_send_buffer[LENGTH];   //buffer on main thread that threads store message to send message to client 2
  char* servBufPtr = server_send_buffer; //pointer to buffer on main that threads use to send message to client 2 
  char* dummyPtr;
  
  
  //Set address to which clients will connect
  server_1.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_1.sin_family = AF_INET;
  server_1.sin_port = htons(8889);

  //Set address to which clients will connect
  server_2.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_2.sin_family = AF_INET;
  server_2.sin_port = htons(9000);

  //create sockets that client will use to connect to HAG
  sock_client1 = socket(AF_INET , SOCK_STREAM , 0);
  if (sock_client1 == -1)
    {
      printf("Could not create socket");
    }

  if (setsockopt(sock_client1, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");
  
  sock_client2 = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_client2 == -1)
    {
      printf("Could not create socket");
    }

  if (setsockopt(sock_client2, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");

//specify port address for client threads to connect to.
  client_1.sin_addr.s_addr = INADDR_ANY;
  client_1.sin_family = AF_INET;
  client_1.sin_port = htons(20000);

  client_2.sin_addr.s_addr = INADDR_ANY;;
  client_2.sin_family = AF_INET;
  client_2.sin_port = htons(20001);


//Bind to the sockaddr_in address structure to receive on from client 1 socket 1=======================================
  if( bind(sock_client1,(struct sockaddr *)&client_1 , sizeof(client_1)) < 0)
    {
      //print the error message
      perror("bind failed1. Error");
      return 1;
    }
  
  //Bind to the sockaddr_in address structure to receive on from client 1 socket 2 =======================================
  if( bind(sock_client2,(struct sockaddr *)&client_2 , sizeof(client_2)) < 0)
    {
      //print the error message
      perror("bind failed2. Error");
      return 1;
    }

//Connect to remote server from client with socket 1
  if (connect(sock_client1 , (struct sockaddr *)&server_1 , sizeof(server_1)) < 0)
    {
      perror("connect failed. Error");
      return (void*)1;
    }

//Connect to remote server from client with socket 2
  if (connect(sock_client2 , (struct sockaddr *)&server_2 , sizeof(server_2)) < 0)
    {
      perror("connect failed. Error");
      return (void*)1;
    }
  
//initialize sock_array that is used to pass socket file handles to pthreads. this would have to be updated if there are more connections
  sock_array[0]=sock_client1;
  sock_array[1]=sock_client2;

  for (i=0;i<n_threads;i++)
    {
    arg[i].c_server_send_buf = &servBufPtr;
    arg[i].prty              = i;
    arg[i].k_flag            = &k_flag_main;
    arg[i].sock_arr_T        = sock_array[i];
    
    }
    if(pthread_create(&sniffer_thread,NULL,client_threads_1,(void*)&arg[0]) <0)
      {
  	perror("could not create thread");
  	return 1;
      }


    if(pthread_create(&sniffer_thread,NULL,client_threads_1,(void*)&arg[1]) <0)
      {
  	perror("could not create thread");
  	return 1;
      }
    
  pthread_join( sniffer_thread , NULL); 
  return 0;
}

// Function: client_threads_1 and client_threads_2
//
void* client_threads_1(ThreadArg_T *argc)
  {
    char *sendBuf_T        = argc->c_server_send_buf;
    int thread_priority    = argc->prty;
    int* k_flag_T          = argc->k_flag;
    int sock               = argc->sock_arr_T;
    
    printf("thread priority %d\n", thread_priority);
    
    ssize_t block_sz;
    memset(sendBuf_T,0,LENGTH);
    block_sz=1;
       
    while(1)
      {
	while(block_sz>0)
	  {
	    block_sz = recv(sock,sendBuf_T,LENGTH,0);
	    if(block_sz<0)
	      {      
		perror("error on recv from client 1");
	      }
	    
	    pthread_mutex_lock(&lock);		

	    file_write(sendBuf_T, block_sz);

	    pthread_mutex_unlock(&lock);
	    
	    
	  }
	
	return; 
      }
  }


void* client_threads_2(ThreadArg_T *argc)
  {
    char *sendBuf_T        = argc->c_server_send_buf;
    int thread_priority    = argc->prty;
    int* k_flag_T          = argc->k_flag;
    int sock               = argc->sock_arr_T;
    
    printf("thread priority %d\n", thread_priority);
    
    ssize_t block_sz;
    memset(sendBuf_T,0,LENGTH);
    block_sz=1;
     while(1)
      {
	while(block_sz>0)
	  {
	    block_sz = recv(sock,sendBuf_T,LENGTH,0);
	    if(block_sz<0)
	      {      
		perror("error on recv from client 1");
	      }
	    
	    pthread_mutex_lock(&lock);   		

	    file_write(sendBuf_T, block_sz);
	   
	    pthread_mutex_unlock(&lock);
	    
	    if(block_sz==0){
	    }
	  }
	printf("after recv while loop \n");     
	
	return; 
      }    
  }


