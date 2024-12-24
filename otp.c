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
// Fonction pour afficher et verrouiller les mots de passe non utilisés

// GLOBAL VARIABLE START:

void update_information(const int free);

// ------------------------------------------------------------------------ DEBUG DIR
static char pass_world_input[100] = "";
#define NBR_OF_DEBUG 3
static const char *debug_file_name[NBR_OF_DEBUG] = {
    "mdp_input", "nbr_pass", "time"};
static struct dentry *debug_dir, *debug_file[NBR_OF_DEBUG];

// Définir les opérations de fichier pour pass_world_input
static ssize_t is_pass_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos);
static ssize_t is_pass_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos);

static ssize_t plopW(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    pr_err("PLOP: W");
    return 0;
}

static ssize_t plopR(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    pr_err("PLOP: W");
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
                                                                  .write = plopW,
                                                              },
                                                              {
                                                                  .owner = THIS_MODULE,
                                                                  .read = plopR,
                                                                  .write = plopW,
                                                              }};

// ------------------------------------------------------------------------ My_LIB
struct file_info
{
    struct file *file_ptr;
    const char *my_file_path;
};

// LE NOMBRE DE FICHIER D'EN LE MODULE A BESOIN
#define NBR_OF_FILE 3
#define MDP_INFO_FILE 0

// TOUT LES PATH DES FICHIER D'EN LE MODULE A BESOIN
static const char *file_path_predef[NBR_OF_FILE] = {
    "/dev/otp0", "/dev/otp1", "/dev/otp2"};

// UNE STRUCTURE POUR AVOIR PLUS FACILEMENT LESINFORMATION DES FICHIER
static struct file_info file_info_tab[NBR_OF_FILE];

// ------------------------------------------------------------------------ GÉNERATEUR DE MDP
// déclaration des function
void add_password_to_list(const char *password);
void generate_passwords(int number);
void print_passwords(void);
void save_and_lock_passwords(void);

// Structure pour chaque mot de passe
struct password_item
{
    char *password;        // Mot de passe généré
    struct list_head list; // Liste chaînée (pour lier les éléments)
    bool use;
};

// Déclaration de la tête de la liste chaînée
static LIST_HEAD(password_list);

// Liste des mots de base
static const char *base_words[] = {
    "Chavel", "Maison", "Jambon", "Tracteur", "Chat", "Chien", "Autruche", "Hipopotame", "Perigord", "Canard", "Fraise"};
static const size_t base_words_count = sizeof(base_words) / sizeof(base_words[0]);

// Paramètre de module pour spécifier le nombre de mots de passe à générer
static int num_passwords = 5; // Valeur par défaut : 5 mots de passe
module_param(num_passwords, int, 0444);
MODULE_PARM_DESC(num_passwords, "Nombre de mots de passe à générer");

// GLOBAL VARIABLE END:

// ------------------------------------------------------------------------ le time

// Paramètre de module pour spécifier le nombre de mots de passe à générer
static int time = 5; // Valeur par défaut : 5 pour 5 segonde
module_param(time, int, 0444);
MODULE_PARM_DESC(time, "le temps d'que les mdp se refreche");

static struct hrtimer test_hrtimer;
static u64 sampling_period_ms = (0 * HZ);

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

// pour écrire dans un fichier
static int write_to_file(const char *data, int nbrf)
{
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

// Fonction pour lire pass_world_input depuis debugfs
static ssize_t is_pass_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    update_information(1);
    return simple_read_from_buffer(user_buf, count, ppos, pass_world_input, strlen(pass_world_input));
}

// function quand le fichier str_parm est modifier
static ssize_t is_pass_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    struct password_item *item;

    // Vérification de la taille maximale du paramètre
    if (count >= sizeof(pass_world_input))
        return -EINVAL;

    // Copie des données depuis l'espace utilisateur vers l'espace noyau
    if (copy_from_user(pass_world_input, user_buf, count))
        return -EFAULT;

    // Ajout du caractère null pour terminer la chaîne
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
                save_and_lock_passwords();
                return count;
            }
            else
            {
                pr_warn("Password '%s' is already marked as used.\n", pass_world_input);
                return -EEXIST; // Code d'erreur pour signaler un conflit
            }
        }
    }

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
void add_password_to_list(const char *password)
{
    struct password_item *new_item;

    // Allocation mémoire pour un nouvel élément
    new_item = kmalloc(sizeof(struct password_item), GFP_KERNEL);
    if (!new_item)
    {
        pr_err("echec de l'allocation de memoire pour un nouvel element\n");
        return;
    }

    // on met le mdp dans la liste
    new_item->password = my_strcpy_kernel(password);
    new_item->use = false;

    // Initialiser et ajouter l'élément à la liste chaînée
    INIT_LIST_HEAD(&new_item->list);
    list_add(&new_item->list, &password_list);

    pr_info("Ajout du mot de passe: %s\n", new_item->password);
}

// Fonction pour générer des mots de passe aléatoires
void generate_passwords(int number)
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
        add_password_to_list(password);
    }
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

void save_and_lock_passwords(void)
{
    struct password_item *item;
    char *all_passwords;
    size_t total_length = 0;
    char expritation_sentence[] = "les mdp ne seront plus valide a: ";
    char *time_mdp_expired = get_time_with_offset(time);

    if (NULL == time_mdp_expired)
    {
        return;
    }
    total_length += strlen(time_mdp_expired) + 1 + strlen(expritation_sentence); // + 1 pour le \n
    pr_info("Liste des mots de passe non utilisés :\n");

    // Calcul de la taille totale nécessaire pour stocker uniquement les mots de passe non utilisés
    list_for_each_entry(item, &password_list, list)
    {
        if (!item->use) // Ne compter que les mots de passe non utilisés
        {
            total_length += strlen(item->password) + 1; // +1 pour '\n'
        }
    }

    // Allocation de mémoire pour la chaîne complète
    all_passwords = kmalloc(total_length + 1, GFP_KERNEL); // +1 pour '\0'
    if (!all_passwords)
    {
        pr_err("Erreur d'allocation mémoire pour: all_passwords\n");
        return;
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
            strcat(all_passwords, item->password);
            strcat(all_passwords, "\n"); // Ajout d'un saut de ligne après chaque mot de passe
        }
    }

    // Écriture dans le fichier avec uniquement les mots de passe non utilisés
    write_to_file(all_passwords, MDP_INFO_FILE);

    pr_info("Les mots de passe non utilisés ont été écrits dans le fichier: %s %zu\n", all_passwords, strlen(all_passwords));

    // Libération de la mémoire allouée
    kfree(all_passwords);
    kfree(time_mdp_expired);
    print_time_with_offset(time);
}

// si free est = 1 ça vas free la liste de mdp precedante
void update_information(const int free)
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
    generate_passwords(num_passwords);

    // Afficher les mots de passe générés
    print_passwords();
    save_and_lock_passwords();
    // FREE LES MDP
}

static enum hrtimer_restart test_hrtimer_handler(struct hrtimer *timer)
{
    if (time > 0)
    {
        pr_info("PROCK\n");
        update_information(1);
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
    update_information(0);
    pr_info("Module decharge \n");
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
}

// ------------------------- GÉNERATEUR DE MDP -------------- END

module_init(password_generator_init);
module_exit(password_generator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Les gougniafier");
MODULE_DESCRIPTION("gestionaire de MDP one time by");
