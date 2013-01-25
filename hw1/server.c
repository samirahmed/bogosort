/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "../lib/net.h"

#define MAXLINE 80

struct Globals {
  int verbose;
} globals;

#define VPRINTF(fmt, ...)					  \
  {								  \
    if (globals.verbose==1) fprintf(stderr, "%s: " fmt,           \
				    __func__, ##__VA_ARGS__);	  \
  }

void
str_echo(int sockfd)
{
  int n;
  int  len;
  char *buf;
  
  while (1) {
    MISSING AND IMPORTANT LINE HERE
    if (n != sizeof(int)) {
      fprintf(stderr, "%s: ERROR failed to read len: %d!=%d"
	      " ... closing connection\n", __func__, n, (int)sizeof(int));
      break;
    } 
    WHAT DOES THE NEXT LINE DO?
    len = ntohl(len);
    if (len) {
      buf = (char *)malloc(len);
      n = net_readn(sockfd, buf, len);
      if ( n != len ) {
	fprintf(stderr, "%s: ERROR failed to read msg: %d!=%d"
		" .. closing connection\n" , __func__, n, len);
	break;
      }
      VPRINTF("got: %d '%s'\n", len, buf);
      net_writen(sockfd, buf, len);
      free(buf);
    }
  }
  close(sockfd);
  return;
}

void *
doit(void *arg)
{
  long val = (long)arg;
  pthread_detach(pthread_self());
  str_echo(val);
  close(val);
  return NULL;
}

int
main(int argc, char **argv)
{
  int listenfd, port=0;
  long connfd;
  pthread_t tid;

  bzero(&globals, sizeof(globals));

  if (!HMMM WHAT SHOULD WE DO HERE)) {
    fprintf(stderr, "net_setup_listen_socket FAILED!\n");
    exit(-1);
  }

  printf("listening on port=%d\n", port);

  if (net_listen(listenfd) < 0) {
    fprintf(stderr, "Error: server listen failed (%d)\n", errno);
    exit(-1);
  }

  for (;;) {
    connfd = net_accept(listenfd);
    if (connfd < 0) {
      fprintf(stderr, "Error: server accept failed (%d)\n", errno);
    } else {
      EXPLAIN WHAT IS HAPPENING HERE IN YOUR LOG
      pthread_create(&tid, NULL, &doit, (void *)connfd);
    }
  }

  VPRINTF("Exiting\n");
}
