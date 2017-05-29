#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ifaddrs.h> 
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stddef.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "microtcp.c"


int main(int argc, char **argv){
      
  ssize_t len;
  int file_size;
  int remain_data = 0;

  microtcp_header_t *header;

  uint8_t buffer[3000];
  memset(buffer, 0, 3000);
  microtcp_sock_t sock;
  ssize_t accepted=1;
  ssize_t received=0;
  
  struct sockaddr_in sin;

  struct sockaddr client_addr;
  socklen_t client_addr_len;

  
  sock=microtcp_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(6886);
  sin.sin_addr.s_addr = INADDR_ANY;

  microtcp_sock_t *r_sock=&sock;
	
  microtcp_bind(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
 
  client_addr_len = sizeof(struct sockaddr);
  FILE *fp;
  fp= fopen("test2_1MB.txt", "w"); //episis paraplanitiko 
  
  sock=microtcp_accept(sock, &client_addr, sizeof(struct sockaddr));

  while(accepted){
    accepted = microtcp_recv(r_sock, buffer, 500, 0);
    fwrite(buffer , 1 , accepted , fp );
  }
  fclose(fp);

  return 1;
}

