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
#include <fcntl.h>
 #include <sys/stat.h>

int main(int argc, char **argv){
  microtcp_sock_t sock;
  ssize_t i = 0;
  uint8_t *buffer;
  struct sockaddr *client_addr;
  socklen_t client_addr_len;
// memset(buffer, 0, 1000);

  sock=microtcp_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  microtcp_sock_t *r_sock=&sock;

  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(6886);
  sin.sin_addr.s_addr = inet_addr("192.168.1.6");


  sock =microtcp_connect(sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
  
  FILE *fp;
  int k=0;
  ssize_t c;
  ssize_t j=0;

  struct stat st;
stat("test_1MB.txt", &st); //paraplanitiko einai merika Kb mono :D
ssize_t size = st.st_size;

   fp= fopen("test_1MB.txt", "r");


  buffer=(uint8_t *)malloc(size);

    while (1){
      c = fgetc(fp);
     
      if (feof(fp) || i==8000){ 
        break;
      }
      buffer[i]=c;
      i++;
    }
      microtcp_send(r_sock, buffer, 8000, 0);
      fclose(fp);
      free(buffer);



  microtcp_shutdown(sock, sock.is_server);
  return 1;
}
 
