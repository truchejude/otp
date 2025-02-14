#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>

#include <linux/device.h>
#include <linux/cdev.h>

// GLOBAL VARIABLE START:

// Structure pour chaque mot de passe
struct password_item
{
    char *password;        // Mot de passe généré
    struct list_head list; // Liste chaînée (pour lier les éléments)
    bool use;
};

// Déclaration de la tête de la liste chaînée
static LIST_HEAD(password_list);

// ------------------------------------------------------------------------ device
#define DEVICE_NAME "OTP"
#define CLASS_NAME "otp_class"
#define MAX_PASSWORD_LEN 100

static int major_number;
static struct class *otp_class = NULL;
static struct device *password_device = NULL;

static char password_buffer[MAX_PASSWORD_LEN] = "";
static size_t password_length = 0;

int update_information(const int free);

// Fonction appelée lors de la lecture du device
static ssize_t device_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    if (*ppos > 0 || password_length == 0)
        return 0; // EOF

    if (copy_to_user(user_buf, password_buffer, password_length))
        return -EFAULT;

    *ppos += password_length;
    return password_length;
}

// Fonction appelée lors de l'écriture dans le device
static ssize_t device_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    struct password_item *item;
    char input[MAX_PASSWORD_LEN];

    if (count > MAX_PASSWORD_LEN - 1)
        return -EINVAL;

    if (copy_from_user(input, user_buf, count))
        return -EFAULT;

    input[count - 1] = '\0';

    list_for_each_entry(item, &password_list, list)
    {
        if (strcmp(item->password, input) == 0)
        {
            if (!item->use)
            {
                item->use = true;
                snprintf(password_buffer, MAX_PASSWORD_LEN, "Password '%s' marked as used.\n", input);
                password_length = strlen(password_buffer);
                pr_info("yep\n");
                return count;
            }
            else
            {
                snprintf(password_buffer, MAX_PASSWORD_LEN, "Password '%s' already used.\n", input);
                password_length = strlen(password_buffer);
                pr_info("pas yep\n");
                return count;
            }
        }
    }

    snprintf(password_buffer, MAX_PASSWORD_LEN, "Password '%s' not found.\n", input);
    password_length = strlen(password_buffer);
    pr_info("PAS DE MDP COMME ça\n");
    return count;
}

// Fonction appelée lors de l'ouverture du device
static int device_open(struct inode *inode, struct file *file)
{
    pr_info("Device opened\n");
    return 0;
}

int save_and_lock_passwords(void);
// Fonction appelée lors de la fermeture du device
static int device_release(struct inode *inode, struct file *file)
{
    pr_info("Device closed\n");
    save_and_lock_passwords();
    return 0;
}

// Définition des opérations du device
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

// Fonction pour définir les permissions des devices
static char *password_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
    {
        *mode = 0666; // Lecture et écriture pour tous les utilisateurs
    }
    return NULL;
}

// ------------------------------------------------------------------------ My_LIB
struct file_info
{
    struct file *file_ptr;
    const char *my_file_path;
};

// LE NOMBRE DE FICHIER D'EN LE MODULE A BESOIN
#define NBR_OF_FILE 3
#define MDP_INFO_FILE 0
#define FUCHIER_SUCESS 1

// TOUT LES PATH DES FICHIER D'EN LE MODULE A BESOIN je n'utilise pas le 1 et le 2 pour le moment mais ils sont la on sais jaja
static const char *file_path_predef[NBR_OF_FILE] = {
    "/dev/otp0", "/dev/otp1", "/dev/otp2"};

// UNE STRUCTURE POUR AVOIR PLUS FACILEMENT LESINFORMATION DES FICHIER
static struct file_info file_info_tab[NBR_OF_FILE];

// ------------------------------------------------------------------------ DEBUG DIR
static char pass_world_input[100] = "";
#define NBR_OF_DEBUG 3
static const char *debug_file_name[NBR_OF_DEBUG] = {
    "mdp_input", // restart le mdp
    "nbr_pass",  // pour changer le mdp
    "time"       // pour changer le temps de restart
};
static struct dentry *debug_dir, *debug_file[NBR_OF_DEBUG];

// Définir les opérations de fichier pour pass_world_input

// si il est écrit ça vas proposer un mdp a la liste
static ssize_t is_pass_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos);
// si il est lu ça vas restart les mdp
static ssize_t is_pass_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);
// si le man écrit dans le fichier nbr_pass (changer de nombre de mdp)
static ssize_t changeMdpNpr(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos);
// si le man écrit dans le fichier time ça change le nombre de mdp
static ssize_t changeProckTime(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos);

/*
// just pour tester
static ssize_t plopW(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    pr_err("PLOP: W");
    return 0;
}
*/

// just pour tester
static ssize_t plopR(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    pr_err("PLOP: R");
    return 0;
}

static const struct file_operations fops_str[NBR_OF_DEBUG] = {{
                                                                  .owner = THIS_MODULE,
                                                                  .read = is_pass_read,
                                                                  .write = is_pass_write,
                                                              },
                                                              {
                                                                  .owner = THIS_MODULE,
                                                                  .read = plopR,
                                                                  .write = changeMdpNpr,
                                                              },
                                                              {
                                                                  .owner = THIS_MODULE,
                                                                  .read = plopR,
                                                                  .write = changeProckTime,
                                                              }};

// ------------------------------------------------------------------------ GÉNERATEUR DE MDP

// déclaration des function
int add_password_to_list(const char *password);
int generate_passwords(int number);
void print_passwords(void);

// Liste des mots de base
static const char *base_words[] = {
    "Chavel", "Maison", "Jambon", "Tracteur", "Chat", "Chien", "Autruche", "Hipopotame", "Perigord", "Canard", "Fraise"};
static const size_t base_words_count = sizeof(base_words) / sizeof(base_words[0]);

// Paramètre de module pour spécifier le nombre de mots de passe à générer
static int num_passwords = 5;
module_param(num_passwords, int, 0444);
MODULE_PARM_DESC(num_passwords, "Nombre de mots de passe à générer");

// GLOBAL VARIABLE END:

// ------------------------------------------------------------------------ le time

// Paramètre de module pour spécifier le nombre de mots de passe à générer
static int time = 5; // Valeur par défaut : 5 pour 5 segonde
module_param(time, int, 0444);
MODULE_PARM_DESC(time, "le temps d'que les mdp se refreche");

static struct hrtimer test_hrtimer;
// static u64 sampling_period_ms = (0 * HZ);

// ------------------------- My_LIB ------------------------- START

/*
    strcpy_kernel:
        c'est pour copier un chaine de caractaire
*/
static char *my_strcpy_kernel(const char *src)
{
    size_t len = strlen(src) + 1;
    char *dest = kmalloc(len, GFP_KERNEL);

    if (!dest)
    {
        pr_err("Erreur d'allocation de mémoire pour strcpy_kernel\n");
        return NULL;
    }

    // Copie la chaîne source dans la mémoire allouée
    memcpy(dest, src, len);

    return dest; // Retourne le pointeur vers la chaîne copiée
}

// pour écrire dans un fichier //MARCHE PLUS

static int write_to_file(const char *data, int nbrf)
{
    pr_info("LE FICHIER VAS éTRE EDITER AVEC: %s %d\n", data, nbrf);
    loff_t pos = 0;
    size_t len = strlen(data);
    ssize_t written;

    // Tronquer le fichier pour éviter le mélange avec l'ancien contenu
    struct inode *inode = file_inode(file_info_tab[nbrf].file_ptr);
    if (inode)
    {
        vfs_truncate(&file_info_tab[nbrf].file_ptr->f_path, 0);
    }

    written = kernel_write(file_info_tab[nbrf].file_ptr, data, len, &pos);
    if (written != len)
    {
        pr_err("Failed to write the entire content to the file (%ld/%zu)\n", written, len);
        return -EIO;
    }

    return 0;
}

// ------------------------- My_LIB ------------------------- END

// ------------------------- DEBUG DIR ---------------------- START

// pour changer le nombre de mdp
static ssize_t changeMdpNpr(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    char kbuf[16];
    int new_value;

    if (count >= sizeof(kbuf))
        return -EINVAL; // Retourne une erreur si l'entrée est trop grande

    // Copie depuis l'espace utilisateur
    if (copy_from_user(kbuf, user_buf, count))
        return -EFAULT;

    kbuf[count] = '\0';

    // Conversion en entier
    if (kstrtoint(kbuf, 10, &new_value) < 0)
    {
        pr_err("Erreur : entrée non valide\n");
        return -EINVAL;
    }

    // Validation de la plage (0 < x <= 20)
    if (new_value <= 0 || new_value > 20)
    {
        pr_err("Erreur : valeur hors de la plage (0 < x <= 20)\n");
        return -EINVAL;
    }

    // Mise à jour de num_passwords
    num_passwords = new_value;
    if (update_information(1) == -ENOMEM)
    {
        pr_err("Failed to update information(\n");
        return -ENOMEM;
    }
    pr_info("num_passwords mis à jour à : %d\n", num_passwords);

    return count; // Retourne le nombre d'octets traités
}

// pour changer le temps de changement des clé
static ssize_t changeProckTime(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    char kbuf[16];
    int new_value;

    if (count >= sizeof(kbuf))
        return -EINVAL; // Retourne une erreur si l'entrée est trop grande

    // Copie depuis l'espace utilisateur
    if (copy_from_user(kbuf, user_buf, count))
        return -EFAULT;

    kbuf[count] = '\0';

    // Conversion en entier
    if (kstrtoint(kbuf, 10, &new_value) < 0)
    {
        pr_err("Erreur : entrée non valide\n");
        return -EINVAL;
    }

    // Mise à jour de num_passwords
    time = new_value;
    if (update_information(1) == -ENOMEM)
    {
        pr_err("Failed to update information(\n");
        return -ENOMEM;
    }
    pr_info("time mis à jour à : %d\n", time);

    return count; // Retourne le nombre d'octets traités
}

// Fonction pour lire pass_world_input depuis debugfs
static ssize_t is_pass_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    if (update_information(1) == -ENOMEM)
    {
        pr_err("Failed to update information(\n");
        return -ENOMEM;
    }
    return simple_read_from_buffer(user_buf, count, ppos, pass_world_input, strlen(pass_world_input));
}

// function quand le fichier str_parm est modifier
// c'est quand on met un mdp //MARCHE PLUS
static ssize_t is_pass_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    struct password_item *item;

    // Vérification de la taille maximale du paramètre
    if (count >= sizeof(pass_world_input))
        return -EINVAL;

    // Copie des données depuis l'espace utilisateur vers l'espace noyau
    if (copy_from_user(pass_world_input, user_buf, count))
        return -EFAULT;
    pass_world_input[count - 1] = '\0';

    // Parcours de la liste des mots de passe
    list_for_each_entry(item, &password_list, list)
    {
        // Comparer pass_world_input avec le mot de passe de l'élément
        if (strcmp(pass_world_input, item->password) == 0)
        {
            pr_info("%s %d", item->password, item->use);
            if (!item->use) // Si le mot de passe n'est pas déjà utilisé
            {
                item->use = true; // Marquer comme utilisé
                pr_info("Password '%s' has been marked as used.\n", pass_world_input);
                write_to_file("SUPER MON BOEUF T'AS MIS LE BON MDP", FUCHIER_SUCESS);
                save_and_lock_passwords();
                return count;
            }
            else
            {
                pr_warn("Password '%s' is already marked as used.\n", pass_world_input);
                write_to_file("Non t'as deja utiliser ça mon boeuf", FUCHIER_SUCESS);
                return -EEXIST; // Code d'erreur pour signaler un conflit
            }
        }
    }
    write_to_file("Non il existe pas ce mdp mon petit cochon", FUCHIER_SUCESS);

    // Si aucun mot de passe ne correspond
    pr_warn("Password '%s' not found in the list.\n", pass_world_input);
    return -ENOENT; // Code d'erreur pour signaler une absence
}

// Fonction d'initialisation du module

static int init_debug(void)
{
    pr_info("Module initialized with pass_world_input = %s\n", pass_world_input);
    // Création de l'entrée dans debugfs
    debug_dir = debugfs_create_dir("my_debugfs_dir", NULL);
    if (!debug_dir)
    {
        pr_err("Failed to create debugfs directory\n");
        return -ENOMEM;
    }

    for (int i = 0; i < NBR_OF_DEBUG; i += 1)
    {
        debug_file[i] = debugfs_create_file(debug_file_name[i], 0660, debug_dir, NULL, &fops_str[i]);
        if (!debug_file[i])
        {
            pr_err("Failed to create pass_world_input file in debugfs\n");
            return -ENOMEM;
        }
    }

    return 0;
}

static void clean_debug(void)
{
    debugfs_remove_recursive(debug_dir);
    pr_info("debug_dir is clear\n");
}

// ------------------------- DEBUG DIR ---------------------- END

// ------------------------- GÉNERATEUR DE MDP -------------- START

// Fonction pour ajouter un mot de passe à la liste
int add_password_to_list(const char *password)
{
    struct password_item *new_item;

    // Allocation mémoire pour un nouvel élément
    new_item = kmalloc(sizeof(struct password_item), GFP_KERNEL);
    if (!new_item)
    {
        pr_err("echec de l'allocation de memoire pour un nouvel element\n");
        return -ENOMEM;
    }

    // on met le mdp dans la liste
    new_item->password = my_strcpy_kernel(password);
    new_item->use = false;

    if (NULL == new_item->password)
    {
        pr_err("echec de l'allocation de memoire pour un MDP dans un element\n");
        return -ENOMEM;
    }

    // Initialiser et ajouter l'élément à la liste chaînée
    INIT_LIST_HEAD(&new_item->list);
    list_add(&new_item->list, &password_list);

    pr_info("Ajout du mot de passe: %s\n", new_item->password);
    return 0;
}

// Fonction pour générer des mots de passe aléatoires
int generate_passwords(int number)
{
    int j;
    char password[64];
    int random_nbr;
    pr_info("Generation de mots de passe\n");

    // Générer un mot de passe pour chaque mot de base
    for (j = 1; j <= number; j++)
    {
        get_random_bytes(&random_nbr, sizeof(random_nbr));
        if (random_nbr < 0)
            random_nbr = random_nbr * -1; // pour qu'il soit positif parce que fuck les negatif

        // Générer un mot de passe avec un mot de base et un chiffre
        snprintf(password, sizeof(password), "%s%s%s%d", base_words[random_nbr % (base_words_count - 1)], base_words[(random_nbr / 2) % (base_words_count - 1)], base_words[(random_nbr / 3) % (base_words_count - 1)], random_nbr);
        if (-ENOMEM == add_password_to_list(password))
        {
            return -ENOMEM;
        }
    }
    return 0;
}

// Fonction pour afficher les mots de passe de la liste
void print_passwords(void)
{
    struct password_item *item;

    pr_info("Liste des mots de passe generes:\n");

    // Parcours de la liste et affichage de chaque mot de passe
    list_for_each_entry(item, &password_list, list)
    {
        pr_info("%s\n", item->password);
    }
}

/*
static char *get_current_time_str(void)
{
    struct timespec64 ts;
    struct tm tm;
    char *time_str;

    // Obtenir le temps actuel en secondes
    ktime_get_real_ts64(&ts);

    // Convertir en composants humains
    time64_to_tm(ts.tv_sec, 0, &tm);

    // Allouer de la mémoire pour la chaîne
    time_str = kmalloc(20, GFP_KERNEL); // 20 caractères pour "jj/mm/aaaa hh:mm" + '\0'
    if (!time_str)
        return NULL;

    // Formater la date et l'heure
    snprintf(time_str, 20, "%02d/%02d/%04ld %02d:%02d",
             tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
             tm.tm_hour, tm.tm_min);

    return time_str;
}
*/

static char *get_time_with_offset(int time_offset)
{
    struct timespec64 ts;
    struct tm tm;
    char *time_str;

    // Obtenir le temps actuel en secondes
    ktime_get_real_ts64(&ts);

    // Ajouter l'offset en secondes
    ts.tv_sec += time_offset;

    // Convertir en composants humains
    time64_to_tm(ts.tv_sec, 0, &tm);

    // Allouer de la mémoire pour la chaîne
    time_str = kmalloc(20, GFP_KERNEL); // 20 caractères pour "jj/mm/aaaa hh:mm" + '\0'
    if (!time_str)
        return NULL;

    // Formater la date et l'heure avec l'offset
    snprintf(time_str, 20, "%02d/%02d/%04ld %02d:%02d",
             tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
             tm.tm_hour, tm.tm_min);

    return time_str;
}

/*
static void print_time(void)
{
    char *time_str = get_current_time_str();
    if (time_str)
    {
        pr_info("Current time: %s %d\n", time_str, time);
        kfree(time_str); // Libérer la mémoire après utilisation
    }
    else
    {
        pr_err("Failed to allocate memory for time string\n");
    }
}
*/

static void print_time_with_offset(int offset)
{
    char *time_str = get_time_with_offset(offset);
    if (time_str)
    {
        pr_info("Current time + %ds offset: %s\n", time, time_str);
        kfree(time_str);
    }
    else
    {
        pr_err("Failed to allocate memory for time string\n");
    }
}

char *time_mdp_expired = NULL;

int save_and_lock_passwords(void)
{
    struct password_item *item;
    char *all_passwords;
    size_t total_length = 0;
    char expritation_sentence[] = "les mdp ne seront plus valide a: ";
    char *time_mdp_expired = get_time_with_offset(time);

    if (NULL == time_mdp_expired)
    {
        // revoir
        return -ENOMEM;
    }
    total_length += strlen(time_mdp_expired) + 1 + strlen(expritation_sentence); // + 1 pour le \n
    pr_info("Liste des mots de passe non utilisés :\n");

    // Calcul de la taille totale nécessaire pour stocker uniquement les mots de passe non utilisés
    list_for_each_entry(item, &password_list, list)
    {
        total_length += strlen(item->password) + 1 + 2; // +1 pour '\n' et + 2 pour "X " ou "o "
    }

    // Allocation de mémoire pour la chaîne complète
    all_passwords = kmalloc(total_length + 1, GFP_KERNEL); // +1 pour '\0'
    if (!all_passwords)
    {
        pr_err("Erreur d'allocation mémoire pour: all_passwords\n");
        return -ENOMEM;
    }

    all_passwords[0] = '\0'; // Initialiser la chaîne à vide
    strcat(all_passwords, expritation_sentence);
    strcat(all_passwords, time_mdp_expired);
    strcat(all_passwords, "\n");
    // Concaténation des mots de passe non utilisés dans la chaîne
    list_for_each_entry(item, &password_list, list)
    {
        if (!item->use) // N'inclure que les mots de passe non utilisés
        {
            strcat(all_passwords, "o ");
            strcat(all_passwords, item->password);
            strcat(all_passwords, "\n"); // Ajout d'un saut de ligne après chaque mot de passe
        }
        else
        {
            strcat(all_passwords, "x ");
            strcat(all_passwords, item->password);
            strcat(all_passwords, "\n"); // Ajout d'un saut de ligne après chaque mot de passe
        }
    }

    // Écriture dans le fichier avec uniquement les mots de passe non utilisés
    write_to_file(all_passwords, MDP_INFO_FILE);

    pr_info("Les mots de passe non utilisés ont été écrits dans le fichier: %s\n", all_passwords);

    // Libération de la mémoire allouée
    kfree(all_passwords);
    kfree(time_mdp_expired);
    print_time_with_offset(time);
    return 0;
}

// si free est = 1 ça vas free la liste de mdp precedante
int update_information(const int free)
{
    if (free == 1)
    {
        struct password_item *item, *tmp;

        list_for_each_entry_safe(item, tmp, &password_list, list) // macro, et pas le poisson
        {
            pr_info("Suppression du mot de passe: %s\n", item->password);
            list_del(&item->list);
            kfree(item);
        }
    }
    // Générer les mots de passe avec le nombre spécifié
    if (generate_passwords(num_passwords))
    {
        pr_err("Failed to generate passwords\n");
        return -ENOMEM;
    }

    // Afficher les mots de passe générés
    print_passwords();
    if (save_and_lock_passwords())
    {
        pr_err("Failed to save passwords\n");
        return -ENOMEM;
    }

    return 0;
}

static enum hrtimer_restart test_hrtimer_handler(struct hrtimer *timer)
{
    if (time > 0)
    {
        if (update_information(1) == -ENOMEM)
        {
            pr_err("Failed to update information(\n");
            return -ENOMEM;
        }
        hrtimer_forward_now(&test_hrtimer, ms_to_ktime(time * HZ));
        return HRTIMER_RESTART;
    }
    // Arrêter le timer
    pr_info("Timer stopped as time <= 0\n");
    return HRTIMER_NORESTART;
}

static void test_hrtimer_init(void)
{
    hrtimer_init(&test_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    // CLOCK_MONOTONIC permet que l'orloge ne recule jamais méme si l'horloge interne bouge
    // HRTIMER_MODE_REL c'est pour lacer un timer a partie de maintenan
    test_hrtimer.function = &test_hrtimer_handler;
    hrtimer_start(&test_hrtimer, ms_to_ktime(time * HZ), HRTIMER_MODE_REL);
}

// Fonction d'initialisation du module
static int __init password_generator_init(void)
{
    pr_info("Module charge : Generation de mots de passe\n");

    // Allouer un numéro majeur
    /*
        L'allocation d'un numéro majeur permet au noyau de savoir quel pilote doit gérer un device donné.

        Sans ce numéro, votre module ne peut pas enregistrer un device et donc interagir avec l'espace utilisateur.
    */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        pr_err("Failed to register a major number\n");
        return major_number;
    }

    // Créer une classe de device
    /*
        Créer une classe de device dans un module du noyau Linux
        permet de regrouper et d'organiser des devices similaires sous une même catégorie logique.

        Cela facilite leur gestion et leur exposition à l'espace utilisateur via le système de fichiers virtuel /sys et les nœuds dans /dev
    */
    otp_class = class_create(CLASS_NAME);
    if (IS_ERR(otp_class))
    {
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to register device class\n");
        return PTR_ERR(otp_class);
    }

    // Associer la fonction devnode pour définir les permissions
    /*
        pour pouvoir l'utiliser sans utiliser sudo
    */
    otp_class->devnode = password_devnode;

    // Créer le device
    password_device = device_create(otp_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(password_device))
    {
        class_destroy(otp_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to create the device\n");
        return PTR_ERR(password_device);
    }
    // le truc tecnique un peux obligatioir quoi fo pas toucher

    // initialisation des Path des fichier
    for (int i = 0; i < NBR_OF_FILE; i += 1)
    {
        file_info_tab[i].my_file_path = file_path_predef[i];
    }

    for (int i = 0; i < NBR_OF_FILE; i += 1)
    {
        // Ouvrir ou créer le fichier
        file_info_tab[i].file_ptr = filp_open(file_info_tab[i].my_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0660);
        if (IS_ERR(file_info_tab[i].file_ptr))
        {
            pr_err("Failed to open or create file %s\n", file_info_tab[i].my_file_path);
            return PTR_ERR(file_info_tab[i].file_ptr);
        }
    }
    init_debug();
    test_hrtimer_init();

    // les truc plus classique
    if (update_information(0) == -ENOMEM)
    {
        pr_err("Failed to update information(\n");
        return -ENOMEM;
    }
    pr_info("Module decharge \n");
    write_to_file("RIEN", FUCHIER_SUCESS);
    return 0;
}

static void cleanup_password_list(void)
{
    struct password_item *item, *tmp;

    // FREE LES MDP
    list_for_each_entry_safe(item, tmp, &password_list, list) // macro, et pas le poisson
    {
        pr_info("Suppression du mot de passe: %s\n", item->password);
        list_del(&item->list);
        kfree(item);
    }
}

static void cleanup_file_resources(void)
{
    // free les file
    for (int i = 0; i < NBR_OF_FILE; i += 1)
    {
        if (file_info_tab[i].file_ptr && !IS_ERR(file_info_tab[i].file_ptr))
        {
            filp_close(file_info_tab[i].file_ptr, NULL);
        }
    }
}

static void cleanup_timer(void)
{
    hrtimer_cancel(&test_hrtimer); // Annuler le timer haute résolution
}

// Fonction de nettoyage du module
static void __exit password_generator_exit(void)
{

    pr_info("Nettoyage du module : Suppression des mots de passe\n");
    cleanup_password_list();
    cleanup_file_resources();
    cleanup_timer();
    pr_info("Module decharge \n");
    clean_debug();

    // Détruire le device et la classe
    device_destroy(otp_class, MKDEV(major_number, 0));
    class_destroy(otp_class);

    // Désenregistrer le numéro majeur
    unregister_chrdev(major_number, DEVICE_NAME);
}

// ------------------------- GÉNERATEUR DE MDP -------------- END

module_init(password_generator_init);
module_exit(password_generator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Les gougniafier");
MODULE_DESCRIPTION("gestionaire de MDP one time by");
MODULE_VERSION("2.0");
