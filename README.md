# Mini P2P-File-Sharing application

This is a mini P2P file-sharing application where each peer has an index of files that it wishes to share over the P2P network. This project is supposed to work in UTD CS department servers thus it expects all its peers to have similar hostnames. So, the user initiating any peer application must know at least one other existing peer in the P2P network. The peers also have to be assigned an ID which only has 1 function to identify its own index file, because index files are numbered.

Once, the peer starts successfully and is a part of the P2P network, it can initiate a search for any desired filename or an associated keyword. The search occurs over the TCP sockets via messages between peers. If a peer has received a search request from a connected peer, then it will search its own index, if it can satisfy that search it responds with a YES message, otherwise forwards the search to other downstream peers. The search happens similar to a BFS traversal in a graph. Every search has 5 retries with 1, 2, 4, 8, and 16 second(s) timeouts, which also expands the number of hops allowed in each search request. If a reply is received after the timeout, it is ignored. If there are yes replies from at least one peer, then the search initiator creates a direct connection to that specific peer on another port just for file transfer. Peers use port 5050 for peer connections and messaging and they use 5051 for file transfer.

[Project Report](./REPORT.md)

## Note for UT Dallas students

This was Project 2 for UTD's CS6378 course.
Please don't use this code as is, because it may be flagged for plagiarism. UTD CS department takes plagiarism very seriously.
Please refer to [UTD's Academic Dishonesty](https://conduct.utdallas.edu/dishonesty) page for more info.

## Salient Features

- This project extensively uses TCP sockets and multithreading.
- At every peer, there are at least 4 threads working in parallel.
  - Thread to accept incoming P2P peer connections, which spawns a new peer-message-listening thread for each connected peer (killed after the peer disconnects).
  - Thread to accept incoming file-sharing requests, which spawns a new file transfer thread for each request (killed after the file transfer completion).
  - Thread acting as a daemon to monitor and clean up memory for peers who have disconnected.
  - At least one peer is connected, so one peer-message-listening thread. This thread is responsible for messaging between peers.
  - Main thread which has user prompts, and initiates the search.
- This P2P file system has been tested successfully with 15 peers connected with many types of overlay P2P network graphs.

## Project 1

Although Project 2 the main focus of this repository, I've also included Project 1 [here](./project1/).

## Server Init

- If the computer is the first peer in the p2p system, then we init with `./p2p-server` or `./p2p-server --id=<id>`. If no ID provided then id is assumed to be `1`.
- The ID will tag the peer as `Ci` and it determines what file it will read as `Fi` to participate in the p2p system.
- If the computer is not the first peer in the p2p system, then it is necessary to provide p2p peers in the arguments. In this case, We init with `./p2p-server --id=<id> --peers=dcXY1,dcXY2,dcXY3,..`. The peers have to be in a comma separated list without any spaces.

## Index File: Fi for Ci

- Each line in the index file must of the form `<fname>:<kw1>,<kw2>,<kw3>,...`

## TODO

- Neighbor reconfig incase of peer disconnect. No more network partitioning. [done]
- Use mutex accross threads for good multi-threading & synchronization [done]
- Full SimpleSearch TM [done]
- Replying to upstream [done]
- Handle File after search completion [done]
- Handle Disconnect after File download complete [done]
- No active exception error on shutdown [done]
- DISCONNECT sent on 5050 as well. Why? [done]
- Index update not happening on server. [done]

## Tests

- Full test after 15 peers conection [done]
- Search after one peer disconnects leads to a segfault after search complete. [done]
