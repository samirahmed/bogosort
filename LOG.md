## 2-12-2013

### Attendees:

Katsutoshi K, Samir A, Will S.

### Questions:

- Who actually calls marshall on the header? since the slen is already set when we send message therefore
the actual body has already been written.
- How does the client/server interface with the Protoclient
- Why does the unmarshall_ll and unmarshal_l code use `htonl` and not `ntohl`
- What is the difference between `htonll` and `htonl` they seem to be used interchangably

### Todo:

Everything else, we are very confused.

### Comments/Notes:

Our understanding of the architecture

![image](http://i.imgur.com/8gWPQYy.jpg)


## 1-25-2013 

### Attendees: 

Katsutoshi Kawakami, Samir Ahmed, Will Seltzer

### Questions:

- Makefile
- What does git stash do?
- What is tagbar toggle?
- What is capture the flag?

### Decisions:

- Finish the completion of the skeleton code

### Todo:

<each item should have an owner>

### Comments/Notes:

We completed the first homework assignment and spent approximately four hours.
Most of the commits are under Katsutoshis account but the work was done collaboratively by all of us.

```c
client/client.c:65:		
	int getInput(){
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

client/client.c:160:	
	int sendStr(char *str, int fd){
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

server/server.c:118:	
    } else {
	// EXPLAIN WHAT IS HAPPENING HERE IN YOUR LOG
	// We create a new pthread (not yet detached)
	// The pthread has start routine set to the doit function
	// The pthread is assigned connfd as an argument for the doit function
      pthread_create(&tid, NULL, &doit, (void *)connfd);
    }
```
