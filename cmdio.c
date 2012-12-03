/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
cmdio.c -- The purpose of this file is to handle the I/O to the database from
player driven commands.
*/

#include "stdh.h"
#include "io.h"


HELP *mysql_find_help(CREATURE *crit, char **arguments)
{
	HELP *help=0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	MYSQL_ROW trow;
	MYSQL_RES *tresult;
	char buf[MAX_BUFFER];
	char buf2[MAX_BUFFER];
	long rownum = 0;
	long found = 0;
	long help_found = 1; 
	long last = last_arg(arguments);
	long count = 0;
	long exact = -1;

	if (!strcmp(arguments[0],"all") && !arguments[1])
	{
		sprintf(buf,"SELECT * FROM help WHERE level <= %li",crit->level);
		mysql_query(mysql,buf);
		result = mysql_store_result(mysql);
		while ((row = mysql_fetch_row(result)))
			sendcritf(crit,"[&+W%s&n]",row[H_KEYWORD]);
		mysql_free_result(result);
		return 0;
	}
	if (!strcmp(arguments[0],"all"))
		exact = -2;
	if (!strargcmp("index category topic topics",arguments[0]))
	{
		sprintf(buf,"SELECT DISTINCT hindex FROM help where level <= %li",crit->level);
		mysql_query(mysql,buf);
		result = mysql_store_result(mysql);
		if (!arguments[1])
		{
			if (mysql_num_rows(result) <= 0)
			{
				sendcrit(crit,"There are no current help indexes.");
				mysql_free_result(result);
				return 0;
			}
			sendcrit(crit,"These are the help topics.  See \"&+Whelp topic &n<&+ctopic&n>\" for results.");
			while ((row = mysql_fetch_row(result)))
				sendcritf(crit,"[&+c%s&n]",row[0]);
			mysql_free_result(result);
			return 0;
		}
		else
		{
			while ((row = mysql_fetch_row(result)))
			{
				if (!strindex(arguments[1],row[0]))
				{
					sprintf(buf,"SELECT * FROM help WHERE hindex=\"%s\" AND level <= %li",row[0],crit->level);
					mysql_query(mysql,buf);
					tresult = mysql_store_result(mysql);
					sendcritf(crit,"Results for topic [&+c%s&n]:",row[0]); 
					while ((trow = mysql_fetch_row(tresult)))
						sendcritf(crit,"[&+W%s&n]",trow[H_KEYWORD]);
					mysql_free_result(tresult);
					mysql_free_result(result);
					return 0;
				}
			}
			mysql_data_seek(result,0);
			sendcrit(crit,"That's an invalid topic.  Here are the topics:");
			while ((row = mysql_fetch_row(result)))
				sendcritf(crit,"[&+c%s&n]",row[0]);
			mysql_free_result(result);
			return 0;
		}
	}

	sprintf(buf,"SELECT * FROM help WHERE level <= %li",crit->level);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
  		sendcrit(crit, "Bug!  Immortals have been notified.");
		mudlog("in mysql_find_help: mysql_error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	if (mysql_num_rows(result) == 0)
	{
		// They'll get this message if they can't see any helpfiles based on level.
		sendcrit(crit,"There are no help files at all right now!");
		mysql_free_result(result);
		return 0;
	}

	if(arguments[last] && is_number(arguments[last]))
		rownum = atoi(arguments[last]);
	if (rownum || !strcmp(arguments[last],"0"))
	{
		exact = 0;
		count = 0;
		while ((row = mysql_fetch_row(result)))
		{
			if (!indexinset(row[H_KEYWORD],split_args(arguments,0,last-1),','))
			{
				count++;
				if(rownum == count)
				{
					help_found = 1;
					break;
				}
			}
			help_found = 0;
		} 
		if (!help_found)
		{
			if (count > 0)
			{
				if (rownum <= 0)
					sendcritf(crit,"That row number is too low for the &+c%s&n keyword.",split_args(arguments,0,last-1));
				else
					sendcritf(crit,"That row number is too high for the &+c%s&n keyword.",split_args(arguments,0,last-1));
				mysql_free_result(result);
				return 0;
			}
		}
	}	
    	else if (!rownum)
	{
		count = 0;
		while ((row = mysql_fetch_row(result)))
		{
			if (exact != -2)
			{
				if (!findinset(row[H_KEYWORD],all_args(arguments,0),','))
					exact = rownum;
			}

			if (!indexinset(row[H_KEYWORD],all_args(arguments,exact == -2 ? 1 : 0),','))
			{
				count++;
				found = rownum;
				if (count == 1)
				{
					sprintf(buf, "Multiple results.  Specify by using the keyword and number below.\r\n");
					strcat(buf, "-----------------------------------------------------------------\r\n");
				}
				sprintf(buf2,"[%li] [&+W%s&n]\r\n",count,row[H_KEYWORD]);
				strcat(buf,buf2);
			}
			rownum++;
		}
		if (exact > -1)
		{
			if (count > 1)
			{
				count--;
				sendcritf(crit,"There %s %li other keyword%s that match%s also.  Type &+Whelp all %s&n to view %s.\r\n",
					count == 1 ? "is" : "are", count, count == 1 ? "" : "s",count == 1 ? "es" : "",
					all_args(arguments,0),	count == 1 ? "it" : "them");
			}
			mysql_data_seek(result,exact);
			row = mysql_fetch_row(result);
		}
		else if (count > 1)
		{
			send_to(crit->socket,buf);
			sendcrit(crit, "-----------------------------------------------------------------");
			mysql_free_result(result);	
			return 0;
		}
		else if (count == 1)
		{
			mysql_data_seek(result,found);
			row = mysql_fetch_row(result);
		}
		else
			help_found = 0;
		rownum = 0;
	}

	if (help_found)
	{
		help		= 		new_help();
		help->id	= 		atoi(row[0]);
		str_dup(&help->keyword, 	row[H_KEYWORD]);
		str_dup(&help->entry,		row[H_ENTRY]);
		str_dup(&help->index,		row[H_INDEX]);
		help->level = 			atoi(row[H_LEVEL]);

		sprintf(buf,"SELECT UNIX_TIMESTAMP('%s')",row[H_LAST]);
		mysql_query(mysql,buf);
		tresult = mysql_store_result(mysql);	
	
		trow = mysql_fetch_row(tresult);
		// last updated date/time
		str_dup(&help->last, trow[0]);
		mysql_free_result(result);
		mysql_free_result(tresult);

		return help;
	}
	
	// no keyword match found
	sprintf(buf,"%s",rownum ? split_args(arguments,0,last-1) : all_args(arguments,0));
	sendcritf(crit, "No help file found for the keyword '&+c%s&n'.", buf);		
	mysql_data_seek(result,0);
	sendcritf(crit,"Checking for any helpfiles that contain '&+c%s&n' anywhere in them:\n\r",buf);
	rownum = 0;
	while ((row = mysql_fetch_row(result)))
	{
		if (strstr(row[H_ENTRY],buf) || strstr(row[H_KEYWORD],buf))
		{
			sendcritf(crit,"[&+W%s&n]",row[H_KEYWORD]);
			rownum++;
		}		
	}
	mysql_free_result(result);
	if (!rownum)
		sendcrit(crit,"None found.");
	return 0;
}


bool can_remove_note(CREATURE *crit, char *names, char *sender)
{
	if (crit->level == LEVEL_IMP)
		return 1;

	if (!strargcmp(crit->name, names))
		return 1;

	if (!strcasecmp(crit->name, sender))
		return 1;

	return 0;
}


bool can_read_note(CREATURE *crit, char *names, char *sender)
{
	if (!strargcmp(names, "all"))
		return 1;

	if (!strargcmp(names, crit->name))
		return 1;

// FIX TODO
	if (!strcasecmp(crit->name,"pip") || !strcasecmp(crit->name,"borlak") || !strcasecmp(crit->name,"crystal"))
		return 1;
//	if (crit->level == LEVEL_IMP)
//		return 1;

	if (!strcasecmp(crit->name, sender))
		return 1;

	return 0;
}


CMD(do_note)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	MSOCKET *socket;
	NOTE *note=0;
	bool show_all = 1;
	char buf[MAX_BUFFER];
	long z = 0;
	long value = 0;
	bool found = 0;
	time_t notetime=0;

	if (!strcasecmp(arguments[0],"list"))
	{
		interpret(crit,"note read");
		return;
	}

	if (!strcasecmp(arguments[0],"read"))
	{
		if (arguments[1] && !is_number(arguments[1]))
		{
			sendcrit(crit,"Which note do you want to read?");
			return;
		}

		if (arguments[1])
		{
			value = atoi(arguments[1]);
			show_all = 0;
		}

		for (note = note_list; note; note = note->next)
		{
			if (!can_read_note(crit, note->sent_to, note->sender))
				continue;

			if (show_all)
			{
				for(socket = socket_list; socket; socket = socket->next)
				{
					if(IsEditing(socket->pc)
					&& (NOTE*)socket->editing == note)
						break;
				}
				sendcritf(crit,"[%3li] %s%s: %s",
					z,
					socket ? "&+Y[EDIT]&N " : "",
					note->sender, note->subject);
			}
			if (arguments[1] && z == value)
			{
				notetime = atoi(note->written);
				sendcritf(crit,"[&+W%3li&n] &+C%s: &n&+c%s&n\n\r%s\n\r&n&+WTo: &+y%s\n\r%s",z,note->sender,note->subject,
				IsPlaying(crit) ? display_timestamp(notetime,crit->socket->hrs) :
				display_timestamp(notetime,0),note->sent_to,note->text);
				found = 1;
				break;
			}
			z++;
		}
		if (!show_all && !found)
			sendcrit(crit,"No such note.");

		if (show_all)
		{
			if (z == 0)
				sendcrit(crit,"The note list is empty.");
			else
				sendcritf(crit,"\n\rA total of %li notes.",z);
		}
		return;
	}
	if (!strcasecmp(arguments[0],"remove"))
	{
		if (!arguments[1])
		{
			sendcrit(crit,"Which note do you want to remove?");
			return;
		}

		if (!is_number(arguments[1]))
		{
			sendcrit(crit,"Use numbers to reference which note you wish to remove.");
			return;
		}

		value = atoi(arguments[1]);

		mysql_query(mysql,"SELECT sender, sent_to, id FROM notes ORDER BY id");
		result = mysql_store_result(mysql);

		while ((row = mysql_fetch_row(result)))
		{
			if (!can_read_note(crit, row[1], row[0]))
				continue;

			if (z == value)
			{
				if (!can_remove_note(crit,row[1],row[0]))
				{
					sendcrit(crit,"You do not have the authority to remove this note.");
					mysql_free_result(result);
					return;
				}

				sprintf(buf,"DELETE FROM notes WHERE id='%s'",row[2]);
				mysql_query(mysql,buf);

				if (mysql_affected_rows(mysql) < 1)
				{
					mudlog("Error deleting note from database: %s",mysql_error(mysql));
					mysql_free_result(result);
					return;
				}
				sendcrit(crit,"Note deleted.");
				found = 1;
				break;
			}
		z++;
		}

		z = 0;
		for (note = note_list; note; note = note->next)
		{
			if (!can_read_note(crit, note->sent_to, note->sender))
				continue;
			if (z == value)
			{
				if (!can_remove_note(crit,row[1],row[0]))
				{
					sendcrit(crit,"You do not have the authority to remove this note.");
					mysql_free_result(result);
					return;
				}
				RemoveFromList(note_list, note)
				DeleteObject(note)
				break;
			}
			z++;
		}
		if (!found)
		{
			sendcrit(crit,"No such note.");
			mysql_free_result(result);
			return;
		}
		mysql_free_result(result);
		return;
	}
	interpret(crit, "note");
}


CMD(do_help)
{
	char buf[MAX_BUFFER];
	HELP *help;
	time_t helptime;

	if (!arguments[0])
	{
		interpret(crit,"help help");
		return;	
	}

	if(!(help = mysql_find_help(crit, arguments)))
		return;

	helptime = atoi(help->last);
	sprintf(buf,"[&+L%s&n] [&+W%s&n]\n\r%s\n\r\n\r&+WLast updated on:&n\n\r%s",
		help->index, help->keyword, help->entry, 
		IsPlaying(crit) ? display_timestamp(helptime,crit->socket->hrs) : display_timestamp(helptime,0));
	free_help(help);

	sendcrit(crit,buf);
}


CMD(do_credits)
{
	interpret(crit, "help credits");
}


// player input commands.. bug/idea/typo
void insert_player_input(CREATURE *crit, char *type, char *input)
{
	char query[MAX_BUFFER*2];

	if(crit->socket->save_time)
	{
		sendcritf(crit, "You must wait %li seconds before you can submit your %s", crit->socket->save_time, type);
		return;
	}

	if(strlen(input) > MAX_BUFFER)
		input[MAX_BUFFER] = '\0';

	sprintf(query, "INSERT INTO player_input (playerID, name, type, room, input)	\
			VALUES(%li, '%s', '%s', %li, '%s')",
		crit->socket->id, crit->name, type, crit->in_room->vnum, input);
	mysql_query(mysql, query);

	crit->socket->save_time = 10;
}


CMD(do_bug)
{
	insert_player_input(crit, "Bug", all_args(arguments,0));
}


CMD(do_idea)
{
	insert_player_input(crit, "Idea", all_args(arguments,0));
}


CMD(do_typo)
{
	insert_player_input(crit, "Typo", all_args(arguments,0));
}



