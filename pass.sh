#!/bin/bash

# Vérifie qu'un argument est fourni
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <new_value>"
    exit 1
fi

NEW_VALUE="$1"
echo "$NEW_VALUE" | sudo tee /sys/kernel/debug/my_debugfs_dir/mdp_input
# ./pass.sh {MDP}