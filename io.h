/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
io.h:
Purpose of this file:  i/o related variables and such.
*/

// MySQL Area Table Variables
enum {
	A_NAME = 1,
	A_BUILDERS,
	A_UPDATED,
	A_LOW,
	A_HIGH
};

// MySQL Room Table Variables
enum {
	R_NAME = 1,
	R_NIGHT_NAME,
	R_LOADED,
	R_ROOMTYPE,
	R_LIGHT,
	R_VNUM,
	R_DESC,
	R_NIGHT_DESC,
	R_FLAGS
};

// MySQL Social Table Variables
enum {
	S_ID,
	S_NAME,
	S_NOTARGET,
	S_CRITTARGET,
	S_OBJECTTARGET,
	S_SELFTARGET
};

// MySQL Shop Table Variables
enum {
	SH_KEEPER = 1,
	SH_TYPE,
	SH_BUY,
	SH_SELL,
	SH_OPEN,
	SH_CLOSE,
	SH_ITEM
};

// MySQL Reset Table Variables
enum {
	X_ID,
	X_CHANCE,
	X_INSIDE,
	X_NESTED,	// order in which resets appear in a room (for loading)
	X_MIN,		// min # of objs per reset
	X_MAX,		// max # of objs at a time
	X_POSITION,	// mob(standing,sitting?) obj(wear or inventory)
	X_STATE,	// mob(dead,meditating,normal)
	X_ROOMVNUM,
	X_TIME,
	X_LOADTYPE,	// obj mob exit
	X_VNUM,
	X_COMMAND
};


// MySQL note table variables.
enum {
	N_NOTENUMBER = 1,
	N_SENDER,
	N_SUBJECT,
	N_WRITTEN,
	N_SENT_TO,
	N_NOTE
};

// MySQL help table variables
enum {
	H_KEYWORD = 1,
	H_ENTRY,
	H_LAST,
	H_INDEX,
	H_LEVEL
};

// MySQL Obj Table Variables
enum {
	O_NAME = 1,
	O_KEYWORDS,
	O_VNUM,
	O_PROTOTYPE,
	O_OWNER_ID,	// few variables for player objects
	O_IN_OBJ,
	O_NESTED,
	O_WORN,		// end player variables
	O_LOADED,
	O_LDESC,
	O_DESC,
	O_WEAR,
	O_OBJTYPE,
	O_OBJVALUES,
	O_TIMER,
	O_WEIGHT,
	O_WORTH,
	O_FLAGS
};

// MySQL Crit Table Variables
enum {
	C_NAME = 1,
	C_KEYWORDS,
	C_VNUM,
	C_PROTOTYPE,
	C_ROOM,
	C_LDESC,
	C_DESC,
	C_WEAR,
	C_LOADED,
	C_LEVEL,
	C_DEX,
	C_INT,
	C_STR,
	C_ALIGNMENT,
	C_SEX,
	C_STATE,
	C_POSITION,
	C_MOVE,
	C_MAXMOVE,
	C_HP,
	C_MAXHP,
	C_FLAGS
};

// MySQL Player Table Variables
enum {
	P_NAME = 1,
	P_HOST,
	P_IP,
	P_LASTCOMMAND,
	P_LASTONLINE,
	P_LINES,
	P_PASSWORD,
	P_PROMPT,
	P_TITLE,
	P_WHONAME,
	P_ONLINE,
	P_DESCRIPTOR,
	P_HRS
};

// MySQL Exit Table Variables
enum {
	E_NAME = 1,
	E_ROOM,
	E_DOOR,
	E_KEY,
	E_TO_VNUM
};

// MySQL Object/Room "Extra Descriptions" Variables
enum {
	ED_KEYWORDS = 1,
	ED_DESCRIPTION,
	ED_TYPE,
	ED_VNUM
};

// MySQL Time Table Variables
enum {
	T_AGE = 1,
	T_MUDSTATUS,	// was last time a crash/reboot ?
	T_BACKUP,	// last time of backup (using seconds/current_time)
	T_START,	// real time start date
	T_END,		// end time, or close to it (within update period)
	T_YEAR,
	T_MONTH,
	T_DAY,
	T_HOUR,
	T_MINUTE,
	T_SECOND
};

// MySQL Ban Table Variables
enum {
	B_IP = 1,
	B_MESSAGE,
	B_NAME,
	B_BANTYPE
};

// MySQL player_input Table Variables
enum {
	I_PLAYERID = 1,
	I_NAME,
	I_TYPE,
	I_ROOM,
	I_INPUT
};

