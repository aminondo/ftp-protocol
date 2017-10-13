#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <mhash.h>
#include <openssl/sha.h>
#include <openssl/md5.h>


#define PORT 41046
#define MAXLINE 256

int throughput( struct timeval *start, struct timeval *fin ) { // quick calc throughput
    int usec = fin->tv_usec - start->tv_usec;
    bzero( start, sizeof( struct timeval ) );
    bzero( fin, sizeof( struct timeval ) );
    return usec;
}

int main( int argc, char * argv[] ) {
  struct hostent *hp;
  struct sockaddr_in sin;
  char *host;
  char buff[MAXLINE], msg[MAXLINE];
  unsigned char digest[MD5_DIGEST_LENGTH];
  int len, s, size;
  FILE *fp;

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
    memset(msg, 0, sizeof(msg));

    if(!strncmp(buff, "QUIT", 4)){ //quit command
      break;
    }
    else if(!strncmp(buff, "LIST", 4)){ //list command
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
    }
    else if(!strncmp(buff, "CDIR", 4)){ //cdir command
      len = strlen(buff) + 1;
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
      //receive status back from server
      if((len = recv(s, msg, sizeof(msg), 0)) == -1){
        perror("Client receive error\n");
        exit(1);
      }
      if(!strncmp(msg, "-2",2))
        printf("The directory does not exist on server.\n");
      else if(!strncmp(msg, "-1",2))
        printf("Error changing directory.\n");
      else if(!strncmp(msg, "1",1))
        printf("Changed current directory.\n");
    }
    else if(!strncmp(buff, "MDIR", 4)){ //create directory in server
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      printf("new folder name: ");
      fgets(buff, sizeof(buff), stdin);
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      //receive status back from server
      if((len = recv(s, msg, sizeof(msg), 0)) == -1){
        perror("Client receive error\n");
        exit(1);
      }
      if(!strncmp(msg, "-2",2))
        printf("The directory already exists on server.\n");
      else if(!strncmp(msg, "-1",2))
        printf("Error in making directory.\n");
      else if(!strncmp(msg, "1",1))
        printf("The directory was successfully made.\n");
    }
    else if(!strncmp(buff, "RDIR", 4)){ //delete empty directory from server
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      printf("folder to delete: ");
      fgets(buff, sizeof(buff), stdin);
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      //receive status back from server
      if((len = recv(s, msg, sizeof(msg), 0)) == -1){
        perror("Client receive error\n");
        exit(1);
      }
      if(!strncmp(msg, "-1",2))
        printf("The directory does not exist on server.\n");
      else if(!strncmp(msg, "1",1)){
        printf("Are you sure you want to delete this folder? (Yes/No) ");
        fgets(buff, sizeof(buff), stdin);
        if(!strncmp(buff, "Yes",3)){ //delete confirmed
          len = strlen(buff) + 1;
          if(send(s, buff, len, 0) == -1){
            perror("Client send error\n");
            exit(1);
          }
          //waiting for status from server
          //clear msg
          memset(msg, 0, sizeof(msg));
          if((len = recv(s, msg, sizeof(msg), 0)) == -1){
            perror("Client receive error\n");
            exit(1);
          }
          if(!strncmp(msg, "1",1))
            printf("Directory deleted.\n");
          else
            printf("Failed to delete directory.\n");

        } else if(!strncmp(buff, "No",2)) {
          printf("Delete abandonded by user!\n");
          len = strlen(buff) + 1;
          if(send(s, buff, len, 0) == -1){
            perror("Client send error\n");
            exit(1);
          }
        } else {
          printf("Error in input. Request canceled.\n");
          strcpy(buff, "No");
          len = strlen(buff) + 1;
          if(send(s, buff, len, 0) == -1){
            perror("Client send error\n");
            exit(1);
          }

        }

      }
    }
    else if(!strncmp(buff, "DWLD", 4)){ // get file
      int flag = 1;

      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      printf("file to download: ");
      fgets(buff, sizeof(buff), stdin);
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      //receive status back from server
      if ( recv( s, &flag, sizeof(flag), 0 ) == -1 ) {
        perror("receive error");
        exit(1);
      }
      if ( flag == -1 ) printf( "The file does not exist on server.\n");

      if ( recv( s, tmp_md5, MD5_DIGEST_LENGTH, 0 ) == -1 ) { // hash
        perror("receive error");
        exit(1);
      }

      if ( ( fp = fopen( buff, "w" ) ) == NULL ){ // open file
          perror("I/O error");
          exit(1);
      }

      gettimeofday(&start, NULL);

      // receive file from server
      do {
          bzero( buff, sizeof(buff) );
          if ( size - tmp_size < sizeof(buff) )
              len = recv( s, buff, ( size - tmp_size ), 0 );
          else len = recv( s, buff, sizeof(buff), 0 );
          if ( len == -1 ) {
              perror("receive error");
              exit(1);
          }
          fwrite( buff, sizeof(char), len, fp );
      } while ( ( tmp_size += len ) < size );

      gettimeofday(&fin, NULL);

      // close file
      fclose( fp );

      // open file in disk
      if ( ( fp = fopen( name, "r" ) ) == NULL ){
          printf("file I/O error\n");
          exit(1);
      }

      MD5_CTX mdContext;

      MD5_Init (&mdContext);

      do {
          bzero( buff, sizeof(buff) );
          len = fread ( buff, sizeof(char), sizeof(buff), fp );
          MD5_Update( &mdContext, buff, len );
      } while ( len != 0 );

      MD5_Final ( digest, &mdContext );
      len = MD5_DIGEST_LENGTH;
      fclose( fp );

      // compare MD5 hashes
      int i;
      int res = 1;
      for ( i = 0; i < MD5_DIGEST_LENGTH; i++ )
          if ( tmp_md5[i] != digest[i] ) {
            res = 0;
          }

      // report result
      if ( flag ) {
          printf("file transfer successful\n");
          printf( "%d bytes transferred in %.6lf seconds: ", size, throughput( &start, &fin ) / 1000000.0 );
          printf( "%.3lf Megabytes/sec\n", ( double ) ( size / throughput( &start, &fin ) ) );
          int i;

          printf( "File MD5sum: " );

          for ( i = 0; i < MD5_DIGEST_LENGTH; i++ )
              printf( "%02hhx", digest[i] );

          printf( "\n" );

      } else printf("file transfer error\n");
    }

    else if(!strncmp(buff, "UPLD", 4)) { // upload file
        int flag = 1;
        len = strlen(buff) + 1;
        if(send(s, buff, len, 0) == -1){
          perror("Client send error\n");
          exit(1);
        }

        printf("file to upload: ");
        fgets(buff, sizeof(buff), stdin);
        len = strlen(buff) + 1;
        if(send(s, buff, len, 0) == -1){
          perror("Client send error\n");
          exit(1);
        }

        if ( recv( s, &flag, sizeof(flag), 0 ) == -1 ) { // ack
          perror("receive error");
          exit(1);
        }
        if ( flag == 0 ) {
            printf( "Cannot write file on server\n" );
            continue;
        }

        // open file to read
        if ( ( fp = fopen( buff, "r" ) ) == NULL ) {
            size = -1;
            if ( send( s, &size, sizeof(size), 0 ) == -1 ) {
                perror("send error");
                exit(1);
            }
            printf( "Cannot find file on drive.\n" );
            continue;
        }

        fseek( fp, 0L, SEEK_END ); // get fsize
        size = ftell(fp);
        fseek( fp, 0, SEEK_SET );

        // send file size to server
        if ( send( s, &size, sizeof(size), 0 ) == -1 ) {
            perror("send error");
            exit(1);
        }
        // send file to server
        do {
            bzero( buff, sizeof(buff) );
            len = fread( buff, sizeof(char), sizeof(buff), fp );
            if(send(s, buff, len, 0) == -1) {
              perror("Client send error\n");
              exit(1);
            }
        } while ( !feof( fp ) );

        fseek( fp, 0, SEEK_SET ); // fp reset
        MD5_CTX mdContext; //setup
        MD5_Init (&mdContext);
        do {
            bzero(buff, sizeof(buff) );
            len = fread (buff, sizeof(char), sizeof(buff), fp );
            MD5_Update( &mdContext, buff, len );
        } while ( len != 0 );

        MD5_Final ( digest, &mdContext );

        len = MD5_DIGEST_LENGTH;
        fclose( fp );

      if ( send( s, digest, len, 0 ) == -1 ) {     // send hash
          perror("send error");
          exit(1);
      }

      if ( recv( s, &flag, sizeof(flag), 0 ) == -1 ) { // result
          perror("receive error");
          exit(1);
      }

      // report result
      if ( flag == 0 ) printf("file transfer error\n");
      else {
        printf("file transfer successful\n");
        printf( "%d Bytes sent in %.6lf sec(s): ", size, flag / 1000000.0 );
        printf( "%.3lf MBps\n", ( double ) ( size / flag ) );
      }
      printf( "File MD5sum: " );

      int i;
      for ( i = 0; i < MD5_DIGEST_LENGTH; i++ ) {
        printf( "%02hhx", digest[i] );
      }

      printf("\n"); // newline

    } else if (!strncmp(buff, "DELF", 4)) { // del file
      int flag = 1;

      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      printf("file to delete: ");
      fgets(buff, sizeof(buff), stdin);
      len = strlen(buff) + 1;
      if(send(s, buff, len, 0) == -1){
        perror("Client send error\n");
        exit(1);
      }
      //receive status back from server
      if ( recv( s, &flag, sizeof(flag), 0 ) == -1 ) {
        perror("receive error");
        exit(1);
      }
      if ( flag == -1 ) printf( "The file does not exist on server.\n");
      else {
          // get confirmation from user
          printf( "Are you sure you want to remove this file? (Yes/No)\n");
          scanf( "%s", buff );
          flag = strncmp( buff, "Yes", 3 );

          if ( send( s, &flag, sizeof(flag), 0 ) == -1 ) {   // conf
              perror("send error");
              exit(1);
          }

          if ( flag == 0 ) {
              // wait for server success/error response
              if((len = recv(s, msg, sizeof(msg), 0)) == -1){
                perror("Client receive error\n");
                exit(1);
              }
              if(!strncmp(msg, "1",1)) printf("delete successful");
              else printf("delete failure");
          } else printf("Delete abandoned by the user!\n");
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
  //exit
  printf("Goodbye. session has been closed.\n");
  return 0;
}
