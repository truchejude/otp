# Gestion des mots de passe OTP

Ce projet comprend plusieurs scripts Bash permettant de gérer et de configurer un système de génération de mots de passe OTP via des fichiers dans le `debugfs`.

Merci de compiler le projet avec make et de l'exécuter avec ./start.sh.
Utilisez ./start.sh -h pour plus d'informations

## Scripts disponibles

### 1. **get_mdp_list.sh**
Ce script permet d'obtenir la liste actuelle des mots de passe OTP.

**Usage :**
```bash
sudo ./get_mdp_list.sh
```

**Commandes associées :**
```bash
sudo cat /dev/otp0
```

---

### 2. **modif_mdp_nbr.sh**
Ce script permet de modifier le nombre de mots de passe générés.  
La valeur doit être un entier compris entre **1** et **20**.

**Usage :**
```bash
./modif_mdp_nbr.sh <new_value>
```

**Exemple :**
```bash
./modif_mdp_nbr.sh 5
```

**Conditions :**
- La valeur doit être un entier positif.
- La valeur doit être comprise entre **1** et **20**.

---

### 3. **modif_time.sh**
Ce script permet de modifier le temps de rechargement des mots de passe OTP.

**Usage :**
```bash
./modif_time.sh <new_value>
```

**Exemple :**
```bash
./modif_time.sh 300
```

**Conditions :**
- La valeur doit être un entier positif.

---

### 4. **pass.sh**
Ce script permet de proposer un mot de passe au système.

**Usage :**
```bash
./pass.sh <MDP>
```

**Exemple :**
```bash
./pass.sh my_secure_password
```

---

### 5. **reset_pass.sh**
Ce script permet de recharger la liste des mots de passe en réinitialisant leur état.

**Usage :**
```bash
sudo ./reset_pass.sh
```

**Commandes associées :**
```bash
sudo cat /sys/kernel/debug/my_debugfs_dir/mdp_input
```

---

### 6. **start.sh**
Ce script initialise le module OTP avec des paramètres personnalisables ou affiche l'aide si aucune option n'est fournie.

**Usage :**
```bash
sudo ./start.sh [-h] [-n <num_passwords>] [-t <time>]
```

**Options :**
- `-h` : Affiche l'aide du script.
- `-n <num_passwords>` : Spécifie le nombre de mots de passe à générer (par défaut : 3).
- `-t <time>` : Définit le temps de rechargement des mots de passe en secondes (par défaut : 240).

**Exemples :**
```bash
sudo ./start.sh -n 5 -t 300
sudo ./start.sh -h
```

**Commandes exécutées dans le script :**
```bash
sudo rmmod otp
sudo insmod otp.ko num_passwords=<num_passwords> time=<time>
./get_mdp_list.sh
```

---