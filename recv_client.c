/*
 * recv_client.c
 * 15 January 2018
 * Prepared by: James Arias
 * Acts as multi-threaded client.  The client1, hag and client2 addresses need to be configured
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define FILE_TO_SEND "test.MP4"
#define LENGTH 1024

int main(int argc, char *argv[]) {
  printf("--------Client 2--------\n\n");
  // port to start the server on

  int sock_listen;   // listening on this socket for connections from HAG
  int sock_accept;   // accept connections on this connection from HAG
  int wait_size = 5; // max number of pending connections 

  //create server structure and clear it
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));

  // socket address used to store client address
  struct sockaddr_in client_address;
  memset(&client_address, 0, sizeof(client_address));

  int client_address_len = 0;

  //  char  pbuffer[2000];
  int recv_temp = 0;  
 
  // create socket to listen on for connectons from HAG
  if ((sock_listen = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    printf("could not create listen socket\n");
    return 1;
  }

  //Prepare the sockaddr_in structure                                                                                
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port = htons(9008);

  //set sock opt so that it is reuseable   
  int flag_1 = 1;
  setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &flag_1, sizeof(flag_1));

  //bind socket that will listen for connections to server address above
  if ((bind(sock_listen,(struct sockaddr *)&server_address, sizeof(server_address))) < 0) {
    printf("could not bind socket\n");
    return 1;
  }
  //listen on socket that is bound to server address for connections from HAG
  if (listen(sock_listen, wait_size) < 0) {
    printf("could not open socket for listening\n");
  }
  //accept connection from HAG on sock
  if ((sock_accept = accept(sock_listen, (struct sockaddr *)&client_address,&client_address_len)) < 0) 
    {
      printf("could not open a socket to accept data\n");
      return 1;
    }
 
  //set sock opt so that it is reuseable   
  int flag_2 = 1;
  setsockopt(sock_accept, SOL_SOCKET, SO_REUSEADDR, &flag_2, sizeof(flag_2));
 
    FILE * fr = fopen(FILE_TO_SEND,"r");
    char sdbuf[LENGTH];
    size_t fs_block_sz;
    ssize_t n;
    while((fs_block_sz = fread(sdbuf,sizeof(char),LENGTH, fr))>0)
      {
        if ((n = send(sock_accept,sdbuf,fs_block_sz,0)) < 0)
          {
            perror("error on send");
          }
      }
    sleep(5);
    close(fr);
    close(sock_accept);
   
    return;
}
