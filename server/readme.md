# Bogosort Server 

`server.c` is the only source file in this folder.

The core of the server implementation is `lib/game_server.c` and `lib/protocol_server.c`

![picture of server](http://i.imgur.com/a7LbL3l.png)

## Building and Running

tested on both mac and linux.

```
$ cd bogosort
$ make clean
$ make
$ server/server
```

it is important that you run server from the bogosort root directory because it searchs for the map in the current directory.

## Commands

| command | description |
|---------|--------------------------|
| `d` | toggle debug spew (default: off) |
| `q` | terminate server |
| `quit` | terminate server | 
| `asciidump console` | dump ascii representation on map to the terminal |
| `textdump console` | dump relevant object/player info to terminal | 
| `asciidump <filename>`| dump ascii representation to file |
| `textdump <filename>`| dump useful text representation to file |
| `teleport` | toggle teleportation on/off (default: off) |
