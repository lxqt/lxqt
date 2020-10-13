#!/bin/sh
#
# required: notify-send app installed (part of libnotify, or libnotify-tools)
#


# basic notifications
notify-send -u low -t 3000 --icon="document-open" "simple notification" "expires in 3s. No action there" &
notify-send -u normal -t 4000 --icon="document-close" "simple notification" "expires in 4s. No action there" &
notify-send -u critical -t 5000 --icon="application-exit" "simple notification" "expires in 5s. No action there" &
notify-send -u normal -t -1 --icon="go-next" "simple notification" "expires when server decies. No action there" &
notify-send -u normal -t 0 --icon="go-up" "simple notification" "never expires. No action there" &

notify-send -u low -t 3000 --icon="document-open" "<b>simple notification</b> with a very long text inside it" "<i>expires in 3s</i>. No action there. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel leo quam. Morbi sit amet lorem vel dui commodo porttitor nec ut libero. Maecenas risus mauris, faucibus id tempus eu, auctor id purus. Vestibulum eget sapien non sem fermentum fermentum id sed turpis. Morbi pretium sem at turpis faucibus facilisis vel lacinia ante. Quisque a turpis lectus, quis posuere magna. Etiam magna velit, sagittis sed tincidunt et, adipiscing rutrum est. Aliquam aliquam aliquet tortor non varius. Quisque sollicitudin, ligula ac pulvinar laoreet, lacus metus sagittis nulla, ac sodales felis diam sed urna. In hac habitasse platea dictumst. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas." &

