/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/
 
/*
creature.c -- stuff all about creatures!
creatures are players and (old mud) mobiles.  living and breathing things that
roam the land!
*/

#include "stdh.h"
#include <stdarg.h>
#include <ctype.h>

// players use name only to target
// mobs use keywords
CREATURE *find_crit(CREATURE *crit, char *argument, long flags)
{
	CREATURE *xcrit=0;
	long num=numbered_arg(argument);
	long curnum=0;

	if(argument && (!strcasecmp(argument, "self") || !strcasecmp(argument,"me")))
		return crit;

	for(xcrit = crit->in_room->crits; xcrit; xcrit = xcrit->next_content)
	{
		if(IsSet(flags, PLAYER_ONLY) && !IsPlayer(xcrit))
			continue;

		if( (IsPlayer(xcrit) && !strargindex(xcrit->name, argument))
		|| (!IsPlayer(xcrit) && !strargindex(xcrit->keywords, argument)) )
		{
			if(++curnum==num)
			{
				curnum	= 0;
				num	= 1;
				break;
			}
		}
	}
	if(!xcrit && IsSet(flags, CRIT_WORLD))
	{
		for(xcrit = creature_list; xcrit; xcrit = xcrit->next)
		{
			if(IsSet(flags, PLAYER_ONLY) && !IsPlayer(xcrit))
				continue;

			if( (IsPlayer(xcrit) && !strargindex(xcrit->name, argument))
			|| (!IsPlayer(xcrit) && !strargindex(xcrit->keywords, argument)) )
			{
				if(++curnum==num)
				{
					curnum	= 0;
					num	= 1;
					break;
				}
			}
		}
	}
	if(!xcrit && curnum)
	{
		char buf[MAX_BUFFER];
		sprintf(buf,"%li.%s",num-curnum,argument);
		strcpy(argument,buf);
	}
	return xcrit;
}


void die(CREATURE *crit)
{
	OBJECT *corpse = new_object(VNUM_CORPSE);
	char buf[MAX_BUFFER];

	message("$n $r &+LDEAD&N, &+RR.I.P.&N",crit,0,0);

	while(crit->equipment)
		remove_obj(crit, crit->equipment);
	while(crit->inventory)
		trans(crit->inventory, corpse);

	sprintf(buf,"corpse %s",crit->name);
	str_dup(&corpse->keywords, buf);
	sprintf(buf,"the corpse of %s",crit->name);
	str_dup(&corpse->name, buf);
	sprintf(buf,"The corpse of %s rests peacefully here.",crit->name);
	str_dup(&corpse->long_descr, buf);

	trans(corpse, crit->in_room);

	if(!IsPlayer(crit))
	{
		free_creature(crit);
	}
	else
	{
		trans(crit, hashfind_room(VNUM_DEATH_ROOM));
		crit->hp = crit->move = 1;
		crit->state = STATE_NORMAL;
		crit->position = POS_STANDING;
	}
}


void heal(CREATURE *crit, long amount)
{
	long before = crit->hp;

	crit->hp += amount;

	if(before < -4 && crit->hp <= 0 && crit->hp > -5)
	{
		message("$z wounds are healed, but $e is still unconscious.",crit,0,0);
		crit->state = STATE_UNCONSCIOUS;
	}
	else if(before <= 0 && crit->hp > 0)
	{
		message("$n become$x conscious.",crit,0,0);
		crit->state = STATE_NORMAL;
	}
}


void hurt(CREATURE *crit, long amount)
{
	long before = crit->hp;
	crit->hp -= amount;

	if(crit->hp < -10)
		die(crit);
	else if(crit->hp <= 0)
	{
		if(before >= 1 && crit->hp > -5)
		{
			message("$n fall$x into a state of unconsciousness.",crit,0,0);
			crit->state = STATE_UNCONSCIOUS;
		}
		if(before >= -4 && crit->hp <= -5)
		{
			message("$n $r mortally wounded, and will die soon, if not aided.",crit,0,0);
			crit->state = STATE_DYING;
		}
	}

	return;
}


void position_check(CREATURE *crit)
{
	switch(crit->state)
	{
	case STATE_ALERT:
	case STATE_NORMAL:
	case STATE_FIGHTING:
	case STATE_RESTING:
	case STATE_MEDITATING:
		break;
	case STATE_SLEEPING:
	case STATE_UNCONSCIOUS:
// do something with flying/swimming people if you want
		if(crit->position < POS_PRONE)
		{
			if(percent() > 50)
			{
				message("$n fall$x to the ground.",crit,0,0);
				crit->position = POS_PRONE;
			}
			else
				message("$n waver$x slightly from side to side.",crit,0,0);
		}
		break;
	case STATE_DYING:
	case STATE_DEAD:
		if(crit->position < POS_PRONE && percent() > 50)
		{
			message("$n falls to the ground.",crit,0,0);
			crit->position = POS_PRONE;
		}
		break;
	}
}


// appends a newline at end, and has varargs capability
void sendcrit(CREATURE *crit, char *buf)
{
	if(!IsPlaying(crit))
		return;

	send_to(crit->socket, (char*)buf);
	send_to(crit->socket, "\n\r");
}


void sendcritf(CREATURE *crit, char *buf, ...)
{
	char varbuf[MAX_BUFFER];
	va_list ap;

	if(!IsPlaying(crit))
		return;

	va_start(ap, buf);
	(void)vsnprintf(varbuf, MAX_BUFFER, buf, ap);

	send_to(crit->socket, (char*)varbuf);
	send_to(crit->socket, "\n\r");

	va_end(ap);
}


//////////////
// DO_FUNCS //
//////////////
CMD(do_clear)
{
	sendcrit(crit,"\033[2J");
}

CMD(do_password)
{
	MSOCKET *sock = crit->socket;

	if(ValidString(sock->password) && strcmp(sock->password, crypt(arguments[0], sock->pc->name)))
	{
		sendcritf(crit, "That is not your current password, try again. %s", sock->password);
		mudlog("%s just tried to change his password and put in the wrong password!", crit->name);
		return;
	}
	if(strcmp(arguments[1], arguments[2]))
	{
		sendcrit(crit, "Your new passwords do not match, try again.");
		return;
	}

	str_dup(&sock->password, crypt(arguments[1], sock->pc->name));
}


CMD(do_prompt)
{
	char buf[MAX_BUFFER];

	sprintf(buf,"config prompt %s",all_args(arguments,0));
	interpret(crit,buf);
}

CMD(do_config)
{
	char buf[MAX_BUFFER];
	char newbuf[MAX_BUFFER];
	long value=0;
	long flag=0;
	long longest=0, current=0, x=0, y=0, symbol=0;

	if(!arguments[0])
	{
		buf[0] = '\0';
		sprintf(buf+strlen(buf)," -------------------\n\r"	);
		sprintf(buf+strlen(buf),"| &+BCharacter Options&N\n\r"	);
		sprintf(buf+strlen(buf),"| see [&+Whelp config&n]\n\r"	);
		sprintf(buf+strlen(buf)," -------------------\n\r"	);
		sprintf(buf+strlen(buf),"| Hrs from GMT : %li\n\r",	crit->socket->hrs);
		sprintf(buf+strlen(buf),"| Lines        : %li\n\r", 	crit->socket->lines);
		sprintf(buf+strlen(buf),"| Ansi         : %s\n\r", 	flag_isset(crit->flags, CFLAG_ANSI) ? "On" : "Off" );
		sprintf(buf+strlen(buf),"| Brief        : %s\n\r",	flag_isset(crit->flags, CFLAG_BRIEF) ? "On" : "Off" );
		sprintf(buf+strlen(buf),"| Prompt       : %s\n\r",	crit->socket->prompt);
		sprintf(buf+strlen(buf),"| Blank        : %s\n\r",	flag_isset(crit->flags, CFLAG_BLANK) ? "On" : "Off");
		sprintf(buf+strlen(buf),"| Menu         : %s\n\r",	flag_isset(crit->flags, CFLAG_NOMENU) ? "Off" : "On");
		sprintf(buf+strlen(buf),"| Anti-Idle    : %s\n\r",	flag_isset(crit->flags, CFLAG_ANTIIDLE) ? "On" : "Off");
		if(crit->level >= LEVEL_BUILDER)
		{
		sprintf(buf+strlen(buf),"| Notify       : %s\n\r", 	flag_isset(crit->flags, CFLAG_NOTIFY) ? "On" : "Off");
		sprintf(buf+strlen(buf),"| WizInvis     : %s\n\r",	flag_isset(crit->flags, CFLAG_WIZINVIS) ? "On" : "Off");
		}
		sprintf(buf+strlen(buf)," -------------------\n\r" );

		// first find the longest line
		for(x = 0; buf[x] != '\0'; x++)
		{
			// check for ansi codes and adjust appropriately
			if(buf[x] == '&')
			{
				if(buf[x+1] == '&')
					current -= 2;
				else if(buf[x+1] == '+' || buf[x+1] == '-')
					current -= 3;
				else if(buf[x+1] == '=')
					current -= 4;
				else
					current -= 2;
			}

			if(buf[x] == '\r')
			{
				if(current > longest)
					longest = current;
				current = 0;
			}
			current++;
		}
		
		// now we have the longest line (in characters) .. we add dashes, spaces and pipes as appropriate
		current = 0;
		for(x = 0, y = 0; buf[x] != '\0'; x++, y++)
		{
			// check for ansi codes and adjust appropriately
			if(buf[x] == '&')
			{
				if(buf[x+1] == '&')
					current -= 2;
				else if(buf[x+1] == '+' || buf[x+1] == '-')
					current -= 3;
				else if(buf[x+1] == '=')
					current -= 4;
				else
					current -= 2;
			}

			if(buf[x] == '\n')
			{
				if(buf[x-1] == '-')
					symbol = 1;
				else
					symbol = 0;
				while(current < longest)
				{
					if(symbol == 1)
						newbuf[y++] = '-';
					else
						newbuf[y++] = ' ';
					current++;
				}
				if(symbol != 1)
					newbuf[y++] = '|';
				current = 0;
			}
			newbuf[y] = buf[x];
			current++;
		}
		newbuf[y] = '\0';

		sendcrit(crit,newbuf);
		return;
	}

	if(arguments[1])
		value = atoi(arguments[1]);

	if (!strindex("hrs", arguments[0]) || !strindex("gmt", arguments[0]))
	{
		if (value < -12 || value > 12)
		{
			sendcrit(crit,"Hours from GMT cannot be higher than 12 or lower than -12");
			return;
		}
		sendcritf(crit, "You set your hours from GMT to %li",crit->socket->hrs = value);
		return;
	}
	if(!strindex("lines", arguments[0]))
	{
		sendcritf(crit, "You will now see a maximum of %li lines at a time.",
			crit->socket->lines = value);
		return;
	}
	else if(!strindex("ansi", arguments[0]))
		flag = CFLAG_ANSI;
	else if(!strindex("brief", arguments[0]))
		flag = CFLAG_BRIEF;
	else if(!strindex("menu", arguments[0]))
		flag = CFLAG_NOMENU;
	else if(!strindex("blank", arguments[0]))
		flag = CFLAG_BLANK;
	else if(!strindex("antiidle", arguments[0]) || !strindex("anti-idle", arguments[0]))
		flag = CFLAG_ANTIIDLE;
	else if(!strindex("notify", arguments[0]) && crit->level >= LEVEL_BUILDER)
		flag = CFLAG_NOTIFY;
	else if(!strindex("wizinvis", arguments[0]) || !strindex("invis", arguments[0]))
		flag = CFLAG_WIZINVIS;
	else if(!strindex("prompt", arguments[0]))
	{
		const char *prompts[] =
		{
		"&+g<%h/%H %m/%M>&N ",
		"&+g<%h %m>&N ",
		"&+g<%h/%H %m/%M Room:%r>&N ",
		"&+g<%h/%H %m/%M Room:%r %i>&N ",
		0
		};
		char buf[MAX_BUFFER];
		char orig[MAX_BUFFER];
		char show[MAX_BUFFER];
		long num=0;

		if(!ValidString(arguments[1]))
		{
			strcpy(orig, crit->socket->prompt);
			sendcrit(crit, "Available prompts (config prompt #):");

			for(; prompts[num]; num++)
			{
				show[0] = '\0';
				strcpy(show, prompts[num]);
				str_dup(&crit->socket->prompt, show);
				sprintf(buf, "&+B[%-2li]&N %-25s -->", num, prompts[num]);
				send_immediately(crit->socket,buf);
				prompt(crit,0);
				send_immediately(crit->socket,"\n\r");
			}
			send_immediately(crit->socket,"Type '&+Whelp prompt&N' for more configuration options.\n\r");
			str_dup(&crit->socket->prompt, orig);
			return;
		}
		if(is_number(arguments[1]))
		{
			for(num=0; prompts[num]; num++)
				;
			if(value >= num || value < 0)
			{
				sendcrit(crit, "That prompt is not available.");
				return;
			}
			show[0] = '\0';
			strcpy(show, prompts[value]);
			str_dup(&crit->socket->prompt, show);
			return;
		}
		str_dup(&crit->socket->prompt, all_args(arguments,1));
		return;
	}
	else
	{
		sendcrit(crit, "Not an option.");
		return;
	}


	if(flag_isset(crit->flags, flag))
	{
		if (flag == CFLAG_NOMENU)
			sendcrit(crit,"Turned on.");
		else
			sendcrit(crit,"Turned off.");
		flag_remove(crit->flags, flag);
	}
	else
	{
		if (flag == CFLAG_NOMENU)
			sendcrit(crit,"Turned off.");
		else
			sendcritf(crit,"Turned on.");
		flag_set(crit->flags, flag);
	}
}


CMD(do_cough)
{
	char buf[MAX_BUFFER];

	if(percent() > 50)
	{
		xobj = new_object(VNUM_HAIRBALL);
		sprintf(buf,"a hairball of %s",crit->name);
		str_dup(&xobj->name,buf);
		trans(xobj,crit);

		message("$n cough$x out $p!",crit,0,xobj);
	}
	else
		message("$n cough$x.",crit,0,0);
}


CMD(do_eat)
{
	if(crit->level >= LEVEL_BUILDER)
	{
		message("$n eat$x $p.",crit,0,xobj);
		free_object(xobj);
		return;
	}
}


CMD(do_flex)
{
	char buf[MAX_BUFFER];

	if(crit->strength >= 75 && percent() > 50)
	{
		xobj = new_object(VNUM_SWEAT);
		sprintf(buf,"%s's sweat",crit->name);
		str_dup(&xobj->name,buf);
		trans(xobj,xcrit ? (void*)xcrit : (void*)crit->in_room);

		if(xcrit)
			message("$n flex$b mightily for $N, drenching $M in sweat!",crit,xcrit,0);
		else
			message("$n flex$b mightily for the room, sweat flying everywhere!",crit,xcrit,0);
	}
	else
	{
		if(xcrit)
			message("$n flex$b for $N as best $e can.",crit,xcrit,0);
		else
			message("$n flex$b.",crit,xcrit,0);
	}
}


CMD(do_hunt)
{
	sendcritf(crit, "To find %s you should head %s.",
		xcrit->name, find_path(crit,xcrit,1000));
}


CMD(do_reply)
{
	char buf[MAX_BUFFER];

	if(!ValidString(crit->socket->reply))
	{
		sendcrit(crit,"Reply to whom?");
		return;
	}

	sprintf(buf,"tell %s %s",crit->socket->reply,all_args(arguments,0));
	interpret(crit, buf);
}

CMD(do_say)
{
	message("&+g$n say$x '&+G$p&n&+g'&n",crit,0,all_args(arguments,0));
}


// leave the mud :(
CMD(do_quit)
{
	send_immediately(crit->socket,"Bye!\n\r");
	if (crit->socket->connected != CON_LINKDEAD)
		crit->socket->connected = CON_MENU; // before the rest so during fwrite makes online = 0
	message_all("&+WNotice->&N$p has left the game.",crit,crit->name,1);
	fwrite_player(crit);
	trans(crit,0); // the 0 is where quit ppl go.
	if(flag_isset(crit->flags, CFLAG_NOMENU) || crit->socket->connected == CON_LINKDEAD)
		free_socket(crit->socket); // terminated.
	else
		nanny(crit->socket,"");
}


CMD(do_tell)
{
	sendcritf(crit, "&+yYou tell%s %s, '&+Y%s&n&+y'",
		IsPlaying(xcrit) ? "" : " a linkdead",
		xcrit->name,all_args(arguments,1));
	sendcritf(xcrit,"&+y%s tells you, '&+Y%s&n&+y'",crit->name,all_args(arguments,1));

	if(IsPlaying(xcrit))
		str_dup(&xcrit->socket->reply, crit->name);
}


CMD(do_title)
{
	char buf[MAX_BUFFER];

	buf[0] = '\0';

	if(ispunct(arguments[0][0]))
		strcpy(buf,all_args(arguments,0));
	else
		sprintf(buf," %s",all_args(arguments,0));

	if(!IsImmortal(crit) && strlen(noansi(buf)) > 60)
	{
		sendcrit(crit,"Your title can only be 60 characters long, not counting ansi.");
		return;
	}

	str_dup(&crit->socket->title, buf);

	sendcrit(crit,"Title set.");
}


CMD(do_move)
{
	switch(randnum(1,5))
	{
	case 1: sendcrit(crit,"You can't go that way, pal."); break;
	case 2: sendcrit(crit,"A strange force of nature prevents you from going there."); break;
	case 3: sendcrit(crit,"You nearly fall off the edge of the universe!"); break;
	case 4: sendcrit(crit,"Where?"); break;
	case 5: sendcrit(crit,"You attempt, but fail."); break;
	}
}


CMD(do_mount)
{
	if (!flag_isset(xcrit->flags,CFLAG_MOUNT))
	{
		sendcritf(crit,"You cannot mount %s.",xcrit->name);
		return;
	}

	if (crit->mount)
	{
		sendcrit(crit,"You are already mounted.");
		return;
	}
	trans(crit,xcrit);
	message("$n climb$x up onto $N.",crit,xcrit,0);
}


CMD(do_dismount)
{
	if (!crit->mount)
	{
		sendcrit(crit,"You are not riding anything.");
		return;
	}
	message("$n climb$x down off of $N.",crit,crit->mount,0);
	trans(crit,crit->in_room);
}


// sleep wake stand sit etc
// commands included: sleep wake rest stand lie
// not included (up to coder): sit meditate alert
// currently, resting will force you to sit.  you can sleep standing though
CMD(do_rest)
{
	crit->state = STATE_RESTING;

	if(crit->position == POS_SITTING)
		message("$n rest$x.",crit,0,0);
	else if(crit->position == POS_PRONE)
		message("$n sit$x up and rest$x.",crit,0,0);
	else
		message("$n sit$x down and rest$x.",crit,0,0);

	crit->position = POS_SITTING;
}

CMD(do_stand)
{
	crit->position = POS_STANDING;
	crit->state = STATE_NORMAL;
	message("$n stand$x up.",crit,0,0);
}

CMD(do_sleep)
{
	crit->state = STATE_SLEEPING;
	message("$n fall$x asleep.",crit,0,0);
}

CMD(do_wake)
{
	crit->state = STATE_NORMAL;
	message("$n wake$x up.",crit,0,0);
}

CMD(do_lie)
{
	crit->position = POS_PRONE;
	message("$n lie$x flat on the ground.",crit,0,0);
}


