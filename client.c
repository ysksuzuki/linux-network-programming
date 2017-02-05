#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

/* Connect to a server socket */
int client_socket(const char *hostnm, const char *portnm) {
  char nbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  struct addrinfo hints, *res0;
  int soc, errcode;

  /* Init address info hint */
  (void) memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  /* Determine address info */
  if ((errcode = getaddrinfo(hostnm, portnm, &hints, &res0)) != 0) {
    (void) fprintf(stderr, "getaddrinfo():%s\n", gai_strerror(errcode));
    return (-1);
  }

  if ((errcode = getnameinfo(res0->ai_addr, res0->ai_addrlen, nbuf, sizeof(nbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV)) != 0) {
    (void) fprintf(stderr, "getnameinfo():%s\n", gai_strerror(errcode));
    freeaddrinfo(res0);
    return (-1);
  }
  (void) fprintf(stderr, "addr=%s\n", nbuf);
  (void) fprintf(stderr, "port=%s\n", sbuf);
  /* Create a socket */
  if ((soc = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol)) == -1) {
    perror("socket");
    freeaddrinfo(res0);
    return (-1);
  }
  /* Connect */
  if (connect(soc, res0->ai_addr, res0->ai_addrlen) == -1) {
    perror("connect");
    (void) close(soc);
    freeaddrinfo(res0);
    return (-1);
  }
  freeaddrinfo(res0);
  return (soc);
}

/* Send and receive */
void send_recv_loop(int soc) {
  char buf[512];
  struct timeval timeout;
  int end, width;
  ssize_t len;
  fd_set mask, ready;
  /* select() mask */
  FD_ZERO(&mask);
  /* Set socket descript */
  FD_SET(soc, &mask);
  /* Set stdio */
  FD_SET(0, &mask);
  width = soc + 1;
  /* Send and receive */
  for (end = 0;;) {
    /* Mask */
    ready = mask;
    /* Set timeout */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    switch (select(width, (fd_set *) &ready, NULL, NULL, &timeout)) {
    case -1:
      /* error */
      perror("select");
      break;
    case 0:
      /* Timeout */
      break;
    default:
      /* Ready */
      /* Socket ready */
      if (FD_ISSET(soc, &ready)) {
        /* Receive */
        if ((len = recv(soc, buf, sizeof(buf), 0)) == -1) {
          /* Error */
          perror("recv");
          end = 1;
          break;
        }
        if (len == 0) {
          /* EOF */
          (void) fprintf(stderr, "recv:EOF\n");
          end = 1;
          break;
        }
        /* To string show */
        buf[len] = '\0';
        (void) printf("> %s", buf);
      }
      /* Stdio ready */
      if (FD_ISSET(0, &ready)) {
        /* Read 1 line from stdio */
        (void) fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) {
          end = 1;
          break;
        }
        /* Send */
        if ((len = send(soc, buf, strlen(buf), 0)) == -1) {
          /* Error */
          perror("send");
          end = 1;
          break;
        }
      }
      break;
    }
    if (end) {
      break;
    }
  }
}
    
int main(int argc, char *argv[]) {
  int soc;
  if (argc <= 2) {
    (void) fprintf(stderr, "client server-host port\n");
    return (EX_USAGE);
  }
  /* Connect */
  if ((soc = client_socket(argv[1], argv[2])) == -1) {
    (void) fprintf(stderr, "client_socket():error\n");
    return (EX_UNAVAILABLE);
  } 
  /* Send and receive */
  send_recv_loop(soc);
  /* Close */
  (void) close(soc);
  return (EX_OK);
}



