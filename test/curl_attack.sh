#!/bin/sh

COUNTER=1
NO_REQ=$1
ATTACKED_HOST=$2

while [ $COUNTER -lt $NO_REQ ]
do
 echo $COUNTER
 #curl $ATTACKED_HOST/vmon 1>/dev/null &
 curl -H "Host: $ATTACKED_HOST" vmon1.staging.deflect.ca/vmon 1>/dev/null &
 COUNTER=$(( $COUNTER + 1 ))
done
