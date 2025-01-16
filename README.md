# Gestion des mots de passe OTP

Ce projet comprend plusieurs scripts Bash permettant de gérer et de configurer un système de génération de mots de passe OTP via des fichiers dans le `debugfs`.

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
Ce script initialise le module OTP avec des paramètres par défaut, récupère la liste des mots de passe, et exécute le programme principal.

**Usage :**
```bash
sudo ./start.sh
```

**Commandes exécutées dans le script :**
```bash
sudo rmmod otp
sudo insmod otp.ko num_passwords=3 time=240
./get_mdp_list.sh
```

---

## Prérequis
- Accès root pour exécuter certains scripts (`sudo`).
- Un module kernel `otp.ko` correctement compilé et installé.
- Accès au système de fichiers `debugfs`.

---

## Avertissements
- Les scripts qui modifient des paramètres utilisent des commandes privilégiées, assurez-vous de comprendre leurs effets avant de les exécuter.
- Testez les scripts dans un environnement contrôlé avant de les utiliser en production.

---

## Auteur
Ce projet a été conçu pour simplifier la gestion des mots de passe OTP via un système basé sur `debugfs`.

