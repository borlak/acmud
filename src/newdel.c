/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
newdel.c -- new and delete(free) functions for all the objects in memory
of the mud.. make the files a little cleaner.
as with the other parts of the mud, everything in this file should be in
alphabetical order
*/

#include "stdh.h"

///////////////
// VARIABLES //
///////////////
long total_icreatures;
long total_iobjects;

AREA *area_list=0;

BAN *ban_list=0;

CREATURE *creature_list=0;
CREATURE *creature_free=0;
CREATURE *hash_creature[HASH_KEY];

HELP *help_free=0;

EXIT *exit_free=0;

EXTRA *extra_free=0;

NOTE *note_list=0;

OBJECT *object_list=0;
OBJECT *object_free=0;
OBJECT *hash_object[HASH_KEY];

RESET	*hash_reset[HASH_KEY];

ROOM	*hash_room[HASH_KEY];

SOCIAL	*hash_social[HASH_KEY];

NOTE	*hash_note[HASH_KEY];

SHOP  	*hash_shop[HASH_KEY];

// EXTERNS
extern long mud_shutdown;

////////////////////
// FIND FUNCTIONS //
////////////////////
CREATURE *hashfind_creature(long vnum)
{
	CREATURE *crit=0;

	for( crit = hash_creature[vnum%HASH_KEY]; crit; crit = crit->next_hash )
	{
		if( crit->vnum == vnum )
			break;
	}
	return crit;
}

OBJECT *hashfind_object(long vnum)
{
	OBJECT *obj=0;

	for( obj = hash_object[vnum%HASH_KEY]; obj; obj = obj->next_hash )
	{
		if( obj->vnum == vnum )
			break;
	}
	return obj;
}


RESET *hashfind_reset(long vnum)
{
	RESET *reset=0;

	for( reset = hash_reset[vnum%HASH_KEY]; reset; reset = reset->next_hash )
	{
		if( reset->vnum == vnum )
			break;
	}
	return reset;
}


ROOM *hashfind_room(long vnum)
{
	ROOM *room=0;

	for( room = hash_room[vnum%HASH_KEY]; room; room = room->next_hash )
	{
		if( room->vnum == vnum )
			break;
	}
	return room;
}


SOCIAL *hashfind_social(char *name)
{
	SOCIAL *social=0;

	for( social = hash_social[(int)Lower(*name)%HASH_KEY]; social; social = social->next_hash )
	{
		if( !strindex(social->name, name) )
			break;
	}
	return social;
}

SHOP *hashfind_shop(long vnum)
{
	SHOP *shop=0;

	for ( shop = hash_shop[vnum%HASH_KEY]; shop; shop = shop->next_hash )
	{
		if (shop->keeper == vnum)
			break;
	}
	return shop;
}



///////////////////
// NEW FUNCTIONS //
///////////////////
AREA *new_area(char *name)
{
	AREA *area=0;

	NewObject(area,area)
	AddToList(area_list,area)

	str_dup(&area->name,		name		);
	str_dup(&area->builders,	""		);

	area->type		= TYPE_AREA;		// DO NOT CHANGE
	area->low = area->high	= 0;

	return area;
}


BAN *new_ban(CREATURE *crit)
{
	BAN *ban=0;

	NewObject(ban,ban)
	AddToList(ban_list,ban)

	ban->type	= TYPE_BAN;	// DO NOT CHANGE

	str_dup(&ban->ip,	crit ? crit->socket->ip	: "" 	);
	str_dup(&ban->message,	BAN_MESSAGE			);
	str_dup(&ban->name,	crit ? crit->name	: ""	);

	return ban;
}


CREATURE *new_creature(long vnum)
{
	static MSOCKET blank_socket;
	CREATURE *crit;
	CREATURE *proto = hashfind_creature(vnum);

	if(!proto)
	{
		mudlog("new_creature: bad vnum %li",vnum);
		return 0;
	}

	NewObject(creature_free,crit)
	AddToList(creature_list,crit)

	reset_socket(&blank_socket);

	str_dup(&crit->keywords,	proto->keywords		);
	str_dup(&crit->name,		proto->name		);
	str_dup(&crit->description,	proto->description	);
	str_dup(&crit->long_descr,	proto->long_descr	);
	str_dup(&crit->wear,		proto->wear		);
	str_dup(&crit->loaded,		""			);

	crit->type		= TYPE_CREATURE;	// DO NOT CHANGE
	crit->vnum		= vnum;
	crit->prototype		= 0;
	crit->socket		= &blank_socket;

	crit->alignment		= proto->alignment;
	crit->dexterity		= proto->dexterity;
	crit->extras		= proto->extras;
	crit->max_hp		= proto->max_hp;
	crit->hp		= crit->max_hp;
	crit->intelligence	= proto->intelligence;
	crit->in_room		= hashfind_room(1);
	crit->max_move		= proto->max_move;
	crit->move		= crit->max_move;
	crit->sex		= proto->sex;
	crit->state		= proto->state;
	crit->position		= proto->position;
	crit->strength		= proto->strength;
	crit->level		= proto->level;

	load_flags(crit);
	flag_remove(crit->flags, CFLAG_PLAYER);

	if (proto->shop)
		crit->shop = proto->shop;

	total_icreatures++;
	return crit;
}
CREATURE *new_creature_proto(long vnum)
{
	CREATURE *crit=0;
	char *default_wear = "arms body head held held feet finger finger hands legs neck waist wrist wrist";

	NewObject(crit,crit)
	AddToHashList(hash_creature,crit,vnum)

	crit->type		= TYPE_CREATURE; // DO NOT CHANGE
	crit->vnum		= vnum;
	crit->prototype		= 1;

	str_dup(&crit->keywords,	"creature"			);
	str_dup(&crit->name,		"A creature"			);
	str_dup(&crit->description,	"A creature's description."	);
	str_dup(&crit->long_descr,	"A creature is here."		);
	str_dup(&crit->wear,		default_wear			);
	str_dup(&crit->loaded,		""				);

	crit->flags		= calloc(sizeof(long), ((CFLAG_LAST/32)+1));
	crit->shop 		= 0;
	crit->level		= 0;

	return crit;
}


EXIT *new_exit(ROOM *room)
{
	EXIT *exit=0;

	NewObject(exit_free,exit)
	AddToList(room->exits,exit)

	exit->type	= TYPE_EXIT; // DO NOT CHANGE

	str_dup(&exit->name,	"");

	exit->door	= 0;
	exit->key	= 0;
	exit->to_vnum	= 1;

	return exit;
}


EXTRA *new_extra(void *obj)
{
	CREATURE *critx=0;
	OBJECT *objx=0;
	EXTRA *extra=0, *extrad;
	ROOM *room=0;
	
	NewObject(extra_free,extra)

	extra->type = TYPE_EXTRA;

	switch(*(unsigned int*)obj)
	{
	case TYPE_ROOM:
		room		= (ROOM*)obj;
		extra->loadtype	= TYPE_ROOM;
		extra->vnum	= room->vnum;

		AddToListEnd(room->extras,extra,extrad)
		break;
	case TYPE_EXTRA:
		objx		= (OBJECT*)obj;
		extra->loadtype	= TYPE_OBJECT;
		extra->vnum	= objx->vnum;

		AddToListEnd(objx->extras,extra,extrad)
		break;
	case TYPE_CREATURE:
		critx		= (CREATURE*)obj;
		extra->loadtype	= TYPE_CREATURE;
		extra->vnum	= critx->vnum;

		AddToListEnd(critx->extras,extra,extrad)
	}

	str_dup(&extra->keywords,	"");
	str_dup(&extra->description,	"");

	return extra;
}


HELP *new_help(void)
{
	HELP *help;

	NewObject(help_free,help)

	help->type = TYPE_HELP;   // DO NOT CHANGE

	str_dup(&help->keyword,	"");
	str_dup(&help->entry,	"");
	str_dup(&help->index,	"");
	str_dup(&help->last,	"");
	help->level = 0;

	return help;
}


NOTE *new_note(void)
{
        NOTE *note=0;
	NOTE *dummy;

        NewObject(note,note)
        AddToListEnd(note_list,note,dummy)
	note->type = TYPE_NOTE;

	str_dup(&note->subject,	"");
	str_dup(&note->sent_to,	"");
	str_dup(&note->text,	"");
	str_dup(&note->written,	"");
	str_dup(&note->sender,	"");

        return note;
}


OBJECT *new_object(long vnum)
{
	OBJECT *obj;
	OBJECT *proto = hashfind_object(vnum);
	long i;

	if(!proto)
	{
		mudlog("new_object: no prototype for vnum %li",vnum);
		return 0;
	}

	NewObject(object_free,obj)
	AddToList(object_list,obj)

	str_dup(&obj->keywords,		proto->keywords		);
	str_dup(&obj->name,		proto->name		);
	str_dup(&obj->long_descr,	proto->long_descr	);
	str_dup(&obj->description,	proto->description	);
	str_dup(&obj->wear,		proto->wear		);
	str_dup(&obj->worn,		""			);
	str_dup(&obj->loaded,		""			);

	obj->type	= TYPE_OBJECT;	// DO NOT CHANGE
	obj->id		= 0;
	obj->nested	= 0;
	obj->prototype	= 0;
	obj->vnum	= vnum;

	obj->extras	= proto->extras;
	obj->objtype	= proto->objtype;
	obj->timer	= proto->timer;
	obj->weight	= proto->weight;
	obj->worth	= proto->worth;

	load_flags(obj);

	for(i = 0; i < MAX_OBJVALUES; i++)
		obj->values[i] = proto->values[i];

	if(obj->timer)
	{
		if(obj->timer > 5)
			obj->timer = randnum(obj->timer-5, obj->timer+5);
	}

	total_iobjects++;
	return obj;
}
OBJECT *new_object_proto(long vnum)
{
	OBJECT *obj=0;

	NewObject(obj,obj)
	AddToHashList(hash_object,obj,vnum)

	obj->type	= TYPE_OBJECT; // DO NOT CHANGE
	obj->vnum	= vnum;
	obj->prototype	= 1;

	str_dup(&obj->keywords,		"object"			);
	str_dup(&obj->name,		"An object"			);
	str_dup(&obj->long_descr,	"An object is here."		);
	str_dup(&obj->description,	"An object is described here."	);
	str_dup(&obj->wear,		""				);
	str_dup(&obj->worn,		""				);
	str_dup(&obj->loaded,		""				);

	obj->flags		= calloc(sizeof(long), ((OFLAG_LAST/32)+1));

	return obj;
}


void add_reset(RESET *reset)
{
	long random=randneg(1,10);
	
	RemoveFromHashList(hash_reset,reset,reset->poptime)
	
	if(reset->time)
	{
		AddToHashList(hash_reset,reset,current_time+random)
		reset->poptime = current_time+random;
	}
	else
	{
		AddToHashList(hash_reset,reset,0)
		reset->poptime = 0;
	}

//	AddToHashList(hash_reset,reset,current_time+(reset->time*60)+random)
//	reset->poptime = current_time+(reset->time*60)+random;
}

RESET *new_reset(void)
{
	RESET *reset=0;

	NewObject(reset,reset)
	AddToHashList(hash_reset,reset,0)

	str_dup(&reset->command,	"");

	reset->type		= TYPE_RESET; // DO NOT CHANGE

	reset->loadtype		= TYPE_CREATURE; // set a default to not mess up the switch() in editor
	reset->poptime		= 0;
	reset->time		= 0;
	reset->min		= 1;
	reset->max		= 1;
	reset->chance		= 10;
	reset->crit		= hashfind_creature(1);
	reset->vnum		= 1;

	return reset;
}


SHOP *new_shop(long vnum) // crit vnum
{
	SHOP *shop=0;
	CREATURE *keeper=0;

	NewObject(shop,shop)
	AddToHashList(hash_shop,shop,vnum);

	shop->type	= TYPE_SHOP;  // DO NOT CHANEG

	shop->keeper 	= vnum;
	shop->item	= 0;
	shop->stype	= 0;
	shop->buy	= 0;
	shop->sell	= 0;
	shop->open	= 0;
	shop->close	= 0;

        if(!(keeper = hashfind_creature(shop->keeper)))
        {
		DeleteObject(shop);   
		return 0;
        }
        keeper->shop = shop;

	return shop;
}


ROOM *new_room(long vnum)
{
	ROOM *room=0;

	NewObject(room,room)
	AddToHashList(hash_room,room,vnum)

	str_dup(&room->loaded,		"");
	str_dup(&room->name,		"A room");
	str_dup(&room->night_name,	"");
	str_dup(&room->description,	"A room description.");
	str_dup(&room->night_description,"");

	room->type	= TYPE_ROOM;	// DO NOT CHANGE

	room->flags	= calloc(sizeof(long), ((RFLAG_LAST/32)+1));
	room->vnum	= vnum;
	room->area	= find_area(vnum);

	return room;
}


SOCIAL *new_social(char *name)
{
	SOCIAL *social = 0;

	NewObject(social,social)
	AddToHashList(hash_social, social, (int)(*name));

	social->type = TYPE_SOCIAL;	// DO NOT CHANGE

	str_dup(&social->name,		name);
	str_dup(&social->no_target,	"");
	str_dup(&social->crit_target,	"");
	str_dup(&social->object_target,	"");
	str_dup(&social->self_target,	"");

	return social;
}


///////////////////////////
// DELETE/FREE FUNCTIONS //
///////////////////////////
void free_area(AREA *area)
{
	RemoveFromList(area_list,area)
	DeleteObject(area->name)
	DeleteObject(area->builders)
	DeleteObject(area)
// FIX Delete the area
}


void free_ban(BAN *ban)
{
	RemoveFromList(ban_list,ban)
	DeleteObject(ban->ip)
	DeleteObject(ban->message)
	DeleteObject(ban->name)
	DeleteObject(ban)
}


void free_creature(CREATURE *crit)
{
	while(crit->inventory)
		free_object(crit->inventory);
	while(crit->equipment)
		free_object(crit->equipment);

	if(crit->prototype)
	{
		if(crit->vnum == 1)
		{
			mudlog("FREE_CREATURE: tried to remove creature 1(default)!");
			return;
		}

		while(crit->extras)
			free_extra(crit, crit->extras);

		RemoveFromHashList(hash_creature,crit,crit->vnum)
		update_resets();
	}
	else
	{
		total_icreatures--;
		RemoveFromList(creature_list,crit)
		trans(crit,0);
	}

	free(crit->flags);

	DeleteObject(crit->description)
	DeleteObject(crit->long_descr)
	DeleteObject(crit->name)
	DeleteObject(crit->loaded)

	if(crit->reset)
		crit->reset->loaded--;

	AddToList(creature_free,crit)
}


void free_exit(ROOM *room, EXIT *exit)
{
	RemoveFromList(room->exits, exit)

	update_resets();

	DeleteObject(exit->name)
	AddToList(exit_free,exit)
}


void free_extra(void *obj, EXTRA *extra)
{
	switch(*(unsigned int*)obj)
	{
	case TYPE_ROOM:
		RemoveFromList(((ROOM*)obj)->extras, extra)
		break;
	case TYPE_OBJECT:
		RemoveFromList(((OBJECT*)obj)->extras, extra)
		break;
	case TYPE_CREATURE:
		RemoveFromList(((CREATURE*)obj)->extras, extra)
		break;
	}
	DeleteObject(extra->keywords)
	DeleteObject(extra->description)
	AddToList(extra_free,extra)
}


void free_object(OBJECT *obj)
{
	OBJECT *contents, *contents_next;

	if(obj->prototype)
	{
		while(obj->extras)
			free_extra(obj, obj->extras);

		RemoveFromHashList(hash_object,obj,obj->vnum)
		update_resets();
	}
	else
	{
		total_iobjects--;

		for(contents = obj->contents; contents; contents = contents_next)
		{
			contents_next = contents->next_content;
			free_object(contents);
		}
		RemoveFromList(object_list,obj)
		trans(obj, 0);
	}

	DeleteObject(obj->name)
	DeleteObject(obj->long_descr)
	DeleteObject(obj->description)
	DeleteObject(obj->loaded)

	free(obj->flags);

	if(obj->reset)
		obj->reset->loaded--;

	memset(obj, 0, sizeof(OBJECT));
	AddToList(object_free,obj)
}


void free_reset(RESET *reset)
{
	RemoveFromHashList(hash_reset,reset,reset->poptime)
	RemoveFromList(reset->room->resets, reset)

	DeleteObject(reset->command)
	DeleteObject(reset)
}


void free_room(ROOM *room)
{
	CREATURE *crit;
	OBJECT *obj;
	RESET *reset;

	if(room->vnum == 1)
	{
		mudlog("FREE_ROOM: tried to remove room 1(default)!");
		return;
	}

	RemoveFromHashList(hash_room,room,room->vnum)

	while(room->exits)
		free_exit(room, room->exits);
	while(room->extras)
		free_extra(room, room->extras);
	while((reset=room->resets))
		free_reset(reset);

	DeleteObject(room->name);
	DeleteObject(room->night_name);
	DeleteObject(room->description);
	DeleteObject(room->night_description);
	DeleteObject(room->loaded);

	free(room->flags);

	if(!mud_shutdown)
	{
		for(crit = creature_list; crit; crit = crit->next)
		{
			if(crit->in_room && crit->in_room == room)
				trans(crit, hashfind_room(1));
		}
		for(obj = object_list; obj; obj = obj->next)
		{
			if(obj->in_room && obj->in_room == room)
				trans(obj, hashfind_room(1));
		}
	}
	DeleteObject(room)
}


void free_shop(SHOP *shop)
{
	CREATURE *keeper=0;
	
	RemoveFromHashList(hash_shop,shop,shop->keeper)
	if ((keeper = hashfind_creature(shop->keeper)))
		keeper->shop = 0;
	DeleteObject(shop)
}


void free_help(HELP *help)
{
	DeleteObject(help->keyword)
	DeleteObject(help->entry)
	DeleteObject(help->index)
	DeleteObject(help->last)
	DeleteObject(help)
}


void free_social(SOCIAL *social)
{
	RemoveFromHashList(hash_social,social,(int)(*social->name));

	DeleteObject(social->name);
	DeleteObject(social->no_target);
	DeleteObject(social->crit_target);
	DeleteObject(social->object_target);
	DeleteObject(social->self_target);
	DeleteObject(social)
}


// BIG DADDY
void free_mud(void)
{
	ROOM *room;
	OBJECT *obj;
	MSOCKET *socket, *socket_next;
	CREATURE *crit;
	long hash;
	long rooms=0, objects=0, creatures=0, players=0;

	for(socket = socket_list; socket; socket = socket_next)
	{
		socket_next = socket->next;
		free_socket(socket); // fwrite_player inside of free_socket
		players++;
	}

	for(hash = 0; hash < HASH_KEY; hash++)
	{
		while((room = hash_room[hash%HASH_KEY]))
		{
			RemoveFromHashList(hash_room, room, hash)
			rooms++;
		}

		while((obj = hash_object[hash%HASH_KEY]))
		{
			RemoveFromHashList(hash_object, obj, hash)
			objects++;
		}

		while((crit = hash_creature[hash%HASH_KEY]))
		{
			RemoveFromHashList(hash_creature, crit, hash)
			creatures++;
		}
	}

	mudlog("Number of objects: %li rooms, %li objects, %li creatures, %li players.",
		rooms, objects, creatures, players );
}


