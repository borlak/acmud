/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
stdh.h:
Purpose of this file:  Standard Header File.  This includes
all the other header files, and system header files that are
used often.  It also contains global functions for easy finding.
*/

#include "/usr/include/mysql/mysql.h"
#include "os.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "typedefs.h"
#include "variables.h"
#include "structs.h"

// area.c
EXTRA *find_extra	(void *obj, char *name);
EXIT *find_exit		(ROOM *room, char *name);
char *smash_quotes	(char *str);
extern const struct direction_t directions[];

// cmdio.c
HELP *mysql_find_help	(CREATURE *crit, char **arguments);
char *display_timestamp	(time_t time, long gmt);
void update_resets	(void);

// comm.c
void sendcrit		(CREATURE *crit, char *buf);
void sendcritf		(CREATURE *crit, char *buf, ...);

// command.c
void interpret		(CREATURE *crit, char *buf);
char *all_args		(char **arguments, long start);
char *split_args	(char **arguments, long start, long end);
long last_arg		(char **arguments);

// creature.c
CREATURE *find_crit	(CREATURE *crit, char *arguments, long flags);
void die		(CREATURE *crit);
void heal		(CREATURE *crit, long amount);
void hurt		(CREATURE *crit, long amount);
void position_check	(CREATURE *crit);

// editor.c
AREA *find_area		(long vnum);
void delete_object	(CREATURE *crit, void *obj, long all);
void save_editing	(CREATURE *crit);
void stop_editing	(CREATURE *crit, long show_message);
long check_builder	(CREATURE *crit, void *obj);
long display_resets	(CREATURE *crit);
long editor		(CREATURE *crit, char **arguments);
long object_editor	(CREATURE *crit, char **arguments, OBJECT *obj);
long string_editor	(CREATURE *crit, char *buf);

// flags.c
void load_flags		(void *what);
void flag_remove	(long *flags, long bitnum);
void flag_reverse	(long *flags, long bitnum);
void flag_set		(long *flags, long bitnum);
void read_flags		(char *buf, void *obj);
char *strflags		(void *obj);
char *write_flags	(void *obj);
long flag_isset		(long *flags, long bitnum);
long critflags_name	(char *str);
long objflags_name	(char *str);
long roomflags_name	(char *str);

// io.c
void fwrite_bans	(void);
void fwrite_creature	(long vnum);
void fwrite_extra	(EXTRA *extra);
void fwrite_help	(HELP *help);
void fwrite_note	(NOTE *note);
void fwrite_player	(CREATURE *crit);
void fwrite_object	(OBJECT *obj);
void fwrite_room	(long vnum);
void fwrite_resets	(ROOM *room);
void fwrite_shop	(SHOP *shop);
void fwrite_social	(SOCIAL *social);
void fwrite_time	(void);
void load_db		(void);
void read_values	(char *valuebuf, long values[]);
char *add_int		(char *buf, char *arg, long number);
char *add_str		(char *buf, char *arg, char *string);
char *fread_line	(FILE *fp);
char *fread_word	(FILE *fp);
long fread_area		(MYSQL_ROW row);
long fread_bans		(void);
long fread_number	(FILE *fp);
long fread_player	(MSOCKET *sock, char *name);
long fread_time		(void);
long fwrite_area	(AREA *area, long all);

// info.c
void create_message	(char *buf, void *actor, void *stage, void *extra, bool world);
char *coin_string	(long worth);
char *exit_names	(ROOM *room,bool prompt);
char *get_mudtime	();
extern const struct worn_t worn_table[];

// main.c
void init_mud		(long hotreboot);
void mud_exit		(void);

// newdel.c
CREATURE *hashfind_creature	(long vnum);
CREATURE *new_creature_proto	(long vnum);
CREATURE *new_creature		(long vnum);
OBJECT *hashfind_object		(long vnum);
OBJECT *new_object_proto	(long vnum);
OBJECT *new_object	(long vnum);
SOCIAL *hashfind_social	(char *name);
SOCIAL *new_social	(char *name);
EXTRA *new_extra	(void *obj);
RESET *new_reset	(void);
RESET *hashfind_reset	(long vnum);
AREA *new_area		(char *filename);
EXIT *new_exit		(ROOM *room);
NOTE *new_note		(void);
HELP *new_help		(void);
NOTE *hashfind_note	(long num);
ROOM *hashfind_room	(long vnum);
ROOM *new_room		(long vnum);
SHOP *hashfind_shop	(long vnum);
SHOP *new_shop		(long vnum);
BAN *new_ban		(CREATURE *crit);
void add_reset		(RESET *reset);
void free_area		(AREA *area);
void free_ban		(BAN *ban);
void free_creature	(CREATURE *crit);
void free_exit		(ROOM *room, EXIT *exit);
void free_extra		(void *obj, EXTRA *extra);
void free_help		(HELP *help);
void free_mud		(void);
void free_object	(OBJECT *obj);
void free_reset		(RESET *reset);
void free_room		(ROOM *room);
void free_shop		(SHOP *shop);
void free_social	(SOCIAL *social);

// object.c
OBJECT *find_obj	(CREATURE *crit, char *arguments, long flags);
OBJECT *splitcoins	(OBJECT *coins, void *to, long amount);
void coinify		(OBJECT *coin);
void make_worth		(CREATURE *crit, long worth);
void mysql_update_object (OBJECT *obj);
void remove_obj		(CREATURE *crit, OBJECT *obj);
void trans		(void *obj, void *to);
void wear_obj		(CREATURE *crit, OBJECT *obj, char *worn);
long get_worth		(CREATURE *crit);
long find_worn		(char *worn);
extern const struct coin_t coin_table[];

// os.c
void get_time		(struct timeval *time);
int strcasecmp		(const char *s1, const char *s2);
char *crypt		(const char *key, const char *salt);

// socket.c
MSOCKET *init_socket	(void);
void free_socket	(MSOCKET *sock);
void get_hostname	(MSOCKET *sock);
void nanny		(MSOCKET *sock, char *argument);
void prompt		(CREATURE *crit, bool snoop);
void reset_socket	(MSOCKET *sock);
void send_immediately	(MSOCKET *sock, char *message);
void send_to		(MSOCKET *sock, char *message);
long check_connections	(void);
long create_host	(long port);
long process_input	(MSOCKET *sock);
long process_output	(MSOCKET *sock, bool input, bool snoop);

// update.c
void mud_update		(void);

// util.c
char *capitalize	(char *str);
char *find_path		(void *fromobj, void *toobj, long depth);
char **make_real_arguments (char *buf, char deli);
char *wraptext		(long spaces, char *str, long cutoff);
char *str_add		(char *str, char *addition, char delimeter);
char *str_cut		(long start, long spots, char *str);
char *str_minus		(char *str, char *subtraction, char delimeter);
char *strlistcmp	(char *str1, char *str2);
char *noansi		(char *buf);
void backup_mud		(void);
void free_arguments	(char **arguments);
void init_rand		(void);
void mudlog		(const char *str, ...);
void mudsleep		(long sleep);
void str_dup		(char **destination, char *str);
long countlist		(char *haystack, char *needle);
long findinset		(char *haystack, char *needle, char delim);
long indexinset		(char *haystack, char *needle, char delim);
long is_number		(char *str);
long numbered_arg	(char *argument);
long strindex		(char *haystack, char *needle);
long strargcmp		(char *haystack, char *needle);
long strargindex	(char *haystack, char *needle);
long strstrl		(char *haystack, char *needle);
long randnum		(long start, long end);
long randneg		(long start, long end);
long dice		(long howmany, long type);
long percent		(void);

#include "macros.h"
#include "command.h"

