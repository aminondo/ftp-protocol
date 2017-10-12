#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>


#define PORT 41046
#define MAXLINE 256

int main( int argc, char * argv[] ) {
  struct hostent *hp;
  struct sockaddr_in sin;
  char *host;
  char buff[MAXLINE], msg[MAXLINE];
  int len, s;

  //check arguments
  if(argc == 2){
    host = argv[1];
  } else {
    fprintf(stderr, "ERROR: error in arguments provided\n");
    exit(1);
  }

  //translate host name into peer's ip address
  hp = gethostbyname(host);
  if(!hp) {
    fprintf(stderr, "ERROR: unknown host\n");
    exit(1);
  }

  //build address data structure
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
  sin.sin_port = htons(PORT);

  //create socket
  if ( (s = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
    perror("Error creating socket\n");
    exit(1);
  }
  //welcome message
  printf("Welcome to this simple FTP application. Below is a list of all possible operations you can perform.\n");
  printf("DWLD: download a file from server\n");
  printf("UPLD: upload a file to the server\n");
  printf("DELF: delete file from server\n");
  printf("LIST: list the directory on the server\n");
  printf("MDIR: create a directory on the server\n");
  printf("RDIR: remove a directory from the server\n");
  printf("CDIR: change to a different directory on the server\n");
  printf("QUIT: stop application\n");
  printf("------------------------------------------------------------\n\n");
  printf(">> ");

  //connect created socket to remote server
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    perror("Error connnecting socket\n");
    exit(1);
  }

  //main loop
  while(fgets(buff, sizeof(buff), stdin)) {
    buff[MAXLINE-1] = '\0';

    if(!strncmp(buff, "QUIT", 4)){ //quit command
      printf("Goodbye!\n");
      break;
    } else if(!strncmp(buff, "LIST", 4)){ //list command
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      if((len = recv(s, msg, sizeof(msg), 0)) == -1){
        perror("Client receive error\n");
        exit(1);
      }
      printf("%s\n", msg);
    } else if(!strncmp(buff, "DWLD", 4)){ //dwld command
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
    } else if(!strncmp(buff, "CDIR", 4)){ //cdir command
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      printf("new path: ");
      fgets(buff, sizeof(buff), stdin);
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }

    }
    printf("\n------------------------------------------------------------\n");
    printf("DWLD: download a file from server\n");
    printf("UPLD: upload a file to the server\n");
    printf("DELF: delete file from server\n");
    printf("LIST: list the directory on the server\n");
    printf("MDIR: create a directory on the server\n");
    printf("RDIR: remove a directory from the server\n");
    printf("CDIR: change to a different directory on the server\n");
    printf("QUIT: stop application\n");
    printf("------------------------------------------------------------\n\n");
    printf(">> ");

  }

  //close socket
  close(s);
}
