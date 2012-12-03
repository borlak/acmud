/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
structs.h:
Purpose of this file:  All structure templates within the code
belong in this file.
*/


struct area_t
{
	unsigned int	type;		// for editing.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	AREA		*next;
	AREA		*prev;

	char		*builders;
	char		*name;

	long		high;		// highest vnum in area
	long		low;		// ...lowest <-- these are the identifiers
	long		players;	// how many players are in the area (is area alive)
};


struct ban_t
{
	unsigned int	type;		// for editing.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	BAN		*next;
	BAN		*prev;

	char		*ip;
	char		*message;
	char		*name;

	long		bantype;
};


struct coin_t
{
	long	olcvalue;
	char	*name;
	char	*ansi_name;
	long	worth;
};


struct creature_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	CREATURE	*next;
	CREATURE	*prev;
	CREATURE	*next_content;
	CREATURE	*prev_content;
	CREATURE	*next_hash;
	CREATURE	*prev_hash;
	CREATURE	*mount;
	CREATURE	*rider;
	MSOCKET		*socket;
	OBJECT		*inventory;
	OBJECT		*equipment;
	EXTRA		*extras;
	RESET		*reset;
	ROOM		*in_room;
	SHOP		*shop;   	// those silly shopkeepers

	char		*description;
	char		*keywords;
	char		*loaded;
	char		*long_descr;
	char		*name;
	char		*wear;

	long		*flags;

	long		alignment;
	long		dexterity;
	long		hp;		// hitpoints
	long		max_hp;
	long		intelligence;
	long		level;
	long		move;		// movement
	long		max_move;
	long		position;
	long		prototype;	// is this crit a prototype?
	long		sex;
	long		state;
	long		strength;
	long		vnum;
};


struct direction_t
{
	char 	*abbr;
	char	*name;
	char	*reverse;
};


struct exit_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	EXIT		*next;
	EXIT		*prev;

	char		*name;		// specific exit names

	long		door;		// is there a door? is it locked?
	long		key;		// key to door
	long		to_vnum;
};


struct extra_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	EXTRA		*next;
	EXTRA		*prev;

	char	*description;
	char	*keywords;

	unsigned int	loadtype;
	long		vnum;
};


struct flagtable_type
{
	long flag;
	char *name;
};


struct help_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	HELP 		*next;
	HELP		*prev;

	char		*entry;
	char		*index;
	char		*keyword;
	char		*last;

	long		level;
};


struct mudtime_t
{
	long		id;		// mysql ID

	long		age;		// this is the startup date of the mud
	long		backup;		// time of last backup
	long		last;		// was bootup after a crash, reboot, shutdown?
	long		mudstatus;	// is current status crash, reboot, shutdown?
	long		start;		// mysql bootup real time
	long		end;		// time of last crash/reboot/shutdown

	long		year;
	long		month;
	long		months;		// how many months.. found at bootup
	long		day;
	long		days;		// how many days.. found at bootup
	long		hour;
	long		minute;
	long		second;
};


struct note_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE

	NOTE		*next;
	NOTE		*prev;
	NOTE 		*next_hash;
	NOTE		*prev_hash;

	char		*sender;
	char		*sent_to;
	char		*subject;
	char		*text;
	char		*written;
};


struct object_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	CREATURE	*held_by;
	OBJECT		*next;
	OBJECT		*prev;
	OBJECT		*next_content;
	OBJECT		*prev_content;
	OBJECT		*next_hash;
	OBJECT		*prev_hash;
	OBJECT		*contents;
	OBJECT		*in_obj;
	EXTRA		*extras;
	RESET		*reset;
	ROOM		*in_room;

	char		*description;
	char		*keywords;
	char		*loaded;	// did an imm load this obj? and who
	char		*long_descr;
	char		*name;
	char		*wear;
	char		*worn;

	long		*flags;

	long		nested;		// how deep a container is nested (for sql loading purposes)
	long		objtype;
	long		owner_id;       // so we can call fwrite_object without a lot of haballoh
	long		prototype;	// is obj a prototype?
	long		timer;		// blow up after x seconds
	long		values[MAX_OBJVALUES];
	long		vnum;
	long		worth;		// value, cost...

	float		weight;
};


struct reset_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	RESET		*next;		// room list
	RESET		*prev;
	RESET		*next_hash;	// global hash (timed) list
	RESET		*prev_hash;
	CREATURE	*crit;
	OBJECT		*obj;
	ROOM		*room;

	char		*command;

	long		chance;
	long		inside;
	long		loaded;
	unsigned int	loadtype;
	long		nested;
	long		min;
	long		max;
	long		poptime;
	long		position;
	long		roomvnum;
	long		state;
	long		time;
	long		vnum;
};


struct reset_type
{
	long olcvalue;
	char *name;
};


struct room_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	CREATURE	*crits;
	OBJECT		*objects;
	EXTRA		*extras;
	RESET		*resets;
	AREA		*area;
	ROOM		*next_hash;
	ROOM		*prev_hash;
	ROOM		*next;
	ROOM		*prev;
	EXIT		*exits;

	char		*loaded;
	char		*name;
	char		*night_name;
	char		*description;
	char		*night_description;

	long		*flags;

	long		light;
	long		roomtype;
	long		vnum;
};


struct shop_t
{
	unsigned int	type;		// for scripts.. DO NOT MOVE
	unsigned long	id;		// mysql ID

	SHOP		*next_hash;
	SHOP		*prev_hash;
	SHOP		*next;
	SHOP		*prev;

	long		keeper;   	// store owner
	long		item;		// item type
	long		stype;		// shop type .. 0 = shop, 1 = bank, 2 = storage, 3 = stable, 4 = bank/storage
	long		buy;		// profit
	long		sell;		// profit
	long		open;		// opening time
	long		close;		// closing time
};


struct social_t
{
	unsigned int	type;		// DO NOT CHANGE
	unsigned long	id;		// mysql ID

	SOCIAL	*next_hash;
	SOCIAL	*prev_hash;
	SOCIAL	*next;
	SOCIAL	*prev;

	char	*name;
	char	*no_target;
	char	*crit_target;
	char	*object_target;
	char	*self_target;
};


struct socket_t
{
//	struct sockaddr_in6	*sockad;
	struct sockaddr_in	*sockad;
	unsigned long		id;

	MSOCKET		*next;
	MSOCKET		*prev;
	CREATURE	*pc;

	char		inbuf[MAX_BUFFER];
	char		last_command[MAX_BUFFER];
	char		outbuf[MAX_BUFFER];

	// editing
	void		*editing;
	char		*string;
	char		**variable;
	long		modified;	// changed something

	unsigned long	desc;
	long		command_ready;
	long		connected;
	long		doprompt;
	long		hrs;		// hrs from gmt
	long		lines;
	long		pause;		// line pausing and socket hang checking
	long		repeat;		// spam checking
	long		save_time;	// only type save once every 30 seconds

	char		*host;
	char		*ip;
	char		*last_ip;
	char		*password;
	char		*prompt;
	char		*reply;
	char		*snoop;		// string allows multiple snoopers
	char		*stringbuf;
	char		*title;
	char		*who_name;
};


struct worn_t
{
	char		*keyword;
	char		*name;
	char		*worn;
};

// declarations of any tables
extern const char *ban_types[];
extern const struct reset_type reset_types[];
extern const char *objtype_table[];
extern const char *sex_table[];
extern const char *roomtype_table[];


