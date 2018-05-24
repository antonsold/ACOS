//
// Created by anton on 23.05.18.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <bits/signum.h>
#include <signal.h>
#include <arpa/inet.h>

#define BACKLOG 5
#define BUFLEN 100

int sockfd;

void handler(int return_code, int newsockfd) {
  shutdown(newsockfd, 2);
  close(newsockfd);
  exit(return_code);
}

void term_handler(int s) {
  shutdown(sockfd, 2);
  close(sockfd);
  kill(0, SIGTERM);
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
  struct sigaction sigact = {.sa_handler = term_handler};
  sigaction(SIGINT, &sigact, NULL);
  int newsockfd;
  struct sockaddr_in own_addr, party_addr;
  char buf[BUFLEN];
  int i;
  ssize_t len;
  if (!(sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
    printf("Can't create socket!");
    return EXIT_FAILURE;
  }
  memset(&own_addr, 0, sizeof(own_addr));
  own_addr.sin_family = AF_INET;
  //own_addr.sin_addr.s_addr = INADDR_ANY;
  own_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  own_addr.sin_port = htons(atoi((argv[1])));
  if (bind(sockfd, (struct sockaddr *)&own_addr, sizeof(own_addr)) < 0) {
    printf("Can't bind socket!");
    handler(EXIT_FAILURE, sockfd);
  }
  if (listen(sockfd, BACKLOG) < 0) {
    printf("Can't listen socket!");
    handler(EXIT_FAILURE, sockfd);
  }

  while(1) {
    memset(&party_addr, 0, sizeof(party_addr));
    socklen_t party_len = sizeof(party_addr);
    if ((newsockfd = accept(sockfd, (struct sockaddr *) &party_addr, &party_len)) < 0) {
      printf("Error accepting connection!");
      handler(EXIT_FAILURE, sockfd);
    }
    if (!fork()) {
      close(sockfd);
      while (1) {
        strcpy(buf, "");
        if ((len = recv(newsockfd, &buf, BUFLEN, 0)) < 0) {
          printf("Error reading socket!");
          handler(EXIT_FAILURE, newsockfd);
        }
        buf[len] = '\0';
        printf("Received: %s\n", buf);
        if (!strcmp(buf, "quit")) {
          handler(EXIT_SUCCESS, newsockfd);
        }
        /*char new_buf[BUFLEN] ="";
        strncpy(new_buf, buf, len);
        strcat(new_buf, " Hello");*/
        if (send(newsockfd, buf, len, 0) < 0) {
          printf("Error writing socket!");
          handler(EXIT_FAILURE, newsockfd);
        }
      }
    }
    close(newsockfd);
  }
  return 0;
}
