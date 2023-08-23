# Project 2

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

## Test cases

- Full test after 15 peers conection [done]
- Search after one peer disconnects leads to a segfault after search complete. [done]

## CS6378 Project 2 Report (sxj220028)

## Project Setup

- `index-files` directory is the main config file for each peer. Every file in `index-files` is of the form `f<c_id>.dat` and each `fi.dat` contains multiple lines of the form `fname:keyword1,keyword2,..`, e.g. `a.txt:apple,adam,arcade`
- We can use `generate_search_file.py` to create search files a.txt, b.txt and so on which generates these text files from random words picked from a 1000 word list.
- Or we can use the existing `search-files` directory which contains the same files pre-generated using the same python script.
- Then we use `create-peers.sh` shell script to create peers with `p2p-server` executable and necessary files
- This will create separate peer directories in `aosp2` directory, each with `p2p-server` executable and `F_i.dat` index file and it will also read the index file and copy the text files in the index from `search-files` to respective peer directories.
- The `create-peers.sh` shell script also takes arguments `debug`, `onserver`, `onserverdebug`, `debugonserver`. We may need to update permissions for the shell script using `chmod a+x create-peers.sh`.
  - `debug` option creates debug executables
  - `onserver` option moves this `aosp2` directory to `home` directory.
  - `onserverdebug` and `debugonserver` is just onserver but with debug executables.

<hr>

## Demo

- Go to `aosp2` directory and choose and `peer-<c_id>` ,e.g. `peer-1` subdirectory.
- To run the first P2P server, use `./p2p-server --id=1`. This will promp if this is the first peer in the P2P system. You need to press `Y` or `y` to start it.
- From then on, next peers need to started with previously setup peers, use `./p2p-server --id=2 --peers=dc01` (here dc01 is the dcXY id of the first peer) or `./p2p-server --id=3 --peers=dc02,dc03` (here dc02, dc03 is the dcXY id of the 2nd and 3rd peer).
- These `dcXY` ids in peers list must the the `dcXY` server where already set up peers are running. This peer will randomly select one of the peers in this peer list, and connect to it.
- We can make any kind of overlay network graph from these peers.
- Once any peer is up, the peer will prompt the user for 1 options. `0 = peer shutdown, 1 = initiate Simple Search with filename, 2 = initiate Simple Search with keyword, 3 = print peer info`.
- Once search is initiated, it proceeds as mentioned in the project spec.
- After search completion, if the search is a success then it will prompt the user for the options where the file can be found.
- User chooses one of the file options, this peers connects to that server and downloads the file. It also updates it own `F_i` index so that this peer can help in subsequent searches for that file. This index update is permanent, so even if this peer is restarted, it will read the updated `F_i` index and keep the new file as searchable.
- When User chooses `0` prompt to shut down any server, it will ensure that Network partitioning doesnt happen. As the project spec mentions, It will accumulate all connected peers into a list and then randomly choose one of those peers and sends a message to that chosen peer `DISCONNECT_ADDNBRS_dcXY,dcAB,dcGH` so that chosen peer will establish connections with `dcXY`, `dcAB` and `dcGH`.
- The executable prints detailed logs & all the messages sent or received over the network, so it is easier to understand what is happening.
