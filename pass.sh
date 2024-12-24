#!/bin/bash

# Vérifie qu'un argument est fourni
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <new_value>"
    exit 1
fi

# Affecte l'argument à une variable
NEW_VALUE="$1"

# Écrit la valeur dans le fichier debugfs avec les privilèges nécessaires
echo "$NEW_VALUE" | sudo tee /sys/kernel/debug/my_debugfs_dir/mdp_input
