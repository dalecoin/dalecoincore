#!/bin/sh
export

mkdir -p /data
touch /data/dalecoin.conf
echo "rpcport=80" > /data/dalecoin.conf
echo "rpcuser=${RPC_USER}" >> /data/dalecoin.conf
echo "rpcpassword=${RPC_PASS}" >> /data/dalecoin.conf
echo "rpcallowip=*" >> /data/dalecoin.conf
cat /data/dalecoin.conf

dalecoind -testnet -staking=0 -datadir=/data -logtimestamps -printtoconsole

