/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
flags.c -- infnite flag/bits
*/

#include "stdh.h"

///////////////
// FUNCTIONS //
///////////////
const unsigned long powtable[32] =
{
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128,
	256,
	512,
	1024,
	2048,
	4096,
	8192,
	16384,
	32768,
	65536,
	131072,
	262144,
	524288,
	1048576,
	2097152,
	4194304,
	8388608,
	16777216,
	33554432,
	67108864,
	134217728,
	268435456,
	536870912,
	1073741824,
	2147483648U
};


void load_flags(void *what)
{
	long num=0;
	long i;
	long *newflags;
	long *copyflags;

	switch(*(unsigned int*)what)
	{
	case TYPE_CREATURE:
		num		= CFLAG_LAST;
		newflags	= ((CREATURE*)what)->flags = malloc(sizeof(long)*((num/32)+1));
		copyflags	= hashfind_creature(((CREATURE*)what)->vnum)->flags;
		break;
	case TYPE_OBJECT:
		num		= OFLAG_LAST;
		newflags	= ((OBJECT*)what)->flags = malloc(sizeof(long)*((num/32)+1));
		copyflags	= hashfind_object(((OBJECT*)what)->vnum)->flags;
		break;
	default:
		mudlog("LOAD_FLAGS: no such type!");
		return;
	}

	for(i = 0; i < (num/32)+1; i++)
		newflags[i] = copyflags[i];
}


char *write_flags(void *obj)
{
	static char buf[MAX_BUFFER];
	long i=0;
	long last=0; // the queen!
	long *flags=0;

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		flags	= ((CREATURE*)obj)->flags;
		last	= CFLAG_LAST;
		break;
	case TYPE_OBJECT:
		flags	= ((OBJECT*)obj)->flags;
		last	= OFLAG_LAST;
		break;
	case TYPE_ROOM:
		flags	= ((ROOM*)obj)->flags;
		last	= RFLAG_LAST;
		break;
	default:
		mudlog("write_flags: bad obj type (%li)!", (*(unsigned int*)obj));
		break;
	}

	buf[0]	= '\0';

	for(i = 0; i < (last/32)+1; i++)
	{
		sprintf(buf+strlen(buf),"%li",flags[i]);
		if(i+1 < (last/32)+1)
			strcat(buf,"|");
	}

	return buf;
}


void read_flags(char *flagbuf, void *obj)
{
	long i;
	char *flagp=0;
	long *flags=0;
	long last=0; // the king!

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		flags	= ((CREATURE*)obj)->flags;
		last	= CFLAG_LAST;
		break;
	case TYPE_OBJECT:
		flags	= ((OBJECT*)obj)->flags;
		last	= OFLAG_LAST;
		break;
	case TYPE_ROOM:
		flags	= ((ROOM*)obj)->flags;
		last	= RFLAG_LAST;
		break;
	default:
		mudlog("read_flags: bad obj type (%li)!", (*(unsigned int*)obj));
		break;
	}

	flags[0] = atoi(strtok(flagbuf,"|"));

	for(i = 1; i < (last/32)+1; i++)
	{
		flags[i] = 0;
		if(!(flagp=strtok(0,"|")))
			continue;
		flags[i] = atoi(flagp);
	}
}


inline long flag_isset(long *flags, long bitnum)
{
	return (flags[(bitnum/32)] & (powtable[(bitnum-(32*(bitnum/32)))]));
}


inline void flag_remove(long *flags, long bitnum)
{
	flags[(bitnum/32)] &= ~powtable[(bitnum-(32*(bitnum/32)))];
}

inline void flag_set(long *flags, long bitnum)
{
	flags[(bitnum/32)] |= powtable[(bitnum-(32*(bitnum/32)))];
}

inline void flag_reverse(long *flags, long bitnum)
{
	if(flag_isset(flags, bitnum))
		flag_remove(flags,bitnum);
	else
		flag_set(flags,bitnum);
}

///////////////
// UTILITIES //
///////////////
long critflags_name(char *str)
{
	if(!ValidString(str))
		return -1;

	     if(!strcasecmp(str, "mount"))		return CFLAG_MOUNT;
	else if(!strcasecmp(str,"banker"))		return CFLAG_BANKER;

	return -1;
}

long objflags_name(char *str)
{
	if(!ValidString(str))
		return -1;

	     if(!strcasecmp(str, "notake"))		return OFLAG_NOTAKE;
	else if(!strcasecmp(str, "glow"))		return OFLAG_GLOW;
	else if(!strcasecmp(str, "hum"))		return OFLAG_HUM;

	return -1;
}

long roomflags_name(char *str)
{
	if(!ValidString(str))
		return -1;

	     if(!strcasecmp(str, "heal"))		return RFLAG_HEAL;
//	else if(!strcasecmp(str, "glow"))		return OFLAG_GLOW;

	return -1;
}

char *strflags(void *obj)
{
	CREATURE *xcrit=0;
	OBJECT *xobj=0;
	ROOM *xroom=0;
	static char buf[MAX_BUFFER];

	buf[0] = '\0';

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		xcrit = (CREATURE*)obj;
		if(flag_isset(xcrit->flags, CFLAG_LOG))		strcat(buf,"LOG ");
		if(flag_isset(xcrit->flags, CFLAG_PLAYER))	strcat(buf,"Player ");
		if(flag_isset(xcrit->flags, CFLAG_ANSI))	strcat(buf,"Ansi ");
		if(flag_isset(xcrit->flags, CFLAG_QUIET))	strcat(buf,"Quiet ");
		if(flag_isset(xcrit->flags, CFLAG_MOUNT))	strcat(buf,"Mount ");
		if(flag_isset(xcrit->flags, CFLAG_BANKER))	strcat(buf,"Banker ");
		if(flag_isset(xcrit->flags, CFLAG_ANTIIDLE))	strcat(buf,"Anti-Idle ");
		if(flag_isset(xcrit->flags, CFLAG_BLANK))	strcat(buf,"Blank ");
		if(flag_isset(xcrit->flags, CFLAG_LOG))		strcat(buf,"Log ");
		if(flag_isset(xcrit->flags, CFLAG_NOMENU))	strcat(buf,"No-Menu ");
		if(flag_isset(xcrit->flags, CFLAG_NOTIFY))	strcat(buf,"Notify ");
		if(flag_isset(xcrit->flags, CFLAG_WIZINVIS))	strcat(buf,"Wizinvis ");
		break;
	case TYPE_OBJECT:
		xobj = (OBJECT*)obj;
		if(flag_isset(xobj->flags, OFLAG_NOTAKE))	strcat(buf,"Notake ");
		if(flag_isset(xobj->flags, OFLAG_GLOW))		strcat(buf,"Glow ");
		if(flag_isset(xobj->flags, OFLAG_HUM))		strcat(buf,"Hum ");
		break;
	case TYPE_ROOM:
		xroom = (ROOM*)obj;
		if(flag_isset(xroom->flags, RFLAG_HEAL))	strcat(buf,"Heal ");
		break;
	default:
		mudlog("strflags: invalid object!");
		strcpy(buf,"INVALID OBJECT");
		break;
	}

	if(buf[0] == '\0')
		strcpy(buf,"None ");
	buf[strlen(buf)-1] = '\0';  // strip last space

	return buf;
}
