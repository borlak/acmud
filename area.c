/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
area.c -- areas, rooms, etc.

update: i removed the room prototype.. decided it would be more of a pain than
useful.. primarily in OLC
*/

#include "stdh.h"
#include <math.h>

///////////////
// FUNCTIONS //
///////////////
// need this for sql
char *smash_quotes(char *str)
{
	static char newbuf[MAX_BUFFER*2];
	char *p;

	newbuf[0] = '\0';
	p = newbuf;

	if (!str)
		return "";

	for(; *str != '\0'; str++)
	{
		if(*str == '\"')
		{
			*p++ = '\\';
			*p++ = '\"';
			continue;
		}
		else if(*str == '\'')
		{
			*p++ = '\\';
			*p++ = '\'';
			continue;
		}
		else if (*str == '\\')
		{
			*p++ = '\\';
			*p++ = '\\';
			continue;
		}
		*p++ = *str;
	}
	*p = '\0';
	return newbuf;
}


EXIT *find_exit(ROOM *room, char *name)
{
	EXIT *exit;

	for(exit = room->exits; exit; exit = exit->next)
	{
		if(!strcasecmp(exit->name,name))
			break;
	}
	return exit;
}


EXTRA *find_extra(void *obj, char *name)
{
	EXTRA *extra;

	switch(*(unsigned int*)obj)
	{
	case TYPE_OBJECT:
		extra = ((OBJECT*)obj)->extras;
		break;
	case TYPE_ROOM:
		extra = ((ROOM*)obj)->extras;
		break;
	}

	for(; extra; extra = extra->next)
	{
		if(strlistcmp(extra->keywords,name))
			break;
	}
	return extra;
}


////////////
// TABLES //
////////////
const struct direction_t directions[] =
{
	{"n",	"north",	"south"},
	{"s",	"south",	"north"},
	{"e",	"east",		"west"},
	{"w",	"west",		"east"},
	{"u",	"up",		"down"},
	{"d",	"down",		"up"},
	{"ne",	"northeast",	"southwest"},
	{"se",	"southeast",	"northwest"},
	{"sw",	"southwest",	"northeast"},
	{"nw",	"northwest",	"southeast"},
	{"",	"enter",	"exit"},
	{0,0,0}
};
