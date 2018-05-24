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

#define BUFLEN 100
#define PORTNUM 8090

int sockfd;
char buf[BUFLEN];

void handler(int return_code) {
  shutdown(sockfd, 2);
  close(sockfd);
  exit(return_code);
}

void term_handler(int s) {
  char buf[100] = "quit";
  send(sockfd, buf, strlen(buf), 0);
  shutdown(sockfd, 2);
  close(sockfd);
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
  struct sigaction sigact = {.sa_handler = term_handler};
  sigaction(SIGINT, &sigact, NULL);
  struct sockaddr_in own_addr, party_addr;
  int i;
  ssize_t len;
  if (!(sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
    printf("Can't create socket!");
    exit(EXIT_FAILURE);
  }
  memset(&own_addr, 0, sizeof(own_addr));
  own_addr.sin_family = AF_INET;
  //own_addr.sin_addr.s_addr = INADDR_ANY;
  own_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  own_addr.sin_port = htons(PORTNUM);
  if (connect(sockfd, (struct sockaddr*) &own_addr, sizeof(own_addr)) < 0) {
    printf("Can't connect to server!");
    handler(EXIT_FAILURE);
  }
  while (1) {
    printf("Enter message: ");
    fgets(buf, BUFLEN, stdin);
    buf[strlen(buf)-1] = '\0';
    if (send(sockfd, buf, strlen(buf), 0) < 0) {
      printf("Error sending message!");
      handler(EXIT_FAILURE);
    }

    if (!strcmp(buf, "quit")) {
      handler(EXIT_SUCCESS);
    }
    strcpy(buf, "");
    if ((len = recv(sockfd, buf, BUFLEN, 0)) < 0){
      printf("Error reading socket!");
      handler(EXIT_FAILURE);
    }
    buf[len] = '\0';
    strcpy(buf, "");
  }
}

