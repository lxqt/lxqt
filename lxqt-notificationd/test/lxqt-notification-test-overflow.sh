#!/bin/sh
#
# required: notify-send app installed (part of libnotify, or libnotify-tools)
#

notify-send -t 8000 "tested notification" "Is it still the same size?" &

sleep 2

for i in $(seq 10)
do
    notify-send -t 4000 "a notification" "4 seconds notification #${i}" &
done

for i in $(seq 40)
do
    notify-send -t 2000 "a notification" "2 seconds notification #${i}" &
done

