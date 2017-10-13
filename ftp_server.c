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
  struct stat sb;

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
    if((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0){
      perror("ERROR accpeting connection");
      exit(1);
    }

    while(1) {
      memset(msg, 0, sizeof(msg));
      //memset(path, 0, sizeof(path));
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
        //struct stat is_folder;

        //waiting for message from client
        if((len = recv(new_s, buff, sizeof(buff), 0)) == -1){
          perror("Server recieve error");
          exit(1);
        }
        //build new path
        strcpy(path, "./");
        buff[strlen(buff)-1] = '\0';
        strcat(path, buff);
        if(stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)){ //directory exists
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
      //creates directory on server
      if(!strncmp(buff, "MDIR", 4)){
        //struct stat sb;
        printf("recieved MDIR\n");
        //waiting for message from client
        if((len = recv(new_s, buff, sizeof(buff), 0)) == -1){
          perror("Server recieve error");
          exit(1);
        }
        printf("new folder to create: %s\n", buff);
        //check if folder already exists
        strcpy(path, "./");
        buff[strlen(buff)-1] = '\0';
        strcat(path, buff);
        if(stat(buff, &sb) == 0 && S_ISDIR(sb.st_mode)){ //folder exists
          strcpy(msg, "-2");
          if(send(new_s, msg, strlen(msg), 0) == -1){
            perror("Server send error\n");
            exit(1);
          }
        } else { //folder doesn't exist
          if(mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){ //error creating folder
            strcpy(msg, "-1");
            if(send(new_s, msg, strlen(msg), 0) == -1){
              perror("Server send error\n");
              exit(1);
            }
          } else { //folder creation successful
            strcpy(msg, "1");
            if(send(new_s, msg, strlen(msg), 0) == -1){
              perror("Server send error\n");
              exit(1);
            }
          }
        }
      }
      //removes empty directory from server
      if(!strncmp(buff, "RDIR", 4)){
        //waiting for message from client
        if((len = recv(new_s, buff, sizeof(buff), 0)) == -1){
          perror("Server recieve error");
          exit(1);
        }
        printf("client wants to delete %s", buff);
        strcpy(path, "./");
        buff[strlen(buff)-1] = '\0';
        strcat(path, buff);

        if(stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)){ //folder exists
          printf("folder exists!\n");
          strcpy(msg, "1");
          if(send(new_s, msg, strlen(msg), 0) == -1){
            perror("Server send error\n");
            exit(1);
          }
          //waiting for confirmation from client
          if((len = recv(new_s, buff, sizeof(buff), 0)) == -1){
            perror("Server recieve error");
            exit(1);
          }
          if(!strncmp(buff, "Yes", 3)){
            memset(msg, 0, sizeof(msg)); //clear message buffer
            printf("gonna delete the folder.\n");
            if(rmdir(path) == -1){ //delete failed
              strcpy(msg, "-1");
              if(send(new_s, msg, strlen(msg), 0) == -1){
                perror("Server send error\n");
                exit(1);
              }
            } else { //delete successful
              strcpy(msg, "1");
              if(send(new_s, msg, strlen(msg), 0) == -1){
                perror("Server send error\n");
                exit(1);
              }
            }
          } else {
            printf("delete canceled by user\n");
          }

        }
        else {
          printf("folder doesnt exist\n");
          strcpy(msg, "-1");
          if(send(new_s, msg, strlen(msg), 0) == -1){
            perror("Server send error\n");
            exit(1);
          }
        }
      }

      if(!strncmp(buff, "DELF", 4)) {
        char file[MAXLINE];
        short int fNameLen;

        // receive len of filename
        if((len=recv(new_s, &fNameLen, sizeof(short int),0))==-1)  {
            perror("server received error\n");
            exit(1);
        }

        fNameLen = ntohs(fNameLen);

        // receive file name
        bzero(file, sizeof(file));
        if((len=recv(new_s,file,fNameLen,0))==-1) {
            perror("server received error\n");
            close(new_s);
            close(s);
            exit(1);
        }
        file[len] = '\0';

        // check if file exists
        int flag;
        if (access(file, F_OK) != -1) {
            flag = 1;
        } else {
            flag = -1;
        }
        flag = htonl(flag);
        if(send(new_s, &flag, sizeof(int), 0)==-1) {
            perror("server send error\n");
            close(new_s);
            close(s);
            exit(1);
        }

        flag = ntohl(flag);
        if (flag == 1) { // file exists
            if((len=recv(new_s, &flag, sizeof(int),0))==-1) {
                perror("server send error\n");
                close(new_s);
                close(s);
                exit(1);
            }
            flag = ntohl(flag);

            // if the user confirms deletion
            if (flag == 1) {
                int ret_val = remove(file);
                ret_val = htonl(ret_val);
                if(send(new_s, &ret_val, sizeof(int), 0)==-1) {
                    perror("server send error\n");
                    close(new_s);
                    close(s);
                    exit(1);
                }
            } else if (flag == -1) { // 'NO'
                // do nothing, back to waiting!
            } else {
                perror("invalid response\n");
                close(new_s);
                close(s);
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
