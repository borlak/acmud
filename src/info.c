/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
info.c:  Informational functions.  Things that return information about any
object in the mud.
*/

#include <ctype.h>
#include "stdh.h"

/////////////////////
// LOCAL VARIABLES //
/////////////////////
char * const heshe[] = { "it", "he", "she" };
char * const himher[] = { "it", "him", "her" };
char * const hisher[] = { "its", "his", "her" };

char * const months[] = { "Sojourn", "PK", "Mortal Conquest", "Umgw", "Groundzero", "Evil in the Extreme",
			  "Aliens", "Predators", "Genocide", "Armageddon", "Perilous Realms", "The Three Kingdoms",
			0};
char * const days[] = { "Rip", "Nubrigol", "Durf", "Kalera", "Khor", "Vraal", "Andrade", "Knife", "Basec",
			"Crystal", "Brucelee", "Stab", "Gellig", "Vanidor", "Lazloth", "Jake", "Fate",
			"Karbeck", "Freddie", "Onion", "Howler", "Orsund", "Zrouli", "Claudia", "Jihad",
			"Aretea", "Anechka", "Ung", "Woham", 0};

/////////////////////
// LOCAL FUNCTIONS //
/////////////////////
char *edit_names(CREATURE *crit)
{
	static char buf[MAX_BUFFER];
	char *type;

	buf[0] = '\0';

	switch(*(unsigned int*)crit->socket->editing)   
        {
        case TYPE_CREATURE:	type = "Creature";	break;
        case TYPE_OBJECT:	type = "Object";	break;
        case TYPE_ROOM:		type = "Room";		break;
        case TYPE_AREA:		type = "Area";		break;
        case TYPE_RESET:	type = "Reset";		break;
        case TYPE_HELP:		type = "Help";		break;
        case TYPE_SOCIAL:	type = "Social";	break;
        case TYPE_NOTE:		type = "Note";		break;
        default:		type = "";		break;
	}
	sprintf(buf,"&+Y(%s Editor)&N",type);
	return buf;
}

char *exit_names(ROOM *room, bool prompt)
{
	EXIT *exit;
	static char buf[MAX_BUFFER];
	static char tobuf[MAX_BUFFER];

	buf[0] = '\0';
	for(exit = room->exits; exit; exit = exit->next)
	{
		sprintf(tobuf,": %li",exit->to_vnum);
		if (exit->door == DOOR_NONE)
			sprintf(buf+strlen(buf),"%s%s ",exit->name,prompt?tobuf:"");
		else if (exit->door == DOOR_OPEN)
			sprintf(buf+strlen(buf),"&+G]&+W%s&+G[&+W%s ",exit->name,prompt?tobuf:"");
		else
			sprintf(buf+strlen(buf),"&+R[&+W%s&+R]&+W%s ",exit->name,prompt?tobuf:"");
	}
	if(!room->exits)
		sprintf(buf,"no exits");
	else
		buf[strlen(buf)-1] = '\0'; // strip last space
	return buf;
}

char *show_equipment(CREATURE *crit)
{
	static char buf[MAX_BUFFER];
	OBJECT *obj;

	sprintf(buf,"  %s's equipment: \n\r", crit->name);

	if(!crit->equipment)
		sprintf(buf+strlen(buf),"Nothing.\n\r");
	else
	{
		for (obj = crit->equipment; obj; obj = obj->next_content)
		{
			sprintf(buf+strlen(buf),"<&+W%20s&n> %s\n\r",
				worn_table[find_worn(obj->worn)].name, obj->name);
		}
	}	
	return buf;
}


CMD(do_debug)
{
ROOM *room=0;
char buf[MAX_BUFFER];
CREATURE *critx=0;

if (crit->in_room)
	room = crit->in_room;
else
	return;

buf[0]='\0';
sendcrit(crit,"Crits at start");
for(critx = room->crits; critx; critx = critx->next_content)
                sprintf(buf+strlen(buf),"%s%s",critx->name,
                        critx->next_content?", ":"");
                sprintf(buf+strlen(buf),"&N]");
                sendcrit(crit,buf);
RemoveFromContent(room->crits,crit);
sendcrit(crit,"Crits after removing You.");
buf[0]='\0';

 for(critx = room->crits; critx; critx = critx->next_content)
                sprintf(buf+strlen(buf),"%s%s",critx->name,
                        critx->next_content?", ":"");
                sprintf(buf+strlen(buf),"&N]");
                sendcrit(crit,buf);
AddToContent(room->crits,crit);
sendcrit(crit,"Crits after adding you.");
buf[0]='\0';

 for(critx = room->crits; critx; critx = critx->next_content)
                sprintf(buf+strlen(buf),"%s%s",critx->name,
                        critx->next_content?", ":"");
                sprintf(buf+strlen(buf),"&N]");
                sendcrit(crit,buf);
trans(crit,room);
sendcrit(crit,"Crits after transing you.");
buf[0]='\0';

 for(critx = room->crits; critx; critx = critx->next_content)
                sprintf(buf+strlen(buf),"%s%s",critx->name,
                        critx->next_content?", ":"");
                sprintf(buf+strlen(buf),"&N]");
                sendcrit(crit,buf);
}


void ansicap(char *str)
{
	char *p = str;

	while(*p != '\0')
	{
		if(*p == '&')
		{
			p++;
			switch(*p)
			{
			case '\0': return;
			case '+':
			case '-':
				p++; if(*p == '\0') return;
				p++; if(*p == '\0') return;
				*p = Upper(*p);
				return;
			case '=':
				p++; if(*p == '\0') return;
				p++; if(*p == '\0') return;
				p++; if(*p == '\0') return;
				*p = Upper(*p);
				return;
			default: return;
			}
		}
		if(isalpha(*p))
		{
			*p = Upper(*p);
			return;
		}
		p++;
	}
}

// MESSAGE -- main messaging utility for the mud.  Can message everyone in room, and you can
// use "triggers", for lack of a better word, so the messages make more sense.  An example:
// message("$n hit$x $N with $p!",crit,xcrit,obj);  --> the $n is the actor, all the lowercase
// triggers refer to the actor, and upper case refers to the stage.  $p refers to the extra.
// This would create three messages->
// ACTOR(You hit Pip with a big pointy stick!)
// STAGE(Borlak hits you with a big pointy stick!)
// EXTRA(Borlak hits Pip with a big pointy stick!)
//
// Notice what changed.. The 's' on "hits" is removed for the actor.  You may supply
// a character string for $p. for example, a door exit (borlak exits west...)
//
// $n/$N->name of actor/stage, $s/$S->his/her, $m/$M->him/her, $x/$X->s or es
// $r/$R->is/are $t/$T->your/their $z/$Z->your/Borlak's
// world->1 for all players, 2 for all players get the message _immediately_ (shutdown)
void create_message(char *buf, void *actor, void *stage, void *extra, bool world)
{
	char bufactor[MAX_BUFFER*2];
	char bufstage[MAX_BUFFER*2];
	char bufextra[MAX_BUFFER*2];
	char actor_name[MAX_BUFFER];
	char stage_name[MAX_BUFFER];
	char extra_name[MAX_BUFFER];
	char temp[24];
	CREATURE *crit = 0;
	CREATURE *xcrit = 0;
	CREATURE *ppl = 0;
	MSOCKET *sock = 0;
	OBJECT *obj = 0;
	OBJECT *xobj = 0;
	ROOM *room = 0;
	char *abuf;
	char *sbuf;
	char *ebuf;
	char *acopy;
	char *scopy;
	char *ecopy;
	long to_room_only = 0;
	long possessive = 0;

	actor_name[0]	= '\0';
	stage_name[0]	= '\0';
	extra_name[0]	= '\0';
	bufactor[0]	= '\0';
	abuf		= bufactor;
	bufstage[0]	= '\0';
	sbuf		= bufstage;
	bufextra[0]	= '\0';
	ebuf		= bufextra;
	acopy		= "";
	scopy		= "";
	ecopy		= "";

	if(actor)
	{
		switch(*(unsigned int*)actor)
		{
		case TYPE_CREATURE:
			crit = (CREATURE*)actor;
			room = crit->in_room;
			strcpy(actor_name,crit->name);
			break;
		case TYPE_OBJECT:
			obj = (OBJECT*)actor;
			     if(obj->in_room)	room = obj->in_room;
			else if(obj->held_by)	room = obj->held_by->in_room;
			else if(obj->in_obj)
			{
				if(obj->in_obj->in_room)	room = obj->in_obj->in_room;
				else if(obj->in_obj->held_by)	room = obj->in_obj->held_by->in_room;
			}
			if(obj->objtype == OBJ_COIN)
					sprintf(actor_name,"some %s",coin_table[obj->values[0]].ansi_name);
			else
				strcpy(actor_name,obj->name);
			break;
		case TYPE_ROOM:
			room = (ROOM*)actor;
			strcpy(actor_name,room->name);
			break;
		default:
			mudlog("MESSAGE(%s): bad actor type!",buf);
			return;
		}
	}
	if(stage)
	{
		switch(*(unsigned int*)stage)
		{
		case TYPE_CREATURE:
			xcrit = (CREATURE*)stage;
			strcpy(stage_name,xcrit->name);
			break;
		case TYPE_OBJECT:
			xobj = (OBJECT*)stage;
			if(xobj->objtype == OBJ_COIN)
				sprintf(stage_name,"some %s",coin_table[xobj->values[0]].ansi_name);
			else
				strcpy(stage_name,xobj->name);
			break;
		case TYPE_ROOM:
			// decided to make TYPE_ROOM stages go NOT to the crit.
			// so basically, you have a crit that doesn't get message.
			to_room_only = 1;
			// mudlog("MESSAGE(%s): can't have room as stage!",buf);
			break;
		default:
			mudlog("MESSAGE(%s): bad stage type!",buf);
			return;
		}
	}
	if(extra)
	{
		switch(*(unsigned int*)extra)
		{
		case TYPE_CREATURE:
			mudlog("MESSAGE(%s): can't have creature as extra!",buf);
			break;
		case TYPE_OBJECT:
			xobj = (OBJECT*)extra;
			if(xobj->objtype == OBJ_COIN)
				sprintf(extra_name,"some %s",coin_table[xobj->values[0]].ansi_name);
			else
				strcpy(extra_name,xobj->name);
			break;
		case TYPE_ROOM:
			mudlog("MESSAGE(%s): can't have room as extra!",buf);
			break;
		default:
			strcpy(extra_name,(char*)extra);
			break;
		}
	}

	for(; *buf != '\0'; buf++)
	{
		switch(*buf)
		{
		case '$':
			buf++;
			switch(*buf)
			{
			case '\0':
				break;
			case 'e':
				acopy = "you";
				scopy = ecopy = heshe[crit->sex];
				break;
			case 'E':
				scopy = "you";
				if (xcrit)
					acopy = ecopy = heshe[xcrit->sex];
				else
					acopy = ecopy = "their";
				break;
			case 'm':
				acopy = "your";
				scopy = ecopy = himher[crit->sex];
				break;
			case 'M':
				scopy = "your";
				if (xcrit)
					acopy = ecopy = himher[xcrit->sex];
				else
					acopy = ecopy = "their";
				break;
			case 'n':
				acopy = "you";
				scopy = ecopy = actor_name;
				break;
			case 'N':
				acopy = ecopy = stage_name;
				scopy = "you";
				break;
			case 'z':
				acopy = "your";
				scopy = ecopy = actor_name;
				possessive = 1;
				break;
			case 'Z':
				acopy = ecopy = stage_name;
				scopy = "your";
				possessive = 2;
				break;
			case 'g':
			case 'G':
				acopy = "go";
				scopy = ecopy = "goes";
				break;
			case 'd':
			case 'D':
				acopy = "don't";
				scopy = ecopy = "doesn't";
				break;
			case 'p':
			case 'P':
				acopy = scopy = ecopy = extra_name;
				break;
			case 'r':
			case 'R':
				ecopy = "is";
				scopy = acopy = "are";
				break;
			case 's':
				acopy = "your";
				scopy = ecopy = hisher[crit->sex];
				break;
			case 'S':
				scopy = "your";
				if (xcrit)
					acopy = ecopy = hisher[xcrit->sex];
				else
					acopy = ecopy = "their";
				break;
			case 'y':
				acopy = "y";
				scopy = ecopy = "ies";
				break;
			case 't':
			case 'T':
				acopy = scopy = "your";
				ecopy = "their";
				break;
			case 'x':
				acopy = "";
				scopy = ecopy = "s";
				break;
				
			case 'X':
				acopy = "s";
				scopy = ecopy = "";
				break;
			case 'v':
			case 'V':
				ecopy = "has";
				scopy = acopy = "have";
				break;
			case 'b':
				acopy = "";
				scopy = ecopy = "es";
				break;
			case 'B':
				acopy = "es";
				scopy = ecopy = "";
				break;
			default:
				sprintf(temp, "%c%c", *(buf-1), *buf);
				acopy = scopy = ecopy = temp;
				break;
			}
			while(*acopy != '\0')
				*abuf++ = *acopy++;
			while(*scopy != '\0')
				*sbuf++ = *scopy++;
			while(*ecopy != '\0')
				*ebuf++ = *ecopy++;

			if(possessive == 1)
			{
				*sbuf++ = '\''; *sbuf++ = 's';
				*ebuf++ = '\''; *ebuf++ = 's';
				possessive = 0;
			}
			else if(possessive == 2)
			{
				*abuf++ = '\''; *abuf++ = 's';
				*ebuf++ = '\''; *ebuf++ = 's';
				possessive = 0;
			}

			break;
		default:
			*abuf++ = *buf;
			*sbuf++ = *buf;
			*ebuf++ = *buf;
			break;
		}
	}
	*abuf = '\0';
	*sbuf = '\0';
	*ebuf = '\0';

	ansicap(bufactor);
	ansicap(bufstage);
	ansicap(bufextra);

	if (world)
	{
		for (sock = socket_list; sock; sock = sock->next)
		{
			ppl = sock->pc;

			if(!ppl || !IsPlaying(ppl))
				continue;

			if (ppl == crit)
			{
				if(world > 1 && IsPlayer(crit))
					send_immediately(crit->socket,bufactor);
				else
					sendcrit(crit,bufactor);
			}
			else
			{
				if(world > 1 && IsPlayer(ppl))
					send_immediately(ppl->socket,bufstage);
				else
					sendcrit(ppl,bufstage);
			}
		}
		return;
	}

	for(ppl = room->crits; ppl; ppl = ppl->next_content)
	{
		if(!IsPlaying(ppl) || ppl->state >= STATE_SLEEPING)
			continue;

		if(ppl == crit)
		{
			if(!to_room_only)
				sendcrit(crit,bufactor);
		}
		else if(ppl == xcrit)
			sendcrit(xcrit,bufstage);
		else
			sendcrit(ppl,bufextra);
	}
}


char *coin_string(long worth)
{
	static char cbuf[MAX_BUFFER];
	long coins=0;
	long x;

	for(x = 0; coin_table[x].name; x++)
		;

	cbuf[0] = '\0';
	for(x--; x>=0 ; x--)
	{
		coins = worth/coin_table[x].worth;
		worth -= coins*coin_table[x].worth;

		if(!coins)
			continue;

		sprintf(cbuf+strlen(cbuf),"%li %s, ",coins, coin_table[x].ansi_name);
	}
	if(!ValidString(cbuf))
		strcpy(cbuf,"no coins");
	else
		cbuf[strlen(cbuf)-2] = '\0'; // strip comma and space

	return cbuf;
}

////////////////////////////
// TIME RELATED FUNCTIONS //
////////////////////////////
struct mudtime_t mudtime;

// find out what time it is for the mud.. this saves over reboots.. you can decide how
// many seconds are in a minute, how many minutes are in an hour, etc.. and what year
// the mud should start on
char *get_mudtime(void)
{
	static char buf[MAX_BUFFER];
	long age = mudtime.age;

	// what year is it
	mudtime.year = STARTING_YEAR + ((age/SECONDS_DAY) / (mudtime.days*mudtime.months));
	age -= SECONDS_DAY*(mudtime.days*mudtime.months*(mudtime.year-STARTING_YEAR));

	// month
	mudtime.month = Max(0, age/(mudtime.days*SECONDS_DAY));
	age -= SECONDS_DAY*mudtime.days*mudtime.month;

	// day
	mudtime.day = Max(0, age/SECONDS_DAY);
	age -= SECONDS_DAY*mudtime.day;

	// hours
	mudtime.hour = Max(0, age/(SECONDS_MINUTES*MINUTES_HOURS));
	age -= SECONDS_MINUTES*MINUTES_HOURS*mudtime.hour;

	// minutes
	mudtime.minute = Max(0, age/SECONDS_MINUTES);
	age -= SECONDS_MINUTES*mudtime.minute;

	// your mud may or may not have seconds, depending on how you set up time
	mudtime.second = Max(0, age);

	sprintf(buf,"Year %li, Month of %s, Day of %s.\n\rThe time is %s%li:%s%li",
		mudtime.year, months[mudtime.month], days[mudtime.day],
		mudtime.hour > 9 ? "" : "0", mudtime.hour,
		mudtime.minute > 9 ? "" : "0", mudtime.minute
		/*, mudtime.second*/ );

	return buf;
}

// system time on this machine was off by 419 seconds.
// you will probably be able to drop this.
char *display_timestamp(time_t time, long gmt)
{
	static char buf[MAX_BUFFER];
	
	time = time - 2219; // bad server time.
	time = time + (6 * 3600); // reset to GMT
	time = time + (gmt * 3600); // adjust for configurable time.
	strftime(buf, MAX_BUFFER, "%a: %b %d, %Y - %r",localtime(&time));
	return buf;
}

//////////////////
// DO FUNCTIONS //
//////////////////
CMD(do_areas)
{
	char buf[MAX_BUFFER];
	char sep[256];
	AREA *area;
	long i, total=1;

	sendcritf(crit,"%5s %3s %-30s  %-15s  %-5s  %-5s  %-5s\n\r",
		"Num", "Ppl", "Area Name", "Builder(s)", "LVnum", "HVnum", "Total");

	for(area = area_list; area; area = area->next, total++)
	{
		for(i = 0; i < (long)(32-strlen(area->name)); i++)
			sep[i] = '.';
		sep[i] = '\0';

		sprintf(buf, "[%3li] %3li %s%s%-15s  %-5li  %-5li  %-5li",
			total, area->players, area->name, sep, area->builders,
			area->low, area->high, area->high-area->low);
		sendcrit(crit,buf);
	}
}

CMD(do_time)
{
	double uptime;
	long hrs;
	long days, hours, minutes, seconds;

	uptime = current_time - mudtime.start;
	days = (long) uptime / 86400;
	uptime -= 86400 * days;
	hours = (long) uptime / 3600;
	uptime -= 3600 * hours;
	minutes = (long) uptime / 60;
	uptime -= 60 * minutes;        
	seconds = (long) uptime;

	IsPlaying(crit) ? hrs = crit->socket->hrs : 0;

	sendcrit(crit,"-----------------------------------------------");
	sendcrit(crit,get_mudtime());
	sendcrit(crit,"-----------------------------------------------\n\r");

	// 0=crash 1=reboot 2=shutdown
	sendcritf(crit,"&+WCurrent time: &n&+C%s&n",display_timestamp(current_time,hrs));
	sendcritf(crit,"&+WStartup time: &n&+c%s&n",display_timestamp(mudtime.start,hrs));	
	sendcritf(crit, "A C Mud has been up %li day%s, %li hour%s, %li minute%s and %li second%s.",
                days, days == 1 ? "" : "s",
                hours, hours == 1 ? "" : "s", minutes, minutes == 1 ? "" : "s", seconds, seconds == 1 ? "" : "s");
	sendcritf(crit,"Startup was after a &+W%s&n around %s.",
		mudtime.last > 1 ? "shutdown" : mudtime.last > 0 ? "reboot" : "crash",display_timestamp(mudtime.end,hrs) );
}



CMD(do_sahove)
{
	interpret(crit,"save");
	interpret(crit,"who");
}

CMD(do_socials)
{
	char buf[MAX_BUFFER];
	SOCIAL *social;
	long i;
	long x = 1;
	long total = 0;

	sendcrit(crit,"Socials\n\r-------");
	for(i = 0; i < HASH_KEY; i++)
		for(social = hash_social[i%HASH_KEY]; social; social = social->next_hash)
		{
			total++;
			sprintf(buf,"%-10s%s",social->name,x++%5==0?"\n\r":"\t");
			send_to(crit->socket,buf);
		}
	sendcritf(crit,"\n\r\n\r%li total socials.",total);
}

CMD(do_emote)
{
	char buf[MAX_BUFFER];

	sprintf(buf,"$n %s",all_args(arguments,0));
	if (!xcrit)
	{
		if (!xobj)
			message(buf,crit,0,0);
		else if (xobj)
			message(buf,crit,0,xobj);
	}
	else if (xcrit)
	{
		if (!xobj)
			message(buf,crit,xcrit,0);
		else if (xobj)
			message(buf,crit,xcrit,xobj);
	}
}


CMD(do_who)
{
	MSOCKET *sock;
	char buf[MAX_BUFFER];
	char morts[MAX_BUFFER];
	char wizes[MAX_BUFFER];
	long imms=0, players=0;

	for( sock = socket_list; sock; sock = sock->next )
	{
		if(!IsPlaying(sock->pc))
			continue;

		sprintf(buf,"%s%s%s%s\r\n",
			IsEditing(sock->pc) ? edit_names(sock->pc) : "",
			sock->who_name ? sock->who_name : "",
			sock->pc->name,
			sock->title ? sock->title : "" );

		if (sock->pc->level < LEVEL_IMM)
		{
			players++;
			if (players == 1)
				sprintf(morts,"------------------------ &+CPlayers&N ------------------------\r\n");
			strcat(morts,buf);
		}
		else
		{
			imms++;
			if (imms == 1)
				sprintf(wizes,"----------------------- &+BImmortals&N -----------------------\r\n");
			strcat(wizes,buf);
		}
	}


	// show imm list first
	sendcrit(crit,"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
	sendcrit(crit,"                         A C MUD                         ");
	sendcrit(crit,"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
	if (imms > 0)
		send_to(crit->socket,wizes);
	if (players > 0)
		send_to(crit->socket,morts);
	sendcrit(crit,"---------------------------------------------------------");

	sendcritf(crit,"Immortals: %li\r\nPlayers: %li",imms,players);
}


char *compact_obj_list(char *buf, OBJECT *objs, bool room)
{
	OBJECT *obj;
	OBJECT *xobj;
	long current;
	long count;
	long num;
	long *found;

	for(obj = objs, count=1; obj; obj = obj->next_content)
		count++;

	found = malloc(sizeof(long)*count);
	memset(found, 0, sizeof(long)*count);
	buf[0] = '\0';

	for(obj = objs, current=0; obj; obj = obj->next_content, current++)
	{
		if(found[current])
			continue;

		for(xobj = objs, count=0, num=0; xobj; xobj = xobj->next_content, count++)
		{
			if(found[count])
				continue;

			if(obj->vnum == xobj->vnum
			&& !strcasecmp(room ? obj->long_descr : obj->name,
				       room ? xobj->long_descr : xobj->name))
			{
				num++;
				found[count] = 1;
			}
		}
		if((room && strlen(obj->long_descr)+strlen(buf) > MAX_BUFFER)
		|| (strlen(obj->name)+strlen(buf) > MAX_BUFFER))
		{
			strcat(&buf[MAX_BUFFER-25],"\n\rTRUNCATED...\n\r");
			break;
		}

		if(num>1)
			sprintf(buf+strlen(buf),"(%-2li)%s\n\r", num, 
				room ? obj->long_descr : obj->name);
		else
			sprintf(buf+strlen(buf),"%s\n\r", 
				room ? obj->long_descr : obj->name);
	}
	free(found);
	if (ValidString(buf))
		buf[strlen(buf)-2] = '\0';
	return buf;
}


CMD(do_equipment)
{
	if(!xcrit)
		xcrit = crit;
	sendcritf(crit,"%s",show_equipment(xcrit));
}


CMD(do_inventory)
{
	char buf[MAX_BUFFER];
	if(!crit->inventory)
		sendcrit(crit,"You have nothing in your inventory.");
	else
	{
		sendcrit(crit,"   You are carrying:");
		sendcrit(crit, compact_obj_list(buf, crit->inventory, 0));
	}
}

void use_channel(CREATURE *crit, char *name, char *text, long level, long flag, char *color)
{
	MSOCKET *sock;
	char buf[MAX_BUFFER];

	sprintf(buf,"%s(%s) [&N&+W%s&N%s]: %s&N",color,name,crit->name,color,text);

	for(sock = socket_list; sock; sock = sock->next)
	{
		if(!IsPlaying(sock->pc) || sock->pc->level < level
		|| flag_isset(sock->pc->flags, flag))
			continue;

		sendcrit(sock->pc,buf);
	}
}

#define Channel(chan, name, level, color, flag)					\
CMD(chan) {									\
	if(!arguments[0]) {							\
		flag_reverse(crit->flags, flag);				\
		sendcritf(crit, "The %s channel is now %s.", name,		\
			flag_isset(crit->flags, flag) ? "off" : "on");		\
		return;								\
	}									\
	if(flag_isset(crit->flags, flag))					\
		flag_remove(crit->flags, flag);					\
	use_channel(crit, name, all_args(arguments,0), level, flag, color);	\
}

Channel(do_chat,	"CHAT",		0,		"&+c",	CFLAG_CHANCHAT)
Channel(do_music,	"MUSIC",	0,		"&+G",	CFLAG_CHANMUSIC)
Channel(do_immtalk,	"IMM",		LEVEL_IMM,	"&+M",	CFLAG_CHANIMM)
Channel(do_buildtalk,	"BUILD",	LEVEL_BUILDER,	"&+y", CFLAG_CHANBUILD)


CMD(do_score)
{
	sendcritf(crit,"You are %s.",crit->name);
	sendcritf(crit,"You are currently %s and %s.",
		position_table[crit->position], state_table[crit->state]);
	sendcritf(crit,"Hit points: %5li/%-5li  Movement: %5li/%-5li",
		crit->hp, crit->max_hp, crit->move, crit->max_move);
	sendcritf(crit,"Player ID: %li", crit->id);
}

CMD(do_look)
{
	EXTRA *extra;
	char buf[MAX_BUFFER];
	char editing[MAX_BUFFER];

	if(arguments[0] && strcasecmp(arguments[0],"verbose") && !xcrit && !xobj)
	{
		long num = numbered_arg(arguments[0]);
		long current = 0;

		for(extra = crit->in_room->extras; extra; extra = extra->next)
			if(!strargcmp(extra->keywords, arguments[0])
			&& ++current == num)
				break;
		if(!extra)
		{
			for(xobj = crit->equipment; xobj && !extra; xobj = xobj->next)
				for(extra = xobj->extras; extra; extra = extra->next)
					if(!strargcmp(extra->keywords, arguments[0])
					&& ++current == num)
						break;
		}
		if(!extra)
		{
			for(xcrit = crit->in_room->crits; xcrit && !extra; xcrit = xcrit->next_content)
			{
				if(IsPlayer(xcrit)) continue;
				for(extra = xcrit->extras; extra; extra = extra->next)
					if(!strargcmp(extra->keywords, arguments[0])
					&& ++current == num)
						break;
			}
		}
		if(extra)
		{
			sendcritf(crit,"[&+y%s&N]",extra->keywords);
			sendcritf(crit,"%s",extra->description);
			return;
		}

		sendcrit(crit, "Look at what?");
		return;
	}

	if(xcrit)
	{
		message("$n look$x at $N.",crit,xcrit,0);
		sendcrit(crit,xcrit->description);
		sendcritf(crit,"%s has %li health.",xcrit->name,xcrit->hp);
		sendcritf(crit,"%s is %s and %s.",
			xcrit->name, state_table[xcrit->state], position_table[xcrit->position]);
		sendcritf(crit,"%s",show_equipment(xcrit));
		return;
	}

	if(xobj)
	{
		sendcritf(crit, "[&+y%s&n]\n\r%s", xobj->name, xobj->description);

		if(xobj->objtype == OBJ_CONTAINER)
		{
			sendcritf(crit,"You look inside and see:");
			buf[0] = '\0';
			if (xobj->contents)
				sendcrit(crit, compact_obj_list(buf,xobj->contents,0));
			else
				sendcrit(crit,"Nothing.");
		}
		return;
	}
	buf[0] = '\0';
	sprintf(buf,		"%s\n\r",	crit->in_room->name);
	sprintf(buf+strlen(buf),"&+W[%s]&n",exit_names(crit->in_room,0));
	if(!flag_isset(crit->flags, CFLAG_BRIEF) || (arguments[0] && !strcasecmp(arguments[0],"verbose")))
	sprintf(buf+strlen(buf),"\n\r%s",		crit->in_room->description);
	sendcrit(crit,buf);

	if (crit->in_room->objects)
		sendcrit(crit, compact_obj_list(buf,crit->in_room->objects,1));

	for (xcrit = crit->in_room->crits; xcrit; xcrit = xcrit->next_content)
	{
		if (xcrit != crit)
		{
			if (crit->mount && crit->mount == xcrit)
				sendcritf(crit,"You are riding &+C%s&n.",xcrit->name);
			else if (crit->rider && crit->rider == xcrit)
				sendcritf(crit,"You are being ridden by &+C%s&n.",xcrit->name);
			else 
			{
				if(IsEditing(xcrit))
					sprintf(editing,"[%s]",edit_names(xcrit));

				sendcritf(crit,"%s&+C%s&n is %s here, %s.",
					xcrit->socket->connected == CON_LINKDEAD ? "&+G(Linkdead)&N " 
						: IsEditing(xcrit) ? edit_names(xcrit) : "",
					xcrit->name,
					position_table[xcrit->position],
					state_table[xcrit->state] );
			}
		}
	}
}


////////////
// TABLES //
////////////

// worn keywords, their names, and actual wear locations
//      keyword                 name                    worn
const struct worn_t worn_table[] = 
{
	{"arms",		"arms",			"arms"			},
	{"body",		"body",			"body"			},
	{"held",		"held",			"held"			},
	{"feet",		"feet",			"feet"			},
	{"finger",		"finger",		"finger"		},
	{"hands",		"hands",		"hands"			},
	{"head",		"head",			"head"			},
	{"legs",		"legs",			"legs"			},
	{"neck",		"neck",			"neck"			},
	{"waist",		"waist",		"waist"			},
	{"wrist",		"wrist",		"wrist"			},
	{"bodyarms",		"body and arms",	"body arms"		},
	{"bodyarmslegs",	"body, arms, and legs",	"body arms legs"	},
	{"bodylegs",		"body and legs",	"body legs"		},
	{"twohanded",		"held in both hands",	"held held"		},
	{0,0,0}
};

// states
char *state_table[] =
{
	"alert",
	"awake", // normal, awake
	"fighting",
	"resting",
	"meditating",
	"sleeping",
	"unconscious",
	"dying",
	"dead",
	0
};

// positions
char *position_table[] =
{
	"standing",
	"sitting",
	"lying prone",
	0
};
	
