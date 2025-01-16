#!/bin/bash

# Valeurs par défaut
NUM_PASSWORDS=3
TIME=240

# Fonction d'aide
function usage() {
    echo "Usage: $0 [-n num_passwords] [-t time] [-h]"
    echo "Options:"
    echo "  -n num_passwords   Nombre de mots de passe (1-20)."
    echo "  -t time            Temps en secondes (doit être positif)."
    echo "  -h                 Affiche cette aide."
    exit 1
}

# Analyse des arguments
while getopts "n:t:h" opt; do
    case $opt in
        n)
            if [[ "$OPTARG" =~ ^[0-9]+$ ]] && (( OPTARG > 0 && OPTARG <= 20 )); then
                NUM_PASSWORDS=$OPTARG
            else
                echo "Erreur: num_passwords doit être un entier entre 1 et 20."
                exit 1
            fi
            ;;
        t)
            if [[ "$OPTARG" =~ ^[0-9]+$ ]] && (( OPTARG > 0 )); then
                TIME=$OPTARG
                echo $OPTARG
            else
                echo "Erreur: time doit être un entier positif."
                exit 1
            fi
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done

# Exécution des commandes
sudo rmmod otp
sudo insmod otp.ko num_passwords=$NUM_PASSWORDS time=$TIME
./get_mdp_list.sh
