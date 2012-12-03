/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
command.c -- processing commands
*/

#include "stdh.h"
#include <ctype.h>

#include <unistd.h>

/////////////////////
// LOCAL VARIABLES //
/////////////////////
extern long log_all;


/////////////////////
// LOCAL FUNCTIONS //
/////////////////////
void call_social	(SOCIAL *social, CREATURE *crit, OBJECT *xobj, CREATURE *xcrit);


///////////////
// FUNCTIONS //
///////////////
void interpret(CREATURE *crit, char *buf)
{
	CREATURE *xcrit=0;
	OBJECT *obj=0;
	SOCIAL *social=0;
	EXIT *exit=0;
	ROOM *room=0;
	char command[MAX_BUFFER];
	char temp[MAX_BUFFER];
	char *pbuf = command;
	char **arguments;
	char **editargs;
	long i = 0, x = 0;
	long string = 0;
	bool found = 0;
	bool command_ok = 0;
	bool social_ok = 0;

	strcpy(temp,buf);

	while( isspace(*buf) )
		buf++;

	// check for one character commands without spaces to 
	// seperate arguments. - i.e. chat yo = .yo | pip
	if(ispunct(*buf))
		*pbuf++ = *buf++;
	else
	{
		while( !isspace(*buf) && *buf != '\0' )
			*pbuf++ = *buf++;
	}
	*pbuf = '\0';

	while( isspace(*buf) )
		buf++;

	// moved exits before other commands - pip.
	// insert full exit name for abbreviated one.
	for( i = 0; directions[i].abbr; i++)
	{
		if (directions[i].abbr[0] != '\0' && !strcasecmp(command,directions[i].abbr))
		{
			sprintf(command,"%s",directions[i].name);
			break;
		}
	}

	if(!IsDisabled(crit))
	{
		for(exit = crit->in_room->exits; exit; exit = exit->next )
		{
			if( !strindex(exit->name, command) )
			{
				if((room = hashfind_room(exit->to_vnum)) == 0)
				{
					sendcrit(crit,"That exit enters into a domain far too powerful for you to handle.");
					mudlog("exit(%s) in room(%s:%d) has bad to_vnum(%d)!",
						exit->name, crit->in_room->name, crit->in_room->vnum,
						exit->to_vnum);
					continue;
				}

				if (exit->door >= DOOR_CLOSED && !IsImmortal(crit))
				{
					sendcritf(crit,"The %s door is closed.",exit->name);
					return;
				}
				if (crit->rider)
				{
					sendcritf(crit,"You can't move on your own until %s dismounts you.",crit->rider->name);
					return;
				}
				// adding mounts!
				if (crit->mount)
					message("$n ride$x $p on $N.",crit,crit->mount,exit->name);
				else					
					message("$n leave$x $p.",crit,0,exit->name);
				trans(crit,room);
				if (crit->mount)
				{
					trans(crit->mount,crit);
					message("$n arrive$x riding $N.",crit,crit->mount,crit->in_room);
				}
				message("$n $v arrived.",crit,crit->in_room,0);
				interpret(crit,"look");
				return;
			}
		}
	}

	// check if they in editor and get a successful return (edited something)
	// otherwise let them use normal commands
	if(IsEditing(crit))
	{
		if(crit->socket->string)
		{
			if(!(string = string_editor(crit,temp)))
				return;
			
			str_dup(&(*crit->socket->variable), crit->socket->string);
			DeleteObject(crit->socket->string)
			return;
		}
		else
			temp[0] = '\0';

		if(!ValidString(command))
			strcat(temp,"");
		else
			sprintf(temp,"%s %s",command,buf);

		editargs = make_arguments(temp);
		if(editor(crit,editargs))
		{
			free_arguments(editargs);
			return;
		}
		free_arguments(editargs);
	}

	// reinitialize
	i = 0;

	while(command_table[i].function != 0)
	{
		if(crit->level >= command_table[i].level
		&& !strindex(command_table[i].command, command) ) // command found
		{
			found		= 1;
			command_ok	= 1;
			break;
		}
		i++;
	}

	if(!found && !IsDisabled(crit))
	{
		if((social=hashfind_social(command)))
		{
			social_ok = 1;
			found = 1;
		}
	}
	else
	{
		if(crit->state > command_table[i].state)
		{
			sendcrit(crit,"Your current state prevents you from doing that.");
			return;
		}
		if(crit->position > command_table[i].position)
		{
			sendcrit(crit,"You are in no position to do that!");
			return;
		}
		if(!IsPlayer(crit) && command_table[i].level >= LEVEL_IMM)
		{
			sendcrit(crit,"NPC's cannot use immortal powers.");
			return;
		}
	}

	// GET ALL THE ARGUMENTS AND PASS THE ARRAY
	// make sure buffer is empty, or else you get an invalid string as first argument
	pbuf = &temp[0];
	while(*buf != '\0') *pbuf++ = *buf++;
	*pbuf = '\0';
	arguments = make_arguments(temp);

	for(x=0;arguments[x];x++)
		;
	if(x < command_table[i].arguments)
	{
		sendcritf(crit,"The '%s' command requires at least %li argument%s.\n\r"
			       "Type \"&+Whelp %s&N\" for syntax and description.",
			command_table[i].command, 
			command_table[i].arguments, command_table[i].arguments == 1 ? "" : "s", 
			command_table[i].command);
		free_arguments(arguments);
		return;
	}

	// find the possible object/creature to pass to the func
	if(command_ok && command_table[i].extra)
	{
		if(IsSet(command_table[i].extra, CRIT_REQUIRED))
		{
			if(!arguments[0] || (xcrit = find_crit(crit,arguments[0],command_table[i].extra)) == 0)
			{
				sendcrit(crit,"You must supply a valid creature for this command.");
				command_ok = 0;
			}
		}
		if(IsSet(command_table[i].extra, OBJ_REQUIRED)
		&& (!xcrit || (xcrit && IsSet(command_table[i].extra, OBJ_CRIT_BOTH))))
		{
			if(!arguments[0] || (obj = find_obj(crit,arguments[0],command_table[i].extra)) == 0)
			{
				sendcrit(crit,"You must supply a valid object for this command.");
				command_ok = 0;
			}
		}
		if(arguments[0] && IsSet(command_table[i].extra, CRIT_POSSIBLE) && !xcrit)
			xcrit = find_crit(crit,arguments[0],command_table[i].extra);
		if(arguments[0] && IsSet(command_table[i].extra, OBJ_POSSIBLE) && !obj
		&& (!xcrit || (xcrit && IsSet(command_table[i].extra, OBJ_CRIT_BOTH))))
		{
			obj = find_obj(crit,arguments[0],command_table[i].extra);
		}
		if(IsSet(command_table[i].extra, ONE_REQUIRED) && !obj && !xcrit)
		{
			sendcrit(crit,"You must supply either an object or creature for this command.");
			command_ok = 0;
		}
	}
	if(social_ok && arguments[0])
	{
		xcrit = find_crit(crit,arguments[0],0);
		obj = find_obj(crit,arguments[0],OBJ_HELD|OBJ_GROUND);
	}

	// logging
	if(command_table[i].log != LOG_NEVER
	&& (command_table[i].log == LOG_YES || log_all || flag_isset(crit->flags, CFLAG_LOG)) )
		mudlog("Logging %-15s -> %s %s", crit->name, command, all_args(arguments,0));

	if(found && command_ok)
	{
		(*command_table[i].function) (crit, arguments, obj, xcrit);
	}
	else if(found && social_ok)
	{
		if(arguments[0] && !xcrit && !obj)
		{
			strcpy(command, social->name);
			command[0] = Upper(command[0]);
			sendcritf(crit,"%s at what?", command);
		}
		else
			call_social(social, crit, obj, xcrit);
	}

	if(found || IsEditing(crit))
	{
		free_arguments(arguments);
		return;
	}

	if(ValidString(command))
	{
		mudlog("name: %s || invalid command: %s %s",crit->name,command,all_args(arguments,0));
		sendcrit(crit, "No such command.");
	}

	free_arguments(arguments);
}


// simply returns the last valid argument in a list of arguments
long last_arg(char **arguments)
{
	long i=0;
	while(arguments[i]) i++;
	return i-1;
}


// This copies an array of arguments into buf. {"this", "is", "neat"} will become "this is neat"
// it will work in all the DO_ functions.
char *all_args(char **arguments, long start)
{
	static char buf[MAX_BUFFER];
	long i	= 0;
	buf[0]	= '\0';

	if(start < 0)
		return buf;

	while(i < start)
	{
		if(!arguments[i++])
			return buf;
	}

	while( arguments[i] && (strlen(buf)+strlen(arguments[i]) < (MAX_BUFFER-1)) )
	{
		strcat(buf,arguments[i++]);
		strcat(buf," ");
	}
	// strip last space
	buf[strlen(buf)-1] = '\0';
	return buf;
}


// this splits up a list of arguments.. similar to all_args but with an endpoint specified
char *split_args(char **arguments, long start, long end)
{
	static char buf[MAX_BUFFER];
	long i	= 0;
	buf[0]	= '\0';

	if(start > end || start < 0 || end < 0)
		return buf;

	while(i < start)
	{
		if(!arguments[i++])
			return buf;
	}

	while(i <= end && arguments[i] && (strlen(buf)+strlen(arguments[i]) < (MAX_BUFFER-1)) )
	{
		strcat(buf,arguments[i++]);
		strcat(buf," ");
	}

	// strip last space
	buf[strlen(buf)-1] = '\0';
	return buf;
}


void call_social(SOCIAL *social, CREATURE *crit, OBJECT *xobj, CREATURE *xcrit)
{
	if(xcrit && xcrit!=crit && social->crit_target)
		message(social->crit_target,crit,xcrit,0);
	else if(xobj && social->object_target)
		message(social->object_target,crit,xobj,xobj);
	else if(xcrit && social->self_target)
		message(social->self_target,crit,crit,0);
	else if(social->no_target)
		message(social->no_target,crit,0,0);
}


//////////////////
// DO_FUNCTIONS //
//////////////////
CMD(do_commands)
{
	char buf[MAX_BUFFER];
	long i, total;

	sprintf(buf,"Commands\n\r--------\n\r");
	for( i = 0, total = 0; command_table[i].function; i++ )
	{
		if(command_table[i].level > crit->level)
			continue;
		sprintf(buf+strlen(buf),"%-10s%c",command_table[i].command,'\t');
		if( (i+1) % 5 == 0 )
			strcat(buf,"\n\r");
		total++;
	}
	sprintf(buf+strlen(buf),"\n\r\n\r%li total commands.", total);
	sendcrit(crit,buf);
}


CMD(do_save)
{
	char buf[MAX_BUFFER];
	static AREA *area = 0;

	if(arguments[0])
	{
		if(!area || !area->next)
			area = area_list;
		else
			area = area->next;

		sprintf(buf,"Saving %s area...",area->name);
		send_immediately(crit->socket,buf);
		fwrite_area(area, 1);
	}

	if(crit->socket->save_time)
	{
		sendcrit(crit,"Wait a minute before you save again.");
		return;
	}

	if(!IsImmortal(crit))
		crit->socket->save_time = 20;

//		pthread_create(&save_thread, 0, (void*)check_save ,0);

	fwrite_player(crit);
//	sendcrit(crit,"You have been saved.");
}
