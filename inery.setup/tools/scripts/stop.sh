#!/bin/bash
DATADIR="./blockchain/"

if [ -f $DATADIR"/ined.pid" ]; then
pid=`cat $DATADIR"/ined.pid"`
echo $pid
kill $pid
rm -r $DATADIR"/ined.pid"
echo -ne "Stoping Node"
while true; do
[ ! -d "/proc/$pid/fd" ] && break
echo -ne "."
sleep 1
done
echo -ne "\\rNode Stopped. \\n"
fi 