/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
variables.h:
Purpose of this file:  Global variables that are used often.
*/

#include "config.h"
#include "flags.h"

// important variables
#define MAX_BUFFER		16384	// 4096
#define LOOPS_PER_SECOND	10	// how many times the mud loops...
#define HASH_KEY		512	// for the hash tables

// miscellaneous
#define MAX_OBJVALUES		2

// logging
#define LOG_NO		0
#define LOG_YES		1
#define LOG_NEVER	2

// socket stuff
enum {
	CON_LINKDEAD,
	CON_CONNECTING,
	CON_GET_NAME,
	CON_CONFIRM_NAME,
	CON_GET_PASSWORD,
	CON_CONFIRM_PASSWORD,
	CON_CHECK_PASSWORD,
	CON_MENU,
	CON_PLAYING
};
#define BAN_IP			1
#define BAN_PLAYER		2

// player variables
enum { SEX_NEUTRAL, SEX_MALE, SEX_FEMALE };
// states and positions
enum {
	STATE_ALERT,
	STATE_NORMAL,
	STATE_FIGHTING,
	STATE_RESTING,
	STATE_MEDITATING,
	STATE_SLEEPING,
	STATE_UNCONSCIOUS,
	STATE_DYING,
	STATE_DEAD
};
enum {
	POS_STANDING,
	POS_SITTING,
	POS_PRONE
};


// externs
extern MYSQL *mysql;
extern AREA *area_list;
extern BAN *ban_list;
extern CREATURE *creature_list;
extern MSOCKET *socket_list;
extern NOTE *note_list;
extern OBJECT *object_list;
extern time_t current_time;
extern unsigned long tick;
extern RESET *hash_reset[HASH_KEY];
extern SOCIAL *hash_social[HASH_KEY];
extern NOTE *hash_note[HASH_KEY];
extern struct mudtime_t mudtime;
extern char *position_table[];
extern char *state_table[];

// commands and lookup
#define OBJ_REQUIRED	0x00000001
#define OBJ_POSSIBLE	0x00000002
#define CRIT_REQUIRED	0x00000004
#define CRIT_POSSIBLE	0x00000008
#define OBJ_HELD	0x00000010
#define OBJ_GROUND	0x00000020
#define OBJ_WORLD	0x00000040
#define OBJ_INOBJ	0x00000080
#define OBJ_EQUIPMENT	0x00000100
#define CRIT_WORLD	0x00000200
#define PLAYER_ONLY	0x00000400
#define OBJ_CRIT_BOTH	0x00000800
#define ONE_REQUIRED	0x00001000

// structures types, used for data verification...
#define TYPE_ROOM	0x00000001
#define TYPE_OBJECT	0x00000002
#define TYPE_CREATURE	0x00000004
#define TYPE_NOTE	0x00000008
#define TYPE_HELP	0x00000010
#define TYPE_SOCIAL	0x00000020
#define TYPE_EXIT	0x00000040
#define TYPE_AREA	0x00000080
#define TYPE_SHOP	0x00000100
#define TYPE_BAN	0x00000200
#define TYPE_RESET	0x00000400
#define TYPE_EXTRA	0x00000800

// Levels...
#define LEVEL_IMP	3
#define LEVEL_BUILDER	(LEVEL_IMP-1)
#define LEVEL_IMM	(LEVEL_IMP-2)

// Channel variables
enum {
	CHAN_CHAT,
	CHAN_IMM,
	CHAN_BUILDER,
	CHAN_MUSIC
};

// class types
enum {
	CLASS_UNKNOWN,
	CLASS_BARBARIAN,
	CLASS_BARD,
	CLASS_CLERIC,
	CLASS_DRUID,
	CLASS_FIGHTER,
	CLASS_MONK,
	CLASS_PALADIN,
	CLASS_RANGER,
	CLASS_ROGUE,
	CLASS_SORCERER,
	CLASS_WIZARD
};

// races
enum {
	RACE_UNKNOWN,
	RACE_DWARF,
	RACE_ELF,
	RACE_GNOME,
	RACE_HUMAN,
	RACE_HALFORC,
	RACE_HALFLING,
	RACE_HALFELF
};

// object types
enum {
	OBJ_TREASURE, // default -- anything is a treasure
	OBJ_FOOD,
	OBJ_DRINK,
	OBJ_CONTAINER,
	OBJ_COIN,
	OBJ_ARMOR,
	OBJ_WEAPON,
	OBJ_LIGHT,
	OBJ_KEY
};

//shop types
enum {
	SHOP_STORE,
	SHOP_BANK, // and obj storage
	SHOP_STABLE
};

// room types
enum {
	ROOM_AIR,
	ROOM_ARCTIC,
	ROOM_CITY,
	ROOM_DESERT,
	ROOM_DIRT,
	ROOM_FOREST,
	ROOM_GRASSY,
	ROOM_HILLS,
	ROOM_INSIDE,
	ROOM_MOUNTAIN,
	ROOM_OCEAN,
	ROOM_ROAD,
	ROOM_UNDERWATER,
	ROOM_WATER
};

// door vars
enum
{
	DOOR_NONE,
	DOOR_OPEN,
	DOOR_CLOSED,
	DOOR_LOCKED
};

// VNUMS
#define VNUM_COIN	3
#define VNUM_DEATH_ROOM	1
#define VNUM_CORPSE	5
#define VNUM_HAIRBALL	2
#define VNUM_SWEAT	6

