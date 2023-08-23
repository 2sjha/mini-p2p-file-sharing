#!/bin/bash

ids=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15)
final_dir="aosp2"

copy_search_files_from_index() {
    peer_dir="$1"
    index_file="$2"

    # Read the contents of the index_file into an array
    mapfile -t file_lines < "$index_file"

    # Process each line and split using colon (:)
    for line in "${file_lines[@]}"; do
        IFS=':' read -ra file_kws <<< "$line"

        # Copy file in the index to Peer directory
        cp "search-files/${file_kws[0]}" $peer_dir/"${file_kws[0]}"
    done

}

make_clean() {
    make clean
    echo "make clean done"
}

default_peers_create() {
    # Create directory to store build executables
    mkdir -p $final_dir

    for i in ${ids[@]}
    do
        # Make directory for each peer for isolation
        peer_dir="$final_dir/peer-${i}"
        mkdir -p $peer_dir

        # Copy main executable
        cp p2p-server $peer_dir/p2p-server

        # Copy index file for each peer 
        cp "index-files/f${i}.dat" $peer_dir/"f${i}.dat"

        # Copy search files from index
        copy_search_files_from_index $peer_dir "index-files/f${i}.dat"
    done
}

server_peers_create() {

    default_peers_create

    # Copy build to home directory
    # then clean current directory
    mkdir -p ~/$final_dir
    yes | cp -r aosp2 ~
    make clean
}

if [ $# -eq 0 ]; then
    make_clean
    echo "making default"
    make
    default_peers_create
elif [ "$1" = "debug" ]; then
    make_clean
    echo "making debug"
    make debug
    default_peers_create
elif [ "$1" = "onserver" ]; then
    make_clean
    echo "making default on server"
    make 
    server_peers_create
elif [ "$1" = "onserverdebug" ] || [ "$1" = "debugonserver" ]; then
    make_clean
    echo "making debug on server"
    make debug
    server_peers_create
else
    echo "wrong argument"
    exit 1
fi

echo "${ids[-1]} peers created"
