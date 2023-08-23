## CS6378 Project 2 Report

## Project Setup

- The `index-files` directory is the main config file for each peer. Every file in `index-files` is of the form `f<c_id>.dat` and each `fi.dat` contains multiple lines of the form `fname:keyword1,keyword2,..`, e.g. `a.txt:apple,adam,arcade`
- We can use `generate_search_file.py` to create search files a.txt, b.txt, and so on. These text files are created from random words picked from a 1000-word list.
  - Or we can use the existing `search-files` directory which contains the same files pre-generated using the same python script.
- Then we use the `create-peers.sh` shell script to create peer directories with the `p2p-server` executable and necessary build files for each peer.
  - The `create-peers.sh` will create separate peer directories in `aosp2` directory, each with a `p2p-server` executable and `F_i.dat` index file and it will also read the index file and copy the text files in the index from the `search-files` to respective peer directories.
- The `create-peers.sh` shell script also takes arguments `debug`, `onserver`, `onserverdebug`, `debugonserver`. We may need to update permissions for the shell script using `chmod a+x create-peers.sh`.
  - The `debug` option creates debug executables
  - The `onserver` option moves this `aosp2` directory to the `home` directory.
  - `onserverdebug` and `debugonserver` is just onserver but with debug executables.

<hr>

## Demo

- Go to `aosp2` directory and choose and `peer-<c_id>` ,e.g. `peer-1` subdirectory.
- To run the first P2P server, use `./p2p-server --id=1`. This will prompt if this is the first peer in the P2P system. You need to press `Y` or `y` to start it.
- From then on, next peers need to start with previously setup peers, use `./p2p-server --id=2 --peers=dc01` (here dc01 is the dcXY id of the first peer) or `./p2p-server --id=3 --peers=dc02,dc03` (here dc02, dc03 is the dcXY id of the 2nd and 3rd peer).
- These `dcXY` IDs in the peers list must the the `dcXY` server where already set up peers are running. This peer will randomly select one of the peers in this peer list, and connect to it.
- We can make any kind of overlay network graph from these peers.
- Once any peer is up, the peer will prompt the user for an option. `0 = peer shutdown, 1 = initiate Simple Search with filename, 2 = initiate Simple Search with keyword, 3 = print peer info`.
- Once a search is initiated, it proceeds as mentioned in the project spec.
- After search completion, if the search is a success then it will prompt the user for the options where the file can be found.
The user chooses one of the file options, and this peer connects to that server and downloads the file. It also updates its own `F_i` index so that this peer can help in subsequent searches for that file. This index update is permanent, so even if this peer is restarted, it will read the updated `F_i` index and keep the new file as searchable.
- When the User chooses the `0` prompt to shut down any server, it will ensure that Network partitioning doesn't happen. As the project spec mentions, It will accumulate all connected peers into a list and then randomly choose one of those peers and send a message to that chosen peer `DISCONNECT_ADDNBRS_dcXY,dcAB,dcGH` so that chosen peer will establish connections with `dcXY`, `dcAB` and `dcGH`.
- The executable prints detailed logs & all the messages sent or received over the network, so it is easier to understand what is happening.