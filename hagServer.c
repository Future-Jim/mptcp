/*
 * hagServer.c
 * 15 January 2018
 * Prepared by: James Arias
 * Acts as multi-threaded client.  The client1, hag and client2 addresses need to be configured
 */

#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <stdbool.h>
#include <errno.h>


#define FILENAME "Hagtest.MP4"
#define LENGTH 1024

int globalFlag = 0;
int globalLastSendFlag = 0;
int globalPriority = 0;
ssize_t globalSendBlockSize;
ssize_t block_sz;
int globalCount;
typedef struct ThreadArgs{
  char  *c_server_send_buf;
  int   c_client_sock_T; 
  int   c_thread_priority;
  int   *c_k_flag;
  int   c_thread_type;

}ThreadArgs_T;

pthread_mutex_t lock;

void *buffer_Thread(struct ThreadArgs *t)
{

    int* k_flag_T  = t->c_k_flag;
    int socket_cs_recv; //socket file handle used to send data to client 2
    ssize_t len;
    ssize_t fr_block_sz, last_send_flag;
    char *server_send_buffer_T = t->c_server_send_buf;
    struct sockaddr_in cs_recv; //address to connect to client 2
    
  //=======================================================================================
  //================== used to send data to client 2
    socket_cs_recv = socket(AF_INET,SOCK_STREAM,0);
    if (socket_cs_recv  == -1)
      {
	printf("Could not create sever send socket");
      }
    cs_recv.sin_addr.s_addr = inet_addr("127.0.0.1"); //content server address
    cs_recv.sin_family = AF_INET;
    cs_recv.sin_port = htons(9008);

    //  ================ connect to client 2
    if (connect(socket_cs_recv, (struct sockaddr *)&cs_recv,sizeof(cs_recv)) <0) 
      {
	perror("connect failed. Error\n");
	return 1;
      }
    //======================================================================================   

    FILE *received_file;
    received_file = fopen(FILENAME, "w");
    int kk = 1;
    block_sz=1;   
    
    while(1)
      {
	if (globalFlag == 0)
	  {
	    pthread_mutex_lock(&lock);
	    
	    block_sz = recv(socket_cs_recv,server_send_buffer_T,LENGTH,0); 	    		    
	    
	    if((fwrite(server_send_buffer_T, sizeof(char), block_sz, received_file)<0))
	      {
		perror("error during write to flie on client 2\n");
	      }
	    if(block_sz<=0)
	      {
		globalFlag=1;
		globalLastSendFlag=1;			
		close(received_file);
		return 0;
	      }
	    globalFlag=1;
	    pthread_mutex_unlock(&lock);
	    
	  }
	
      }
}

//==============================

void *server_Threads(struct ThreadArgs *t)
{

     char *sendBuf_T = t->c_server_send_buf;
     int* k_flag_T  = t->c_k_flag;
     int client_sock_T = t->c_client_sock_T;      //socket accepted from client 1                                 
     int thread_priority = t->c_thread_priority;  
     ssize_t send_block_size,last_send_flag;
     int kk=0;
    
     //     sleep(3);
     while(1)
       {
     
	 if(thread_priority==globalPriority)

	   if(globalFlag==1)
	   {
	       {
		 pthread_mutex_lock(&lock);
		 
		 send_block_size=send(client_sock_T, sendBuf_T,block_sz,0);
		 memset(sendBuf_T,0,block_sz);
		 
		 if(globalLastSendFlag==1)
		   {
		     printf("closing server threads \n");
		     return 0;	   
		   }
		 
		 globalFlag=0; 	 
		 pthread_mutex_unlock(&lock);
		 
		 
	       }
	     
	   }
       }
     return;
}

//==============================

int main(int argc , char *argv[])
{
   printf("---HAG---\n") ;
   //socket option variables
   int flag_1 = 1; // for reuseaddr option
   int flag_2 = 1; // for reuseaddr  option

   pthread_mutex_init( &lock, NULL);

   
  //HAG server receive variables
   int socket_desc_1,socket_desc_2;
   int client_sock_1,client_sock_2;
   int wait_size = 3;

  //socket structures
   struct sockaddr_in server_1,server_2, client_1,client_2; //server and client addresses
  
  //variables used for threads
  pthread_t sniffer_thread;         

  int i;                            //argc structure loop count
  int n_threads = 2;                //in this implementation of MPTCP, we will only use 2 connections and two threads.
  ThreadArgs_T *arg =malloc(sizeof(ThreadArgs_T) * n_threads); //allocate memory for arg structure passed to pthread

  pthread_mutex_t lock_1;
  pthread_mutex_init(&lock_1,NULL); //initialize mutex variable lock



  //int* new_sock = &socket_desc;    //listening socket on server
  char server_send_buffer[LENGTH];   //buffer on main thread that threads store message to send message to client 2
  char* servBufPtr = server_send_buffer; //pointer to buffer on main that threads use to send message to client 2
  int main_k_flag = 0;             //flag used to close sockets and threads on non-priority threads
  
  //Create socket_1 to receive from client 1 =====================================================================
  socket_desc_1 = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_desc_1 == -1)
    {
      printf("Could not create socket");
    }
  //Create socket_2 to receive from client 1 =====================================================================
  socket_desc_2 = socket(AF_INET , SOCK_STREAM , 0);
  if (socket_desc_2 == -1)
    {
      printf("Could not create socket");
    }
  
  //Prepare the sockaddr_in_1 structure to receive from client 1 ===================================================
  server_1.sin_family = AF_INET;
  server_1.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_1.sin_port = htons(8889);

  //Prepare the sockaddr_in_2 structure to receive from client 1 ===================================================
  server_2.sin_family = AF_INET;
  server_2.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_2.sin_port = htons(9000);

  if (setsockopt(socket_desc_1, SOL_SOCKET, SO_REUSEADDR, &flag_1, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");

  if (setsockopt(socket_desc_2, SOL_SOCKET, SO_REUSEADDR, &flag_1, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");

  //Bind to the sockaddr_in address structure to receive on from client 1 =======================================
  if( bind(socket_desc_1,(struct sockaddr *)&server_1 , sizeof(server_1)) < 0)
    {
      //print the error message
      perror("bind failed. Error");
      return 1;
    }
  
  //Bind to the sockaddr_in address structure to receive on from client 1 =======================================
  if( bind(socket_desc_2,(struct sockaddr *)&server_2 , sizeof(server_2)) < 0)
    {
      //print the error message
      perror("bind failed. Error");
      return 1;
    }
  
  //Listen on binded socket to receive on from client 1 =========================================================
  if(listen(socket_desc_1 , wait_size) < 0){
    printf("could not open socket for listening\n");
    return 1;
  }
  
  //Listen on binded socket to receive on from client 1 =========================================================
  if(listen(socket_desc_2 , wait_size) < 0){
    printf("could not open socket for listening\n");
    return 1;
  }
  
  //Accept an incoming connection from client 1 ================================================================
  int  c_1 = sizeof(struct sockaddr_in);
  int  c_2 = sizeof(struct sockaddr_in);
  
  int client_sock_T_1, client_sock_T_2;
  
  client_sock_T_1 = accept(socket_desc_1, (struct sockaddr *)&client_1, (socklen_t*)&c_1);   
  if (client_sock_T_1 < 0)
    {
      perror("accept failed");
      return 1;
    }
  
  client_sock_T_2 = accept(socket_desc_2, (struct sockaddr *)&client_2, (socklen_t*)&c_2);   
  if (client_sock_T_2 < 0)
    {
      perror("accept failed");
      return 1;
    }
  int client_sock_T[2];
  client_sock_T[0]=client_sock_T_1;
  client_sock_T[1]=client_sock_T_2;
  
  //set sock option so that sockets can be reused immediately
  if (setsockopt(client_sock_T_1, SOL_SOCKET, SO_REUSEADDR, &flag_2, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");
  
  
  if (setsockopt(client_sock_T_2, SOL_SOCKET, SO_REUSEADDR, &flag_2, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");
  
  //Initialize structure that is passed as arguments to server threads
  // n_thread + 1 is the buffer thread
  for(i=0;i<n_threads+1;i++)
    {
      arg[i].c_server_send_buf  = &servBufPtr;
      arg[i].c_client_sock_T    = client_sock_T[i];
      arg[i].c_thread_priority  = i; // 0 is priority thread.
      arg[i].c_k_flag           = &main_k_flag; //flag used to kill threads
      arg[i].c_thread_type      = i; // i = 3 (hag <-> content server) connection thread
    }
  
  //create threads
  /* //this can be fancied up eventually, but for now its sufficient. */
  if(pthread_create( &sniffer_thread , NULL ,  server_Threads , (void*)&arg[0]) <0)
    {
      perror("could not create thread");
      return 1;
    }
  
  if(pthread_create( &sniffer_thread , NULL ,  server_Threads , (void*)&arg[1]) <0)
    {
      perror("could not create thread");
      return 1;
    }
  
  if(pthread_create( &sniffer_thread , NULL ,  buffer_Thread , (void*)&arg[2]) <0) 
    {
      perror("could not create thread");
      return 1;
    }
  
  
  sleep(8.3);
  globalPriority=1;
  sleep(2);
  globalPriority=0;
  sleep(2);
  globalPriority=1;
  sleep(2);
  globalPriority=0;
  sleep(1);
  globalPriority=1;
  sleep(1);
  globalPriority=0;
  sleep(1);
  globalPriority=1;
  sleep(1);
  globalPriority=0;
  sleep(1);
  globalPriority=1;
  sleep(1);
  globalPriority=0;
  
  
  pthread_join(sniffer_thread,NULL);
  close(socket_desc_1);
  close(socket_desc_2);
  printf("completed transmission\n");
  
  return 0;
}
