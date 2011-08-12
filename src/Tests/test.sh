#!/bin/bash
#launch it e.g. like this:
#./test.sh -e "offerstreamer-ml" -n 5 -s "-m 1" -p "-b 40 -o 5 -c 25" -f "chbuf" -A "other args"
#./test.sh -e "offerstreamer-ml" -f chbuf

PIDS=""

#defaults
SOURCE_PORT=6666
PEER_PORT_BASE=5555
NUM_PEERS=1
FILTER="neigh"
CLIENT="./topology_test"
CLIENT_ARGS=""
IFACE=""

CHURN_MAX=30
CHURN_MIN=30
CHURN_WAIT=10
CHURN_PORT_INCR=100
STARTUP_WAIT=0

function churn {
  # Kill everything we've started on exit (with trap).
  trap "ps -o pid= --ppid $BASHPID | xargs kill 2>/dev/null" 0

  MIN=$1
  MAX=$2
  PAUSE=$3
  PORT=$4
  PORT_INCREMENT=$5

  if [ $MIN -lt $MAX ]; then
    let "RUN=$MIN+($RANDOM%($MAX-$MIN))"
  else
    RUN=$MIN
  fi

  while [ 1 ] ; do
#    echo "$VALGRIND $CLIENT $PEER_OPTIONS $IFACE -P $PORT -i 127.0.0.1 -p $SOURCE_PORT $CLIENT_ARGS" > cmd.$PORT
    $VALGRIND $CLIENT $PEER_OPTIONS $IFACE -P $PORT -i 127.0.0.1 -p $SOURCE_PORT $CLIENT_ARGS 2>stderr.$PORT > stdout.$PORT &
    PID=$!
    sleep $RUN
    kill $PID
    sleep $PAUSE
    ((PORT+=PORT_INCREMENT))
  done

}



#process options
while getopts "s:S:p:I:P:N:f:e:v:VX:A:C:c:t:T:w:W:" opt; do
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
#    P)
#      PEER_PORT_BASE=$OPTARG
#      ;;
    I)
     IFACE="-I $OPTARG"
     ;;
    N)
      NUM_PEERS=$OPTARG
      ;;
    f)  #TODO
      FILTER=$OPTARG
      ;;
    e)
      CLIENT=$OPTARG
      ;;
    v)
      VIDEO=$OPTARG
      ;;
    V)  #TODO
      VALGRIND="valgrind --error-exitcode=1 --track-origins=yes  --leak-check=full "
      VALGRIND="valgrind --error-exitcode=1 --leak-check=full "
      ;;
    X)
      NUM_PEERS_X=$OPTARG
      ;;
    A)
      CLIENT_ARGS=$OPTARG
      ;;
    C)
      NUM_PEERS_C=$OPTARG
      ;;
    c)
      CHURN_PORT_INCR=$OPTARG
      ;;
    t)
      CHURN_MIN=$OPTARG
      ;;
    T)
      CHURN_MAX=$OPTARG
      ;;
    w)
      CHURN_WAIT=$OPTARG
      ;;
    W)
      STARTUP_WAIT=$OPTARG
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

$VALGRIND $CLIENT $SOURCE_OPTIONS $IFACE $CLIENT_ARGS &
PIDS=$!

sleep 2
((PEER_PORT_MAX=PEER_PORT_BASE + NUM_PEERS - 1))
for PORT in `seq $PEER_PORT_BASE 1 $PEER_PORT_MAX`; do
    $VALGRIND $CLIENT $PEER_OPTIONS $IFACE -P $PORT -i 127.0.0.1 -p $SOURCE_PORT $CLIENT_ARGS 2>stderr.$PORT > stdout.$PORT &
    PIDS="$!,$PIDS"
done

((PEER_PORT_BASE = PEER_PORT_MAX + 1))
((PEER_PORT_MAX=PEER_PORT_BASE + NUM_PEERS_X - 1))
if [ $PEER_PORT_BASE -le $PEER_PORT_MAX ]; then
    for PORT in `seq $PEER_PORT_BASE 1 $PEER_PORT_MAX`; do
    #  valgrind --track-origins=yes  --leak-check=full \ TODO!!!
        FIFO=fifo.$PORT
        rm -f $FIFO
        mkfifo $FIFO
        xterm -e "$CLIENT $PEER_OPTIONS $IFACE -P $PORT -i 127.0.0.1 -p $SOURCE_PORT $CLIENT_ARGS >$FIFO 2>stderr.$PORT | grep '$FILTER' $FIFO" &
        PIDS="$!,$PIDS"
    done
fi

((PEER_PORT_BASE = PEER_PORT_MAX + 1))
((PEER_PORT_MAX=PEER_PORT_BASE + NUM_PEERS_C - 1))
for PORT in `seq $PEER_PORT_BASE 1 $PEER_PORT_MAX`; do
    churn $CHURN_MIN $CHURN_MAX $CHURN_WAIT $PORT $CHURN_PORT_INCR &
    sleep $STARTUP_WAIT
done

# Kill everything we've stared on exit (with trap).
if [ `uname -s` = "Linux" ]; then
    echo "Installing linux trap..."
    trap "ps -o pid= --ppid $$ | xargs kill -9 2>/dev/null" 0;
elif [ `uname -s` = "Darwin" ]; then
    echo "Installing darwin trap..."

    trap "ps -o pid= -p '$PIDS' | xargs kill -9 2>/dev/null" 0;
fi

#valgrind --track-origins=yes  --leak-check=full TODO!
wait
