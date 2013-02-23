# Distributed Systems Spring 2013

## Team: BOGOSORT

Members: 

- Samir Ahmed, samahmed@bu.edu
- Katsutoshi Kawakami, kkawakam@bu.edu
- William Seltzer, wseltzer@bu.edu

## TicTacToe

The following functions are supported

- Connect : to a host and port i.e `connect localhost:41515` or `connect 148.24.4.11:52524`. If you connect successfully you will be part of a game
- Disconnect : disconnect from game/server, `disconnect`
- Where :  figure out where you are connected, `where`
- Quit : kill client, `quit`
- Move : move by typing in a number from 0-8, i.e `2` to mark the 4th position
- Refresh: just hit enter to print the latest game

## Cleaning and Building

Run `sh make.sh` and it will clean and build lib/client/server

or 

Run 

```
make clean
make
```
