#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define PORT 41046
#define MAX_PENDING 5
#define MAXLINE 256


int main() {
  struct sockaddr_in sin;
  char buff[MAXLINE], msg[MAXLINE], path[MAXLINE];
  int len, s, new_s, opt;
  DIR * d;

  //build address data structure
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  //sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(PORT);

  //setup passive open
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    perror("ERROR creating socket");
    exit(1);
  }

  //setup socket option
  if((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)& opt, sizeof(int))) < 0){
    perror("ERROR creating set socket option");
    exit(1);
  }

  //bind created socket to specified address
  if((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("ERROR binding socket");
    exit(1);
  }

  //listening
  if((listen(s, MAX_PENDING)) < 0){
    perror("ERROR listenign");
    exit(1);
  }


  //wait for connection
  while(1){
    memset(msg, 0, sizeof(msg));

    if((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0){
      perror("ERROR accpeting connection");
      exit(1);
    }

    while(1) {
      if((len = recv(new_s, buff, sizeof(buff), 0)) == -1){
        perror("Server recieve error");
        exit(1);
      }
      if(len==0)
        break;
      printf("TCP recieved: %s\n", buff);

      // Program gets directory listing and sends back to client
      if(!strncmp(buff, "LIST", 4)){
        struct dirent * dir;
        struct stat fileStat;
        d = opendir(".");
        if(d == NULL){
          perror("ERROR: error reading directory");
        }
        //loops through all directory items
        while((dir = readdir(d)) != NULL){
          //gets all file permissions
          if(!stat(dir->d_name, &fileStat)){
            strcat(msg,(S_ISDIR(fileStat.st_mode))  ? "d" : "-");
            strcat(msg,(fileStat.st_mode & S_IRUSR) ? "r" : "-");
            strcat(msg,(fileStat.st_mode & S_IWUSR) ? "w" : "-");
            strcat(msg,(fileStat.st_mode & S_IXUSR) ? "x" : "-");
            strcat(msg,(fileStat.st_mode & S_IRGRP) ? "r" : "-");
            strcat(msg,(fileStat.st_mode & S_IWGRP) ? "w" : "-");
            strcat(msg,(fileStat.st_mode & S_IXGRP) ? "x" : "-");
            strcat(msg,(fileStat.st_mode & S_IROTH) ? "r" : "-");
            strcat(msg,(fileStat.st_mode & S_IWOTH) ? "w" : "-");
            strcat(msg,(fileStat.st_mode & S_IXOTH) ? "x" : "-");
          } else {
            perror("Error in stat");
          }
          //attach file name
          strcat(msg, " ");
          strcat(msg, dir->d_name);
          strcat(msg, "\n");
        }
        //sends back to client
        if(send(new_s, msg, strlen(msg), 0) == -1){
          perror("Server send error\n");
          exit(1);
        }
        closedir(d);
      }
      //changes active directory
      if(!strncmp(buff, "CDIR", 4)){
        struct stat is_folder;

        //waiting for message from client
        if((len = recv(new_s, buff, sizeof(buff), 0)) == -1){
          perror("Server recieve error");
          exit(1);
        }
        //build new path
        memset(path, 0, sizeof(path));
        strcat(path, "./");
        buff[strlen(buff)-1] = '\0';
        strcat(path, buff);
        printf("changing directory to: [%s]\n", path);
        if(stat(path, &is_folder) == 0 && S_ISDIR(is_folder.st_mode)){ //directory exists
          //attempt changing directory
          if(chdir(path) != 0){ //directory change unsucessful
            strcpy(msg, "-1");
            if(send(new_s, msg, strlen(msg), 0) == -1){
              perror("Server send error\n");
              exit(1);
            }
          } else { //directory change successful
            strcpy(msg, "1");
            if(send(new_s, msg, strlen(msg), 0) == -1){
              perror("Server send error\n");
              exit(1);
            }
          }
        } else { //directory does not exist
          strcpy(msg, "-2");
          if(send(new_s, msg, strlen(msg), 0) == -1){
            perror("Server send error\n");
            exit(1);
          }
        }
      }
    }

    //close connection
    printf("Client finishes, close connection!\n");
    close(new_s);
  }


}
