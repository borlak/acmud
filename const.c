/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
const.c -- any permanent tables.  definitions of structures will be in struct.h
*/

#include "stdh.h"

const char *objtype_table[] =
{
	"treasure",
	"food",
	"drink",
	"container",
	"coin",
	"armor",
	"weapon",
	"light",
	"key",
	0
};


const char *sex_table[] =
{
	"neutral",
	"male",
	"female",
	0
};


const char *roomtype_table[] =
{
	"air",
	"arctic",
	"city",
	"desert",
	"dirt",
	"forest",
	"hills",
	"inside",
	"grassy",
	"mountain",
	"ocean",
	"road",
	"underwater",
	"water",
	0
};


const char *ban_types[] =
{
	"Broken",
	"Player",
	"IP Address",
	0
};


const struct reset_type reset_types[] =
{
	{TYPE_CREATURE,	"creature"	},
	{TYPE_OBJECT,	"object"	},
	{TYPE_EXIT,	"exit"		},
	{0,0}
};


