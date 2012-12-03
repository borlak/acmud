/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
update.c -- timers and what not
*/

#include "stdh.h"

///////////////
// FUNCTIONS //
///////////////
// this update is for backing up the MySQL database, roughly once a day since
// the last time the database was saved
void backup_update(void)
{
	long difference = current_time - mudtime.backup;

	if(difference >= 60*60*BACKUP_HOURS) // 24 hours default
		backup_mud();
}


void creature_update(void)
{
	CREATURE *crit=0;
	CREATURE *crit_next=0;
	char buf[MAX_BUFFER];
	static int counter=0;

	counter++; // tick counter

	for(crit = creature_list; crit; crit = crit_next)
	{
		crit_next = crit->next;

		// don't process anything on mobs that are in areas w/no players (idle area)
		if(!IsPlayer(crit) && crit->in_room->area->players < 1)
			continue;

/*		if(counter % 4 == 0 && IsPlayer(crit)
		&& flag_isset(crit->flags, CFLAG_ANTIIDLE))
			sendcrit(crit,"Anti-Idle");
*/
		if (counter % 4 == 0 && IsPlayer(crit) && IsPlaying(crit)) // No-Op! 0
		{
			buf[0] = '\0';
			send_to(crit->socket,buf);
		}
	}
}


void object_update(void)
{
	OBJECT *obj=0;
	OBJECT *obj_next=0;
	ROOM *room=0;
	char *action=0;

	for(obj = object_list; obj; obj = obj_next)
	{
		obj_next = obj->next;

		if(obj->timer && --obj->timer == 0)
		{
			room = obj->in_room ? obj->in_room : obj->held_by ? obj->held_by->in_room : 0;
			if(room)
			{
				switch(obj->objtype)
				{
				case OBJ_DRINK:	action = "dries up"; break;
				case OBJ_FOOD:	action = "rots away"; break;
				case OBJ_LIGHT:	action = "goes out for good"; break;
				default:	action = "crumbles to dust"; break;
				}
				message("$n $p.",obj,room,action);
			}
			free_object(obj);
		}
	}
}


void reset_nexts(RESET *reset, CREATURE *crit, OBJECT *obj)
{
	OBJECT *xobj, *container;
	RESET *rnext;
	char buf[MAX_BUFFER];
	char min=0;

	for(rnext = reset->next; rnext; rnext = rnext->next)
	{
		if(rnext->loadtype != TYPE_OBJECT
		|| !rnext->crit)
			break;

		min = rnext->min;
		while(min > 0)
		{
			xobj		= new_object(reset->obj->vnum);
			xobj->reset	= rnext;
			rnext->loaded++;
			trans(xobj,crit?(void*)crit:(void*)obj);
	
			if(crit)
			{
				sprintf(buf,"wear %s",rnext->command);
				interpret(crit,buf);
			}
			if(xobj->objtype == OBJ_CONTAINER)
				container = xobj;
			min--;
		}
	}
}
void reset_update(void)
{
	CREATURE *crit=0;
	OBJECT *obj=0;
	RESET *reset=0, *reset_next=0;
	EXIT *exit=0;
	long min=0;

	for(reset = hash_reset[(current_time)%HASH_KEY]; reset; reset = reset_next)
	{
		reset_next = reset->next_hash;

		if(current_time < reset->poptime)
			continue;

		if( percent() < reset->chance
		|| (reset->max && reset->loaded >= reset->max)
		|| reset->poptime == 0)
		{
			add_reset(reset);
			continue;
		}
	
		obj = 0;
		crit = 0;
	
		min = reset->min;
		while(min > 0 && reset->loaded < reset->max)
		{
			switch(reset->loadtype)
			{
			case TYPE_CREATURE:
				crit		= new_creature(reset->crit->vnum);
				crit->reset	= reset;
				reset->loaded++;
				trans(crit,reset->room);
				break;
			case TYPE_OBJECT:
				obj		= new_object(reset->obj->vnum);
				obj->reset	= reset;
				reset->loaded++;
				trans(obj,reset->room);
				break;
			case TYPE_EXIT:
				if(!(exit = find_exit(reset->room, reset->command)))
				{
					mudlog("RESET_UPDATE: no exit found for reset#%li",reset->id);
					break;
				}
				exit->door = reset->loaded;
				break;
			}
			min--;
		}

		reset_nexts(reset,crit,obj);
		add_reset(reset);
	}
}


void heal_update(void)
{
	CREATURE *crit=0;

	for(crit = creature_list; crit; crit = crit->next)
	{
		if(crit->hp < -4)
			hurt(crit,1);
		else if(crit->hp < crit->max_hp)
			heal(crit,1);

		if(crit->move < crit->max_move && crit->hp > 0)
			crit->move++;

		position_check(crit);
	}
}


// this just updates anything on sockets only every second
void socket_update()
{
	MSOCKET *socket;
	for(socket = socket_list; socket; socket = socket->next)
	{
		if(socket->save_time > 0)
			socket->save_time--;
	}
}


void mud_update(void)
{
	static long creature_time = 0;
	static long object_time = 0;
	static long reset_time = 0;
	static long mysql_time = 0;
	static long mudtime_time = 0;
	static long backup_time = 0;
	static long heal_time = 0;
	static long socket_time = 0;

	if( ++socket_time % (LOOPS_PER_SECOND) == 0 )
		socket_update();
	
	if( ++heal_time % (LOOPS_PER_SECOND*5) == 0 )
		heal_update();	

	if( ++creature_time % (LOOPS_PER_SECOND*60) == 0 )
		creature_update();

	if( ++object_time % (LOOPS_PER_SECOND) == 0 )
		object_update();

	if( ++reset_time % (LOOPS_PER_SECOND) == 0 )
		reset_update();

	if( ++mysql_time % (LOOPS_PER_SECOND*60*60) == 0 )
		mudlog("Pinging mysql server == %s", mysql_ping(mysql) == 0 ? "Connected" : "Disconnected");

	if( ++mudtime_time % (LOOPS_PER_SECOND) == 0 )
	{
		if(mudtime_time % (LOOPS_PER_SECOND*60*15) == 0) // 15 minutes
			fwrite_time();
		mudtime.age++;
	}

	if( ++backup_time % (LOOPS_PER_SECOND*60*60) == 0) // check at startup and every hour
		backup_update();
}


