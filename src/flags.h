/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
flags.h -- all the flags for players, rooms, objects, etc.
make sure you keep these in order.  there is a delete command ingame
you will need to use.
*/

enum critflags
{
	CFLAG_PLAYER,
	CFLAG_ANSI,
	CFLAG_QUIET,
	CFLAG_MOUNT,
	CFLAG_NOTIFY,
	CFLAG_WIZINVIS,
	CFLAG_BLANK,
	CFLAG_LOG,
	CFLAG_NOMENU,
	CFLAG_ANTIIDLE,
	CFLAG_BANKER,
	CFLAG_CHANIMM,
	CFLAG_CHANMUSIC,
	CFLAG_CHANCHAT,
	CFLAG_CHANBUILD,
	CFLAG_BRIEF,
	CFLAG_LAST
};


enum objflags
{
	OFLAG_NOTAKE,
	OFLAG_GLOW,
	OFLAG_HUM,
	OFLAG_STORED,
	OFLAG_LAST
};


enum roomflags
{
	RFLAG_HEAL,
	RFLAG_LAST
};

