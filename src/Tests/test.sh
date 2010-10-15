#!/bin/bash
#launch it e.g. like this:
#./test.sh -e "offerstreamer-ml" -n 5 -s "-m 1" -p "-b 40 -o 5 -c 25" -f "chbuf"
#./test.sh -e "offerstreamer-ml" -f chbuf

# Kill everything we've stared on exit (with trap).
trap "ps -o pid= --ppid $$ | xargs kill -9 2>/dev/null" 0

#defaults
SOURCE_PORT=6666
PEER_PORT_BASE=5555
NUM_PEERS=1
FILTER="neigh"
CLIENT="./topology_test"

#process options
while getopts "s:S:p:P:N:f:e:v:VX:" opt; do
  case $opt in
    s)
      SOURCE_OPTIONS=$OPTARG
      ;;
    S)
      SOURCE_PORT=$OPTARG
      ;;
    p)
      PEER_OPTIONS=$OPTARG
      ;;
    P)
      PEER_PORT_BASE=$OPTARG
      ;;
    N)
      NUM_PEERS=$OPTARG
      ;;
    f)	#TODO
      FILTER=$OPTARG
      ;;
    e)
      CLIENT=$OPTARG
      ;;
    v)
      VIDEO=$OPTARG
      ;;
    V)	#TODO
      VALGRIND=1
      ;;
    X)
      NUM_PEERS_X=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

valgrind --error-exitcode=1 --track-origins=yes  --leak-check=full $CLIENT $SOURCE_OPTIONS 2> /dev/null &

sleep 2
((PEER_PORT_MAX=PEER_PORT_BASE + NUM_PEERS - 1))
for PORT in `seq $PEER_PORT_BASE 1 $PEER_PORT_MAX`; do
    valgrind --error-exitcode=1 $CLIENT $PEER_OPTIONS -P $PORT -i 127.0.0.1 -p $SOURCE_PORT 2>stderr.$PORT > stdout.$PORT &
done

((PEER_PORT_BASE = PEER_PORT_MAX + 1))
((PEER_PORT_MAX=PEER_PORT_BASE + NUM_PEERS_X - 1))
for PORT in `seq $PEER_PORT_BASE 1 $PEER_PORT_MAX`; do
#  valgrind --track-origins=yes  --leak-check=full \ TODO!!!
    FIFO=fifo.$PORT
    rm -f $FIFO
    mkfifo $FIFO
    xterm -e "$CLIENT $PEER_OPTIONS -P $PORT -i 127.0.0.1 -p $SOURCE_PORT >$FIFO 2>/dev/null | grep '$FILTER' $FIFO" &
done

#valgrind --track-origins=yes  --leak-check=full TODO!

wait
