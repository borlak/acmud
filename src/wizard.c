/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
wizard.c -- immortal commands
*/

#include "stdh.h"
#include "io.h"
#include <unistd.h>


  ///////////////
 // FUNCTIONS //
///////////////

/////////////
// UTILITY //
/////////////
long edit_check(void)
{
	MSOCKET *socket;
	long editing = 0;

	for(socket = socket_list; socket; socket = socket->next)
	{
		if(socket->string)
		{
			editing = 1;
			continue;
		}
		if(IsEditing(socket->pc))
		{
			save_editing(socket->pc);
			stop_editing(socket->pc,1);
		}
	}
	return editing;
}

  //     #include <sys/types.h>
       #include <sys/stat.h>

//////////////////
// DO FUNCTIONS //
//////////////////
CMD(do_test)
{
str_dup(&crit->socket->string,"test");
//crit->socket->string = str_dup("test");
/*
	if(arguments[0] && !strcasecmp(arguments[0],"yes"))
	{
		CREATURE *dude;
		long x=0;

		for(x = 0; x < 100000; x++)
		{
			dude = new_creature_proto(40000000+x);
			fwrite_creature(dude->vnum);
			free_creature(dude);
		}
	}
*/
}


CMD(do_wizhelp)
{
	char buf[MAX_BUFFER];
	long level;
	long i;
	long cr=0;

	for(level = LEVEL_IMP; level >= LEVEL_IMM; level--)
	{
		if(crit->level < level)
			continue;

		sendcritf(crit,"&+y[Level %li Powers]&N",level);

		buf[0] = '\0';
		cr = 1;
		for(i = 0; command_table[i].command; i++)
		{
			if(command_table[i].level == level)
			{
				sprintf(buf+strlen(buf),"&+Y%-15s&N%s",
					command_table[i].command,
					cr++ % 5 == 0 ? "\n\r" : " ");
			}
		}
		if((cr-1) % 6 != 0)
			strcat(buf,"\n\r");
		sendcrit(crit,buf);
	}
}


CMD(do_wizlist)
{
	interpret(crit, "help wizlist");
}


CMD(do_at)
{
	ROOM *room = crit->in_room;
	ROOM *toroom = 0;
	long vnum = 0;

	if(xcrit)
	{
		trans(crit,xcrit->in_room);
		interpret(crit,all_args(arguments,1));
		if(crit)
			trans(crit,room);
		return;
	}

	if(!is_number(arguments[0]))
	{
		sendcrit(crit,"You must supply a room vnum.");
		return;
	}

	vnum = atoi(arguments[0]);

	if(!(toroom = hashfind_room(vnum)))
	{
		sendcrit(crit,"That room doesn't exist.");
		return;
	}

	trans(crit,toroom);
	interpret(crit,all_args(arguments,1));
	if(crit)
		trans(crit,room);
}


CMD(do_backup)
{
	backup_mud();
	sendcrit(crit,"Done.");
}


CMD(do_ban)
{
	MSOCKET *sock, *sock_next;
	BAN *ban;
	char buf[MAX_BUFFER];
	long x=0;

	if(!arguments[0])
	{
		sendcrit(crit,"Bans\n\r----");
		for(ban = ban_list, x = 1; ban; ban = ban->next, x++)
		sendcritf(crit,"[%2li] %-15s [%s]", x, ban->name, ban->ip );

		if(!x)
			sendcrit(crit,"No bans are in place!");
		return;
	}

	if(xcrit)
	{
		ban		= new_ban(xcrit);
		ban->bantype	= BAN_PLAYER;

		if(arguments[1])
			str_dup(&ban->message, all_args(arguments,1));

		sendcritf(xcrit,"%s:\n\r%s\n\r", BAN_MESSAGE, ban->message);
		sendcritf(crit,"%s banned.", xcrit->name);
		mudlog("%s BANNED %s :: %s", crit->name, xcrit->name, ban->message);

		free_socket(xcrit->socket);
		fwrite_bans();
		return;
	}
	// else he typed in something like an IP address

	ban		= new_ban(0);
	ban->bantype	= BAN_IP;

	str_dup(&ban->ip, arguments[0]);

	if(arguments[1])
		str_dup(&ban->message, all_args(arguments,1));

	mudlog("%s BANNED IP %s :: %s", crit->name, arguments[0], ban->message);

	for(sock = socket_list; sock; sock = sock_next)
	{
		sock_next = sock->next;

		if(!strindex(ban->ip, sock->ip))
		{
			sprintf(buf,"%s:\n\r%s\n\r", BAN_MESSAGE, ban->message);
			send_immediately(sock,buf);
			free_socket(sock);
		}
	}
	fwrite_bans();
}


CMD(do_coins)
{
	long amount=0;
	long cointype=0;

	if(!is_number(arguments[1]))
	{
		interpret(crit,"help coins");
		return;
	}

	for(cointype = 0; coin_table[cointype].name; cointype++)
	{
		if(!strcasecmp(coin_table[cointype].name, arguments[0]))
			break;
	}
	if(!coin_table[cointype].name)
	{
		sendcrit(crit, "That is not a valid cointype.");
		return;
	}

	if(!(amount = atoi(arguments[1])))
	{
		sendcrit(crit, "Zero is not an acceptable number.");
		return;
	}

	xobj		= new_object(VNUM_COIN);
	xobj->values[0]	= cointype;
	xobj->values[1]	= amount;

	coinify(xobj);
	trans(xobj,crit);

	sendcritf(crit, "You create %li %s coin%s!",
		amount, coin_table[cointype].ansi_name, amount == 1 ? "" : "s");
}


CMD(do_disconnect)
{
	free_socket(xcrit->socket);
}


CMD(do_echo)
{
	message_all(all_args(arguments,0),crit,0,1);
}


CMD(do_force)
{
	CREATURE *crits, *crits_next;
	char buf[MAX_BUFFER];
	long all=0;

	if(!strcasecmp(arguments[0],"all"))
		all = 1;
	if(!strcasecmp(arguments[0],"allmobs") && crit->level == LEVEL_IMP)
		all = 2;

	if(!all && !xcrit)
	{
		sendcritf(crit, "Creature '%s' not found.", arguments[0]);
		return;
	}

	for(crits = creature_list; crits; crits = crits_next)
	{
		crits_next = crits->next;

		if (all)
			xcrit = crits;

		if(!IsPlayer(xcrit) && all == 1)
			continue;

		if(IsPlayer(xcrit) && xcrit->level > crit->level)
		{
			if(!all)
			{
				sendcrit(crit, "You can't force someone that is a higher level than you.");
				return;
			}
			continue;
		}

		if((!all && arguments[1] && !strcasecmp(arguments[1], "silent"))
		|| (all && arguments[2] && !strcasecmp(arguments[2], "silent")) )
		{
			interpret(xcrit, all_args(arguments, all ? 3 : 2));
			if(!all)
				break;
		}
		else
		{
			strcpy(buf, all_args(arguments, 1));
			sendcritf(xcrit, "%s forces you to '%s'", crit->name, buf);
			interpret(xcrit, buf);
			if(!all)
				break;
		}
	}
}


CMD(do_heal)
{
	heal(xcrit,atoi(arguments[1]));
}


CMD(do_hurt)
{
	hurt(xcrit,atoi(arguments[1]));
}


CMD(do_input)
{
	MYSQL_ROW row;
	MYSQL_RES *result;
	AREA *area=0;
	char query[MAX_BUFFER];
	long lvnum=-1, hvnum=-1;
	long update=0;

	if(!ValidString(arguments[0]))
		sprintf(query, "SELECT * FROM player_input WHERE 1=1"); // silly 1=1 for the dynamic addon to the where below :p
	else if(arguments[1] && !strcasecmp(arguments[1], "vnum"))
	{
		if(!arguments[2] || (arguments[2] && !is_number(arguments[2])))
		{
			interpret(crit, "help input");
			return;
		}
		lvnum = atoi(arguments[2]);

		if(arguments[3] && is_number(arguments[3]))
			hvnum = atoi(arguments[3]);

		if(hvnum >= 0 && lvnum >= 0)
			sprintf(query, "SELECT * FROM player_input WHERE type='%s' AND room >= %li AND room <= %li", 
				arguments[0], lvnum, hvnum);
		else
			sprintf(query, "SELECT * FROM player_input WHERE type='%s' AND room = %li", 
				arguments[0], lvnum);
	}
	else if(is_number(arguments[0]))
	{
		if(!ValidString(arguments[1]) || strcasecmp(arguments[1], "done"))
			sprintf(query, "SELECT * FROM player_input WHERE ID >= %d", atoi(arguments[0]));
		else
		{
			if(crit->level != LEVEL_IMP)
			{
				area = find_area(atoi(arguments[0]));
				if (area &&
				   (strargcmp(area->builders, crit->name)
				&& strargcmp(area->builders, "all")))
				{
					sendcrit(crit, "You don't have access to that vnum.");
					return;
				}
			}
			update = 1;
			sprintf(query, "UPDATE player_input SET done=1 WHERE ID=%d", atoi(arguments[0]));
		}
	}
	else
	{
		if(ValidString(arguments[1]))
		{
			if(is_number(arguments[1]))
				sprintf(query, "SELECT * FROM player_input WHERE type='%s' AND ID > %d",
					arguments[0], atoi(arguments[1]));
			else
				sprintf(query, "SELECT * FROM player_input WHERE type='%s' AND name LIKE '%s'",
					arguments[0], arguments[1]);
		}
		else
			sprintf(query, "SELECT * FROM player_input WHERE type='%s'",
				arguments[0]);
	}

	sprintf(query+strlen(query), " AND done=0");
	mysql_query(mysql, query);
	if(update)
	{
		sendcrit(crit, "That player input has been marked as done/completed.");
		return;
	}

	result = mysql_store_result(mysql);
	update = 0; // use this as a counter...I'm such a min-maxer!

	sendcrit(crit, "&+W[ID] [Type] [Name] [Vnum] [Message]&N");
	while((row = mysql_fetch_row(result)))
	{
		sendcritf(crit, "%s[%-2d] [%-4s] %s [%d] %s", 
			update % 2 == 0 ? "&+y" : "&+Y",
			atoi(row[0]),
			row[I_TYPE],
			row[I_NAME],
			atoi(row[I_ROOM]),
			row[I_INPUT]);
		update++;
	}
	if(!update)
		sendcrit(crit, "No results returned.  Either database is all marked done or you made a typo!");
}


CMD(do_log)
{
	extern long log_all;

	if(!strcasecmp(arguments[0],"all"))
	{
		if(log_all)
			log_all = 0;
		else
			log_all = 1;
		sendcritf(crit,"Logging ALL turned %s!", log_all ? "on" : "off");
		return;
	}

	if(xcrit)
	{
		if(flag_isset(xcrit->flags, CFLAG_LOG))
			flag_remove(xcrit->flags, CFLAG_LOG);
		else
			flag_set(xcrit->flags, CFLAG_LOG);
		sendcritf(crit,"%s logging %s's actions.",
			flag_isset(xcrit->flags, CFLAG_LOG) ? "Started" : "No longer", xcrit->name);
		return;
	}
	interpret(crit,"log");
}


// this calls the system command PS
// depending on what type of linux you are running the mud on, you may have to modify
// this function to work correctly.
CMD(do_memory)
{
	char buf[MAX_BUFFER];
	char pmem[MAX_BUFFER];
	char pcpu[MAX_BUFFER];
	long found = 0;
	FILE *fp;
	long pid = getpid();
	char *file;

	sprintf(buf,"ps up%li",pid);

	if(!(fp = popen(buf,"r")))
	{
		sendcrit(crit,"Unable to popen.");
		return;
	}
	while ((file = fread_line(fp)) != (char*)EOF)
	{
		sscanf(file,"%*s %s",buf);
		if (atoi(buf) == pid)
		{
			sscanf(file,"%*s %*s %s %s %*s %s",pcpu, pmem, buf);
			sendcritf(crit,"The mud is currently using %skb of memory. (%s%%)\n\r"
				       "The mud is currently using %s%%CPU.",buf, pmem, pcpu);
			found = 1;
			break;
		}
	}
	if (!found)
		sendcrit(crit,"Command failed, try again.");
	pclose(fp);
}

// this calls the system command TOP
// depending on what type of linux you are running the mud on, you may have to modify
// this function to work correctly.
CMD(do_top)
{
	char buf[MAX_BUFFER];
	FILE *fp;
	long pid = getpid();
	char *file;

	sprintf(buf,"top b p %li n 1",pid);

	if(!(fp = popen(buf,"r")))
	{
		sendcrit(crit,"Unable to popen.");
		return;
	}
	while ((file = fread_line(fp)) != (char*)EOF)
		sendcritf(crit,"%s",file);
	pclose(fp);
}


CMD(do_playerpurge)
{
	char buf[MAX_BUFFER];
	MYSQL_RES *result;
	MYSQL_ROW row;
	long time;

	if(!is_number(arguments[1]))
	{
		interpret(crit,"help playerpurge");
		return;
	}

	time = current_time - atoi(arguments[1]);

	if(atoi(arguments[1]) <= 605000)
	{
		sendcrit(crit,"This would delete any characters that haven't logged in for a week or longer!");
		sendcrit(crit,"If you want to do this, go modify the code, or do it manually in SQL.");
		sendcrit(crit,"This is a safety precaution.");
		return;
	}

	sprintf(buf,"SELECT id,name FROM player WHERE last_online <= %li",time);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);
	
	while((row = mysql_fetch_row(result)))
	{
		sendcritf(crit,"Player %s will be deleted",row[1]);
	}

	mysql_free_result(result);

/* real shit to FIX TODO later

	while((row = mysql_fetch_row(result)))
	{
		sprintf(buf,"DELETE FROM object WHERE owner_id=%li",atoi(row[0]));
		mysql_query(mysql,buf);

		sprintf(buf,"DELETE FROM creature WHERE name=%s",row[1]));
		mysql_query(mysql,buf);
	}
	mysql_free_result(result);

	sprintf(buf,"DELETE FROM player WHERE last_online <= %li",time);
	mysql_query(mysql,buf);
*/



}	


CMD(do_purge)
{
	CREATURE *xcrit_next;

	if(xcrit)
	{
		if(IsPlayer(xcrit))
		{
			sendcrit(crit, "You can't purge players.");
			return;
		}
		message("$n purge$x $N.",crit,xcrit,0);
		free_creature(xcrit);
		return;
	}
	if(xobj)
	{
		message("$n purge$x $p.",crit,0,xobj);
		free_object(xobj);
		return;
	}

	message("$n purge$x the room!",crit,0,0);
	for(xcrit = crit->in_room->crits; xcrit; xcrit = xcrit_next)
	{
		xcrit_next = xcrit->next_content;

		if(IsPlayer(xcrit))
			continue;

		free_creature(xcrit);
	}

	while(crit->in_room->objects)
		free_object(crit->in_room->objects);
}


// this is a HOT reboot, replacing the current executable, and copying descriptors
// over to the new program.  players will not be disconnected
CMD(do_reboot)
{
	CREATURE *plyr;
	CREATURE *plyr_next;
	MYSQL_RES *result;
	char buf[512];
	long update=1;
	extern long		port;
	extern unsigned long	host;

	if(edit_check())
	{
		sendcrit(crit, "Someone is in the string editor.");
		return;
	}

	if(arguments[0])
		message_all("&+Y[Reboot by $n]&N $p",crit,all_args(arguments,0),2);
	else
		message_all("&+Y[Reboot by $n]&N",crit,0,2);

	for(plyr = creature_list; plyr; plyr = plyr_next)
	{
		plyr_next = plyr->next;
		if (!plyr->socket || !IsPlayer(plyr))
			continue;

		if (plyr->socket->connected == CON_LINKDEAD)
			free_socket(plyr->socket);
		else if (!(plyr->socket->connected == CON_PLAYING))
		{
			send_immediately(plyr->socket,"Mud is rebooting.\r\nReconnect.\r\n");
			free_socket(plyr->socket);
		}
		else
			fwrite_player(plyr);
	}

	mudlog("HOTREBOOT by %s",crit->name);

	// make sure 'host' player exists
	mysql_query(mysql, "SELECT name FROM player WHERE name='host' LIMIT 1");
	result = mysql_store_result(mysql);
	if(mysql_num_rows(result) == 0)
		update = 0;
	mysql_free_result(result);

	// write host socket to 'host' player
	sprintf(buf, "%s player SET name='host', descriptor='%li', online='1'%s",
		update ? "UPDATE" : "INSERT INTO", host,
		update ? " WHERE name='host'" : "");
	mysql_query(mysql, buf);

	// update time again with reboot! 1=reboot 2=shutdown 0=crash
	mudtime.mudstatus = 1;
	fwrite_time();

	// disconnect from database server
	mysql_close(mysql);

	sprintf(buf,"%li",port);
	execl("../src/mud", "mud", buf, "hotreboot", NULL);
	mudlog("Hotreboot FAILED");
	exit(1);
}


CMD(do_resets)
{
	char buf[MAX_BUFFER];
	RESET *reset=0;
	long total=0, i=0;

	if(arguments[0] && (!strcasecmp(arguments[0],"all") || !strcasecmp(arguments[0],"count")))
	{
		for(i = 0; i < HASH_KEY; i++)
		{
			buf[0] = '\0';
			for(reset = hash_reset[i%HASH_KEY]; reset; reset = reset->next_hash)
			{
				total++;
				sprintf(buf+strlen(buf),"\n\r[%s]",
					reset->loadtype == TYPE_CREATURE ? reset->crit->name :
					reset->loadtype == TYPE_OBJECT ? reset->obj->name :
					reset->command );
			}
			if(buf[0] != '\0' && !strcasecmp(arguments[0],"all"))
				sendcrit(crit,buf);
		}
		if(!strcasecmp(arguments[0],"count"))
		{
			sprintf(buf,"TOTAL RESETS: %li",total);
			sendcrit(crit, buf);
		}
		return;
	}

	total = display_resets(crit);
	if(!total)
		sendcrit(crit,"No resets in this room.");
}


void restore(CREATURE *crit)
{
	crit->hp	= crit->max_hp;
	crit->move	= crit->max_move;
	sendcrit(crit,"You have been restored.");
}
CMD(do_restore)
{
	MSOCKET *socket=0;

	if(!strcasecmp(arguments[0], "all"))
	{
		for(socket = socket_list; socket; socket = socket->next)
			restore(socket->pc);
		sendcrit(crit,"You restore all players!");
		return;
	}
	if(xcrit)
	{
		sendcritf(crit,"You restore %s!",xcrit->name);
		restore(xcrit);
		return;
	}

	sendcrit(crit, "Restore who?");
}


CMD(do_shutdown)
{
	extern long mud_shutdown;

	if(edit_check())
	{
		sendcrit(crit, "Someone is in the string editor.");
		return;
	}

	// update time again with reboot! 1=reboot 2=shutdown 0=crash
	mudtime.mudstatus = 2;
	fwrite_time();

	message_all("&=YR-=Shutdown by $n=-&n\n\r",crit,0,2);
	mud_shutdown = 1;
}


CMD(do_slay)
{
	message("$n slay$x $N in cold blood!",crit,xcrit,0);
	die(xcrit);
}


CMD(do_snoop)
{
	MSOCKET *zsock=0;

	if (!xcrit)
	{
		sendcrit(crit,"Snoop who?");
		return;
	}

	if (xcrit == crit)
	{
		for (zsock = socket_list; zsock; zsock = zsock->next)
		{
			if (strlistcmp(crit->name,zsock->snoop))
				str_dup(&zsock->snoop, str_minus(zsock->snoop, crit->name, ' '));
		}
		sendcrit(crit,"All snoops have been cancelled.");
		return;
	}
	else if (crit->socket->snoop)
	{
		if (strlistcmp(xcrit->name,crit->socket->snoop))
		{
			sendcrit(crit,"You can't snoop them... they are already snooping you.");
			return;
		}
	}
	if (xcrit->socket->snoop)
	{
		if (strlistcmp(crit->name,xcrit->socket->snoop))
		{
			sendcrit(crit,"You are already snooping them.");
			return;
		}
		else
			str_dup(&xcrit->socket->snoop, str_add(xcrit->socket->snoop, crit->name, ' '));
	}
	else
		str_dup(&xcrit->socket->snoop,crit->name);
	sendcritf(crit,"You start snooping %s.",xcrit->name);
}


CMD(do_transfer)
{
	if(xcrit)
	{
		message("$n dissapear$x in a puff of smoke.",xcrit,0,0);
		trans(xcrit, crit->in_room);
		interpret(xcrit, "look");
		message("$n arrive$x in a puff of smoke.",xcrit,0,0);
	}
	else
	{
		message("$n dissapear$x in a puff of smoke.",xobj,0,0);
		trans(xobj, crit);
		message("$n pops in $Z hands.",xobj,crit,0);
	}
}


CMD(do_goto)
{
	ROOM *room;

	if (xcrit)
		room = xcrit->in_room;
	else if (xobj)
		room = xobj->in_room;
	else if (!(room = hashfind_room(atoi(arguments[0]))))
	{
		sendcrit(crit,"No such room.");
		return;
	}

	message("$n leave$x in a swirling mist.",crit,0,0);
	trans(crit,room);
	message("$n appears in a swirling mist.",crit,crit->in_room,0);
	interpret(crit,"look");
	return;
}


CMD(do_load)
{
	CREATURE *critp;
	OBJECT *objp;
	long vnum=0;

	if(!is_number(arguments[1]))
	{
		interpret(crit,"help load");
		return;
	}

	vnum = atoi(arguments[1]);

	if(!strindex("crit", arguments[0]) || !strindex("creature", arguments[0]))
	{
		if(!(critp = hashfind_creature(vnum)))
		{
			sendcrit(crit, "No such creautre.");
			return;
		}

		xcrit = new_creature(critp->vnum);
		str_dup(&xcrit->loaded, crit->name);
		trans(xcrit, crit->in_room);
		message("$n load$x $N!",crit,xcrit,0);
	}
	else if(!strindex("object", arguments[0]))
	{
		if(!(objp = hashfind_object(vnum)))
		{
			sendcrit(crit, "No such object.");
			return;
		}

		xobj = new_object(objp->vnum);
		str_dup(&xobj->loaded, crit->name);
		trans(xobj,crit);
		message("$n load$x $p!",crit,0,xobj);
	}
	else
	{
		interpret(crit, "load");
	}
}


CMD(do_stat)
{
	char buf[MAX_BUFFER];
	MYSQL_RES *result=0;
	MYSQL_ROW row;
	CREATURE *critp=0;
	CREATURE *critx=0;
	RESET *reset=0;
	EXIT *exit=0;
	ROOM *room=0;
	long time = current_time-604800;
	long num;

	if(!strindex("world", arguments[0]))
	{
		extern long total_creatures, total_objects, total_rooms, total_extras,
			total_exits, total_resets, total_shops, total_socials, total_notes;
		extern long total_icreatures, total_iobjects; // instances

		sendcritf(crit,"Creatures:      (%li) Prototypes and (%li) Instances",	total_creatures, total_icreatures );
		sendcritf(crit,"Objects:        (%li) Prototypes and (%li) Instances",
			total_objects, total_iobjects );
		sendcritf(crit,"Rooms:          %li",		total_rooms	);
		sendcritf(crit,"Exits:          %li",		total_exits	);
		sendcritf(crit,"Extras:         %li",    	total_extras	);
		sendcritf(crit,"Resets:         %li",		total_resets	);
		sendcritf(crit,"Shops:          %li",		total_shops	);
		sendcritf(crit,"Socials:        %li",		total_socials	);
		sendcritf(crit,"Notes:          %li",		total_notes	);

		mysql_query(mysql,"SELECT COUNT(*) FROM player");
		result = mysql_store_result(mysql);
		sendcritf(crit,"Players:        %-5li",	atoi(mysql_fetch_row(result)[0]));
		mysql_free_result(result);

		sprintf(buf,"SELECT COUNT(*) FROM player WHERE last_online > %li", time);
		mysql_query(mysql,buf);
		result = mysql_store_result(mysql);
		sendcritf(crit,"Active Players: %-5li",	atoi(mysql_fetch_row(result)[0]));
		mysql_free_result(result);

		mysql_query(mysql,"SELECT COUNT(*) FROM object WHERE owner_id > 0");
		result = mysql_store_result(mysql);
		sendcritf(crit,"Player Objects: %-5li",	atoi(mysql_fetch_row(result)[0]));
		mysql_free_result(result);
		return;
	}
	if(!strindex("active", arguments[0]))
	{
		sprintf(buf,"SELECT name FROM player WHERE last_online > %li ORDER BY last_online DESC", time);
		mysql_query(mysql,buf);
		result = mysql_store_result(mysql);

		buf[0] = '\0';
		num = 0;
		while((row = mysql_fetch_row(result)))
		{
			sprintf(buf+strlen(buf),"%-12s",row[0]);
			if(++num % 6 == 0)
				sprintf(buf+strlen(buf),"\n\r");
		}
		sendcritf(crit,"Active Players:\n\r%s",	buf);
		mysql_free_result(result);
		return;
	}
        if((!strindex("room", arguments[0]) || !strindex("here", arguments[0])) && !arguments[1])
                room = crit->in_room;
        if((!strindex("crit", arguments[0]) || !strindex("self", arguments[0])) && !arguments[1])
                critx = crit;
        if(!strindex("obj", arguments[0])  && !arguments[1])
        {
                sendcrit(crit,"What vnum do you wish to stat?");
                return;
        }
                        
        if(!strindex("obj", arguments[0]) && arguments[1] && !(xobj = hashfind_object(atoi(arguments[1]))))
        {
                sendcrit(crit,"No such object vnum.");
                return;
        }
 
        if(!strindex("room", arguments[0]) && arguments[1] && !(room = hashfind_room(atoi(arguments[1]))))
        {
                sendcrit(crit,"No such room vnum.");
                return;   
        }
        
        if(!xcrit && !strindex("crit", arguments[0]) && arguments[1] && !(critp = hashfind_creature(atoi(arguments[1]))))
        {
                sendcrit(crit,"No such creature vnum.");
                return;
        }
        if (xcrit)
                critx = xcrit;
                
        if (xobj)
        { 
                crit->socket->editing = (void*)xobj;
                interpret(crit,"");
		crit->socket->editing = 0;
        }
	if(critp || critx)
	{
		if (!critx)
			critx = new_creature(critp->vnum);
		sendcritf(crit,"Name:              %s",		critx->name);
		sendcritf(crit,"Hit points:        %li|%li",	critx->hp,critx->max_hp);
		sendcritf(crit,"Movement:          %li|%li",	critx->move,critx->max_move);
		sendcritf(crit,"Level:             %li",	critx->level);
		sendcritf(crit,"In Room:           %li",	critx->in_room ? critx->in_room->vnum : -1);
		sendcritf(crit,"&+W %s&n\n\r",
			critx->in_room ? critx->in_room->description : "Not in a room.\n\r");
		sendcritf(crit,"Wear locations:    %s",		critx->wear); 
		sendcritf(crit,"Position:          %s",		position_table[crit->position]);
		sendcritf(crit,"State:             %s",		state_table[crit->state]);
		if (IsPlayer(critx))
		{
			sendcritf(crit,"Last command:      %s",	critx->socket->last_command);
			sendcritf(crit,"Prompt:            %s",	critx->socket->prompt);
			sendcritf(crit,"Hrs from GMT:	   %li",	critx->socket->hrs);
       	        	sendcritf(crit,"Lines: 		   %li",    critx->socket->lines);
			sendcritf(crit,"Host:              %s",	critx->socket->host);
			sendcritf(crit,"IP:                %s",	critx->socket->ip);
			sendcritf(crit,"Crit Flags:        %s",strflags(critx));
		}
		else
		{
			sendcritf(crit,"Vnum:              %li",critp ? critp->vnum : -1);
	 		sendcritf(crit,"Shopkeeper:        %s",	critx->shop ? "&+GYes.&n" : "&+RNo.&n");
			if (!xcrit)
				free_creature(critx);
		}
	}
	if(room)
	{
		sendcritf(crit,"[%li][&+R%s&N]",room->vnum,room->name);
		sendcritf(crit,"[&+MDescription&N]\n\r%s\n\r&N[&+MEnd Description&N]",room->description);

		for(exit = room->exits; exit; exit = exit->next)
		sendcritf(crit,"[&+GExit %s to [%li]%s&N][Door:%s][Key:%li]",
			exit->name,exit->to_vnum,
			hashfind_room(exit->to_vnum)?hashfind_room(exit->to_vnum)->name:"????",
			exit->door > DOOR_NONE ? "Yes" : "No",
			exit->key);

		buf[0] = '\0';
		send_to(crit->socket,"Crit Contents[");
		for(critx = room->crits; critx; critx = critx->next_content)
		sprintf(buf+strlen(buf),"&+B%s&N%s",critx->name,
			critx->next_content?", ":"");
		sprintf(buf+strlen(buf),"&N]");
		sendcrit(crit,buf);

		buf[0] = '\0';
		send_to(crit->socket,"Object Contents[");
		for(xobj = room->objects; xobj; xobj = xobj->next_content)
		sprintf(buf+strlen(buf),"&+B%s&N%s",xobj->name,xobj->next_content?", ":"");
		sprintf(buf+strlen(buf),"&N]");
		sendcrit(crit,buf);

		for(reset = room->resets, num=0; reset; reset = reset->next, num++)
		{
			sprintf(buf,		"&+C[%2li]Reset[%-4s][C:%s][V:%li][T:%li][%%:%li]\n\r", num,
				reset->loadtype == TYPE_CREATURE ? "Crit" : reset->loadtype == TYPE_EXIT ? "Exit" : "Obj",
				ValidString(reset->command) ? reset->command : "None", reset->vnum,
				reset->time, reset->chance);
			sprintf(buf+strlen(buf),"           &+C[Load:%li][Max:%li]",
				reset->loaded, reset->max );
			sendcrit(crit,buf);
		}

		return;
	}
}


CMD(do_repeat)
{
	char buf[MAX_BUFFER];
	long i, num;

	if(!is_number(arguments[0]))
	{
		interpret(crit,"help repeat");
		return;
	}

	num = atoi(arguments[0]);
	strcpy(buf, all_args(arguments,1));

	for(i = 0; i < num; i++)
		if(IsPlaying(crit))
			interpret(crit,buf);
	if(IsPlaying(crit))
		sendcrit(crit,"Done!");
}


CMD(do_users)
{
	MSOCKET *socket;
	extern char **connect_attempts;
	long i;

		sendcritf(crit, "%-15s : %-15s : %s\n\r%s", "Name","IP Address","Hostname",
				"------------------------------------------------------");
	for(socket = socket_list; socket; socket = socket->next)
	{
		if(!ValidString(socket->host) || strcmp(socket->ip, socket->last_ip)
		|| (arguments[0] && !strcasecmp(arguments[0],"all")) )
			get_hostname(socket);

		sendcritf(crit, "%-15s : %-15s : %s",
			socket->pc ? socket->pc->name : "???",
			socket->ip, socket->host);
	}

	sendcrit(crit,"\n\rRecent connection attempts:");

	if(!connect_attempts[0])
		sendcrit(crit,"None.");
	else
		for(i = 0; connect_attempts[i]; i++)
			sendcritf(crit,"%s",connect_attempts[i]);
}


CMD(do_findvnum)
{
	extern CREATURE *hash_creature[HASH_KEY];
	extern OBJECT *hash_object[HASH_KEY];

	char name[MAX_BUFFER];
	long found, key;

	strcpy(name,all_args(arguments,0));

	sendcritf(crit,"Searching for creatures with keywords and names containing '%s'",name);
	key = 0;
	found = 0;
	while(key < HASH_KEY)
	{
		for(xcrit = hash_creature[key%HASH_KEY]; xcrit; xcrit = xcrit->next_hash )
		{
			if(strlistcmp(xcrit->name,name)
			|| strlistcmp(xcrit->keywords,name))
			{
				sendcritf(crit,"[%5li] %s",xcrit->vnum, xcrit->name);
				found++;
			}
		}
		key++;
	}		
	if(!found)
		sendcrit(crit,"No creatures found.");

	sendcritf(crit,"\n\rSearching for objects with keywords and names containing '%s'",name);
	key = 0;
	found = 0;
	while(key < HASH_KEY)
	{
		for(xobj = hash_object[key%HASH_KEY]; xobj; xobj = xobj->next_hash )
		{
			if(strlistcmp(xobj->name,name)
			|| strlistcmp(xobj->keywords,name))
			{
				sendcritf(crit,"[%5li] %s",xobj->vnum, xobj->name);
				found++;
			}
		}
		key++;
	}
	if(!found)
		sendcrit(crit,"No objects found.");
}
