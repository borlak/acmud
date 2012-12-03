/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
editor.c -- edit socials, mobs, objects, etc.
*/

#include "stdh.h"
#include <ctype.h>


///////////////
// UTILITIES //
///////////////
void olc_help(CREATURE *crit, char *help)
{
	char buf[MAX_BUFFER];
	char *type;
	char **arguments;
	long x;

	switch(*(unsigned int*)crit->socket->editing)
	{
	case TYPE_CREATURE:	type = "crit";		break;
	case TYPE_OBJECT:	type = "obj";		break;
	case TYPE_ROOM:		type = "room";		break;
	case TYPE_AREA:		type = "area";		break;
	case TYPE_EXTRA:	type = "extra";		break;
	case TYPE_RESET:	type = "reset";		break;
	case TYPE_HELP:		type = "help";		break;
	case TYPE_SOCIAL:	type = "social";	break;
	case TYPE_NOTE:		type = "note";		break;
	case TYPE_BAN:		type = "ban";		break;
	default:		type = "";		break;
	}

	if (!strcasecmp(type,"obj") && !strcasecmp(help,"wear"))
	{
		sendcrit(crit,"&+WKeywords            Actual slots taken&n");
		sendcrit(crit,"--------            ------------------\n\r");
		for (x = 0; worn_table[x].keyword; x++)
			sendcritf(crit,"%-15s     %s",worn_table[x].keyword,worn_table[x].worn);
		sendcrit(crit,"\n\rYou can also do: # held remove\n\r to remove a wear place.");
		interpret(crit,"");
		return;
	}

	sprintf(buf,"olc_%s_%s",type,help);

	// can't call interpret sicne it checks if you are editing
	arguments = make_arguments(buf);
	do_help(crit, arguments, 0, 0);
	free_arguments(arguments);
}


void check_keywords(CREATURE *crit, void *obj)
{
	CREATURE *xcrit=0;
	OBJECT *xobj=0;
	char **arguments;
	char *keywords=0;
	char *name=0, *long_descr=0;
	char *bad_words = "a the is self me all remove wear worn";
	char *p=0;
	long remove;
	long x;
	long nfound=0, lfound=0;

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		xcrit = (CREATURE*)obj;
		if( IsPlayer(xcrit) )
			return;

		keywords	= xcrit->keywords;
		name		= xcrit->name;
		long_descr	= xcrit->long_descr;
		break;
	case TYPE_OBJECT:
		xobj		= (OBJECT*)obj;
		keywords	= xobj->keywords;
		name		= xobj->name;
		long_descr	= xobj->long_descr;
		break;
	}

	arguments = make_arguments(keywords);
	for(x = 0, remove=0; arguments[x]; x++, remove=0)
	{
		if(!strargcmp(bad_words, arguments[x]))
		{
			sendcritf(crit, "&+RKeywords may not contain these words:\n\r&+R%s&N",bad_words);
			remove = 1;
		}
		p = arguments[x];
		while(*p != '\0')
		{
			if(!isalnum(*p) && !isspace(*p))
			{
				sendcrit(crit, "&+ROnly alpha/numeric characters are allowed for keywords.&N");
				remove = 1;
				break;
			}
			p++;
		}

		if(remove)
		{
			if(xcrit)
				str_dup(&xcrit->keywords, str_minus(xcrit->keywords,arguments[x],' '));
			else
				str_dup(&xobj->keywords, str_minus(xobj->keywords,arguments[x],' '));
		}
		if (!strargcmp(name,arguments[x]) || !strargcmp(long_descr, arguments[x]))
		{
			if(!strargcmp(name, arguments[x]))
				nfound = 1;
			if(!strargcmp(long_descr, arguments[x]))
				lfound = 1;
		}
	}
	free_arguments(arguments);
	if(!nfound && !lfound)
	{
		sendcritf(crit,"&+RNone of the keyword were found in the name or long description.&N",
			arguments[x]);
		return;
	}


	if(!nfound || !lfound)
	{
		sendcritf(crit,"&+MIt is suggested that you add a keyword for the %s.&N",
			nfound ? "Long Description" : "Name" );
	}
}


void check_worn(CREATURE *crit, void *obj)
{
	CREATURE *xcrit=0;
	OBJECT *xobj=0;
	RESET *reset=0;
	char **arguments;
	char *worn=0;
	long x=0;

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		xcrit = (CREATURE*)obj;
		worn = xcrit->wear;
		break;
	case TYPE_OBJECT:
		xobj = (OBJECT*)obj;
		worn = xobj->wear;
		break;
	case TYPE_RESET:
		reset	= (RESET*)obj;
		xobj	= reset->obj;
		xcrit	= reset->crit;

		if(!xobj || !ValidString(reset->command))
			return;

		if(!strlistcmp(xobj->wear, xcrit->wear))
		{
			sendcritf(crit,"&+R%s can not be worn by %s.&N",xobj->name,xcrit->name);
		}
		else if(strargcmp(xcrit->wear, reset->command))
		{
			sendcritf(crit,"&+R%s does not have a '%s' wear slot.&N",xcrit->name,reset->command);
		}
		return;
	}

	arguments = make_arguments(worn);
	for(x = 0; arguments[x]; x++)
	{
		if(find_worn(arguments[x]) < 0)
		{
			if(xcrit)
				str_dup(&xcrit->wear, str_minus(xcrit->wear, arguments[x], ' '));
			else if(xobj)
				str_dup(&xobj->wear, str_minus(xobj->wear, arguments[x], ' '));
			sendcritf(crit,"'%s' is an invalid wear type.  These must be added in the code first."
					"\n\rType \"wear ?\" to get a listing of available wear types.",
				arguments[x]);
		}
	}
	free_arguments(arguments);
}


void save_editing(CREATURE *crit)
{
	CREATURE *xcrit=0;
	OBJECT *xobj=0;
	RESET *reset=0;
	ROOM *xroom=0;

	if(!crit->socket->modified)
		return;

	switch(*(unsigned int*)crit->socket->editing)
	{
	case TYPE_AREA:
		fwrite_area((AREA*)crit->socket->editing, 0);
		break;
	case TYPE_BAN:
		fwrite_bans();
		break;
	case TYPE_CREATURE:
	{
		xcrit = (CREATURE*)crit->socket->editing;
		if(IsPlayer(xcrit))
			fwrite_player(xcrit);
		else if(xcrit->prototype)
			fwrite_creature(xcrit->vnum);
		if(strargcmp(xcrit->loaded, crit->name))
			str_dup(&xcrit->loaded, str_add(xcrit->loaded,crit->name,' '));
		break;
	}
	case TYPE_OBJECT:
		xobj = (OBJECT*)crit->socket->editing;
		if(strargcmp(xobj->loaded, crit->name))
			str_dup(&xobj->loaded, str_add(xobj->loaded,crit->name,' '));
		fwrite_object(xobj);
		break;
	case TYPE_ROOM:
		xroom = (ROOM*)crit->socket->editing;
		if(strargcmp(xroom->loaded, crit->name))
			str_dup(&xroom->loaded, str_add(xroom->loaded,crit->name,' '));
		fwrite_room(xroom->vnum);
		break;
	case TYPE_EXTRA:
		fwrite_extra((EXTRA*)crit->socket->editing);
		break;
	case TYPE_RESET:
		reset = (RESET*)crit->socket->editing;
		fwrite_resets(crit->in_room);
		break;
	case TYPE_SOCIAL:
		fwrite_social((SOCIAL*)crit->socket->editing);
		break;
	case TYPE_HELP:
		fwrite_help((HELP*)crit->socket->editing);  // writes to mysql, deletes help from memory.
		break;
	case TYPE_NOTE:
		fwrite_note((NOTE*)crit->socket->editing);
		break;
	case TYPE_SHOP:
		fwrite_shop((SHOP*)crit->socket->editing);
		break;
	default:
		mudlog("EDITOR(done): broken type (%d)", *(unsigned int*)crit->socket->editing);
		break;
	}
}

char *get_reset_name(RESET *reset)
{
	CREATURE *crit;
	OBJECT *obj;

	switch(reset->loadtype)
	{
	case TYPE_CREATURE:
		if(!(crit=hashfind_creature(reset->vnum)))
			return "nothing";
		return crit->name;
	case TYPE_OBJECT:
		if(!(obj=hashfind_object(reset->vnum)))
			return "nothing";
		return obj->name;
	case TYPE_EXIT:
		return "n/a";
	}
	return "broke";
}

void stop_editing(CREATURE *crit, long show_message)
{
        switch(*(unsigned int*)crit->socket->editing)
	{
	case TYPE_HELP:
		free_help((HELP*)crit->socket->editing);
		break;
	default:
		break;
	}	
	crit->socket->editing	= 0;
	crit->socket->variable	= 0;
	if(ValidString(crit->socket->string))
	{
		DeleteObject(crit->socket->string)
	}
	if(show_message)
		message("$n stop$x editing.",crit,0,0);
}


void delete_object(CREATURE *crit, void *obj, long all)
{
	char buf[MAX_BUFFER];
	CREATURE *critx=0;
	MYSQL_RES *result=0;
	MSOCKET *socket=0;
	OBJECT *objx=0;
	OBJECT *obj_next=0;
	SOCIAL *social=0;
	EXTRA *extra=0;
	RESET *reset=0;
	AREA *area=0;
	HELP *help=0;
	ROOM *room=0;
	SHOP *shop=0;
	BAN *ban=0;
	long vnum=0;
	long num=0;

	switch(*(unsigned int*)obj)
	{
	case TYPE_CREATURE:
		vnum = ((CREATURE*)obj)->vnum;
		critx = (CREATURE*)obj;

		if(IsPlayer(critx))
		{
			if(!IsImp(crit))
			{
				sendcrit(crit, "Only an implementor may delete players.");
				return;
			}
			if(!all)
			{
				sendcrit(crit, "You must put in 'all' after playername for this to work.");
				return;
			}

			sprintf(buf,"SELECT name FROM creature WHERE name=\"%s\" AND vnum=1", critx->name);
			mysql_query(mysql, buf);

			result = mysql_store_result(mysql);

			if(mysql_num_rows(result) == 0)
			{
				sendcrit(crit, "There is no such player.");
				mysql_free_result(result);
				return;
			}
			mysql_free_result(result);

			sprintf(buf,"DELETE FROM creature WHERE name=\"%s\" AND vnum=1", critx->name);
			mysql_query(mysql, buf);

			sprintf(buf,"DELETE FROM object WHERE owner_id='%li'", critx->id);
			mysql_query(mysql, buf);

			for(socket = socket_list; socket; socket = socket->next)
			{
				if(!strcasecmp(socket->pc->name, critx->name))
				{
					free_socket(socket);
					break;
				}
			}
			sendcritf(crit, "%s deleted.", critx->name);
			break;
		}

		sprintf(buf,"DELETE FROM creature WHERE vnum='%li' %s",
			vnum, all ? "" : "AND prototype='0'");

		if(!(critx = hashfind_creature(vnum)))
		{
			sendcrit(crit,"Not in memory.");
			break;
		}
		free_creature(critx);
		sendcrit(crit,"Deleted from memory.");
		break;
	case TYPE_OBJECT:
		vnum = ((OBJECT*)obj)->vnum;
		for(objx = object_list, num=0; all && objx; objx = obj_next)
		{
			obj_next = objx->next;
			if(objx->vnum == vnum)
			{
				num++;
				free_object(objx);
			}
		}
		if(num)
			sendcritf(crit,"%li instances deleted from memory.", num);

		sprintf(buf,"DELETE FROM object WHERE vnum='%li' %s",
			vnum, all ? "" : "AND prototype='0'");

		if(!(objx = hashfind_object(vnum)))
		{
			sendcrit(crit,"No prototype in memory.");
			break;
		}
		free_object(objx);
		sendcrit(crit,"Deleted prototype from memory.");
		break;
	case TYPE_ROOM:
		vnum = ((ROOM*)obj)->vnum;

		sprintf(buf,"DELETE FROM reset WHERE roomvnum='%li'",vnum);
		mysql_query(mysql,buf);

		sprintf(buf,"DELETE FROM room_exit WHERE room='%li'",vnum);
		mysql_query(mysql,buf);

		sprintf(buf,"DELETE FROM room WHERE vnum='%li'",vnum);

		if(!(room = hashfind_room(vnum)))
		{
			sendcrit(crit,"Not in memory.");
			break;
		}
		free_room(room);
		sendcrit(crit,"Deleted from memory.");
		break;
	case TYPE_EXIT:
		room = (ROOM*)crit->socket->editing;
		sprintf(buf,"DELETE FROM room_exit WHERE room='%li' AND name=\"%s\"",
			room->vnum, smash_quotes(((EXIT*)obj)->name));

		free_exit(room, ((EXIT*)obj));
		sendcrit(crit,"Deleted from memory.");
		break;
	case TYPE_EXTRA:
		extra = (EXTRA*)crit->socket->editing;
		switch(extra->loadtype)
		{
		case TYPE_ROOM:
			room = hashfind_room(extra->vnum);
			break;
		case TYPE_OBJECT:
			objx = hashfind_object(extra->vnum);
			break;
		case TYPE_CREATURE:
			critx = hashfind_creature(extra->vnum);
			break;
		}
		crit->socket->editing = extra->next ? (void*)extra->next : extra->prev ? (void*)extra->prev : 
					room ? (void*)room : objx ? (void*)objx : (void*)critx;
		sprintf(buf,"DELETE FROM extra WHERE vnum='%li' AND loadtype='%d' AND keywords='%s'",
			room ? room->vnum : objx ? objx->vnum : critx->vnum, 
				extra->loadtype, extra->keywords);
		free_extra(room ? (void*)room : objx ? (void*)objx : (void*)critx, ((EXTRA*)obj));
		sendcrit(crit,"Deleted from memory.");
		break;
	case TYPE_RESET:
		reset = (RESET*)obj;
		vnum = reset->id;
		free_reset(reset);

		sendcrit(crit, "Deleted from memory.");
		sprintf(buf,"DELETE FROM reset WHERE ID='%li'",vnum);
		break;
	case TYPE_AREA:
		area = (AREA*)obj;
		sendcritf(crit,"Deleting area %s... OMG.",area->name);
		for(num = area->low; num <= area->high; num++)
		{
			if((objx = hashfind_object(num)))
				delete_object(crit,objx,1);
			if((critx = hashfind_creature(num)))
				delete_object(crit,critx,1);
			if((room = hashfind_room(num)))
				delete_object(crit,room,1);
		}
		sprintf(buf,"DELETE FROM area WHERE name=\"%s\"",area->name);
		mysql_query(mysql,buf);
		free_area(area);

		sprintf(buf,"DELETE FROM room WHERE vnum>='%li' AND vnum<='%li'",area->low,area->high);
		mysql_query(mysql,buf);
		sprintf(buf,"DELETE FROM object WHERE vnum>='%li' AND vnum<='%li'",area->low,area->high);
		mysql_query(mysql,buf);
		sprintf(buf,"DELETE FROM creature WHERE vnum>='%li' AND vnum<='%li'",area->low,area->high);
		mysql_query(mysql,buf);
		sprintf(buf,"DELETE FROM extra WHERE vnum>='%li' AND vnum<='%li'",area->low,area->high);
		mysql_query(mysql,buf);
		sprintf(buf,"DELETE FROM reset WHERE roomvnum>='%li' AND roomvnum<='%li'",area->low,area->high);
		mysql_query(mysql,buf);
		sprintf(buf,"DELETE FROM room_exit WHERE room>='%li' AND room<='%li'",area->low,area->high);
		mysql_query(mysql,buf);
		sprintf(buf,"DELETE FROM shop WHERE keeper>='%li' AND keeper<='%li'",area->low,area->high);

		sendcrit(crit,"Area destroyed.");
		break;
	case TYPE_HELP:
		help = (HELP*)obj;
		sprintf(buf,"DELETE FROM help WHERE keyword=\"%s\"",help->keyword);
		free_help(help);
		break;
	case TYPE_SOCIAL:
		social = (SOCIAL*)obj;
		sprintf(buf,"DELETE FROM social WHERE name=\"%s\"",social->name);
		free_social(social);
		break;
	case TYPE_SHOP:
		shop = (SHOP*)obj;
		sprintf(buf,"DELETE FROM shop WHERE keeper=\"%li\"",shop->keeper);
		free_shop(shop);
		break;
	case TYPE_BAN:
		ban = (BAN*)obj;
		mudlog("%s REMOVED BAN %s [%s]",
			crit->name,
			ban->bantype == BAN_PLAYER ? ban->name : ban->ip,
			ban->message );

		sprintf(buf,"DELETE FROM ban WHERE id='%li'",ban->id);
		free_ban(ban);
		break;
	default:
		mudlog("delete_object called with invalid type.");
		break;
	}

	mysql_query(mysql,buf);
	if((num = mysql_affected_rows(mysql)) == 0)
		sendcrit(crit,"No instances found in database.");
	else
		sendcritf(crit,"%li instances deleted from database.", num);
}


/////////////
// EDITORS //
/////////////
#define ExtraDescMacro(what)											\
	if(arguments[0] && (!strcasecmp(arguments[0], "extra") || !strcasecmp(arguments[0], "ed"))) {		\
		long zExtra = 1;										\
		if(!arguments[1]) {										\
			sendcrit(crit,"Syntax: ed/extra <#/new>");						\
			return 1;										\
		}												\
		if(is_number(arguments[1])) {									\
			for(extra = what->extras; extra; extra = extra->next, zExtra++)				\
				if(zExtra == atoi(arguments[1]))						\
					break;									\
			if(extra) {										\
				crit->socket->editing = (EXTRA*)extra;						\
				interpret(crit,"");								\
			}											\
			else      sendcrit(crit,"Invalid number!");						\
			return 1;										\
		}												\
		if(!strcasecmp(arguments[1],"new")) {								\
			extra = new_extra(what);								\
			crit->socket->editing = (EXTRA*)extra;							\
			interpret(crit,"");									\
			return 1;										\
		}												\
	}


long ban_editor(CREATURE *crit, char **arguments, BAN *ban)
{
	OlcInit()

	OlcStr("name",		ban->name,		2,50)
	OlcStr("ip",		ban->ip,		3,16)
	OlcValues("type",	ban->bantype,		ban_types)
	OlcStr("message",	ban->message,		0,5000)

	OlcSendInit()
	OlcSend("Name:    %s",	ban->name		);
	OlcSend("IP:      %s",	ban->ip			);
	OlcSend("Type:    %s",	ban_types[ban->bantype]	);
	OlcSend("Message:\n\r%s", ban->message		);
//	send_to(crit->socket,"Option: ");
	return 1;
}


long social_editor(CREATURE *crit, char **arguments, SOCIAL *social)
{
	OlcInit()

	OlcStr("name",		social->name,		2,100)
	OlcStr("notarget",	social->no_target,	5,100)
	OlcStr("creature",	social->crit_target,	5,100)
	OlcStr("object",	social->object_target,	5,100)
	OlcStr("self",		social->self_target,	5,100)

	OlcSendInit()
	OlcSend("Name:            %s",	social->name);
	OlcSend("No target:       %s",	social->no_target);
	OlcSend("Creature target: %s",	social->crit_target);
	OlcSend("Object target:   %s",	social->object_target);
	OlcSend("Self target:     %s",	social->self_target);
//	send_to(crit->socket,"Option: ");

	for(i = 0; command_table[i].command; i++)
	{
		if(!strcasecmp(command_table[i].command, social->name))
		{
			sendcrit(crit, "You can't make socials named after commands.");
			str_dup(&social->name, "fixme");
			break;
		}
	}
	return 1;
}


long help_editor(CREATURE *crit, char **arguments, HELP *help)
{
	OlcInit()

	OlcStrSeq("keyword",	help->keyword,		3,100,',')
	OlcStr("index",		help->index,		3,MAX_BUFFER)
	OlcInt("level",		help->level,		0,3)
	OlcStr("text",		help->entry,		5,MAX_BUFFER)

	OlcSendInit()
	OlcSend("Keyword:  %s",	help->keyword);
	OlcSend("Index:    %s",	help->index);
	OlcSend("Level:    %li",help->level);
	OlcSend("Text:\n\r%s",		help->entry);
//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long shop_editor(CREATURE *crit, char **arguments, SHOP *shop)
{
	OlcInit()

	OlcInt("critvnum",		shop->keeper,	1,2100000000)
	OlcInt("item",			shop->item,	1,2100000000)
	OlcInt("shop",			shop->stype,	0,4)
	OlcInt("buy",			shop->buy,	0,100)
	OlcInt("sell",			shop->sell,	0,100)
	OlcInt("open",			shop->open,	0,2359) // 12:00am - 11:59pm
	OlcInt("close",			shop->close,	0,2359) //  same

	OlcSendInit()
	OlcSend("Crit Vnum:       %li",	shop->keeper	);
	OlcSend("Item Type:       %li",	shop->item	);
	OlcSend("Shop Type:       %li",	shop->stype	);
	OlcSend("Buy Percentage:  %li",	shop->buy	);
	OlcSend("Sell Percentage: %li",	shop->sell	);
	OlcSend("Open Time:       %li",	shop->open	);
	OlcSend("Close Time:      %li",	shop->close	);
//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long extra_editor(CREATURE *crit, char **arguments, EXTRA *extra)
{
	CREATURE *critx=0;
	OBJECT *objx=0;
	ROOM *room=0;
	OlcInit()

	switch(extra->loadtype)
	{
	case TYPE_ROOM:
		room = hashfind_room(extra->vnum);
		break;
	case TYPE_OBJECT:
		objx = hashfind_object(extra->vnum);
		break;
	case TYPE_CREATURE:
		critx = hashfind_creature(extra->vnum);
		break;
	}

	OlcStrSeq("keywords",	extra->keywords,	3,25, ' ')
	OlcStr("description",	extra->description,	5,256)

	OlcSendInit()
	sendcritf(crit,"Extra Description for: (%s)%s [%li]",
		room ? "Room" : objx ? "Object" : "Creature",
		room ? room->name : objx ? objx->name : critx->name,
		room ? room->vnum : objx ? objx->vnum : critx->vnum );
		
	OlcSend("Keywords:    %s",	extra->keywords		);
	OlcSend("Description:\n\r%s",	extra->description	);

//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long note_editor(CREATURE *crit, char **arguments, NOTE *note)
{
	OlcInit()

	OlcStr("to",		note->sent_to,	3,100)
	OlcStr("subject",	note->subject,	3,100)
	OlcStr("text",		note->text,	5,MAX_BUFFER)

	OlcSendInit()
	OlcSend("To:       %s",	note->sent_to	);
	OlcSend("Subject:  %s",	note->subject	);
	OlcSend("Text:\n\r%s",		note->text	);
//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long area_editor(CREATURE *crit, char **arguments, AREA *area)
{
	AREA *xarea;
	OlcInit()

	OlcStrSeq("builders",	area->builders,		3,100, ' ')
	OlcStr("name",		area->name,		5,125)
	OlcInt("low",		area->low,		1,2100000000)
	OlcInt("high",		area->high,		1,2100000000)

	for(xarea = area_list; xarea; xarea = xarea->next)
	{
		if(xarea == area)
			continue;
		if((xarea->low < area->low && xarea->high > area->low)
		|| (xarea->high > area->high && xarea->low < area->high))
		{
			area->low = area->high = 0;
			sendcrit(crit,"Area vnums conflict with another area.");
			break;
		}
	}

	OlcSendInit()
	OlcSend("Builders:    %s",	area->builders	);
	OlcSend("Name:        %s",	area->name	);
	OlcSend("Low vnum:    %li",	area->low	);
	OlcSend("High vnum:   %li",	area->high	);
//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long creature_editor(CREATURE *crit, char **arguments, CREATURE *critp)
{
	EXTRA *extra;
	long x;
	char name[MAX_BUFFER];
	OlcInit()

	strcpy(name, critp->name);

	OlcStrSeq("keywords",	critp->keywords,	3,80, ' ')
	OlcStr("name",		critp->name,		3,80)
	OlcInt("level",		critp->level,		0,100)
	OlcStr("description",	critp->description,	5,512)
	OlcStr("longdescr",	critp->long_descr,	5,80)
	OlcStrSeq("wear",	critp->wear,		3,100, ' ')
	OlcInt("dexterity",	critp->dexterity,	1,100)
	OlcInt("intelligence",	critp->intelligence,	1,100)
	OlcInt("strength",	critp->strength,	1,100)
	OlcInt("alignment",	critp->alignment,	-1001,1001);
	OlcInt("hp",		critp->max_hp,		-10,100)
	OlcInt("movement",	critp->max_move,	1,100)
	OlcValues("sex",	critp->sex,		sex_table)
	OlcValues("position",	critp->position,	position_table)
	OlcValues("state",	critp->state,		state_table)
	OlcFlags("flags",	critp->flags,		critflags_name)

	if(IsPlayer(critp) && IsPlaying(critp))
	{
	OlcStr("title",		critp->socket->title,	1, 100)
	OlcStr("who_name",	critp->socket->who_name, 1, 20)
	}

	if(!IsPlayer(critp))
	{
	ExtraDescMacro(critp)
	}

	if(IsPlayer(critp) && strcmp(name, critp->name))
	{
		sendcrit(crit, "You can not change a player's name this way.");
		str_dup(&critp->name, name);
	}

	check_keywords(crit, (void*)critp);
	check_worn(crit, (void*)critp);

	OlcSendInit()
	sendcritf(crit,"[%s&N] &+yBuilders[%s]&N",
		critp->prototype ? "&+RPROTOTYPE" : IsPlayer(critp) ? "&+BPLAYER" : "&+GINSTANCE",
		critp->loaded);
	sendcritf(crit,
		"    Vnum:           %li",	critp->vnum);
	OlcSend("Keywords:       %s",	critp->keywords			);
	OlcSend("Name:           %s",	critp->name			);
	OlcSend("Level:          %li",	critp->level			);
	OlcSend("&+G"
		"Description:    \n\r%s", critp->description		);
	OlcSend("&+g"
		"Long Descr:     %s",	critp->long_descr		);
	OlcSend("Wear Locations: %s",	critp->wear			);
	OlcSend("Dexterity:      %li",	critp->dexterity		);
	OlcSend("Intelligence:   %li",	critp->intelligence		);
	OlcSend("Strength:       %li",	critp->strength			);
	OlcSend("Alignment:      %li",	critp->alignment		);
	OlcSend("Hp:             %li",	critp->max_hp			);
	OlcSend("Movement:       %li",	critp->max_move			);
	OlcSend("Sex:            %s",	sex_table[critp->sex]		);
	OlcSend("Position:       %s",   position_table[critp->position]	);
	OlcSend("State:          %s",	state_table[critp->state]	);
	OlcSend("Flags:          [%s]",	strflags(critp)			);
	if(IsPlayer(critp) && IsPlaying(critp))
	{
	sendcrit(crit, "&+B[Player Variables]&N");
	OlcSend("Title           %s&N",	critp->socket->title		);
	OlcSend("Who_name        %s&N",	critp->socket->who_name		);
	}
	else
	{
	if(critp->extras) sendcrit(crit,"");
	for(x=1, extra = critp->extras; extra; extra = extra->next)
		sendcritf(crit,"[&+G%li&N] Extra Description [&+G%s&N]", x++, extra->keywords);
	}
//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long object_editor(CREATURE *crit, char **arguments, OBJECT *obj)
{
	EXTRA *extra;
	long x;
	OlcInit()

	OlcStrSeq("keywords",	obj->keywords,		3,80,' ')
	OlcStr("name",		obj->name,		3,80)
	OlcStr("longdescr",	obj->long_descr,	5,80)
	OlcStr("description",	obj->description,	5,512)
	OlcStrSeq("wear",	obj->wear,		3,100,' ')
	OlcValues("objtype",	obj->objtype,		objtype_table)
	OlcInt("timer",		obj->timer,		0,86400)
	OlcInt("weight",	obj->weight,		0,1000)
	OlcInt("worth",		obj->worth,		0,100000000)

	ExtraDescMacro(obj)

	switch(obj->objtype)
	{
	case OBJ_WEAPON:
		OlcInt("mindamage",		obj->values[0], 1,50)
		OlcInt("maxdamage",		obj->values[1], 1,50)
		break;
	case OBJ_ARMOR:
		OlcInt("absorbtion1615",		obj->values[0], 0,25)
		break;
	case OBJ_COIN:
		OlcValuesStruct("cointype",	obj->values[0],	coin_table)
		OlcInt("coins",			obj->values[1], 1,1000000)
		break;
	case OBJ_KEY:
		OlcInt("uses",			obj->values[0], 0,100) // 0 is infinite
		break;
	case OBJ_LIGHT:
		OlcInt("brightness",		obj->values[0], 1,10)
		OlcInt("duration",		obj->values[1], 0,1000) // 0 is infinite
		break;
	case OBJ_CONTAINER:
		OlcInt("maxweight",		obj->values[0], 1, 1000)
		break;
	}

	check_keywords(crit, (void*)obj);
	check_worn(crit, (void*)obj);

	OlcSendInit()
	sendcritf(crit,"[%s&N] &+yBuilders[%s]&N",
		obj->prototype ? "&+RPROTOTYPE" : "&+GINSTANCE",
		obj->loaded );
	sendcritf(crit,
		"    Vnum:           %li", obj->vnum			);
	OlcSend("Keywords:       %s",	obj->keywords			);
	OlcSend("Name:           %s",	obj->name			);
	OlcSend("Long Descr:     %s",	obj->long_descr			);
	OlcSend("Description:    \n\r%s", obj->description		);
	OlcSend("Wear Locations: %s",	obj->wear			);
	OlcSend("Obj Type:       %s",	objtype_table[obj->objtype]	);
	OlcSend("Timer:          %li",	obj->timer			);
	OlcSend("Weight:         %li",	obj->weight			);
	sendcritf(crit, "%2li. "
		"Worth:          %li (%s)", i++, obj->worth, coin_string(obj->worth));

	// obj->values..
	switch(obj->objtype)
	{
	case OBJ_WEAPON:
	OlcSend("Min damage:     %li",	obj->values[0]);
	OlcSend("Max damage:     %li",	obj->values[1]);
	break;

	case OBJ_ARMOR:
	OlcSend("Absorbtion:     %li",	obj->values[0]);
	break;

	case OBJ_COIN:
	OlcSend("Coin Type:      %s",	coin_table[obj->values[0]].ansi_name);
	OlcSend("Coins:          %li",	obj->values[1]);
	break;

	case OBJ_KEY:
	OlcSend("Uses:           %li",	obj->values[0]);
	break;

	case OBJ_LIGHT:
	OlcSend("Brightness:     %li",	obj->values[0]);
	OlcSend("Duration:       %li",	obj->values[1]);
	break;

	case OBJ_CONTAINER:
	OlcSend("Max weight:     %li",	obj->values[0]);
	break;
	}

	if(obj->extras) sendcrit(crit,"");
	for(x=1, extra = obj->extras; extra; extra = extra->next)
		sendcritf(crit,"[&+G%li&N] Extra Description [&+G%s&N]", x++, extra->keywords);

//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long display_resets(CREATURE *crit)
{
	CREATURE *xcrit;
	OBJECT *xobj;
	RESET *reset, *parent=0;
	RESET *editing = 0;
	ROOM *room = crit->in_room;
	long total = 0;
	long nested = 0;
	char *pname, ptype;
	char *name, *location, type;

	if(IsEditing(crit))
		editing = (RESET*)crit->socket->editing;

	for(reset = room->resets; reset; reset = reset->next)
	{
		total++;
		reset->nested = nested++;

		switch(reset->loadtype)
		{
		case TYPE_CREATURE:
			if(!(xcrit = reset->crit))
			{
				name	= "INVALID";
				pname	= "INVALID";
			}
			else
			{
				name	= xcrit->name;
				pname	= xcrit->name;
			}
			type	= 'C';
			parent	= reset;

			ptype	= 'C';
			location = "in room";

			break;
		case TYPE_OBJECT:
			if(!(xobj = reset->obj))
				name	= "INVALID";
			else
				name = xobj->name;

			type = 'O';
			if(xobj && xobj->objtype == OBJ_CONTAINER)
			{
				if(xobj->objtype == OBJ_CONTAINER)
				{
					location	= "in object";
					pname		= xobj->name;
					ptype		= 'O';
				}
				else
					location	= "in room";
				parent = reset;
			}
			else if(parent && !parent->obj)
				location = reset->command[0] != '\0' ? reset->command : "in inventory";
			else
				location = "in room";
			break;
		case TYPE_EXIT:
			if(!ValidString(reset->command))
				name = "&-MNon-existant";
			else
				name = hashfind_creature(reset->vnum)->name;
			type = 'E';
			break;
		}

		if(room->resets == reset)
		sendcrit(crit,  "No Loads     Name                   Location     Chance Min-Max\n\r"
				"== ========= ====================== ============ ====== =======");
		sendcritf(crit, "%s%-2li %c-%-7li %-22.22s %-12.12s %-6li %3li-%-3li&N",
			editing == reset ? "&+R":"",
			total,
			type,
			reset->vnum,
			name,
			location,
			reset->chance,
			reset->min, reset->max,
			pname );


	}
	return total;
}
long reset_editor(CREATURE *crit, char **arguments, RESET *reset)
{
	OlcInit()

	OlcInt("vnum",		reset->vnum,		1,2000000000)
	OlcInt("chance",	reset->chance,		1,100)
	OlcInt("min",		reset->min,		1,35)
	OlcInt("max",		reset->max,		1,100)
	OlcInt("time",		reset->time,		1,1200)
	OlcValuesStruct("type",	reset->loadtype,	reset_types)

	switch(reset->loadtype)
	{
	case TYPE_CREATURE:
	OlcValues("position",	reset->position,	position_table)
	OlcValues("state",	reset->state,		state_table)
	break;
	case TYPE_OBJECT:
	OlcStr("worn",		reset->command,		0, 24)
	check_worn(crit, (void*)reset);
	OlcInt("inside",	reset->inside,		0, 1)
	break;
	case TYPE_EXIT:
	OlcStr("direction",	reset->command,		1, 24)
	break;
	default: break;
	}

	switch(reset->loadtype)
	{
	case TYPE_CREATURE:
		reset->crit	= hashfind_creature(reset->vnum);
		reset->obj	= 0;
		break;
	case TYPE_OBJECT:
		reset->crit	= reset->prev ? reset->prev->crit : 0;
		reset->obj	= hashfind_object(reset->vnum);
		break;
	}

	OlcSendInit()
	sendcrit(crit,"[RESET]");
	sendcritf(crit,"   Loads:        %s", get_reset_name(reset) );

	OlcSend("Vnum:        %li",	reset->vnum		);
	OlcSend("Chance Load: %li",	reset->chance		);
	OlcSend("Min#:        %li",	reset->min		);
	OlcSend("Max#:        %li",	reset->max		);
	OlcSend("Time:        %li",	reset->time		);
	OlcSend("Type:        %s",
		reset->loadtype == TYPE_CREATURE ? "&+BCreature&N" :
		reset->loadtype == TYPE_OBJECT ? "&+GObject&N" : "&+RExit&N" );

	switch(reset->loadtype)
	{
	case TYPE_CREATURE:
	OlcSend("Position:    %s",	position_table[reset->position]);
	OlcSend("State:       %s",	state_table[reset->state]);
	break;
	case TYPE_OBJECT:
	OlcSend("Worn:        %s",	reset->command		);
	OlcSend("Inside:      %li",	reset->inside		);
	break;
	case TYPE_EXIT:
	OlcSend("Direction:   %s",	reset->command		);
	break;
	}

	sendcrit(crit,"");
	display_resets(crit);

//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


long room_editor(CREATURE *crit, char **arguments, ROOM *room)
{
	EXTRA *extra;
	EXIT *exit;
	char *exitname = 0;
	long x;
	OlcInit()

	OlcStr("name",		room->name,		3,80)
	OlcStr("description",	room->description,	0,2048)
	OlcStr("nightname",	room->night_name,	3,80)
	OlcStr("nightdescription",room->night_description,	0,2048)
	OlcInt("light",		room->light,		-100,100)
	OlcValues("roomtype",	room->roomtype,		roomtype_table)

	ExtraDescMacro(room)

	if(arguments[0] && !strcasecmp(arguments[0], "exit")) // exit <dir> <vnum> door(type) <key vnum>
	{
		if(!arguments[1] || !arguments[2]
		|| (!is_number(arguments[2])
		    && strcasecmp(arguments[2],"delete") && strcasecmp(arguments[2],"remove")))
		{
			sendcrit(crit,"Syntax: exit <dir> <vnum> (none/door/closed/locked:optional) (key vnum:optional)");
			sendcrit(crit,"        exit <dir> delete (both:optional)");
			sendcrit(crit,"Deleting an exit with the 'only' option will not delete the reverse exit.");
			return 1;
		}
		for(exit = room->exits; exit; exit = exit->next)
		{
			if(!strcasecmp(exit->name, arguments[1]))
				break;
		}

		if( exit
		&& (!strcasecmp(arguments[2],"delete") || !strcasecmp(arguments[2],"remove")))
		{
			ROOM *oroom = hashfind_room(exit->to_vnum);
			EXIT *oexit;

			if(oroom && arguments[3] && strcasecmp(arguments[3],"only"))
			{
				for(oexit = oroom->exits; oexit; oexit = oexit->next)
					if(oexit->to_vnum == room->vnum)
						break;
				if(oexit)
					delete_object(crit, oexit, 0);
			}

			delete_object(crit, exit, 0);
		}
		else
		{
			if(!exit)
				exit = new_exit(room);
	
			for(i = 0; directions[i].abbr; i++)
				if(!strcasecmp(arguments[1], directions[i].abbr))
				{
					exitname = directions[i].name;
					break;
				}
			if(!directions[i].abbr)
				exitname = arguments[1];
	
			exit->to_vnum = atoi(arguments[2]);
			str_dup(&exit->name, exitname);
	
			if(arguments[3])
			{
				if(!strcasecmp(arguments[3],"door"))		exit->door = DOOR_OPEN;
				else if(!strcasecmp(arguments[3],"closed"))	exit->door = DOOR_CLOSED;
				else if(!strcasecmp(arguments[3],"locked"))	exit->door = DOOR_LOCKED;
				else if(!strcasecmp(arguments[3],"none"))	exit->door = DOOR_NONE;
			}
			if(arguments[3] && arguments[4] && is_number(arguments[4]))
				exit->key = atoi(arguments[4]);
		}
		crit->socket->modified = 1;
	}


	OlcSendInit()

	sendcritf(crit,"[&+RPROTOTYPE&N] &+yBuilders[%s]&N", room->loaded );
	sendcritf(crit,
		"    Vnum:        %li",	room->vnum			);
	OlcSend("Name:        %s",	room->name			);
	OlcSend("Description: \n\r%s",	room->description		);
	OlcSend("&+LNight Name:&N  %s",	room->night_name		);
	OlcSend("&+LNight Desc:&N  \n\r%s", room->night_description	);
	OlcSend("Light:       %li",	room->light			);
	OlcSend("Room Type:   %s",	roomtype_table[room->roomtype]	);

	if(room->extras) sendcrit(crit,"");
	for(x=1, extra = room->extras; extra; extra = extra->next)
		sendcritf(crit,"[&+G%li&N] Extra Description [&+G%s&N]", x++, extra->keywords);

	if(room->exits) sendcrit(crit,"");
	for(i = 0, exit = room->exits; exit; i++, exit = exit->next)
	{
	sendcritf(crit,"Exit: &+G%-5s&N to &+G%li&N [%s] :: Door:%s  KeyVnum:%li",
		exit->name, exit->to_vnum,
		hashfind_room(exit->to_vnum) ? hashfind_room(exit->to_vnum)->name : "Non-existant",
		!exit->door ? "None" : exit->door == DOOR_OPEN ? "Open" :
		exit->door == DOOR_CLOSED ? "Closed" : "Locked",
		exit->key );
	}
//	send_to(crit->socket,"\n\rOption: ");
	return 1;
}


/////////////////
// Main Editor //
/////////////////
long find_next(void *(*function)(long vnum), long num, long max)
{
	long i;

	for(i = num+1; i <= (max+1); i++)
	{
		if(function(i))
			break;
	}

	return i == (max+2) ? -1 : i;
}
long find_prev(void *(*function)(long vnum), long num, long min)
{
	long i;

	for(i = num-1; i >= (min-1); i--)
	{
		if(function(i))
			break;
	}

	return i == (min-2) ? -1 : i;
}


AREA *find_area(long vnum)
{
	AREA *area;

	for(area = area_list; area; area = area->next)
	{
		if(vnum >= area->low && vnum <= area->high)
			return area;
	}
	return area;
}

// backward/forward macro
#define BFMacro(findfunc, type)									\
	area = find_area(((type*)crit->socket->editing)->vnum);					\
	if(!strcasecmp(arguments[0],">")) {							\
		if((vnum = find_next((void*)findfunc,						\
				    ((type*)crit->socket->editing)->vnum, area->high)) < 0) {	\
			sendcrit(crit, "End of the line.");					\
			return 1;								\
		}										\
	}											\
	else {											\
		if((vnum = find_prev((void*)findfunc,						\
				    ((type*)crit->socket->editing)->vnum, area->low)) < 0) {	\
			sendcrit(crit, "End of the line.");					\
			return 1;								\
		}										\
	}											\
	crit->socket->editing = (void*)findfunc(vnum);						\
	crit->socket->modified = 0;


long check_extra_editing(CREATURE *crit)
{
	if(*(unsigned int*)crit->socket->editing == TYPE_EXTRA)
	{
		EXTRA *extra = (EXTRA*)crit->socket->editing;

		crit->socket->editing = extra->loadtype == TYPE_ROOM ?
			(void*)hashfind_room(extra->vnum) :
			extra->loadtype == TYPE_OBJECT ? (void*)hashfind_object(extra->vnum) :
			(void*)hashfind_creature(extra->vnum);
		interpret(crit,"");
		return 1;
	}
	return 0;
}


long editor(CREATURE *crit, char **arguments)
{
	AREA *area=0;
	long vnum=0;
	long all=0;

	if(arguments[0] && (!strcasecmp(arguments[0],"cancel") || !strcasecmp(arguments[0],"quit")))
	{
		stop_editing(crit,1);
		sendcrit(crit, "Your work has not been saved.");
		return 1;
	}

	if(arguments[0] && (!strcasecmp(arguments[0],"done") || !strcasecmp(arguments[0],"save")))
	{
		save_editing(crit);

		if(check_extra_editing(crit))
			return 1;

		if(!strcasecmp(arguments[0],"save"))
		{
			if(crit->socket->modified)
				sendcrit(crit,"Your work has been saved.");
			else
				sendcrit(crit,"You haven't changed anything.");
		}
		else
		{
			stop_editing(crit,1);
			if(crit->socket->modified)
				sendcrit(crit,"Your work has been saved.");
			return 1;
		}
	}

	if(arguments[0] && !strcasecmp(arguments[0],"delete"))
	{
		if(!arguments[1])
		{
			sendcrit(crit, "You must type: delete yes [all]");
			return 1;
		}
		if(arguments[2] && !strcasecmp(arguments[2], "all"))
			all = 1;
		delete_object(crit, crit->socket->editing, all);

		if(check_extra_editing(crit))
			return 1;

		stop_editing(crit,1);
		return 1;
	}

	if(arguments[0] && (!strcasecmp(arguments[0],">") || !strcasecmp(arguments[0],"<")))
	{
		save_editing(crit);

		switch(*(unsigned int*)crit->socket->editing)
		{
		case TYPE_CREATURE:
			BFMacro(hashfind_creature, CREATURE)
			interpret(crit,"");
			break;
		case TYPE_OBJECT:
			BFMacro(hashfind_object, OBJECT)
			interpret(crit,"");
			break;
		case TYPE_ROOM:
			BFMacro(hashfind_room, ROOM)
			interpret(crit,"");
			break;
		case TYPE_EXTRA:
		{
			EXTRA *extra = (EXTRA*)crit->socket->editing;

			if(!strcasecmp(arguments[0],">"))
			{
				if(extra->next)
					extra = extra->next;
				else
					extra = new_extra(extra->loadtype == TYPE_ROOM ? 
						(void*)hashfind_room(extra->vnum) : (void*)hashfind_object(extra->vnum));
				crit->socket->editing = (void*)extra;
				interpret(crit,"");
			}
			if(!strcasecmp(arguments[0],"<"))
			{
				if(extra->prev)
					extra = extra->prev;
				else
					sendcrit(crit,"End of the line.");
				crit->socket->editing = (void*)extra;
				interpret(crit,"");
			}
			break;
		}
		case TYPE_RESET:
		{
			RESET *reset;
			reset = (RESET*)crit->socket->editing;

			for(reset = ((RESET*)crit->socket->editing)->room->resets; reset; reset = reset->next)
			{
				if(!strcasecmp(arguments[0],"<")
				&& reset->next == (RESET*)crit->socket->editing)
					break;			
				else if(!strcasecmp(arguments[0],">")
				&& reset->prev == (RESET*)crit->socket->editing)
					break;
			}
			if(!reset)
			{
				sendcrit(crit,"There is no available reset in that direction.");
				break;
			}
			crit->socket->editing = (void*)reset;
			interpret(crit,"");
			break;
		}
		default:
			break;
		}
	}

	switch(*(unsigned int*)crit->socket->editing)
	{
	case TYPE_AREA:
		return area_editor(crit, arguments, (AREA*)crit->socket->editing);
	case TYPE_BAN:
		return ban_editor(crit, arguments, (BAN*)crit->socket->editing);
	case TYPE_CREATURE:
		return creature_editor(crit, arguments, (CREATURE*)crit->socket->editing);
	case TYPE_EXTRA:
		return extra_editor(crit, arguments, (EXTRA*)crit->socket->editing);
	case TYPE_OBJECT:
		return object_editor(crit, arguments, (OBJECT*)crit->socket->editing);
	case TYPE_SHOP:
		return shop_editor(crit, arguments, (SHOP*)crit->socket->editing);
	case TYPE_ROOM:
		return room_editor(crit, arguments, (ROOM*)crit->socket->editing);
	case TYPE_SOCIAL:
		return social_editor(crit, arguments, (SOCIAL*)crit->socket->editing);
	case TYPE_HELP:
		return help_editor(crit, arguments, (HELP*)crit->socket->editing);
	case TYPE_NOTE:
		return note_editor(crit, arguments, (NOTE*)crit->socket->editing);
	case TYPE_RESET:
		return reset_editor(crit, arguments, (RESET*)crit->socket->editing);
	default:
		crit->socket->editing = 0;
		mudlog("EDITOR: broken type");
		return 1;
	}
	return 0;
}


CREATURE *check_editing(void *obj)
{
	MSOCKET *socket;

	for(socket = socket_list; socket; socket = socket->next)
	{
		if(!IsEditing(socket->pc))
			continue;

		if(TypeHelp(obj) && TypeHelp(obj))
		{
			if(!strcmp(((HELP*)obj)->keyword,
				   ((HELP*)socket->editing)->keyword))
			{
				return socket->pc;
			}
		}

		if(obj == socket->editing)
			return socket->pc;
	}
	return 0;
}


long check_builder(CREATURE *crit, void *obj)
{
	CREATURE *xcrit=0;
	OBJECT *xobj=0;
	AREA *area=0;
	NOTE *note=0;
	SHOP *shop=0;

	if(crit->level == LEVEL_IMP)
		return 1;

	switch(*(unsigned int*)obj)
	{
	case TYPE_AREA:
		area = (AREA*)obj;
		if(!strargcmp(area->builders, crit->name))
			return 1;
		break;
	case TYPE_BAN:
		if(crit->level != LEVEL_IMP)
			return 0;
		break;
	case TYPE_CREATURE:
		xcrit = (CREATURE*)obj;
		area = find_area(xcrit->vnum);

		if(IsPlayer(xcrit) && crit->level != LEVEL_IMP)
			return 0;

		if(area && 
		  (!strargcmp(area->builders, crit->name)
		|| !strargcmp(area->builders, "all"))) // FIX :: REMOVE
			return 1;
		break;
	case TYPE_OBJECT:
		xobj = (OBJECT*)obj;
		area = find_area(xobj->vnum);
		xcrit = HeldBy(xobj);

		if(xcrit && IsPlayer(xcrit) && crit->level != LEVEL_IMP)
		{
			if(!strargcmp(xobj->loaded, crit->name))
				return 1;
			return 0;
		}

		if(area &&
		  (!strargcmp(area->builders, crit->name)
		|| !strargcmp(area->builders, "all"))) // FIX :: REMOVE
			return 1;
		break;
	case TYPE_ROOM:
		area = find_area(((ROOM*)obj)->vnum);
		if(area && 
		  (!strargcmp(area->builders, crit->name)
		|| !strargcmp(area->builders, "all"))) // FIX :: REMOVE
			return 1;
		break;
	case TYPE_SHOP:
		shop = (SHOP*)obj;
		area = find_area(shop->keeper);
		if (area && 
		   (!strargcmp(area->builders, crit->name)
		|| !strargcmp(area->builders, "all"))) // FIX :: REMOVE
			return 1;
		break;
	case TYPE_SOCIAL:
		if(IsImmortal(crit))
			return 1;
		break;
	case TYPE_HELP:
		if(IsImmortal(crit))
			return 1;
		break;
	case TYPE_NOTE:
		note = (NOTE*)obj;
		if(strcasecmp(note->sender, crit->name))
			return 0;
		return 1;
		break;
	default:
		mudlog("check_builder: invalid type");
		break;
	}

	return 0;
}


// i'm such a peeping tom!!
char *edit_quickinfo(CREATURE *crit)
{
	static char buf[MAX_BUFFER];
	void *obj = crit->socket->editing;

	buf[0] = '\0';
	switch(*(unsigned int*)obj)
	{
	case TYPE_AREA:
		sprintf(buf,"Area: %s",((AREA*)obj)->name);
		break;
	case TYPE_BAN:
		sprintf(buf,"Ban: %s",((BAN*)obj)->name);
		break;
	case TYPE_CREATURE:
		sprintf(buf,"%s[%s][%li]: %s", 
			IsPlayer(((CREATURE*)obj)) ? "Player" : "Creature",
			((CREATURE*)obj)->prototype == 1 ? "Prototype" : "Instance",
			((CREATURE*)obj)->vnum, ((CREATURE*)obj)->name);
		break;
	case TYPE_OBJECT:
		sprintf(buf,"Object[%s][%li]: %s",
			((OBJECT*)obj)->prototype == 1 ? "Prototype" : "Instance",
			((OBJECT*)obj)->vnum, ((OBJECT*)obj)->name);
		break;
	case TYPE_ROOM:
		sprintf(buf,"Room[%li]: %s", ((ROOM*)obj)->vnum, ((ROOM*)obj)->name);
		break;
	case TYPE_RESET: // this will get ugly!
		sprintf(buf,"Reset[%li]:%s", ((RESET*)obj)->vnum,
			((RESET*)obj)->loadtype == TYPE_CREATURE ? "Creature" : 
			((RESET*)obj)->loadtype == TYPE_OBJECT ? "Object" : "Exit");
		break;
	case TYPE_SHOP:
		sprintf(buf,"Shop[%li]", ((SHOP*)obj)->keeper);
		break;
	case TYPE_SOCIAL:
		sprintf(buf,"Social: %s", ((SOCIAL*)obj)->name);
		break;
	case TYPE_HELP:
		sprintf(buf,"Help: %s", ((HELP*)obj)->keyword);
		break;
	case TYPE_NOTE:
		sprintf(buf,"Note: %s", ((NOTE*)obj)->subject);
		break;
	default:
		mudlog("edit_quickinfo: unknown type");
		break;
	}
	return buf;
}
	

//////////////
// DO FUNCS //
//////////////
CMD(do_dig)
{
	AREA *area = 0;
	ROOM *room = 0;
	ROOM *wasin = 0;
	EXIT *exit;
	char *dir=0, *reverse=0;
	long i;

	for(i = 0; directions[i].abbr; i++)
		if(!strcasecmp(directions[i].abbr, arguments[0])
		|| !strcasecmp(directions[i].name, arguments[0]))
		{
			dir = directions[i].name;
			reverse = directions[i].reverse;
			break;
		}
	if(!dir)
		dir = arguments[0];
	if(arguments[1])
		reverse = arguments[1];

	for(exit = crit->in_room->exits; exit; exit = exit->next)
	{
		if(!strcasecmp(exit->name, dir))
		{
			sendcrit(crit, "That exit already exists.");
			return;
		}
	}

	area = find_area(crit->in_room->vnum);
	for(i = area->low; i <= area->high; i++)
	{
		if(hashfind_room(i))
			continue;
		room = new_room(i);
		break;
	}
	if(!room)
	{
		sendcrit(crit,"There are no more rooms available in this area.");
		return;
	}

	exit		= new_exit(crit->in_room);
	exit->to_vnum	= i;
	str_dup(&exit->name, dir);
	fwrite_room(crit->in_room->vnum);

	wasin = crit->in_room;
	trans(crit, room);
	sendcritf(crit, "%s exit added, moving you to the next room.", dir);

	// add reverse exit
	if(!reverse)
	{
		sendcrit(crit, "No reverse exit found, you may need to edit the room and create one.");
		fwrite_room(room->vnum);
		return;
	}

	for(exit = room->exits; exit; exit = exit->next)
	{
		if(!strcasecmp(exit->name, reverse)) // already here
		{
			fwrite_room(room->vnum);
			return;
		}
	}

	exit		= new_exit(room);
	exit->to_vnum	= wasin->vnum;
	str_dup(&exit->name, reverse);
	fwrite_room(room->vnum);
}


CMD(do_edit)
{
	CREATURE *critp=0;
	SOCIAL *social=0;
	RESET *reset=0;
	OBJECT *objp=0;
	AREA *area=0;
	ROOM *room=0;
	SHOP *shop=0;
	HELP *help=0;
	NOTE *note=0;
	void *toedit=0;
	BAN *ban=0;
	long x=0;
	char **arguments2;

	if(!strindex("social", arguments[0]))
	{
		if(!(social = hashfind_social(arguments[1])))
			social = new_social(arguments[1]);

		if(!ValidString(social->name))
			str_dup(&social->name, arguments[1]);

		for(x = 0; command_table[x].command; x++)
		{
			if(!strcasecmp(command_table[x].command, arguments[1]))
			{
				sendcrit(crit, "You can't make socials named after commands.");
				return;
			}
		}

		toedit = (void*)social;
	}
	else if(!strindex("creature", arguments[0]) || !strindex("crit", arguments[0])
	     || !strindex("mobile", arguments[0])
	     || !strindex("player", arguments[0]))
	{
		if(is_number(arguments[1]) && !arguments[2])
		{
			if(!(critp = hashfind_creature(atoi(arguments[1]))))
				critp = new_creature_proto(atoi(arguments[1]));
		}
		else
		{
			arguments2 = make_arguments(all_args(arguments,1));
			if(!(critp = find_crit(crit, arguments2[0], CRIT_WORLD)))
			{
				free_arguments(arguments2);
				sendcrit(crit, "No such creature/player found.");
				return;
			}
			free_arguments(arguments2);
		}
		toedit = (void*)critp;
	}
	else if(!strindex("shop", arguments[0]))
	{
		if(!is_number(arguments[1]))
		{
			sendcrit(crit, "What shop vnum do you wish to edit?");
			return;
		}
		if (!(shop = hashfind_shop(atoi(arguments[1]))))
			shop = new_shop(atoi(arguments[1]));
		if (!shop)
		{	
			sendcrit(crit,"No shop can be created until a creature exists for it.");
			return;
		}
		toedit = (void*)shop;
	}
	else if(!strindex("object", arguments[0])
	     || !strindex("thing", arguments[0]))
	{
		if(is_number(arguments[1]) && !arguments[2])
		{
			if(!(objp = hashfind_object(atoi(arguments[1]))))
				objp = new_object_proto(atoi(arguments[1]));
		}
		else
		{
			arguments2 = make_arguments(all_args(arguments,1));
			if(!(objp = find_obj(crit, arguments2[0], OBJ_GROUND|OBJ_HELD)))
			{
				free_arguments(arguments2);
				sendcrit(crit, "No such object found.");
				return;
			}
			free_arguments(arguments2);
		}
		toedit = (void*)objp;
	}
	else if(!strindex("room", arguments[0]))
	{
		if(!is_number(arguments[1]))
		{
			sendcrit(crit, "What vnum?");
			return;
		}
		if(!(room = hashfind_room(atoi(arguments[1]))))
			room = new_room(atoi(arguments[1]));

		toedit = (void*)room;
	}
	else if(!strindex("reset", arguments[0]))
	{
		RESET *newr = 0;
		long num = 0;
		char buf[MAX_BUFFER];

		if(!strcasecmp(arguments[1],"new"))
		{
			if(arguments[2])
				num	= atoi(arguments[2]);
			newr		= new_reset();
			newr->room	= crit->in_room;
			newr->roomvnum	= crit->in_room->vnum;
		}
		else
			num		= atoi(arguments[1]);

		for(x = 1, reset = crit->in_room->resets; reset; reset = reset->next, x++)
		{
			if(x == num)
				break;
		}
		if(newr)
		{
			if(!reset && crit->in_room->resets)
			{
				crit->in_room->resets->prev = newr;
				newr->next = crit->in_room->resets;
				crit->in_room->resets = newr;
				reset = newr;
			}
			else if(reset)
			{
				newr->next	= reset->next;
				reset->next	= newr;
				newr->prev	= reset;
				reset		= newr;
			}
			else
			{
				crit->in_room->resets = newr;
				newr->next = newr->prev = 0;
				reset = newr;
			}
		}
		if(!reset)
		{
			sprintf(buf, "help resets");
			interpret(crit, buf);
			return;
		}
		toedit = (void*)reset;
	}
	else if(!strindex("area", arguments[0]))
	{
		if(!strcasecmp("new", arguments[1]) && crit->level == LEVEL_IMP)
			area = new_area("new area");

		if(!is_number(arguments[1]) && !area)
		{
			sendcrit(crit,"Syntax: edit area <areanum>\n\rUse area command to get num.");
			return;
		}

		if(!area)
		{
			for(area = area_list, x=1; area && x != atoi(arguments[1]); area = area->next, x++)
				;

			if(!area)
			{
				sendcrit(crit,"Invalid area number.\n\rUse area command to get num.");
				return;
			}
		}
		toedit = (void*)area;
	}
	else if(!strindex("help", arguments[0]))
	{
		if(!strcasecmp("new", arguments[1]))
		{
			help = new_help();

			str_dup(&help->index,"General");

			if(arguments[2] && ValidString(arguments[2]))
				str_dup(&help->keyword, all_args(arguments,2));
			else
				str_dup(&help->keyword, "unknown");
		}

		arguments2 = make_arguments(all_args(arguments, 1));
		if(!help && !(help = mysql_find_help(crit, arguments2)))
		{
			sendcrit(crit, "\n\rTo create a new help file, use-> edit help new <keyword>");
			free_arguments(arguments2);
			return;
		}
		free_arguments(arguments2);

		toedit = (void*)help;
	}
	else if(!strindex("note", arguments[0]))
	{
		note = new_note();
		if(arguments[1])
			str_dup(&note->subject, all_args(arguments, 1));
		str_dup(&note->sender, crit->name);

		toedit = (void*)note;
	}
	else if(!strindex("ban", arguments[0]))
	{
		long num = atoi(arguments[1]);

		for(ban = ban_list, x=1; ban; ban = ban->next, x++)
		{
			if(num == x)
				break;
		}
		if(!ban)
		{
			sendcrit(crit,"Invalid ban number: type ban");
			return;
		}

		toedit = (void*)ban;
	}
	else
	{
		sendcrit(crit, "No such editor.");
		return;
	}

	// see if someone else is already editing this object
	if((critp = check_editing(toedit)))
	{
		sendcritf(crit,"\n\r&+Y%s is already editing that object.&N",
			critp->name);
		return;
	}

// FIX (added check to not check resets for security because check_builder uses what they are
// already editing
	if(!check_builder(crit, toedit))
	{
		sendcrit(crit,"\n\r&+YYou do not have permissions to edit that object.&N");
		return;
	}

	if(IsEditing(crit))
	{
		save_editing(crit);
		stop_editing(crit,0);
	}

	if(toedit)
	{
		crit->socket->editing = toedit;
		sendcritf(crit,"You begin editing %s.",edit_quickinfo(crit));
		message("$n starts editing $p.",crit,crit->in_room,edit_quickinfo(crit));
	}

	interpret(crit,"");
	crit->socket->modified = 0;
}
CMD(do_cedit)
{
	char buf[MAX_BUFFER];
	sprintf(buf,"edit creature %s",all_args(arguments,0));
	interpret(crit,buf);
}
CMD(do_hedit)
{
	char buf[MAX_BUFFER];
	sprintf(buf,"edit help %s",all_args(arguments,0));
	interpret(crit,buf);
}
CMD(do_oedit)
{
	char buf[MAX_BUFFER];
	sprintf(buf,"edit object %s",all_args(arguments,0));
	interpret(crit,buf);
}
CMD(do_redit)
{
	char buf[MAX_BUFFER];
	if(!arguments[0])
		sprintf(buf,"edit room %li",crit->in_room->vnum);
	else
		sprintf(buf,"edit room %s",all_args(arguments,0));
	interpret(crit,buf);
}
CMD(do_sedit)
{
	char buf[MAX_BUFFER];
	sprintf(buf,"edit social %s",all_args(arguments,0));
	interpret(crit,buf);
}
CMD(do_xedit)
{
	char buf[MAX_BUFFER];
	sprintf(buf,"edit reset %s",all_args(arguments,0));
	interpret(crit,buf);
}

void show_string_position(CREATURE *crit)
{
	char buf[MAX_BUFFER];
	long i,x=0;

	if(ValidString(crit->socket->string))
	{
		for(i = 0, x = 1; crit->socket->string[i] != '\0'; i++)
		{
			if(crit->socket->string[i] == '\n' || crit->socket->string[i] == '\r')
			{
				if(crit->socket->string[i+1] == '\n' || crit->socket->string[i+1] == '\r')
					i++;
				x++;
			}
		}
	}
	sprintf(buf,"%2li. ",x+1);
	send_to(crit->socket, buf);
}


// Dropping this at bottom cuz it's so messy :/
// the string editor assumes all strings end in \n\r or \r\n.. \n\n\r will confuse things..
long string_editor(CREATURE *crit, char *buf)
{
	MSOCKET *sock = crit->socket;
	char buf2[MAX_BUFFER];
	char command[MAX_BUFFER];
	char fix[MAX_BUFFER];
	char **arguments=0;
	char *pbuf=0, *xbuf=0;
	long done=0;
	long i=0, x=0, num=0,xbuf_num=0,last_xbuf=0,before=-1;

	if(!crit->socket->string)
	{
		sendcrit(crit,"\n\rString Editor");
		sendcrit(crit,"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
		sendcrit(crit,"Put a &+B@&N on a blank line to terminate the editor.");
		sendcrit(crit,"---------------------------------------------------");
		sendcrit(crit,"Commands: @show, @delete #, @format, @replace x y  ");
		sendcrit(crit,"          @after # text, @before # text, @help     ");
		sendcrit(crit,"          @clear                                   ");
		sendcrit(crit,"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");

		str_dup(&sock->string, buf);
		return 0;
	}

	if(!ValidString(buf))
		buf[0] = '\0';

	if(buf[0] == '@')
	{
		if(strlen(buf) == 1)
		{
			if(strcmp((*crit->socket->variable), crit->socket->string))
				crit->socket->modified	= 1;
			done			= 1;
			buf[0]			= '\0';
		}
		else // get command
		{
			arguments = make_arguments(buf+1); // skip @
			strcpy(command,arguments[0]);

			if(arguments[1] && is_number(arguments[1]))
				num = atoi(arguments[1]);

			if(!strcasecmp(command,"help"))
			{
				sendcrit(crit,"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
				sendcrit(crit,"@&+Bshow&N     - shows what your string looks like currently         ");
				sendcrit(crit,"@&+Bclear&N    - clears entire string				    ");
				sendcrit(crit,"@&+Bformat&N   - formats your string to fit roughly 80 characters a line");
				sendcrit(crit,"@&+Bdelete #&N - deletes line #                                      ");
				sendcrit(crit,"@&+Bafter # text&N  - places 'text' after line #                     ");
				sendcrit(crit,"@&+Bbefore # text&N - places 'text' before line #                    ");
				sendcrit(crit,"@&+Breplace x y&N - replaces all occurnces of x with y.  this command");
				sendcrit(crit,"               requires x have a space before and after.  therefore, ");
				sendcrit(crit,"               you cannot replace anything that is at the start or   ");
				sendcrit(crit,"               end of a line.                                        ");
				sendcrit(crit,"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
				strcpy(command,"show");
			}
			else if(!strcasecmp(command,"clear"))
			{
				str_dup(&sock->string,"");
				strcpy(command,"show");
			}
			else if(!strcasecmp(command,"format"))
			{
				xbuf	= sock->string;
				pbuf	= fix;

				// get rid of newlines
				while(*xbuf != '\0')
				{
					if(*xbuf == '\n' || *xbuf == '\r')
					{
						xbuf += 2;
						*pbuf++ = ' ';
						continue;
					}
					*pbuf++ = *xbuf++;
				}
				*pbuf = '\0';

				strcpy(fix,wraptext(0,fix,70));
				strcat(command, fix);
				str_dup(&sock->string,command);
				strcpy(command,"show");
			}
			else if(!strcasecmp(command,"delete"))
			{
				if(!num)
				{
					sendcrit(crit,"You need to supply a line number greater than 0 to delete.");
					free_arguments(arguments);
					return done;
				}

				pbuf = fix;
				i = 1;
				for(x=0; sock->string[x] != '\0'; x++)
				{
					if(sock->string[x] == '\n' || sock->string[x] == '\r')
					{
						if(i != num)
						{
							*pbuf++ = sock->string[x++];
							*pbuf++ = sock->string[x];
						}
						else
							x++;
						i++;
						continue;
					}
					if(i != num)
						*pbuf++ = sock->string[x];
				}
				*pbuf = '\0';

				// get rid of extra newlines when you delete last line 
				if (ValidString(fix) && num == i) 
					*(pbuf-2) = '\0';

				if(num > i)
				{
					sendcrit(crit,"Line number is too high.");
					free_arguments(arguments);
					return done;
				}

				str_dup(&sock->string,fix);
				strcpy(command,"show");
			}
			else if(!strcasecmp(command,"after") || !strcasecmp(command,"before"))
			{
				if(!num)
				{
					sendcrit(crit,"You need to supply a line number greater than 0 to delete.");
					free_arguments(arguments);
					return done;
				}

				if(!ValidString(buf))
				{
					sendcrit(crit,"Use 'delete #' if you wish to delete a line.");
					free_arguments(arguments);
					return done;
				}

				pbuf = fix;

				for(i = 0, x = 0; i < strlen(buf); i++)
				{
					if(isspace(buf[i]))
					{
						if(++x == 2)
						{
							xbuf = &buf[i+1];
							break;
						}
					}
				}

				// before or after?
				if(!strcasecmp(command,"after"))
					before = 0;
				else
					before = 1;

				i = 1;
				for(x = 0; sock->string[x] != '\0'; x++)
				{
					if(sock->string[x] == '\n' || sock->string[x] == '\r')
					{
						i++;
						*pbuf++ = sock->string[x++];
						*pbuf++ = sock->string[x++];
					}
					if((i == (num+1) && before == 0) || (i == num && before == 1))
					{
						if (xbuf)
						{
							while(*xbuf != '\0')
								*pbuf++ = *xbuf++;
						}
						*pbuf++ = '\n';
						*pbuf++ = '\r';
						num = MAX_BUFFER*2;
					}
					*pbuf++ = sock->string[x];
				}
				if(num <= MAX_BUFFER)
				{
					sendcrit(crit,"Line number is too high.");
					free_arguments(arguments);
					return done;
				}

				*pbuf = '\0';
				str_dup(&sock->string,fix);
				strcpy(command,"show");
			}
			else if(!strcasecmp(command,"replace"))
			{
				char hold[MAX_BUFFER];
				char start[MAX_BUFFER];
				char buf[MAX_BUFFER];
				char finished[MAX_BUFFER];

				if(!arguments[1] || !arguments[2])
				{
					sendcrit(crit,"Syntax: @replace <what to replace> <with what>\n\rThis replaces single words only.");
					free_arguments(arguments);
					return done;
				}
				sprintf(command,"%s",arguments[1]);
				sprintf(hold,"%s",sock->string);
				if ((xbuf_num = strstrl(hold,command)) == -1)
				{
					sendcritf(crit,"Could not find '%s' to be replaced.",command);
					return done;
				}			
				sprintf(start,"%s",str_cut(0,xbuf_num,hold)); // start
				strcat(start,arguments[2]); 		      // middle
				sprintf(buf,"%s",str_cut(xbuf_num+strlen(command)+1,strlen(hold)-(xbuf_num+strlen(command)),hold)); // end
				while (1)
				{
					xbuf_num = strstrl(buf,command);
					if (xbuf_num == -1)
					{
						strcat(start,buf);
						break; 
					}
					sprintf(finished,"%s",str_cut(0,xbuf_num,buf));
					strcat(start,finished);
					strcat(start,arguments[2]);
					last_xbuf = xbuf_num+strlen(command);
					sprintf(finished,"%s",str_cut(last_xbuf+1,strlen(buf)-last_xbuf,buf));
					sprintf(buf,"%s",finished);
				}
				str_dup(&sock->string,start);			
				strcpy(command,"show");
			}
			if(!strcasecmp(command,"show"))
			{
				sprintf(fix," 1. ");
				pbuf = fix+4;
				x = 1;

				for(i=0; sock->string[i] != '\0'; i++)
				{
					if(sock->string[i] == '\n' || sock->string[i] == '\r')
					{
						i++;
						sprintf(command,"\n\r%2li. ",++x);
						xbuf = command;

						while(*xbuf != '\0')
							*pbuf++ = *xbuf++;
						continue;
					}
					*pbuf++ = sock->string[i];
				}
				*pbuf = '\0';

				if(strlen(fix) > 4)
					sendcrit(crit,fix);

				show_string_position(crit);
			}
			free_arguments(arguments);
		}
		return done;
	}
	sprintf(buf2,"%s%s%s",sock->string, strlen(sock->string)>0?"\n\r":"",buf);
	str_dup(&sock->string, buf2);

	show_string_position(crit);

	return done;
}
