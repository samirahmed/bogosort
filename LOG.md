# Meeting Log

## 5-7-2013

### Attendees:

Samir A, Katsutoshi K. Will S.

### Questions: none;

### Todo:

nothing critical... we have some buggy things but we wont be able to fix everything

### Comments/Notes:

Wrapped up almost everything... 

- testing
- benchmarking
- blocking on client side

## 5-2-2013

### Attendees:
Samir A, Katsutoshi K. Will S.

### Questions:

- how do we manage the ui and cli at the same time

### Todo:

- Will & Katsu need to integrate the ui+cli into one client
- Samir needs to update server with game state info + serialized event updates

### Comments/Notes:

Needed to change some of the ui code, because it gets caught up when the NUMLOCK is enabled on ubuntu.

We need to maintain the cli for testing purposes, it would appear that they we have 3 threads...

- UI
- Event Update
- RPC

## 4-27-2013

### Attendees:
Samir A, Katsutoshi K.

### Questions:

none

### Todo:

- Katsu needs to fix client parsing of EC updates
- Samir needs to fix up the client + server integration testing framework **CHEF**

### Comments/Notes:

We still need to figure out how to integrate the UI + Client, but it should not be as difficult as the server+client

## 3-19-2013
### Attendees:
Samir A, Katsutoshi K.

### Questions:

none

### Todo:

- Server Load and Dump - this involves reading the map file/parsing it and initializing the map struct
- Client Querying and RPC using the maze object and cell structs

### Comments/Notes:

We implemented `maze.h` and `maze.c` files in the lib folder. 
The only dynamic object is the maze itself, which consists of cells.

maze_init
cell_init

We also implemented the marshalling of a cell into a header and unmarshalling.
cell_marshall
cell_unmarshall

There are no locks but we have planned for them in the next phase.


## 3-16-2013
### Attendees:
Samir A, Will S.

### Questions:
- Where should we store the player's location for each client? 

### Todo:
- Work on the server to implement the load functionality from HW 3 section 1.2.1 (Will)
- Work on the client to implement the functionality from HW3 section 1.2.2 (Samir)

### Comments/Notes:
We finished all of the architecture documentation. See the docs subdirectory.



## 2-18-2013
### Attendees:
Katsutoshi K, Samir A, Will S.

### Questions:
- What's the best way to have the client timeout? Sigalarm? 
- Should the client session end on a disconnect? 

### Todo:
- Redo the tic-tac-toe specs so that they conform to the specs given in the assignment doc. 
- Implement locking (Samir)
- Test game logic (Will)
- Figure out client/server logic for starting and quitting (Katsu)
- Write up document with game specs and encoding (Will).

### Comments/Notes:
We're practically finished. We have both the event and RPC channel working we just have a few functionalities 
left to implement for the client such as closing the session on a disconnect, etc. 


## 2-16-2013
### Attendees:
Katsutoshi K, Samir A, Will S.

### Questions:
- How do the event handlers work? 
- How do we encode player moves?

### Todo:

-Attempt to finish the client and server side code in next meeting (Monday).

### Comments/Notes:
We're understanding the architecture a bit better now that we understand that there are sessions for both events and RPC
calls.
#### Game Encoding:
Player 1 and Player 2 are each given an int (PV0 and PV1 respectively).
We number the boxes 1-9 from left to right and have the ith bit in each int correspond to box i being marked by that player. 


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
