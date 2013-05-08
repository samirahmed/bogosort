# Distributed Systems Spring 2013

## Team: BOGOSORT

Members: 

- Katsutoshi Kawakami, `kkawakam@bu.edu`
- William Seltzer, `wseltzer@bu.edu`
- Samir Ahmed, `samahmed@bu.edu`

See all the planning and specifications for the game in this [google doc](https://docs.google.com/document/d/1k-GoO7uVXnsxbEDet3T3o6b-2WuqdsMCtxiKncwQ33o/edit?usp=sharing)

Currently Bogosort CTF is not supported on OSX (linux only)

## UI
| command | description |
|---------|-------------|
|`←`| move left|
|`→`| move right|
|`↑`| move up|
|`↓`| move down|
|`w`| pan up|
|`a`| pan left|
|`d`| pan right|
|`s`| pan down|
|`z`| zoom in|
|`x`| zoom out|
|`r`| pickup / drop flag|
|`g`| pickup / drop shovel|
## [Client](/client)

## [Server](/server)

![server](http://i.imgur.com/RFvIMkF.png)

## [Unit Test](/test)

Test for the `lib` directory's `game_server`, `game_client`, `game_commons` files, where the core of the game logic lies.

![unit-test](http://i.imgur.com/f4JdyUg.png)

## Integration Test

Tests for the server and client binaries.  Spawns clients (and optinonally a server too) and feed commands to manipulate them.
Runs preset recipes

![integration test](http://i.imgur.com/Pp54cIN.png)

## Chef

`chef.rb` is a ruby script for server/client integration testing. 

you can run chef by typing `ruby chef.rb <test-name> <parameters> ... `

Chef supports the following parameters.

|command line argument | description|
|----------------------|------------|
| 4-5 digit Port number | matches any 4 or 5 digit number the port i.e `34255` (spawns server if not port) |
| IP address | any valid ip address will be matched i.e `126.52.251.112` (defaults to localhost) |
| `ALL` or `--all` or `-a` | run all the recipes in `test/` |
| `-v` or `--verbose` | display more run time details including each line issued by chef (default false for large recipes) |
| `-d` or `--debug` | display the debug spew |
| `-c` or  `--client-only`| do not spawn a server (default true when port or ip specified) |
| `-p` or `--pause` | chef will pause after running the recipe so that you can play with the server | 
| `-s` or `--slow` | chef will automatically issue longer delays |
| `console` | interactive menu for creating recipes |

### Recipes

Save your recipes in the `test/` folder

Recipes require the following format to be understood by `chef.rb`.
All recipes are CSV files

**First Line**

The first line declares some parameters (only the first one is necessary)

`<total number of clients>, <parameters> ,...`

parameters = `teleport`. Teleport sends the server (if there is one) a signal to turn on teleportation

**Every other line**

`<client_no>, <delay in seconds>, <command>, <optional argument>`

- 0 <= `<client_no>`< total number of clients.
- the delay may be specificed in seconds.. this means `0.245` is  245 millisecond delay
- command can be either an `MOV`,`HEL`,`BYE`,`SYN`,`TEL`... (move/hello/goodbye/sync/teleport)
- argument can be `0-3` or `right|left|up|down` for `MOV` and any valid `x,y` value for `TEL`

## Building, Cleaning

Building and running

<pre>
$ git clone https://github.com/BU-CS451-DS/bogosort.git
$ cd bogosort
$ make
$ server/server
</pre>

#### Max File Limit

It is possible that your machine has limits the number of open files per process.

For Testing, some test might require over 1000 files open (2*500 clients)
For normal usage the server would need ` 375 * 2 ` or **1000** to be safe.

You can check the limit by running `$ ulimit -n`
On both OSX (no long supported :( ) and Linux you can set the limit with `$ ulimit -n 2048`

## Make commands

Assuming these commands are issued from the root bogosort directory

| command | description |
|---------|-------------|
| `make clean`| recursively destroys all binaries |
| `make` | build project if not built |
| `make clean-log` | cleans out the log/ folder |
| `make clean-docs` | cleans out the doc/full folder |
| `make utest` | run all unit tests|
| `make itest` | run all integration tests |
| `make test` | run both integration and unit tests |

`make clean` will clean and build lib/client/server
`make test` will run tests

## Complete Documentation

For the the complete documentation install doxygen

`brew install doxygen` or `sudo apt-get install doxygen` and then run
`doxygen Doxyfile` to create the complete documentation that can be viewed in `doc/full/html/index.html`


