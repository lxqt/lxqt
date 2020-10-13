#!/bin/sh
#
# required: notify-send app installed (part of libnotify, or libnotify-tools)
#


# basic notifications
notify-send -u low -t 3000 --icon="document-open" "simple notification" "expires in 3s. No action there" &
notify-send -u normal -t 4000 --icon="document-close" "simple notification" "expires in 4s. No action there" &
notify-send -u low -t 3000 --icon="document-open" "simple notification" "expires in 3s. No action there" &
notify-send -u low -t 3000 --icon="document-open" "simple notification" "expires in 3s. No action there" &

