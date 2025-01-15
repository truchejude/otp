#!/bin/bash

# Vérifie qu'un argument est fourni
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <new_value>"
    exit 1
fi

NEW_VALUE="$1"

# Vérifie que l'argument est un entier
if ! [[ "$NEW_VALUE" =~ ^[0-9]+$ ]]; then
    echo "Erreur : La valeur doit être un entier positif."
    exit 1
fi

# Écrit la valeur dans le fichier debugfs avec une redirection
echo "$NEW_VALUE" | sudo sh -c 'cat > /sys/kernel/debug/my_debugfs_dir/time'
# ./pass.sh {MDP}