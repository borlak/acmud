/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
object.c -- stuff all about objects!
This file takes care of all object manipulation.
*/

#include "stdh.h"
#include "io.h"
#include <ctype.h>


///////////////
// FUNCTIONS //
///////////////
// move anything to anything.. rooms to creatures, creatures to objects...
// although not everything is possible atm, and you may want to keep it that
// way.. right now if you do creature to creature, it puts crit1 in crit2's
// room
void trans(void *obj, void *to)
{
	CREATURE *xcrit=0;
	CREATURE *tocrit=0;
	OBJECT *xobj=0;
	OBJECT *toobj=0;
	ROOM *room=0;

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		xcrit = (CREATURE*)obj;
		room = xcrit->in_room;

		if( xcrit->in_room )
		{
			RemoveFromContent(room->crits,xcrit)
			if(IsPlayer(xcrit))
				room->area->players--;
		}

		if( xcrit->mount )
		{
			xcrit->mount->rider = 0;
			xcrit->mount = 0;
		}

		if (!to)
			return;

		switch(*(unsigned int*)to)
		{
		case TYPE_CREATURE:
			tocrit = (CREATURE*)to;
			AddToContent(room->crits,xcrit)
			tocrit->rider = xcrit;
			xcrit->mount = tocrit;
			return;
		case TYPE_ROOM:
			room = (ROOM*)to;
			AddToContent(room->crits,xcrit)
			xcrit->in_room = room;
			if(IsPlayer(xcrit))
				room->area->players++;
			return;
		case TYPE_OBJECT: // not now
		default:
			return;
			break;
		}
		break;
	case TYPE_OBJECT:
		xobj = (OBJECT*)obj;

		if( xobj->in_room )		{ RemoveFromContent(xobj->in_room->objects,xobj)	}
		else if( xobj->in_obj )		{ RemoveFromContent(xobj->in_obj->contents,xobj)	}
		else if( xobj->held_by )
		{
			if(ValidString(xobj->worn))
				remove_obj(xobj->held_by, xobj);

			RemoveFromContent(xobj->held_by->inventory, xobj)
		}
		xobj->in_room	= 0;
		xobj->held_by	= 0;
		xobj->in_obj	= 0;
		xobj->nested	= 0;

		if(!to)
			return;

		switch(*(unsigned int*)to)
		{
		case TYPE_ROOM:
			room = (ROOM*)to;

			AddToContent(room->objects,xobj)

			xobj->in_room	= room;

			if(xobj->owner_id)
			{
				xobj->owner_id	= 0;
				mysql_update_object(xobj);
			}
			return;
		case TYPE_CREATURE:
			tocrit = (CREATURE*)to;
	
			AddToContent(tocrit->inventory,xobj)

			xobj->held_by	= tocrit;

			if(IsPlayer(tocrit))	xobj->owner_id = tocrit->id;
			else			xobj->owner_id = 0;

			// don't update object if player is in loading state
			if((IsPlayer(tocrit) && tocrit->socket->connected != CON_GET_NAME)
			|| !IsPlayer(tocrit))
				mysql_update_object(xobj);
			return;
		case TYPE_OBJECT:
			toobj = (OBJECT*)to;

			AddToContent(toobj->contents,xobj)

			xobj->in_obj	= toobj;
			xobj->owner_id	= toobj->owner_id;
			xobj->nested	= toobj->nested + 1;

			if(toobj->owner_id)
				mysql_update_object(xobj);
			return;
		default:
			break;
		}
		break;
	default:
		mudlog("TRANS: Unknown type of object!");
		break;
	}
	mudlog("TRANS: bad to_obj!");
}


// this will split coins into two objects (of the same coin type)
// it will make the original pointer point to the new, splitted
// coins. the old pointer will be updated, amount-wise and
// description wise
OBJECT *splitcoins(OBJECT *coins, void *to, long amount)
{
	OBJECT *newcoins=0;

	for(newcoins =
		(TypeCreature(to)) ? ((CREATURE*)to)->inventory :
		(TypeObject(to))  ? ((OBJECT*)to)->contents :
		((ROOM*)to)->objects
	; newcoins; newcoins = newcoins->next_content)
		if(newcoins->values[0] == coins->values[0])
			break;

	if(!amount)
		amount = coins->values[1];

	if(!newcoins)
	{
		if(amount == coins->values[1])
			return coins;
		newcoins		= new_object(VNUM_COIN);
		newcoins->values[1]	= 0;
	}
	newcoins->values[0]	= coins->values[0];
	newcoins->values[1]	+= amount;
	coins->values[1]	-= amount;

	coinify(newcoins);

	if(!coins->values[1])
		free_object(coins);
	else
		coinify(coins);

	return newcoins;
}


long get_worth(CREATURE *crit)
{
	OBJECT *obj;
	long worth = 0;

	for(obj = crit->inventory; obj; obj = obj->next_content)
		if(obj->objtype == OBJ_COIN)
			worth += coin_table[obj->values[0]].worth * obj->values[1];
	return worth;
}


char *purchase(CREATURE *crit, long cost)
{
	OBJECT *obj, *obj_next, *splitcoin;
	static char buf[MAX_BUFFER];
	long coins, count, worth;
	long x;

	// this goes from lowest value to highest
	// theory is to spend all your lowest coins first
	// then split the next biggest
	buf[0] = '\0';
	count = 0;
	for(x = 0; coin_table[x].worth; x++)
	{
		for(obj = crit->inventory; obj; obj = obj_next)
		{
			obj_next = obj->next_content;
	
			if(obj->objtype == OBJ_COIN && obj->values[0] == x)
			{
				if(!(cost/coin_table[x].worth))
				{
					splitcoin = obj;
					break;
				}
				
				coins		= 0;	
				while(cost/coin_table[x].worth)
				{
					coins++;
					obj->values[1]--;	
					cost -= coin_table[x].worth;

					if(obj->values[1] == 0)
					{
						free_object(obj);
						break;
					}
					else
						coinify(obj);
				}
				sprintf(buf+strlen(buf),"%s%li %s",
					count?", ":"", coins, 
					coin_table[x].ansi_name);
				count++;
				break;
			}
		}
	}

	// no cost
	if(!cost)
		sprintf(buf+strlen(buf),".  No change is needed!");
	else
	{
		if(!ValidString(buf))
			sprintf(buf,"You split a %s coin and get ",
				coin_table[splitcoin->values[0]].ansi_name);
		else
			sprintf(buf+strlen(buf)," and split a %s coin.  You get ",
				coin_table[splitcoin->values[0]].ansi_name);

		worth = coin_table[splitcoin->values[0]].worth-cost;

		if(--splitcoin->values[1] == 0)
			free_object(splitcoin);

		count = 0;
		for(x--; x >= 0; x--)
		{
			if(!(worth/coin_table[x].worth))
				continue;
			for(obj = crit->inventory; obj; obj = obj->next_content)
			{
				if(obj->objtype == OBJ_COIN && obj->values[0] == x)
				{
					coins		= worth/coin_table[x].worth;
					obj->values[1]	+= coins;
					worth		-= coins*coin_table[x].worth;
					break;
				}
			}
			if(!obj)
			{
				obj		= new_object(VNUM_COIN);
				obj->values[0]	= x;
				obj->values[1]	= worth/coin_table[x].worth;
				coins		= obj->values[1];
				worth		-= obj->values[1]*coin_table[x].worth;
				trans(obj,crit);
			}
			coinify(obj);

			sprintf(buf+strlen(buf),"%s%li %s",
				count?", ":"", coins, 
				coin_table[x].ansi_name);
			count++;
		}
		sprintf(buf+strlen(buf)," in return.");
	}
			
	return buf;
}

// make the crit worth a new amount.. can add or subtract from his original worth
// this will magically make new coins dissapear or appear
void make_worth(CREATURE *crit, long worth)
{
	OBJECT *obj, *obj_next_content;
	long x;

	for(obj = crit->inventory; obj; obj = obj_next_content)
	{
		obj_next_content = obj->next_content;

		if(obj->objtype == OBJ_COIN)
			free_object(obj);
	}

	for(x = 0; coin_table[x].name && worth; x++)
	{
		if(worth / coin_table[x].worth > 0)
		{
			obj		= new_object(VNUM_COIN);
			obj->values[0]	= x;
			obj->values[1]	= worth / coin_table[x].worth;
			worth		-= coin_table[x].worth*obj->values[1];

			coinify(obj);
			trans(obj,crit);
		}
	}
}


// find the (num) object in the room or on the crit...
OBJECT *find_obj(CREATURE *crit, char *argument, long flags)
{
	OBJECT *obj=0;
	OBJECT *xobj=0;
	long num=numbered_arg(argument);
	long curnum=0;

	if(IsSet(flags, OBJ_GROUND))
	{
		for(obj = crit->in_room->objects; obj; obj = obj->next_content)
		{
			if(IsSet(flags, OBJ_INOBJ))
			{
				for(xobj = obj->contents; xobj; xobj = xobj->next_content)
				{
					if(!strargindex(xobj->keywords, argument))
					{
						if(++curnum == num)
						{
							curnum	= 0;
							num	= 1;
							break;
						}
					}
				}
				continue;
			}

			if( !strargindex(obj->keywords, argument) )
			{
				if(++curnum == num)
				{
					curnum	= 0;
					num	= 1;
					break;
				}
			}
		}
	}
	if(!obj && IsSet(flags, OBJ_HELD))
	{
		for(obj = crit->inventory; obj; obj = obj->next_content)
		{
			if(IsSet(flags, OBJ_INOBJ))
			{
				for(xobj = obj->contents; xobj; xobj = xobj->next_content)
				{
					if(!strargindex(xobj->keywords, argument))
					{
						if(++curnum == num)
						{
							curnum	= 0;
							num	= 1;
							break;
						}
					}
				}
				continue;
			}

			if( !strargindex(obj->keywords, argument) )
			{
				if(++curnum == num)
				{
					curnum	= 0;
					num	= 1;
					break;
				}
			}
		}
	}
	if(!obj && IsSet(flags, OBJ_EQUIPMENT))
	{
		for(obj = crit->equipment; obj; obj = obj->next_content)
		{
			if( !strargindex(obj->keywords, argument) )
			{
				if(++curnum == num)
				{
					curnum	= 0;
					num	= 1;
					break;
				}
			}
		}
	}
	if(!obj && IsSet(flags, OBJ_WORLD))
	{
		for(obj = object_list; obj; obj = obj->next)
		{
			if( !strargindex(obj->keywords, argument) )
			{
				if(++curnum == num)
				{
					curnum	= 0;
					num	= 1;
					break;
				}
			}
		}
	}
	if(!obj && curnum)
	{
		char buf[MAX_BUFFER];
		sprintf(buf,"%li.%s",num-curnum,argument);
		strcpy(argument,buf);
	}
	return obj;
}


void coinify(OBJECT *coins)
{
	char buf[MAX_BUFFER];

	// Name
	if(coins->values[1] == 1)
		sprintf(buf,"a %s coin", coin_table[coins->values[0]].ansi_name);
	else
		sprintf(buf,"%li %s coins", coins->values[1], coin_table[coins->values[0]].ansi_name);
	str_dup(&coins->name, buf);

	// Long(room) description
	if(coins->values[1] == 1)
		str_dup(&coins->long_descr, "A single coin lies here.");
	else if(coins->values[1] < 10)
		str_dup(&coins->long_descr, "Several coins lie here.");
	else if(coins->values[1] < 100)
		str_dup(&coins->long_descr, "A bunch of coins lie here.");
	else if(coins->values[1] < 500)
		str_dup(&coins->long_descr, "A large pile of coins lie here.");
	else
		str_dup(&coins->long_descr, "A mountain of coins lie here.");

	// Keyword
	sprintf(buf,"coin%s %s", coins->values[1] == 1 ? "" : "s", coin_table[coins->values[0]].name);
	str_dup(&coins->keywords, buf);

	// Description
	sprintf(buf,"You are looking at %s%s coin%s.", 
		coins->values[1] == 1 ? "a " : "",
		coin_table[coins->values[0]].ansi_name,
		coins->values[1] == 1 ? "" : "s");
	str_dup(&coins->description, buf);

	fwrite_object(coins);
}


// used when someone drops/gives/takes an item
// this is to prevent duplication of objects during crashes
void mysql_update_object(OBJECT *obj)
{
	OBJECT *xobj;
	char buf[MAX_BUFFER];

	for(xobj = obj->contents; xobj; xobj = xobj->next_content)
	{
		xobj->owner_id	= obj->owner_id;
		xobj->nested	= obj->nested + 1;
		mysql_update_object(xobj);
	}

	if(!obj->id)
	{
		fwrite_object(obj);
		return;
	}

	sprintf(buf, "UPDATE object SET owner_id='%li', in_obj='%li', worn='%s', nested='%li' WHERE id='%li'",
		obj->owner_id,
		obj->in_obj ? obj->in_obj->id : 0,
		obj->worn,
		obj->nested,
		obj->id);
	mysql_query(mysql, buf);
}


//////////////////
///DO_FUNCTIONS //
//////////////////


CMD(do_drop)
{
	OBJECT *xobj_next;
	char *oarg=0;
	long amount=0;

	if(is_number(arguments[0]))
	{
		amount	= atoi(arguments[0]);
		
		if(!arguments[1])
		{
			interpret(crit,"help drop");
			return;
		}
		oarg	= arguments[1];
	}
	else
		oarg	= arguments[0];

	if (!strcasecmp(arguments[0],"all"))
	{
		for (xobj = crit->inventory; xobj; xobj = xobj_next)
		{
			xobj_next = xobj->next_content;
			if (xobj->objtype == OBJ_COIN)
				xobj = splitcoins(xobj, crit->in_room, xobj->values[1]);
			trans(xobj,crit->in_room);
			message("$n drop$x $p.",crit,0,xobj);
		}
		return;
	}
	if (!(xobj=find_obj(crit,oarg,OBJ_HELD)))
	{
		sendcrit(crit,"Drop what?");
		return;
	}

	if(!xobj->held_by)
	{
		sendcrit(crit,"The object must be in your inventory.");
		return;
	}

	if(xobj->objtype == OBJ_COIN)
		xobj = splitcoins(xobj, crit->in_room, amount);

	trans(xobj,crit->in_room);
	message("$n drop$x $p.",crit,0,xobj);
}


CMD(do_open)
{
	EXIT *exit=0;
	ROOM *room=0;

	for (exit = crit->in_room->exits; exit; exit = exit->next)
	{
		if ((!strindex(exit->name, arguments[0]) && exit->door >= DOOR_OPEN)||
		   (exit->door >= DOOR_OPEN && !strindex("door", arguments[0])))
		{
			if (exit->door == DOOR_OPEN)
			{
				sendcrit(crit,"The door is already open.");
				return;
			}

			if (exit->door == DOOR_LOCKED)
			{
				sendcritf(crit,"The door to the %s is locked.",exit->name);
				return;
			}

			message("$n open$x the $p door.",crit,0,exit->name);
			exit->door = DOOR_OPEN;

			// open the other side!
			if ((room = hashfind_room(exit->to_vnum)))
			{
				for (exit = room->exits; exit; exit = exit->next)
				{
					if (exit->to_vnum == crit->in_room->vnum)
					{
						message("The $p door opens.", room,0,exit->name);
						exit->door = DOOR_OPEN;
						break;
					}
				}
			}
			return;
		}
	}
	if (!strindex("door", arguments[0]))
		sendcrit(crit,"There is no door here.");
	else
		sendcrit(crit,"There is no door there.");
	return;
}


CMD(do_close)
{
	EXIT *exit=0;
	ROOM *room=0;

	for (exit = crit->in_room->exits; exit; exit = exit->next)
	{
		if ((exit->door >= DOOR_OPEN && !strindex(exit->name, arguments[0])) ||
		   (exit->door >= DOOR_OPEN && !strindex("door", arguments[0])))
		{
			if (exit->door != DOOR_OPEN)
			{
				sendcritf(crit,"The %s door is already closed.",exit->name);
				return;
			}

			message("$n close$x the $p door.",crit,0,exit->name);
			exit->door = DOOR_CLOSED;

			// close the other side!
			if ((room = hashfind_room(exit->to_vnum)))
			{
				for (exit = room->exits; exit; exit = exit->next)
				{
					if (exit->to_vnum == crit->in_room->vnum)
					{
						message("The $p door closes.", room,0,exit->name);
						exit->door = DOOR_CLOSED;
					}
				}
			}

			return;
		}
	}
	if (!strindex("door", arguments[0]))
		sendcrit(crit,"There is no door here.");
	else
		sendcrit(crit,"There is no door there.");
	return;
}


CMD(do_lock)
{
	EXIT *exit=0;
	ROOM *room=0;
	OBJECT *objs=0;

	for (exit = crit->in_room->exits; exit; exit = exit->next)
	{
		if (!strindex(exit->name, arguments[0]) ||
		   (exit->door >= DOOR_OPEN && !strindex("door", arguments[0])))
		{
			if (exit->door == DOOR_LOCKED)
			{
				sendcritf(crit,"The %s door is already locked.",exit->name);
				return;
			}
			if (exit->door == DOOR_OPEN)
			{
				sendcrit(crit,"You must close the door first.");
				return;
			}
			for (objs = crit->inventory; objs; objs = objs->next_content)
			{
				if (objs->vnum == exit->key)
				{
					message("$n lock$x the $p door.",crit,0,exit->name);
					exit->door = DOOR_LOCKED;

					// lock the other side!
					if ((room = hashfind_room(exit->to_vnum)))
					{
						for (exit = room->exits; exit; exit = exit->next)
						{
							if (exit->to_vnum == crit->in_room->vnum)
							{
								message("You hear a clicking sound from the $p door.",
									room,0,exit->name);
								exit->door = DOOR_LOCKED;
							}
						}
					}
					return;
				}
			}
			sendcrit(crit,"You do not have the key for that door.");
			return;
		}
	}
	if (!strindex("door", arguments[0]))
		sendcrit(crit,"There is no door here.");
	else
		sendcrit(crit,"There is no door there.");
}


CMD(do_unlock)
{
	EXIT *exit=0;
	ROOM *room=0;
	OBJECT *objs=0;

	for (exit = crit->in_room->exits; exit; exit = exit->next)
	{
		if (!strindex(exit->name, arguments[0]) ||
		   (exit->door >= DOOR_OPEN && !strindex("door", arguments[0])))
		{
			if (exit->door != DOOR_LOCKED)
			{
				sendcritf(crit,"The %s door is already unlocked.",exit->name);
				return;
			}

			for (objs = crit->inventory; objs; objs = objs->next_content)
			{
				mudlog("OBJS->VNUM: %li .. KEY: %li",
					objs->vnum,exit->key);
				if (objs->vnum == exit->key)
				{
					message("$n unlock$x the $p door.",crit,0,exit->name);
					exit->door = DOOR_CLOSED;
					// unlock the other side!
					if ((room = hashfind_room(exit->to_vnum)))
					{
						for (exit = room->exits; exit; exit = exit->next)
						{
							if (exit->to_vnum == crit->in_room->vnum)
							{
								message("You hear a clicking sound from the $p door.",
									room,0,exit->name);
								exit->door = DOOR_CLOSED;
							}
						}
					}
					return;
				}
			}
			sendcrit(crit,"You do not have the key for that door.");
			return;
		}
	}
	if (!strindex("door", arguments[0]))
		sendcrit(crit,"There is no door here.");
	else
		sendcrit(crit,"There is no door there.");
	return;
}


CMD(do_put)
{
        OBJECT *cobj=0; // container object
	OBJECT *xobj_next=0; // switcher
	OBJECT *hold=0; // holds stuff?
	char *oarg=0, *carg=0;
	long amount=0;

	if(is_number(arguments[0]))
	{
		amount	= atoi(arguments[0]);
		
		if(!arguments[2])
		{
			interpret(crit,"help give");
			return;
		}
		oarg	= arguments[1];
		carg	= arguments[2];
	}
	else
	{
		oarg	= arguments[0];
		carg	= arguments[1];
	}

        cobj = find_obj(crit,carg,OBJ_HELD|OBJ_GROUND|OBJ_EQUIPMENT);
	xobj = find_obj(crit,oarg,OBJ_REQUIRED|OBJ_HELD);
        if (!cobj)
                sendcrit(crit,"You don't have that container.");
        else if (cobj->objtype != OBJ_CONTAINER)
                sendcrit(crit,"That item is not a container.");
        else
        {
		if (!strcasecmp(arguments[0],"all"))
		{
			for (hold = crit->inventory; hold; hold = xobj_next)
			{
				xobj_next = hold->next_content;
				if (hold->objtype == OBJ_CONTAINER)
					continue;
		                message("$n put$x $p inside $N.",crit,cobj,hold);
		                trans(xobj,cobj);

			}
		}
		else if (xobj)
		{
			if(xobj->objtype == OBJ_COIN)
			{
				hold = xobj;
				xobj = splitcoins(hold, cobj, amount);
			}
	                message("$n put$x $p inside $N.",crit,cobj,xobj);
	                trans(xobj,cobj);
		}
		else
			sendcrit(crit,"Put what into what?");
        }
}


CMD(do_give)
{
	char *oarg=0, *carg=0;
	long amount=0;

	if(is_number(arguments[0]))
	{
		amount	= atoi(arguments[0]);
		
		if(!arguments[2])
		{
			interpret(crit,"help give");
			return;
		}
		oarg	= arguments[1];
		carg	= arguments[2];
	}
	else
	{
		oarg	= arguments[0];
		carg	= arguments[1];
	}

	if(!(xobj = find_obj(crit,oarg,OBJ_HELD)))
	{
		sendcrit(crit,"You don't have that object.");
		return;
	}
	if(!(xcrit = find_crit(crit,carg,0)))
	{
		sendcritf(crit,"Give %s to whom?",xobj->name);
		return;
	}

	if(xobj->objtype == OBJ_COIN)
		xobj = splitcoins(xobj, xcrit, amount);

	trans(xobj,xcrit);
	message("$n give$x $p to $N.",crit,xcrit,xobj);
}


// do_get
CMD(do_take)
{
	OBJECT *xobj_next=0;
	OBJECT *cobj=0;  // to hold container object
	char *oarg=0, *carg=0;
	long amount=0;

	if(is_number(arguments[0]))
	{
		amount = atoi(arguments[0]);

		if(!arguments[1])
		{
			interpret(crit,"help take");
			return;
		}
		oarg = arguments[1];
		if(arguments[2])
			carg = arguments[2];
	}
	else
	{
		if (strcasecmp(arguments[0],"all"))
			oarg = arguments[0];
		if(arguments[1])
			carg = arguments[1];
	}

	if(carg) // get something from a container
	{
		cobj = find_obj(crit, carg, OBJ_GROUND|OBJ_HELD);
		if(!cobj || cobj->objtype != OBJ_CONTAINER)
		{
			sendcrit(crit,"There is no container by that name here.");
			return;
		}

		if (oarg)
		{
			for(xobj = cobj->contents; xobj; xobj = xobj->next_content)
			{
				if(!strargindex(xobj->keywords, oarg))
					break;
			}
			if(!xobj)
			{
				sendcritf(crit,"%s does not contain that object.", capitalize(cobj->name));
				return;
			}
		}
	}
	if (!oarg) // "get all" or "get all <container>"
	{

		if (cobj)
			xobj = cobj->contents;
		else
			xobj = crit->in_room->objects;

		if (!xobj && cobj)
		{
			sendcrit(crit,"There is nothing inside that.");
			return;
		}

		for ( ; xobj; xobj = xobj_next)
		{
			xobj_next = xobj->next_content;
			trans(xobj,crit);
			if (cobj)
				message("$n get$x $p from $N.",crit,cobj,xobj);
			else
				message("$n take$x $p.",crit,0,xobj);
		}
		return;
	}

	// no container
	if (!xobj && !(xobj = find_obj(crit, oarg, OBJ_GROUND|OBJ_HELD)))
	{
		sendcrit(crit,"You don't see anything by that name to get.");
		return;
	}

	if (xobj->objtype == OBJ_COIN)
		xobj = splitcoins(xobj,crit,amount);

	trans(xobj,crit);

	if(cobj)
		message("$n take$x $p from $N.",crit,cobj,xobj);
	else
		message("$n take$x $p.",crit,0,xobj);
}


// to retrieve worn table id from keyword   .. -1 if no match
long find_worn(char *worn)
{
	long x = 0;

	for ( ; worn_table[x].keyword != 0 ; x++)
	{
		if (!strcasecmp(worn,worn_table[x].keyword))
			return x;
	}
	return -1;
}


void wear_obj(CREATURE *crit, OBJECT *obj, char *worn)
{
	str_dup(&obj->worn, worn_table[find_worn(worn)].keyword);
	RemoveFromContent(crit->inventory,obj)
	AddToContent(crit->equipment,obj)
	mysql_update_object(obj);
}


// for objs with multiple wear locations: wear obj (wear location)
CMD(do_wear)
{
	OBJECT *eobj, *xobj_next;
	char objarg[MAX_BUFFER];
	char buf[MAX_BUFFER];
	char **okeywords;
	char *p = 0;
	char **places = 0;
	char **wplaces = 0;
	long l = 0;
	long o = 0;
	long y = 0;
	long q = 0;
	long x = 0;
	long z = 0;

	if (strcasecmp(arguments[0],"all") && (!xobj || !ValidString(xobj->wear)))
	{
		sendcrit(crit,"You cannot wear that.");
		return;
	}

	else if (!xobj && !strcasecmp(arguments[0],"all"))
	{
		for (xobj = crit->inventory; xobj; xobj = xobj_next)
		{
			xobj_next = xobj->next_content;
			okeywords = make_arguments(xobj->keywords);
			sprintf(buf,"wear %s",okeywords[0]);
			free_arguments(okeywords);
			interpret(crit,buf);
		}
		return;
	}
	places = make_arguments(xobj->wear);
	objarg[0] = '\0';

	for(x = 0; places[x]; x++)
	{
		if(arguments[1])
 		{
			strcpy(objarg, arguments[1]);
			if(!strcasecmp(arguments[1], places[x]))
				break;
			continue;
		}
		if(ValidString(strlistcmp(worn_table[find_worn(places[x])].worn, crit->wear)))
		{
			sprintf(objarg,"%s",xobj->wear);
			break;
		}
	}

	if(!places[x])
	{
		sendcritf(crit,"You can only wear that object on these locations: %s", xobj->wear);
		free_arguments(places);
		return;
	}

	for(x = 0; places[x]; x++)
	{
		// places this wear spot will take up (for example two handed would take up 2xheld)
		wplaces = make_arguments(worn_table[find_worn(places[x])].worn);
		y = 0;
		for (z = 0; wplaces[z]; z++)
		{
			o = countlist(worn_table[find_worn(places[x])].worn, wplaces[z]); // held x 2 for twohanded
			l = countlist(crit->wear, wplaces[z]); // held x 2 returns 2
			for(eobj = crit->equipment; eobj; eobj = eobj->next_content)
			{
				if (!strargcmp(worn_table[find_worn(eobj->worn)].worn, wplaces[z])
				|| (arguments[1] && strcasecmp(arguments[1],"")
				&& !strargcmp(worn_table[find_worn(eobj->worn)].worn,arguments[1])))
				{
					if (arguments[1] && strcasecmp(arguments[1],""))
						q = countlist(worn_table[find_worn(eobj->worn)].worn, arguments[1]);
					else
						q = countlist(worn_table[find_worn(eobj->worn)].worn, wplaces[z]);
					y += q;  // count worn positions
				}
			}
		}
// l = number of places available on body (head would be one normally... unless you have a two headed mob, then it would be 2)
// o = number of places on body that will be needed for this object
// y = number of places on body being worn currently
// p = the target location on the body to wear this piece of equipment
		if ( l >= (o + y) && (o + y) > -1 && (p = strlistcmp(objarg,places[x])) != 0)
		{
			wear_obj(crit, xobj, p);
			if (xobj->objtype == OBJ_WEAPON)
				sprintf(buf,"$n wield$x %s.",xobj->name);
			else if (!strargcmp(worn_table[find_worn(xobj->worn)].worn,"held"))
				sprintf(buf,"$n hold$x %s.",xobj->name);
			else
				sprintf(buf,"$n wear$x %s on $s %s.",
					xobj->name, worn_table[find_worn(xobj->worn)].name);
			message(buf,crit,0,0);
			free_arguments(places);
			free_arguments(wplaces);
			return;
		}
		free_arguments(wplaces);
	}
	free_arguments(places);
	if (z > 1)
		sendcritf(crit,"You need these slots free: %s",worn_table[find_worn(xobj->wear)].worn);
	else
	{
		if (!strcmp("held",worn_table[find_worn(xobj->wear)].worn))
			sendcrit(crit,"You are already holding too many things.");
		else
			sendcritf(crit,"You are already wearing something on your %s.",worn_table[find_worn(xobj->wear)].worn);
	}
}


void remove_obj(CREATURE *crit, OBJECT *obj)
{
	RemoveFromContent(crit->equipment,obj)
	AddToContent(crit->inventory,obj)
	str_dup(&obj->worn, "");
	mysql_update_object(obj);
}


CMD(do_remove)
{
	char buf[MAX_BUFFER];
	char **okeywords;
	OBJECT *xobj_next;

	if (!xobj && strcasecmp(arguments[0],"all"))
	{
		sendcrit(crit,"You must supply a valid object for this command.");
		return;
	}
	else if (!xobj && !strcasecmp(arguments[0],"all"))
	{
		for (xobj = crit->equipment; xobj; xobj = xobj_next)
		{
			xobj_next = xobj->next_content;
			okeywords = make_arguments(xobj->keywords);
			sprintf(buf,"remove %s",okeywords[0]);
			free_arguments(okeywords);
			interpret(crit,buf);
		}
		return;
	}

	if (xobj->objtype == OBJ_WEAPON)
		sprintf(buf,"$n stop$x wielding $p.");
	else if (!strargcmp(worn_table[find_worn(xobj->worn)].worn,"held"))
		sprintf(buf,"$n stop$x holding $p.");
	else
		sprintf(buf,"$n remove$x $p from $s %s.", worn_table[find_worn(xobj->worn)].name);
	message(buf,crit,0, xobj);
	remove_obj(crit, xobj);
}


CMD(do_buy) // buy bread ..  buy bread baker ..  buy 10 bread baker
{
	SHOP *shop = 0;
	char *p = 0;
	char buf[MAX_BUFFER];
	long amt = 0;
	long quantity = 0;
	long x = 0;

	if (xcrit == crit)
	{
		sendcrit(crit,"You cannot buy items from yourself.");
		return;
	}

	if (!xcrit)
	{
		for (xcrit = crit->in_room->crits; xcrit; xcrit = xcrit->next_content)
		{
			if (crit != xcrit && xcrit->shop)
				break;
		}
		if (!xcrit || !xcrit->shop)
		{
			sendcrit(crit,"There is no shop here.");
			return;
		}
	}
	shop = xcrit->shop;
	if (!shop)
	{
		sendcritf(crit,"%s does not have anything for sale.",xcrit->name);
		return;
	}
	if (is_number(arguments[0]))
	{
		quantity = atoi(arguments[0]);
		if (quantity > 50)
		{
			sendcrit(crit,"You can only buy 50 items at a time.");
			return;
		}
		if (quantity < 1)
		{
			sendcrit(crit,"You must buy at least one of this item.");
			return;
		}
	}

	for (xobj = xcrit->inventory; xobj; xobj = xobj->next_content)
	{
		if (!xobj->worth)
			continue;
		if (quantity)
		{
			if ((p = strlistcmp(arguments[1],xobj->name)) != 0)
				break;
		}
		else
		{
			if ((p = strlistcmp(arguments[0],xobj->name)) != 0)
				break;
		}
	}
	if (!xobj)
	{
		sendcritf(crit,"%s does not have that item for sale.",xcrit->name);
		return;
	}
	// check shop times..
	if (!quantity)
		quantity = 1;
	amt = xobj->worth * shop->buy / 100;
	amt *= quantity;
	if (amt > get_worth(crit))
	{
		sendcrit(crit,"You cannot afford that.");
		return;
	}
	sprintf(buf,"%s",purchase(crit,amt));
	message(buf,crit,xcrit,xobj);
	for (x = 0; x < quantity; x++)
	{
		xobj = new_object(xobj->vnum);
		trans(xobj,crit);
	}
}


CMD(do_sell) // sell bread ..  sell bread baker
{
	SHOP *shop = 0;
	char buf[MAX_BUFFER];
	long amt = 0;
	long quantity = 0;

	if (xcrit == crit)
	{
		sendcrit(crit,"You cannot sell items to yourself.");
		return;
	}

	if (!xcrit)
	{
		for (xcrit = crit->in_room->crits; xcrit; xcrit = xcrit->next_content)
		{
			if (xcrit->shop)
				break;
		}
		if (!xcrit || !xcrit->shop)
		{
			sendcrit(crit,"There is no shop here.");
			return;
		}
	}
	shop = xcrit->shop;
	if (!shop || shop->stype != SHOP_STORE)
	{
		sendcritf(crit,"%s cannot buy anything from you.",xcrit->name);
		return;
	}

	if (!xobj->worth)
	{
		sendcrit(crit,"That item isn't worth anything.");
		return;
	}

	if (xobj->objtype != shop->item)
	{
		sendcrit(crit,"This store doesn't sell those types of items.");
		return;
	}

	// check shop times..

	amt = xobj->worth * shop->sell / 100;
	if (amt > get_worth(xcrit))
	{
		sendcritf(crit,"%s doesn't have enough money to buy that.",xcrit->name);
		return;
	}
	make_worth(xcrit,get_worth(xcrit)-amt);
	make_worth(crit,get_worth(crit)+amt);
	sprintf(buf,"$n give$x %s to $N in exchange for %li $p%s",coin_string(amt),quantity,quantity == 1 ? "":"s");
	message(buf,xcrit,crit,xobj);
	trans(xobj,xcrit);
}


CMD(do_list)  //   list ..    list baker
{
	char buf[MAX_BUFFER]; // dynamic list length.. im bored!
	SHOP *shop	= 0;
	long numb	= 0;
	long len	= 0;

	if (xcrit == crit)
	{
		sendcrit(crit,"Try checking out your inventory.");
		return;
	}

	if (!xcrit)
	{
		for (xcrit = crit->in_room->crits; xcrit; xcrit = xcrit->next_content)
		{
			if (crit != xcrit && xcrit->shop)
				break;
		}
		if (!xcrit || !xcrit->shop)
		{
			sendcrit(crit,"There is no shop here.");
			return;
		}
	}
	shop = xcrit->shop;
	if (!shop || shop->stype != SHOP_STORE)
	{
		sendcritf(crit,"%s does not have anything for sale.",xcrit->name);
		return;
	}

	len = 5; // "Item"
        for (xobj = xcrit->inventory; xobj; xobj = xobj->next_content)
		if(strlen(xobj->name) > len)
			len = strlen(xobj->name);

	sprintf(buf,"%%-%lis %%s",len);
	sendcritf(crit,"%s's store has the following items for sale: ",xcrit->name);
	sendcritf(crit,buf,"Item","Worth");
	sendcritf(crit,buf,"----","-----");

	sprintf(buf,"%%-%lis %%li",len);
	for (xobj = xcrit->inventory; xobj; xobj = xobj->next_content)
	{
		if (xobj->worth > 0)
		{
			numb++;
			sendcritf(crit,buf,xobj->name,xobj->worth);
		}
	}
	if (!numb)
		sendcrit(crit,"\n\rNo items are for sale at this time.");
	else
	        sendcritf(crit,"\n\rA total of %li item%s.",numb,numb == 1 ? "":"s");
}


CMD(do_value) // cost for object housing
{
	SHOP *shop = 0;
	long cpd = 0; // cost per day (if you add it!)

        if (!xcrit)
        {
                for (xcrit = crit->in_room->crits; xcrit; xcrit = xcrit->next_content)
                {
                        if (xcrit->shop)
                                break;
                }
                if (!xcrit || !xcrit->shop)
                {
                        sendcrit(crit,"There is no storage bank here.");
                        return;
                }
        }
        shop = xcrit->shop;
        if (!shop || shop->stype != SHOP_BANK)
        {
                sendcritf(crit,"%s does not house items.",xcrit->name);
                return;
        }

	if (xobj->weight < 10)
		cpd = 1000;
	else
		cpd = (xobj->weight / 10) * 1000;

	sendcritf(crit,"%s tells you 'It will cost %li to house &+C%s&n.'",
		xcrit->name,coin_string(cpd), xobj->name);
}

// do xcrit required.. have them tell them
// what is left to retrieve...  .. check the payment thing
CMD(do_retrieve) // retrieve (obj) (xcrit)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
  	char buf[MAX_BUFFER];
	char *nothing_stored = "You do not have any items stored.";
	long count=0;

	xcrit = find_crit(crit,arguments[1],0);
	if (!xcrit)
	{
		sendcrit(crit,"No one by that name is here.");
		return;
	}

	if (xcrit == crit)
	{
		sendcrit(crit,"You cannot store your own stuff.");
		return;
	}

	if (!flag_isset(xcrit->flags,CFLAG_BANKER))
	{
		sendcrit(crit,"They cannot retrieve objects.");
		return;
	}

	// won't allow containers to be stored
	sprintf(buf,"SELECT * FROM object WHERE owner_id='%li'",crit->id);	

	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);
	if (mysql_num_rows(result) < 1)
	{
		sendcrit(crit,nothing_stored);
		mysql_free_result(result);
		return;
	}

	if (!ValidString(arguments[1]))
	{
		sprintf(buf,"%s asks you, 'Retrieve which item?'\n\r",xcrit->name);
		strcat (buf,"--------------------\n\r");
	}

	while((row = mysql_fetch_row(result)))
	{
		xobj = new_object(atoi(row[O_VNUM]));
		if (!xobj)
			xobj = new_object_proto(atoi(row[O_VNUM]));
		ReadObjectMysql(xobj)

		if (!flag_isset(xobj->flags, OFLAG_STORED))
			free_object(xobj);
		else
		{
			if (!ValidString(arguments[1]))
			{
				sprintf(buf+strlen(buf),"%s\n\r",xobj->name);
				count++;
				free_object(xobj);
			}
			else if (!strargindex(xobj->keywords,arguments[1]))
			{
				trans(xobj,crit);
				message("$n retrieves $p from $N's storage.",crit,xcrit,xobj);
				flag_remove(xobj->flags, OFLAG_STORED);
				fwrite_object(xobj);
				mysql_free_result(result);
				return;
			}
			else
				free_object(xobj);
		}
	}
	if (count)
	{
		sendcrit(crit,buf);
		sendcritf(crit,"A total of %li items stored.",count);
	}
	else
		sendcrit(crit,nothing_stored);
	mysql_free_result(result);
}


CMD(do_store) // command to store an object
{
	long cpd=0;
	char buf[MAX_BUFFER];
	char costbuf[MAX_BUFFER]; 
	     /* this is declared because you can't call the same function
		more than once with sprintf that returns a static char, otherwise you'll get
		the same result.  You can either split it up into two lines or handle it this
		way. */

	if (!flag_isset(xcrit->flags,CFLAG_BANKER))
	{
		sendcrit(crit,"They cannot store objects.");
		return;
	}

	if (xcrit == crit)
	{
		sendcrit(crit,"You can't store your own objects.");
		return;
	}

	if (xobj->objtype == OBJ_COIN)
	{
		sendcrit(crit,"Use deposit to store money.");
		return;
	}

	if (xobj->weight < 10)
		cpd = 10;
	else
		cpd = xobj->weight / 10;

	if (cpd > get_worth(crit))
	{
		sprintf(costbuf,"%s",coin_string(cpd));
		sendcritf(crit,"It costs %s to store %s, and you have %s.",costbuf,
			xobj->name, coin_string(get_worth(crit)));
		return;
	}
	sprintf(buf,"$n give$x %s to $N to house $p.",coin_string(get_worth(crit)));
	make_worth(crit,get_worth(crit)-cpd);
	make_worth(xcrit,get_worth(xcrit)+cpd);
	flag_set(xobj->flags,OFLAG_STORED);
	message(buf,crit,xcrit,xobj);
	sprintf(buf,"$N give$X $n %s in change.",coin_string(get_worth(crit)));
	message(buf,crit,xcrit,0);
	fwrite_object(xobj);
	free_object(xobj);
}


// these aren't quite done yet... these don't save anything to database
// just assumes they have the money actually.
CMD(do_deposit) // dep 50 gold
{
	long amount, cointype =0;
	char buf[MAX_BUFFER];

	if (xobj->objtype != OBJ_COIN)
	{    
		sendcrit(crit,"That is not a coin.");
		return;
	}

        if(is_number(arguments[0]))
        	amount = atoi(arguments[0]);
	else
	{
		sendcrit(crit,"Syntax:  deposit <amount> <cointype>");
		return;
	}
	for (cointype = 0; coin_table[cointype].name; cointype++)
	{
		if (!strcasecmp(coin_table[cointype].name,arguments[1]))
			break;
	}
	if (!coin_table[cointype].name)
	{
		sendcrit(crit,"That is not a valid cointype.\n\rSyntax:  deposit [amount] <cointype>");
		return;
	}
	amount = coin_table[cointype].worth * amount;
	if (amount > get_worth(crit))
	{
		sendcrit(crit,"You do not have that much to deposit");
		return;
	}
	make_worth(crit,get_worth(xcrit)-amount);
	make_worth(xcrit,get_worth(xcrit)+amount);
	sprintf(buf,"$n deposit$x %s in $Z bank.",coin_string(amount));
	message(buf,crit,xcrit,0);
}


CMD(do_withdraw) // withdraw 50 gold
{
	long amount, cointype =0;
	char buf[MAX_BUFFER];

        if(is_number(arguments[0]))
        	amount = atoi(arguments[0]);

	for (cointype = 0; coin_table[cointype].name; cointype++)
	{
		if (!strcasecmp(coin_table[cointype].name,arguments[1]))
			break;
	}
	if (!coin_table[cointype].name)
	{
		sendcrit(crit,"That is not a valid cointype.\n\rSyntax:  withdraw [amount] <cointype>");
		return;
	}
	amount = coin_table[cointype].worth * amount;
	make_worth(crit,get_worth(crit)+amount);
	sprintf(buf,"$n withdraw$x %s from $Z bank.",coin_string(amount));
	message(buf,crit,xcrit,0);
}


////////////
// TABLES //
////////////

// put lowest worth coins first!
const struct coin_t coin_table[] =
{
	{0, "copper",	"&+ycopper&N",		1	},
	{1, "silver",	"silver",		10	},
	{2, "gold",	"&+Ygold&N",		100	},
	{3, "platinum",	"&+Wplatinum&N",	1000	},
	{4, "obsidian",	"&+Lobsidian&N",	10000	},
	{0,0,0,0}
};
