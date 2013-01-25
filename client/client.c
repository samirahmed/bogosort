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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../lib/net.h"

#define TEAMNAME "bogosort" 

#define BUFLEN 16384
#define STRLEN 80
#define XSTR(s) STR(s)
#define STR(s) #s

struct LineBuffer {
  char data[BUFLEN];
  int  len;
  int  newline;
};

struct Globals {
  struct LineBuffer in;
  char server[STRLEN];
  PortType port;
  FDType serverFD;
  int connected;
  int verbose;
} globals;

#define VPRINTF(fmt, ...)					  \
  {								  \
    if (globals.verbose==1) fprintf(stderr, "%s: " fmt,           \
				    __func__, ##__VA_ARGS__);	  \
  }

int 
getInput()
{
  int len;
  char *ret;

  //STUDY THIS CODE AND EXPLAIN WHAT IT DOES AND WHY IN YOUR LOG FILE

  // to make debugging easier we zero the data of the buffer
  bzero(globals.in.data, sizeof(globals.in.data));
  globals.in.newline = 0;

  // read at most BUFLEN bytes into globals.in from user input (bad use of sizeof for static buffer)
  ret = fgets(globals.in.data, sizeof(globals.in.data), stdin);

  // remove newline if it exists
  len = (ret != NULL) ? strlen(globals.in.data) : 0;

  // Remove the new line ?
  if (len && globals.in.data[len-1] == '\n') {
    globals.in.data[len-1]=0;
    globals.in.newline=1;
  } 
  globals.in.len = len;
  return len;
}

int
prompt(int menu) 
{
  static char MenuString[] = "\n" XSTR(TEAMNAME) "$ ";
  int len;

  if (menu) printf("%s", MenuString);
  fflush(stdout);

  len = getInput();

  return (len) ? 1 : -1;
}

int
doVerbose(void)
{
  globals.verbose = (globals.verbose) ? 0 : 1;
  return 1;
}

int
doConnect(void)
{
  globals.port=0;
  globals.server[0]=0;
  int i, len = strlen(globals.in.data);

  VPRINTF("BEGIN: %s\n", globals.in.data);

  if (globals.connected==1) {
    //Add some code here ... probably a useful error message to stderr
    //  eg. 
	fprintf(stderr, "Connection already established ...");
	
  } else {
    // be sure you understand what the next two lines are doing
    for (i=0; i<len; i++) { 
		if (globals.in.data[i]==':') globals.in.data[i]=' ';
	}
	// Scan the char buffer in.data
	// for some string, a string of length STRLEN and a number
	// Write the 2nd string matched to globals.server
	// Write the number to globals.port
    sscanf(globals.in.data, "%*s %" XSTR(STRLEN) "s %d", globals.server,
	   &globals.port);
    
    if (strlen(globals.server)==0 || globals.port==0) {
      //Add some code here ... probably a useful error message to stderr
      ///eg. 
	  fprintf(stderr, "Server and port not specified in correct format <server-name>:<port> ...");
    } else {
      VPRINTF("connecting to: server=%s port=%d...", 
	      globals.server, globals.port);
      if (net_setup_connection( &globals.serverFD , globals.server, globals.port )<0){ //probably need to pass proper args here)<0) {
	fprintf(stderr, " failed NOT connected server=%s port=%d\n", 
		globals.server, globals.port);
      } else {
	globals.connected=1;
	VPRINTF("connected serverFD=%d\n", globals.serverFD);
      }
    }
  }

  VPRINTF("END: %s %d %d\n", globals.server, globals.port, globals.serverFD);
  return 1;
}

int 
sendStr(char *str, int fd)
{
  int len=0, nlen=0;
  char *buf;
  
  //  STUDY THIS FUNCTION AND EXPAIN IN YOUR LOG FILE WHAT IT DOES AND HOW

  // Pack the length of the string into network byte order  
  len = strlen(str);
  if (len==0) return 1;
  nlen = htonl(len);

  // write this 'len' to the connection ensuring that all the bytes are written successfully
  if (net_writen(fd, &nlen, sizeof(int)) != sizeof(int)) return -1;

  // Write len chars from the str character array ensuring that all 'len' characters are written successfully
  if (net_writen(fd, str, len) != len) return -1;

  // Allocate a new cstring of equal size to the str sent
  buf = (char *)malloc(len);

  // Read from the connection, 'len' bytes into the buffer
  if (net_readn(fd, buf, len) != len) {
	
	// if failure free the memory
    free(buf); 
    return -1; 
  }

  // Write the stdout the string just recieved
  write(STDOUT_FILENO, buf, len);

  // discard the string
  free(buf);

  return 1;
}

int
doSend(void)
{
  int len=0, nlen=0;
  int n;
  char *str, buf[BUFLEN];
 
  VPRINTF("BEGIN %d\n", globals.connected);
 
  if (globals.connected) {
    if (strlen(globals.in.data)<=strlen("send")+1) {
      fprintf(stderr, "ERROR: send <string>\n");
    } else {
	str = &(globals.in.data[strlen("send")+1]);
    send:	
	if (sendStr(str, globals.serverFD)<0) {
	  fprintf(stderr, "send failed %d!=%ld... lost connection\n",
		  n, sizeof(int));
	  globals.connected = 0;
	  close(globals.serverFD);
	} else if (!globals.in.newline) {
	  getInput();
	  str = globals.in.data;
	  goto send;
	}
    } 
  }

  VPRINTF("END %d\n", len);
  return 1;  
}

int
doQuit(void)
{
  return -1;
}

int 
doCmd(void)
{
  int rc = 1;

  if (strlen(globals.in.data)==0) return rc;
  else if (strncmp(globals.in.data, "connect", 
		   sizeof("connect")-1)==0) rc = doConnect();// add a call to the right func
  else if (strncmp(globals.in.data, "send", 
		   sizeof("send")-1)==0) rc = doSend(); //add a call to the right func
  else if (strncmp(globals.in.data, "quit", 
		   sizeof("quit")-1)==0) rc = doQuit();  //add a call to the right func
  else if (strncmp(globals.in.data, "verbose", 
		   sizeof("verbose")-1)==0) rc = doVerbose(); // add a call to the right func
  else printf("Unknown Command\n");

  return rc;
}

int
main(int argc, char **argv)
{
  int rc, menu=1;

  bzero(&globals, sizeof(globals));

  while (1) {
    if (prompt(menu)>=0) 
		rc=doCmd(); 
	else 
		rc=-1;
    if (rc<0) 
		break;
    //What do you think the next line is for
	//idk ?
    // Keeps prompting you unless there is an error? 
	if (rc==1) 
		menu=1; 
	else 
		menu=0;
  }

  VPRINTF("Exiting\n");
  fflush(stdout);

  return 0;
}
