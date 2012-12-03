/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
io.c:
Purpose of this file: Input/Output.  Use this file for reading
and writing with files and to MySQL server.
*/

#include "stdh.h"
#include "io.h"
#include <ctype.h>

/////////////////////
// LOCAL VARIABLES //
/////////////////////
long	total_rooms		= 0;
long	total_creatures		= 0;
long	total_extras		= 0;
long	total_notes		= 0;
long	total_objects		= 0;
long	total_exits		= 0;
long	total_resets		= 0;
long	total_socials		= 0;
long	total_shops		= 0;

long	fread_creatures		(AREA *area);
long	fread_exits		();
long	fread_extras		();
long	fread_notes		();
long	fread_objects		(AREA *area);
long	fread_resets		(AREA *area);
long	fread_rooms		(AREA *area);
long	fread_socials		();
long	fread_shops		(AREA *area);

void	fwrite_creature		(long vnum);
void	fwrite_object		(OBJECT *obj);
void	fwrite_room		(long vnum);
void	fwrite_shop		(SHOP *shop);




extern	long			mud_shutdown;

///////////////////////
// UTILITY FUNCTIONS //
///////////////////////
// read a number from a file...negative numbers work
long fread_number(FILE *fp)
{
	long num,total;
	char k;

	do
	{
		k = getc(fp);
	} while(isspace(k));
	ungetc(k,fp);

	fscanf(fp,"%li",&num);

	total = num;
	while(1)
	{
		k = getc(fp);
		if(k == '|')
		{
			fscanf(fp,"%li",&num);
			total += num;
		}
		else
		{
			ungetc(k, fp);
			break;
		}
	}
	return total;
}


// These are used to build MySQL Query Strings.  You supply the keyword and value,
// and it appends it onto the current query string.  Makes code a lot cleaner.
char *add_str(char *buf, char *keyword, char *value)
{
        sprintf(buf+strlen(buf),",%s=\"%s\" ",keyword, smash_quotes(value));
        return buf;
}
char *add_int(char *buf, char *keyword, long value)
{
	sprintf(buf+strlen(buf),",%s='%li' ",keyword, value);
	return buf;
}

// this keeps reading all characters, numbers and letters and anything,
// until the end of a line
char *fread_line(FILE *fp)
{
	static char buf[MAX_BUFFER];
	long i;

	buf[0] = '\0';
	if(!fgets(buf, MAX_BUFFER, fp))
		return (char*)EOF;

	// chop off newline
	for(i = 0; buf[i] != '\0'; i++)
	{
		if(buf[i] == '\n' || buf[i] == '\r')
		{
			buf[i] = '\0';
			break;
		}
	}

	return buf;
}

char *fread_word(FILE *fp)
{
	static char buf[MAX_BUFFER];
	long i;

	buf[0] = '\0';
	if (!fgets(buf,MAX_BUFFER, fp))
		return (char*)EOF;

	i = 0;
	while(buf[i] == ' ')
		i++;
	for ( ;buf[i] != '\0'; i++)
	{
		if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == ' ')
		{
			buf[i] = '\0';
			break;
		}
	}
	return buf;
}


// These two functions make/read token strings that are stored in mysql, so you
// don't have to keep multiple columns for these values.  Example string would
// be "0|23|1|22"
char *write_values(long values[])
{
	static char buf[MAX_BUFFER];
	long i;

	buf[0]	= '\0';

	for(i = 0; i < MAX_OBJVALUES; i++)
	{
		sprintf(buf+strlen(buf),"%li",values[i]);
		if(i+1 < MAX_OBJVALUES)
			strcat(buf,"|");
	}

	return buf;
}
void read_values(char *valuebuf, long values[])
{
	long i;
	char *flagp;

	values[0] = atoi(strtok(valuebuf,"|"));
	for(i = 1; i < MAX_OBJVALUES; i++)
	{
		values[i] = 0;
		if(!(flagp=strtok(0,"|")))
			continue;
		values[i] = atoi(flagp);
	}
}


/////////////////////
// FWRITE - SAVING //
/////////////////////
long fwrite_area(AREA *area, long all)
{
	char buf[666]; // cute
	long i;
	OBJECT *obj=0;

	sprintf(buf,"%s area SET name=\"%s\"", area->id ? "UPDATE" : "INSERT INTO",smash_quotes(area->name));
	add_str(buf,"builders",	area->builders	);
	add_int(buf,"low",	area->low	);
	add_int(buf,"high",	area->high	);
	if(area->id)
		sprintf(buf+strlen(buf)," WHERE id='%li'", area->id);

	mysql_query(mysql,buf);

	if(!area->id)
		area->id = mysql_insert_id(mysql);

	if(all)
	{
		for( i = area->low; i <= area->high; i++ )
		{
			fwrite_room(i);
			if ((obj = hashfind_object(i)))
				fwrite_object(obj);
			fwrite_creature(i);
		}
	}

	return 1;
}


void fwrite_bans(void)
{
	BAN *ban;
	char buf[MAX_BUFFER];

	for(ban = ban_list; ban; ban = ban->next)
	{
		sprintf(buf,"%s ban SET ip=\"%s\"",
			ban->id ? "UPDATE" : "INSERT INTO",
			ban->ip);
		add_str(buf,"message",	ban->message	);
		add_str(buf,"name",	ban->name	);
		add_int(buf,"bantype",	ban->bantype	);
		if(ban->id)
			sprintf(buf+strlen(buf)," WHERE id='%li'", ban->id);
		mysql_query(mysql, buf);

		if(!ban->id)
			ban->id = mysql_insert_id(mysql);
	}
}


void fwrite_creature(long vnum)
{
	CREATURE *crit;
	char buf[MAX_BUFFER];

	if((crit = hashfind_creature(vnum)) != 0)
	{
		sprintf(buf,"%s creature SET name=\"%s\"",
			crit->id ? "UPDATE" : "INSERT INTO",
			smash_quotes(crit->name));

		WriteCreatureMysql()

		if(crit->id)
			sprintf(buf+strlen(buf)," WHERE id='%li'",crit->id);

		mysql_query(mysql,buf);

		if(!crit->id)
			crit->id = mysql_insert_id(mysql);

		fwrite_shop(crit->shop);
	}
}


void fwrite_extra(EXTRA *extra)
{
	char buf[MAX_BUFFER];

	sprintf(buf,"%s extra SET keywords=\"%s\"",
			extra->id ? "UPDATE" : "INSERT INTO",
			smash_quotes(extra->keywords));
	add_str(buf,"description",	extra->description	);
	add_int(buf,"loadtype",		extra->loadtype		);
	add_int(buf,"vnum",		extra->vnum		);

	if(extra->id)
		sprintf(buf+strlen(buf)," WHERE id='%li'",extra->id);
	mysql_query(mysql,buf);

	if(!extra->id)
		extra->id = mysql_insert_id(mysql);
}


void fwrite_help(HELP *help)
{
	char buf[MAX_BUFFER];

	// save to mysql.
	sprintf(buf,"%s help SET keyword=\"%s\"",
			help->id ? "UPDATE" : "INSERT INTO",
			smash_quotes(help->keyword));
	add_str(buf,"entry",       	help->entry);
	add_str(buf,"hindex",		help->index);
	add_int(buf,"level",		help->level);
	if(help->id)
		sprintf(buf+strlen(buf)," WHERE id='%li'",help->id);
	mysql_query(mysql,buf);

	// free from memory.
	free_help(help);
}


void fwrite_note(NOTE *note)
{
	char buf[MAX_BUFFER];

	// save to mysql.
	sprintf(buf,"INSERT INTO notes SET note='%s'", smash_quotes(note->text));
	add_str(buf,"sender",		note->sender)		;
	add_str(buf,"subject",		note->subject		);
	add_str(buf,"sent_to",		note->sent_to)		;
	mysql_query(mysql,buf);
	sprintf(buf,"%li",current_time);
	str_dup(&note->written,buf);
}


void fwrite_object(OBJECT *obj)
{
	CREATURE *crit=0;
	char buf[MAX_BUFFER];

	crit = HeldBy(obj);

	// mobs don't save their objects.. mobs aren't persistant
	if(crit && !flag_isset(crit->flags, CFLAG_PLAYER))
		return;

	sprintf(buf,"%s object SET name=\"%s\"",
		obj->id ? "UPDATE" : "INSERT INTO",
		smash_quotes(obj->name));

	WriteObjectMysql()

	add_int(buf,"owner_id",	crit ? crit->id : 0			);
	add_int(buf,"in_obj",	obj->in_obj ? obj->in_obj->id : 0	);

	if(obj->id)
		sprintf(buf+strlen(buf)," WHERE id='%li'",obj->id);

	mysql_query(mysql,buf);

	if(!obj->id)
		obj->id = mysql_insert_id(mysql);
}


void fwrite_player(CREATURE *crit)
{
	MSOCKET *sock = crit->socket;
	char buf[MAX_BUFFER];

	if(!IsPlayer(crit))
		return;

	sprintf(buf,"%s creature SET name=\"%s\"",
		crit->id ? "UPDATE" : "INSERT INTO",
		smash_quotes(crit->name));

	WriteCreatureMysql()

	add_int(buf,"room",crit->in_room->vnum);

	if(crit->id)
		sprintf(buf+strlen(buf),"WHERE id='%li'",crit->id);
	mysql_query(mysql,buf);

	if (!crit->id)
		crit->id = mysql_insert_id(mysql);

	if(sock)
	{
		sprintf(buf,"%s player SET name=\"%s\"",
			sock->id ? "UPDATE" : "INSERT INTO", smash_quotes(crit->name));
		add_str(buf,"host",		sock->host				);
		add_str(buf,"ip",		sock->ip				);
		add_str(buf,"last_command",	sock->last_command			);
		add_int(buf,"last_online",	current_time				);
		add_int(buf,"hrs",		sock->hrs				);
		add_int(buf,"numlines",		sock->lines				);
		add_str(buf,"password",		sock->password				);
		add_str(buf,"prompt",		sock->prompt				);
		add_str(buf,"title",		sock->title				);
		add_str(buf,"who_name",		sock->who_name				);
		add_int(buf,"online",		mud_shutdown ? 0 : sock->connected == CON_PLAYING ? 1 : 0 );
		add_int(buf,"descriptor",	sock->desc				);

		if(sock->id)
			sprintf(buf+strlen(buf),"WHERE id='%li'",sock->id);

		mysql_query(mysql,buf);
	
		if(!sock->id)
			sock->id = mysql_insert_id(mysql);
	}
	return;
}


void fwrite_resets(ROOM *room)
{
	char buf[MAX_BUFFER];
	RESET *reset;

	for(reset = room->resets; reset; reset = reset->next)
	{
		sprintf(buf,"%s reset SET chance='%li'",
			reset->id ? "UPDATE" : "INSERT INTO", reset->chance);

		WriteResetMysql(reset)

		if(reset->id)
			sprintf(buf+strlen(buf)," WHERE ID='%li'",reset->id);
		mysql_query(mysql,buf);

		if(!reset->id)
			reset->id = mysql_insert_id(mysql);
	}
}


void fwrite_room(long vnum)
{
	char buf[MAX_BUFFER];
	EXIT *exit;
	ROOM *room;

	if( (room = hashfind_room(vnum)) != 0 )
	{
		sprintf(buf,"%s room SET name=\"%s\"",
			room->id ? "UPDATE" : "INSERT INTO", room->name);
		add_str(buf,"night_name",	room->night_name	);
		add_int(buf,"vnum",		vnum			);
		add_str(buf,"loaded",		room->loaded		);
		add_str(buf,"description",	room->description	);
		add_str(buf,"night_description",room->night_description	);
		add_int(buf,"roomtype",		room->roomtype		);
		add_int(buf,"light",		room->light		);
		add_str(buf,"flags",		write_flags(room)	);
		if(room->id)
			sprintf(buf+strlen(buf)," WHERE id='%li'",room->id);

		mysql_query(mysql,buf);

		if(!room->id)
			room->id = mysql_insert_id(mysql);

		for (exit = room->exits; exit; exit = exit->next)
		{
			sprintf(buf,"%s room_exit SET name=\"%s\"",
				exit->id ? "UPDATE" : "INSERT INTO", exit->name);
			add_int(buf,"room",	room->vnum	);
			add_int(buf,"door",	exit->door	);
			add_int(buf,"dkey",	exit->key	);
			add_int(buf,"to_vnum",	exit->to_vnum	);
			if(exit->id)
				sprintf(buf+strlen(buf)," WHERE id='%li'", exit->id);
			mysql_query(mysql,buf);

			if(!exit->id)
				exit->id = mysql_insert_id(mysql);
		}
		fwrite_resets(room);
	}
}


void fwrite_shop(SHOP *shop)
{
	char buf[MAX_BUFFER];

	if (!shop)
		return;

	sprintf(buf,"%s shop SET keeper ='%li'",
		shop->id ? "UPDATE" : "INSERT INTO", shop->keeper);
	add_int(buf,"type",		shop->stype		);
	add_int(buf,"buy",		shop->buy		);
	add_int(buf,"item",		shop->item		);
	add_int(buf,"sell",		shop->sell		);
	add_int(buf,"open",		shop->open		);
	add_int(buf,"close",		shop->close		);
	if(shop->id)
		sprintf(buf+strlen(buf)," WHERE id='%li'",shop->id);
	mudlog(buf);
	mysql_query(mysql,buf);

	if(!shop->id)
		shop->id = mysql_insert_id(mysql);
}


void fwrite_social(SOCIAL *social)
{
	char buf[MAX_BUFFER];

	sprintf(buf,"%s social SET name=\"%s\"",
		social->id ? "UPDATE" : "INSERT INTO", social->name);
	add_str(buf,"no_target",	social->no_target	);
	add_str(buf,"crit_target",	social->crit_target	);
	add_str(buf,"object_target",	social->object_target	);
	add_str(buf,"self_target",	social->self_target	);
	if(social->id)
		sprintf(buf+strlen(buf)," WHERE id='%li'", social->id);

	mysql_query(mysql,buf);

	if(!social->id)
		social->id = mysql_insert_id(mysql);
}


void fwrite_time(void)
{
	char buf[MAX_BUFFER];

	get_mudtime();

	sprintf(buf,"%s time SET start=\"%li\"",
		mudtime.id ? "UPDATE" : "INSERT INTO",
		mudtime.id ? mudtime.start : current_time );
	add_int(buf,"end",	current_time		);
	add_int(buf,"backup",	mudtime.backup		);
	add_int(buf,"mudstatus",mudtime.mudstatus	);
	add_int(buf,"age",	mudtime.age		);
	add_int(buf,"year",	mudtime.year		);
	add_int(buf,"month",	mudtime.month		);
	add_int(buf,"day",	mudtime.day		);
	add_int(buf,"hour",	mudtime.hour		);
	add_int(buf,"minute",	mudtime.minute		);
	add_int(buf,"second",	mudtime.second		);
	if(mudtime.id)
		sprintf(buf+strlen(buf)," WHERE id=\"%li\"", mudtime.id);
	mysql_query(mysql,buf);

	if(!mudtime.id)
		mudtime.id = mysql_insert_id(mysql);
}


/////////////////////
// FREAD - LOADING //
/////////////////////
long fread_area(MYSQL_ROW row)
{
        AREA *area;
	long rooms, objects, creatures, resets, shops;

	area = new_area(row[A_NAME]);

	area->id			= atoi(row[0]);
	area->low			= atoi(row[A_LOW]);
	area->high			= atoi(row[A_HIGH]);
	str_dup(&area->builders,	row[A_BUILDERS]);

	// make sure rooms are loaded before resets...
	total_rooms	+= rooms	= fread_rooms(area);
	total_objects	+= objects	= fread_objects(area);
	total_creatures	+= creatures	= fread_creatures(area);
	total_resets	+= resets	= fread_resets(area);
	total_shops 	+= shops	= fread_shops(area);

	mudlog("Loading area %s: %li rooms, %li objects, %li creatures, %li shops, and %li resets loaded.",
		area->name, rooms, objects, creatures, shops, resets );

	return 1;
}


long fread_bans(void)
{
	BAN *ban;
	MYSQL_RES *result;
	MYSQL_ROW row;
	long z = 0;

	mysql_query(mysql,"SELECT * FROM ban");
	result = mysql_store_result(mysql);

	while((row = mysql_fetch_row(result)))
	{
		ban = new_ban(0);

		ban->id		= atoi(row[0]);

		str_dup(&ban->ip,	row[B_IP]	);
		str_dup(&ban->message,	row[B_MESSAGE]	);
		str_dup(&ban->name,	row[B_NAME]	);

		ban->bantype	= atoi(row[B_BANTYPE]);

		z++;
	}
	mysql_free_result(result);
	return z;
}


long fread_creatures(AREA *area)
{
	CREATURE *crit;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];
	long z=0;
	long vnum;

	sprintf(buf,"SELECT * FROM creature WHERE vnum >= '%li' AND vnum <= '%li' AND prototype='1'",
		area->low, area->high);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_creature, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		vnum = atoi(row[C_VNUM]);

		if(hashfind_creature(vnum))
		{
			mudlog("Found creature %d already in hashfind in fread_creature!",vnum);
			continue;
		}

		crit = new_creature_proto(atoi(row[C_VNUM]));

		ReadCreatureMysql()

		z++;
	}
	mysql_free_result(result);
	return z;
}


long fread_exits(void)
{
	char buf[MAX_BUFFER];
	MYSQL_RES *result;
	MYSQL_ROW row;
	ROOM *room;
	EXIT *exit;

	sprintf(buf,"SELECT * FROM room_exit			\
		     ORDER BY					\
			CASE WHEN name='north'		THEN 1	\
			     WHEN name='northeast'	THEN 2	\
			     WHEN name='east'		THEN 3	\
			     WHEN name='southeast'	THEN 4	\
			     WHEN name='south'		THEN 5	\
			     WHEN name='southwest'	THEN 6	\
			     WHEN name='west'		THEN 7	\
			     WHEN name='northwest'	THEN 8	\
			     WHEN name='up'		THEN 9	\
			     WHEN name='down'		THEN 10	\
			ELSE 11 END DESC;");
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_exits, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		if(!(room = hashfind_room(atoi(row[E_ROOM]))))
		{
			mudlog("Exit ID#%li has invalid room(%il)!",atoi(row[0]),atoi(row[E_ROOM]));
			continue;
		}
		// this is just a warning
		if(!hashfind_room(atoi(row[E_TO_VNUM])))
			mudlog("Exit in room %li has invalid to-room %li",atoi(row[E_ROOM]),atoi(row[E_TO_VNUM]));

		exit		= new_exit(room);
		exit->id	= atoi(row[0]);
		exit->door 	= atoi(row[E_DOOR]);
		exit->key 	= atoi(row[E_KEY]);
		exit->to_vnum	= atoi(row[E_TO_VNUM]);
		str_dup(&exit->name, row[E_NAME]);
		total_exits++;
	}
	mysql_free_result(result);
	return total_exits;
}


long fread_extras(void)
{
	char buf[MAX_BUFFER];
	MYSQL_RES *result;
	MYSQL_ROW row;
	CREATURE *critx=0;
	OBJECT *obj=0;
	EXTRA *extra;
	ROOM *room=0;
	long loadtype;

	sprintf(buf,"SELECT * FROM extra");
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_extras, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		loadtype = atoi(row[ED_TYPE]);
		switch(loadtype)
		{
		case TYPE_ROOM:
			if(!(room = hashfind_room(atoi(row[ED_VNUM]))))
			{
				mudlog("Extra ID#%li has invalid room(%il)!",atoi(row[0]),atoi(row[ED_VNUM]));
				continue;
			}
			break;
		case TYPE_OBJECT:
			if(!(obj = hashfind_object(atoi(row[ED_VNUM]))))
			{
				mudlog("Extra ID#%li has invalid object(%il)!",atoi(row[0]),atoi(row[ED_VNUM]));
				continue;
			}
			break;
		case TYPE_CREATURE:
			if(!(critx = hashfind_creature(atoi(row[ED_VNUM]))))
			{
				mudlog("Extra ID#%li has invalid creature(%il)!",atoi(row[0]),atoi(row[ED_VNUM]));
				continue;
			}
			break;

		}

		extra		= new_extra(room?(void*)room:obj?(void*)obj:(void*)critx);
		extra->id	= atoi(row[0]);
		str_dup(&extra->keywords, row[ED_KEYWORDS]);
		str_dup(&extra->description, row[ED_DESCRIPTION]);
		total_extras++;
	}
	mysql_free_result(result);
	return total_extras;
}


long fread_notes(void)
{
	NOTE *note;
	MYSQL_RES *result;
	MYSQL_ROW row;
	MYSQL_RES *tresult;
	MYSQL_ROW trow;
	char buf[MAX_BUFFER];
	long z = 0;

	sprintf(buf,"SELECT * FROM notes ORDER BY id");
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);
	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_notes, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}
	while((row = mysql_fetch_row(result)))
	{
		note = new_note();
		sprintf(buf,"SELECT UNIX_TIMESTAMP('%s')",row[N_WRITTEN]);
		mysql_query(mysql,buf);
		tresult = mysql_store_result(mysql);
		trow = mysql_fetch_row(tresult);

		str_dup(&note->written,		trow[0]			);
		str_dup(&note->sent_to,		row[N_SENT_TO]		);
		str_dup(&note->sender,		row[N_SENDER]		);
		str_dup(&note->subject,		row[N_SUBJECT]		);
		str_dup(&note->text,		row[N_NOTE]		);
		mysql_free_result(tresult);

		z++;
	}
	mysql_free_result(result);
	return z;
}


long fread_objects(AREA *area)
{
	OBJECT *obj;
	MYSQL_RES *oresult;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];
	long z = 0;
	long vnum;

	sprintf(buf,"SELECT * FROM object WHERE vnum >= '%li' AND vnum <= '%li' AND prototype='1'",
		area->low, area->high);
	mysql_query(mysql,buf);
	oresult = mysql_store_result(mysql);

	if (mysql_num_rows(oresult) < 0)
	{
		mudlog("in fread_object, mysql error: %s",mysql_error(mysql));
		mysql_free_result(oresult);
		return 0;
	}

	while ((row = mysql_fetch_row(oresult)))
	{
		vnum = atoi(row[O_VNUM]);

		if(hashfind_object(vnum))
		{
			mudlog("Found object %d already in hashfind in fread_object!",vnum);
			continue;
		}
		obj = new_object_proto(vnum);

		ReadObjectMysql(obj)

		z++;
	}
	mysql_free_result(oresult);

	return z;
}


long fread_player(MSOCKET *sock, char *name)
{
	CREATURE *crit = sock->pc;
	OBJECT *objp=0;
	OBJECT *obj=0, *objc=0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];
	long value=0;

	// check to make sure there is pfile to be read.
	sprintf(buf,"SELECT * FROM creature WHERE name=\"%s\" LIMIT 1",smash_quotes(name));
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);
	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_player(creature) where no pfile found: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}
	else if(mysql_num_rows(result) != 1)
	{
		// no creature player found, search player database
		mysql_free_result(result);
		sprintf(buf,"SELECT * FROM player WHERE name=\"%s\" LIMIT 1",smash_quotes(name));
		mysql_query(mysql,buf);
		result = mysql_store_result(mysql);
		if (mysql_num_rows(result) > 0)
		{
			mysql_free_result(result);
			return 2;
		}
		mysql_free_result(result);
		return 0;
	}
	while((row = mysql_fetch_row(result)))
	{
		ROOM *room;
		long roomv = 1;
		if (row[C_ROOM])
			roomv = atoi(row[C_ROOM]);

		ReadCreatureMysql()

		if(!(room=hashfind_room(roomv)))
			crit->in_room = hashfind_room(1);
		else
			crit->in_room = room;
		break;
	}
	mysql_free_result(result);

	// get player specific values from player table
	sprintf(buf,"SELECT * FROM player WHERE name=\"%s\" LIMIT 1",smash_quotes(name));
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if(mysql_num_rows(result) < 0)
	{
		mudlog("in fread_player(player) where no pfile found: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		sock->id = atoi(row[0]);

		strcpy(sock->last_command,	row[P_LASTCOMMAND]	);

		// only get host again if IP changes (see do_users)
		str_dup(&sock->host,		row[P_HOST]		);
		str_dup(&sock->last_ip,		row[P_IP]		);
		str_dup(&sock->password,	row[P_PASSWORD]		);
		str_dup(&sock->prompt,		row[P_PROMPT]		);
		str_dup(&sock->title,		row[P_TITLE]		);
		str_dup(&sock->who_name,	row[P_WHONAME]		);

		sock->lines	= atoi(row[P_LINES]);
		sock->hrs 	= atoi(row[P_HRS]);
	}
	mysql_free_result(result);

	// LOAD OBJECTS
	// sort by nested so containers get loaded first
	sprintf(buf,"(SELECT * FROM object WHERE objtype = '%d' AND owner_id = '%li' ORDER BY nested ASC) UNION (SELECT * FROM object WHERE objtype <> '%d' AND owner_id='%li' ORDER BY in_obj ASC)",OBJ_CONTAINER,crit->id,OBJ_CONTAINER,crit->id);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	while((row = mysql_fetch_row(result)))
	{
		if(!(objp = hashfind_object(atoi(row[O_VNUM]))))
			objp = hashfind_object(1);
		obj = new_object(objp->vnum);

		ReadObjectMysql(obj)

		if (flag_isset(obj->flags,OFLAG_STORED))
			free_object(obj);
		else
		{
			trans(obj,crit);
			str_dup(&obj->worn,	row[O_WORN]);

			if(ValidString(obj->worn))
			{
				strcpy(buf,obj->worn);
				wear_obj(crit, obj, buf);
			}
			else if((value=atoi(row[O_IN_OBJ])) > 0)
			{
				for(objc = object_list; objc; objc = objc->next)
				{
					if(objc->id == value)
					{
						trans(obj,objc);
						break;
					}
				}
			}				
		}
	}

	mysql_free_result(result);

	// update whether or not player online
	sprintf(buf,"UPDATE player SET online='1' WHERE id='%li'",sock->id);
	mysql_query(mysql,buf);
	return 1;
}


long fread_resets(AREA *area)
{
	RESET *reset, *dummy;
	ROOM *room;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];
	long z = 0;

	// toid = 0 = main reset.. anything else is subreset
	sprintf(buf,"SELECT * FROM reset WHERE roomvnum >= '%li' AND roomvnum <= '%li' ORDER BY nested ASC",
		area->low, area->high);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_resets, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		reset = new_reset();

		ReadResetMysql(row)

		add_reset(reset);

		if((room = hashfind_room(reset->roomvnum)))
		{	
			AddToListEnd(room->resets,reset,dummy)
		}

		z++;
	}
	mysql_free_result(result);
	return z;
}


long fread_rooms(AREA *area)
{
	ROOM *room;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];
	long z = 0;
	long vnum;

	sprintf(buf,"SELECT * FROM room WHERE vnum >= '%li' AND vnum <= '%li'",
		area->low, area->high);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_room, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while ((row = mysql_fetch_row(result)))
	{
		vnum = atoi(row[R_VNUM]);

		if(hashfind_room(vnum))
		{
			mudlog("Found room %d already in hashfind in fread_room!",vnum);
			continue;
		}
	 	room = new_room(vnum);

		room->id	= atoi(row[0]);
		room->roomtype	= atoi(row[R_ROOMTYPE]);
		room->light	= atoi(row[R_LIGHT]);

		str_dup(&room->loaded,			row[R_LOADED]		);
		str_dup(&room->name,			row[R_NAME]		);
		str_dup(&room->night_name,		row[R_NIGHT_NAME]	);
		str_dup(&room->description,		row[R_DESC]		);
		str_dup(&room->night_description,	row[R_NIGHT_DESC]	);

		read_flags(row[R_FLAGS], room);

		z++;
	}
	mysql_free_result(result);
	return z;
}


long fread_shops(AREA *area)
{
	SHOP *shop=0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	long z = 0;
	char buf[MAX_BUFFER];

	sprintf(buf,"SELECT * FROM shop WHERE keeper >= '%li' AND keeper <= '%li'",
		area->low, area->high);
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_shops, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		shop			= new_shop(atoi(row[SH_KEEPER]));
		if (!shop)
			continue;
		shop->id		= atoi(row[0]);
		shop->keeper		= atoi(row[SH_KEEPER]);
		shop->stype		= atoi(row[SH_TYPE]);
		shop->item		= atoi(row[SH_ITEM]);
		shop->buy		= atoi(row[SH_BUY]);
		shop->sell		= atoi(row[SH_SELL]);
		shop->open		= atoi(row[SH_OPEN]);
		shop->close		= atoi(row[SH_CLOSE]);

		z++;
	}

	mysql_free_result(result);
	return z;
}


long fread_socials(void)
{
	SOCIAL *social;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];
	long z = 0;

	sprintf(buf,"SELECT * FROM social");
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);

	if (mysql_num_rows(result) < 0)
	{
		mudlog("in fread_socials, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	while((row = mysql_fetch_row(result)))
	{
		social		= new_social(row[S_NAME]);
		social->id	= atoi(row[0]);

		str_dup(&social->no_target,	row[S_NOTARGET]		);
		str_dup(&social->crit_target,	row[S_CRITTARGET]	);
		str_dup(&social->object_target,	row[S_OBJECTTARGET]	);
		str_dup(&social->self_target,	row[S_SELFTARGET]	);
		z++;
	}
	mysql_free_result(result);
	return z;
}


long fread_time(void)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];

	sprintf(buf,"SELECT * FROM time");
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);
	if (mysql_num_rows(result) == 0)
	{
		mudtime.age		= 0;
		mudtime.last		= 0;
		mudtime.mudstatus	= 0;
		mudtime.backup		= 0;
		mudtime.id		= -1;
		mudtime.start		= current_time;
		mudtime.year		= STARTING_YEAR;
		mudtime.month		= 0;
		mudtime.day		= 0;
		mudtime.hour		= 0;
		mudtime.minute		= 0;
		mudtime.second		= 0;
		mysql_free_result(result);
		return 0;
	}
	if(mysql_num_rows(result) < 0)
	{
		mudlog("in fread_time, mysql error: %s",mysql_error(mysql));
		mysql_free_result(result);
		return 0;
	}

	// get last row
	mysql_data_seek(result, mysql_num_rows(result)-1);
	row = mysql_fetch_row(result);

	if(NEW_TIME_LOGS)
		mudtime.id	= 0;
	else
		mudtime.id	= atoi(row[0]);
	mudtime.age		= atoi(row[T_AGE]);
	mudtime.last		= atoi(row[T_MUDSTATUS]);
	mudtime.mudstatus	= 0;
	mudtime.backup		= atoi(row[T_BACKUP]);
	mudtime.end		= atoi(row[T_END]);
	mudtime.start		= current_time;
	mudtime.year		= atoi(row[T_YEAR]);
	mudtime.month		= atoi(row[T_MONTH]);
	mudtime.day		= atoi(row[T_DAY]);
	mudtime.hour		= atoi(row[T_HOUR]);
	mudtime.minute		= atoi(row[T_MINUTE]);
	mudtime.second		= atoi(row[T_SECOND]);

	mysql_free_result(result);
	return 1;
}
////////////////////////
// DATABASE FUNCTIONS //
////////////////////////

// this updates the ->room ->obj ->crit pointers in resets.. I am using these
// pointers so we dont have to hashfind every time a reset goes off.. more
// code, but more optimization!
void update_resets()
{
	extern ROOM *hash_room[HASH_KEY];
	CREATURE *crit;
	OBJECT *obj;
	RESET *reset, *reset_next, *parent;
	EXIT *exit;
	ROOM *room;
	char buf[MAX_BUFFER];
	long i, delete;

	for(i = 0; i < HASH_KEY; i++)
	{
	    for(reset = hash_reset[i%HASH_KEY]; reset; reset = reset_next)
	    {
		reset_next = reset->next_hash;

		if(!(room = hashfind_room(reset->roomvnum)))
		{
			mudlog("UPDATE_RESETS: bad room vnum for reset");
			sprintf(buf,"DELETE FROM reset WHERE id='%li'",reset->id);
			mysql_query(mysql, buf);
			free_reset(reset);
			continue;
		}
		reset->room = room;
	    }
	}

	for(i = 0; i < HASH_KEY; i++)
	{
	    for(room = hash_room[i%HASH_KEY]; room; room = room->next_hash)
	    {
		delete = 0;
		for(reset = room->resets; reset; reset = reset_next)
		{
			reset_next = reset->next;

			switch(reset->loadtype)
			{
			case TYPE_CREATURE:
				if(!(crit = hashfind_creature(reset->vnum)))
				{
					mudlog("UPDATE_RESETS: bad vnum for Creset vnum#%li in room %li",
						reset->vnum, reset->room->vnum);
					sprintf(buf,"DELETE FROM reset WHERE id='%li'",reset->id);
					mysql_query(mysql, buf);
					free_reset(reset);
					delete = 1;
					break;
				}
				reset->crit	= crit;
				parent		= reset;
				delete		= 0;
				break;
			case TYPE_OBJECT:
				if(delete || !(obj = hashfind_object(reset->vnum)))
				{
					if(!delete)
						mudlog("UPDATE_RESETS: Oreset has bad vnum vnum#%li in room %li",
							reset->vnum, reset->room->vnum);
					sprintf(buf,"DELETE FROM reset WHERE id='%li'",reset->id);
					mysql_query(mysql, buf);
					free_reset(reset);
					break;
				}
				reset->obj	= obj;

				if(parent)
					reset->crit = parent->crit;
				break;
			case TYPE_EXIT:
				if(!(exit = find_exit(reset->room,reset->command)))
				{
					mudlog("UPDATE_RESETS: can't find exit(%s) for reset in room %li",
						reset->command, reset->room->vnum);
					sprintf(buf,"DELETE FROM reset WHERE id='%li'",reset->id);
					mysql_query(mysql, buf);
					free_reset(reset);
					break;
				}
				break;
			default:
				mudlog("UPDATE_RESETS: bad type -- deleting",reset->id);
				sprintf(buf,"DELETE FROM reset WHERE id='%li'",reset->id);
				mysql_query(mysql, buf);
				free_reset(reset);
				break;
			}
		}
	    }
	}
}


void load_db(void)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	char buf[MAX_BUFFER];

	sprintf(buf,"SELECT * FROM area");
	mysql_query(mysql,buf);
	result = mysql_store_result(mysql);
	while( (row = mysql_fetch_row(result)) )
		fread_area(row);
	mysql_free_result(result);

	fread_exits();
	fread_extras();

	total_socials	= fread_socials();
	total_notes	= fread_notes();

	update_resets();

	mudlog("Loading areas done.  Total Rooms=%li, Objects=%li, Creatures=%li, "
		"Exits=%li, Extras=%li, Resets=%li, Shops=%li, Socials=%li,  Notes=%li",
		total_rooms, total_objects, total_creatures, total_exits, total_extras,
		total_resets, total_shops, total_socials, total_notes );

	mudlog("Bans: %li", fread_bans());

	// clear objects that don't belong to anyone
	mysql_query(mysql,"DELETE FROM object WHERE owner_id='0' AND prototype='0'");
}

