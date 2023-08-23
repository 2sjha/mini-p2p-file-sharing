# Project 1

The purpose of this project is to demonstrate file sharing over a TCP socket. We consider 3 processes, namely P1, P2 & P3. The processes P1 and P2 act as file servers for just 1 file F1 and F2 respectively. P3 acts as the client with user input enabled. When P3 starts, it prompts the user to input a file name, and then P3 first requests P1 for that file, If the file is found at P1, then that file is transferred over the TCP socket. If the file is not found at P1, then it requests P2 for the same. If the file is not found at either of the servers then the client outputs an error message.
The code was meant to work on the UTD CS dept. machines and it has hardcoded machine hostnames for P1 and P2.

## Note for UTDallas students

This was Project 1 for UTD's CS6378 course.
Please don't use this code as is, because it may be flagged for plagiarism. UTD CS dept. takes plagiarism very seriously.
Please refer to [UTD's Academic Dishonesty](https://conduct.utdallas.edu/dishonesty) page for more info.

## F1 and F2 contents.

- F1 = `f1.dat` and F2 = `f2.dat`
- Both files have 99 lines of text. Each line ends with `\n`.
- First line is `start`
- Then 97 lines of 2 length random strings.
- Last line is `end`

## Build

- `make` builds `./server` and `./client` executables.
- `make clean` removes all built executables.

## Server.cpp

- This is the code for P1 and P2, which act as the servers.
- Usage: `./server <c_id>`. The server executable expects a `c_id` as an argument. If argument is not provided, then it will prompt user for `c_id`. Allowed C_IDs are `1` are `2` only.
<!-- - After server is started, It performs its functions automatically, but if user wishes to terminate server preemptively, user can press 0 to terminate the server. I implemented this so that server socket can be closed nicely, instead of `ctrl + c` which can lead to socket connection error on subsequent runs. -->

## Client.cpp

- This is the code for P3, which acts as the client.
- Usage: `./client --p1=dc01 --p2=dc02` or `./client dc01 dc02`. If arguments are not provided, then it will prompt user for p1 and p2 address in `dcXY` format, where `01 <= XY <=45`.

## Testing

- After build, copy `./server` and `f1.dat` as server 1 to some `dcXY1` machine.
- copy `./server` and `f2.dat` as server 2 to some other `dcXY2` machine.
- copy `./client` to a third `dcXY3` machine.
- Run `./server 1` to start P1.
- Run `./server 2` to start P2.
- Run `./client dcXY1 dcXY2` or `./client --p1=dcXY1 --p2=dcXY2` to start P3.
- Client will prompt user for file name. File will be transferred from P1 or P2 as described in project spec.
- After file transfer is complete. P3 and (P1 or P2) will shut down gracefully.
- The other server needs to be terminated using `ctrl + c` for subsequent tests.
- In case of file transfer failure, both P1 and P2 need to be forcefully terminated for subsequent tests.
