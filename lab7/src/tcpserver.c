#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <PORT> <BUFSIZE>\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);
  int bufsize = atoi(argv[2]);

  if (port <= 0 || bufsize <= 0) {
    printf("Invalid PORT or BUFSIZE value\n");
    exit(1);
  }

  int lfd, cfd;
  int nread;
  char buf[bufsize];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(lfd, (SADDR *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    exit(1);
  }

  printf("TCP Server listening on port %d\n", port);

  while (1) {
    socklen_t clilen = sizeof(cliaddr);

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      exit(1);
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Connection established from %s:%d\n", 
           client_ip, ntohs(cliaddr.sin_port));

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");
      exit(1);
    }
    
    printf("Connection closed\n");
    close(cfd);
  }
}