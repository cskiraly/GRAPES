NUM_PEERS=20
PEER_PORT_BASE=5555
((PEER_PORT_MAX=PEER_PORT_BASE + NUM_PEERS - 1))


function cnt {
  echo -n "$1 "; grep ":$1" n-*.txt | wc -l
}

for PORT in `seq $PEER_PORT_BASE 1 $PEER_PORT_MAX`;
 do
  cnt $PORT
 done
