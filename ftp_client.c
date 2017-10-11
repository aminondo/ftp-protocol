#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#define PORT 41046
#define MAXLINE 256

int main( int argc, char * argv[] ) {
  struct hostent *hp;
  struct sock addr_in sin;
  char *host;
  char buff[MAXLINE];
  int len, s;

  //check arguments
  if(argc == 2){
    host = argv[1];
  } else {
    fprintf(stderr, "ERROR: error in arguments provided");
    exit(1);
  }

  //translate host name into peer's ip address
  hp = gethostbyname(host);
  if(!hp) {
    fprintf(stderr, "ERROR: unknown host");
    exit(1);
  }

  //build address data structure
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
  sin.sin_port = htons(PORT);

  //create socket
  if ( (s = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
    perror("Error creating socket");
    exit(1);
  }
  printf("Welcome TCP Client\n");

  //connect created socket to remote server
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    perror("Error connnecting socket");
    exit(1);
  }

  //main loop
  while(fgets(buff, sizeof(buff), stdin)) {
    buff(MAXLINE-1) = '\0';

    len = strlen(buff) + 1;
    if(send(s, buff, len, 0) == -1){
      perror("Client send error\n");
      exit(1);
    }
  }

  //close socket
  close(s);
}
