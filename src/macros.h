/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
macros.h: ...
*/


//////////////////////
// DEBUGGING MACROS //
//////////////////////
#include <assert.h>
//#define realloc(x,y) realloc(x,y); assert(x)
//#define memset(x,y,z) assert(x); memset(x,y,z); assert(x)
//#define mysql_free_result(x) mudlog("was free result")
#define mysql_query(x,y) mudlog("QUERY: %s",y); mysql_query(x,y)
//#define mysql_query(x,y) if(mysql_query(x,y)) mudlog("Mysql query error: %s (%s)",mysql_error(&mysql), y);

///////////////////
// MISCELLANEOUS //
///////////////////
#define CMD(x)	\
	void (x)(CREATURE *crit, char **arguments, OBJECT *xobj, CREATURE *xcrit)
#define Minutes(x)	(LOOPS_PER_SECOND*60*x)
#define Seconds(x)	(LOOPS_PER_SECOND*x)
#define message(buf,stage,actor,extra)		create_message(buf,stage,actor,extra,0)
#define message_all(buf,stage,extra,world)	create_message(buf,stage,0,extra,world)
#define make_arguments(buf)			make_real_arguments(buf,' ')
#define make_arguments_deli(buf, deli)		make_real_arguments(buf,deli)

///////////////////////////////
// DATA TYPE HANDLING MACROS //
///////////////////////////////
#define ValidString(str)	((str) && (str[0]) != '\0')
#define Upper(c)		((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define Lower(c)		((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define IsSet(c,bit)		(c & bit)
#define Max(a,b)		(a>b?a:b)
#define Min(a,b)		(a>b?b:a)

//////////////////////
// CHARACTER MACROS //
//////////////////////
#define IsPlayer(crit)		(flag_isset(crit->flags, CFLAG_PLAYER))
#define IsPlaying(crit)		(!IsPlayer(crit) || crit->socket->connected == CON_PLAYING)
#define IsEditing(crit)		(crit->socket && (crit->socket->editing || crit->socket->string))
#define IsImmortal(crit)	(crit->level >= LEVEL_IMM)
#define IsImp(crit)		(crit->level == LEVEL_IMP)
#define IsDisabled(crit)	(crit->state >= STATE_SLEEPING)

///////////////////
// OBJECT MACROS //
///////////////////
#define HeldBy(obj)		(obj->held_by ? obj->held_by : \
				 obj->in_obj ? obj->in_obj->held_by ? obj->in_obj->held_by : 0 : 0)

//////////////////////
// SCRIPTING MACROS //
//////////////////////
#define TypeRoom(obj)		(*(unsigned int*)obj) == TYPE_ROOM
#define TypeObject(obj)		(*(unsigned int*)obj) == TYPE_OBJECT
#define TypeCreature(obj)	(*(unsigned int*)obj) == TYPE_CREATURE
#define TypeHelp(obj)		(*(unsigned int*)obj) == TYPE_HELP

////////////////////////
// LINKED LIST MACROS //
////////////////////////
#define NewObject(freelist,obj)			\
	if( freelist == 0 ) {			\
		obj = malloc(sizeof(*obj));	\
		memset(obj,0,sizeof(*obj));	\
	}					\
	else {					\
		obj = freelist;			\
		freelist = freelist->next;	\
		if(freelist)			\
			freelist->prev = 0;	\
		obj->next = obj->prev = 0;	\
	}

#define DeleteObject(obj)		\
	free(obj);			\
	obj = 0;

#define AddToList(list,obj)			\
	if( list != 0 ) {			\
		list->prev = obj;		\
		obj->next = list;		\
		obj->prev = 0;			\
		list = obj;			\
	}					\
	else {					\
		list = obj;			\
		list->prev = list->next = 0;	\
	}

#define AddToListEnd(list,obj,dummy)					\
	if( list != 0 ) {						\
		for(dummy = list; dummy->next; dummy = dummy->next) ;	\
		dummy->next = obj;					\
		obj->prev = dummy;					\
		obj->next = 0;						\
	}								\
	else {								\
		list = obj;						\
		list->prev = list->next = 0;				\
	}

#define RemoveFromList(list,obj)			\
	if( obj->prev == 0 ) {				\
		if( obj->next == 0 ) { 			\
			if(list == obj)			\
				list = 0;		\
		}					\
		else {					\
			list = obj->next;		\
			list->prev = obj->next = 0;	\
		}					\
	}						\
	else {						\
		if( obj->next )				\
			obj->next->prev = obj->prev;	\
		obj->prev->next = obj->next;		\
		obj->next = obj->prev = 0;		\
	}

#define AddToContent(list,obj)					\
	if( list != 0 ) {					\
		list->prev_content = obj;			\
		obj->next_content = list;			\
		obj->prev_content = 0;				\
		list = obj;					\
	}							\
	else {							\
		list = obj;					\
		list->next_content = list->prev_content = 0;	\
	}

#define AddToContentEnd(list,obj,dummy)							\
	if( list != 0 ) {								\
		for(dummy = list; dummy->next_content; dummy = dummy->next_content) ;	\
		dummy->next_content = obj;						\
		obj->prev_content = dummy;						\
		obj->next_content = 0;							\
	}										\
	else {										\
		list = obj;								\
		list->prev_content = list->next_content = 0;				\
	}

#define RemoveFromContent(list,obj)						\
	if( obj->prev_content == 0 ) {						\
		if( obj->next_content == 0 ) { 					\
			if (list == obj)					\
				list = 0;					\
		}								\
		else {								\
			list = obj->next_content;				\
			list->prev_content = obj->next_content = 0;		\
		}								\
	}									\
	else {									\
		if( obj->next_content )						\
			obj->next_content->prev_content = obj->prev_content;	\
		obj->prev_content->next_content = obj->next_content;		\
		obj->next_content = obj->prev_content = 0;			\
	}

//////////////////////
// HASH LIST MACROS //
//////////////////////
#define AddToHashList(list,obj,key)			\
	if( list[(key)%HASH_KEY] != 0 ) {		\
		obj->next_hash = list[(key)%HASH_KEY];	\
		obj->prev_hash = 0;			\
		list[(key)%HASH_KEY]->prev_hash = obj;	\
		list[(key)%HASH_KEY] = obj;		\
	}						\
	else {						\
		list[(key)%HASH_KEY] = obj;		\
		obj->next_hash = obj->prev_hash = 0;	\
	}


#define RemoveFromHashList(list,obj,key)					\
	if( obj->prev_hash == 0 ) {						\
		if( obj->next_hash == 0 ) {					\
			if(list[(key)%HASH_KEY] == obj)				\
				list[(key)%HASH_KEY] = 0;			\
		}								\
		else {								\
			list[(key)%HASH_KEY] = obj->next_hash;			\
			list[(key)%HASH_KEY]->prev_hash = obj->next_hash = 0;	\
		}								\
	}									\
	else {									\
		if( obj->next_hash )						\
			obj->next_hash->prev_hash = obj->prev_hash;		\
		obj->prev_hash->next_hash = obj->next_hash;			\
		obj->next_hash = obj->prev_hash = 0;				\
	}

//////////////////////////////////
// INPUT/OUTPUT (MySQL) MACROS ///
//////////////////////////////////
// fread
#define ReadCreatureMysql()					\
	str_dup(&crit->name,		row[C_NAME]	);	\
	str_dup(&crit->keywords,	row[C_KEYWORDS]	);	\
	str_dup(&crit->description,	row[C_DESC]	);	\
	str_dup(&crit->long_descr,	row[C_LDESC]	);	\
	str_dup(&crit->wear,		row[C_WEAR]	);	\
	str_dup(&crit->loaded,		row[C_LOADED]	);	\
								\
	crit->id		= atoi(row[0]);			\
	crit->prototype		= atoi(row[C_PROTOTYPE]);	\
	crit->level		= atoi(row[C_LEVEL]);		\
	crit->dexterity		= atoi(row[C_DEX]);		\
	crit->hp		= atoi(row[C_HP]);		\
	crit->max_hp		= atoi(row[C_MAXHP]);		\
	crit->intelligence	= atoi(row[C_INT]);		\
	crit->move		= atoi(row[C_MOVE]);		\
	crit->max_move		= atoi(row[C_MAXMOVE]);		\
	crit->sex		= atoi(row[C_SEX]);		\
	crit->state		= atoi(row[C_STATE]);		\
	crit->position		= atoi(row[C_POSITION]);	\
	crit->strength		= atoi(row[C_STR]);		\
	crit->alignment		= atoi(row[C_ALIGNMENT]);	\
								\
	read_flags(row[C_FLAGS], crit);

#define ReadObjectMysql(obj)					\
	str_dup(&obj->name,		row[O_NAME]     );	\
	str_dup(&obj->keywords,		row[O_KEYWORDS] );	\
	str_dup(&obj->description,	row[O_DESC]     );	\
	str_dup(&obj->long_descr,	row[O_LDESC]    );	\
	str_dup(&obj->wear,		row[O_WEAR]     );	\
	str_dup(&obj->loaded,		row[O_LOADED]   );	\
								\
	obj->id		= atoi(row[0]);				\
	obj->vnum	= atoi(row[O_VNUM]);			\
	obj->nested	= atoi(row[O_NESTED]);			\
	obj->owner_id	= atoi(row[O_OWNER_ID]);		\
	obj->prototype	= atoi(row[O_PROTOTYPE]);		\
	obj->objtype	= atoi(row[O_OBJTYPE]);			\
	obj->timer	= atoi(row[O_TIMER]);			\
	obj->worth	= atoi(row[O_WORTH]);			\
	obj->weight	= atoi(row[O_WEIGHT]);			\
								\
	read_values(row[O_OBJVALUES], obj->values);		\
	read_flags(row[O_FLAGS], obj);

#define ReadResetMysql(row)					\
	reset->id		= atoi(row[X_ID]);		\
	reset->chance		= atoi(row[X_CHANCE]);		\
	reset->inside		= atoi(row[X_INSIDE]);		\
	reset->loadtype		= atoi(row[X_LOADTYPE]);	\
	reset->nested		= atoi(row[X_NESTED]);		\
	reset->min		= atoi(row[X_MIN]);		\
	reset->max		= atoi(row[X_MAX]);		\
	reset->position		= atoi(row[X_POSITION]);	\
	reset->state		= atoi(row[X_STATE]);		\
	reset->roomvnum		= atoi(row[X_ROOMVNUM]);	\
	reset->time		= atoi(row[X_TIME]);		\
	reset->vnum		= atoi(row[X_VNUM]);		\
	str_dup(&reset->command, row[X_COMMAND]);

// fwrite
#define WriteCreatureMysql()						\
	add_int(buf,"vnum",		crit->vnum		);	\
	add_int(buf,"prototype",	crit->prototype		);	\
	add_str(buf,"keywords",		crit->keywords		);	\
	add_str(buf,"long_descr",	crit->long_descr	);	\
	add_str(buf,"description",	crit->description	);	\
	add_str(buf,"wear",		crit->wear		);	\
	add_str(buf,"loaded",		crit->loaded		);	\
	add_int(buf,"level",		crit->level		);	\
	add_int(buf,"dexterity",	crit->dexterity		);	\
	add_int(buf,"intelligence",	crit->intelligence	);	\
	add_int(buf,"hp",		crit->hp		);	\
	add_int(buf,"max_hp",		crit->max_hp		);	\
	add_int(buf,"strength",		crit->strength		);	\
	add_int(buf,"alignment",	crit->alignment		);	\
	add_int(buf,"sex",		crit->sex		);	\
	add_int(buf,"state",		crit->state		);	\
	add_int(buf,"position",		crit->position		);	\
	add_int(buf,"move",		crit->move		);	\
	add_int(buf,"max_move",		crit->max_move		);	\
	add_str(buf,"flags",		write_flags(crit)	);

#define WriteObjectMysql()						\
	add_int(buf,"vnum",		obj->vnum		);	\
	add_int(buf,"prototype",	obj->prototype		);	\
	add_str(buf,"keywords",		obj->keywords		);	\
	add_str(buf,"long_descr",	obj->long_descr		);	\
	add_str(buf,"description",	obj->description	);	\
	add_str(buf,"wear",		obj->wear		);	\
	add_str(buf,"loaded",		obj->loaded		);	\
	add_int(buf,"nested",		obj->nested		);	\
	add_int(buf,"objtype",		obj->objtype		);	\
	add_str(buf,"objvalues",	write_values(obj->values));	\
	add_int(buf,"timer",		obj->timer		);	\
	add_int(buf,"weight",		obj->weight		);	\
	add_str(buf,"worn",		obj->worn		);	\
	add_int(buf,"worth",		obj->worth		);	\
	add_str(buf,"flags",		write_flags(obj)	);

#define WriteResetMysql(reset)							\
	add_int(buf,"inside",		reset->inside			);	\
	add_int(buf,"nested",		reset->nested			);	\
	add_int(buf,"min",		reset->min			);	\
	add_int(buf,"max",		reset->max			);	\
	add_int(buf,"position",		reset->position			);	\
	add_int(buf,"roomvnum",		reset->roomvnum			);	\
	add_int(buf,"time",		reset->time			);	\
	add_int(buf,"loadtype",		reset->loadtype			);	\
	add_int(buf,"state",		reset->state			);	\
	add_int(buf,"vnum",		reset->vnum			);	\
	add_str(buf,"command",		reset->command			);	\

///////////////////////
// OLC/EDITOR MACROS //
///////////////////////
// NOTE on Olc macro:  for any olc/editor you use this for, you need a buf and var
// variable.  var for ints and buf for strings.  you also need an index variable,
// for the OLC editor..
#define OlcSendInit()			\
	i = 1;				\
	if(arguments[0]) return 0;	
//	send_to(crit->socket,"KEKE");
//	sendcrit(crit,"");

#define OlcSend(str, var)					\
	sendcritf(crit, "%2li. "str, i++, var);

#define OlcInit()								\
	char buf[MAX_BUFFER];							\
	long var=0;								\
	long index=0;								\
	long indexarg=0;							\
	long i=0;								\
	long stredit=0;								\
										\
	if(arguments[0] && arguments[1] && !strcasecmp(arguments[1],"@"))	\
		stredit = 1;							\
										\
	if(arguments[0] && arguments[1] && is_number(arguments[1]))		\
		i = var = atoi(arguments[1]);					\
										\
	if(arguments[0] && is_number(arguments[0]))				\
		indexarg = atoi(arguments[0]);					\
										\
	strcpy(buf, all_args(arguments,1));					


#define OlcStr(str, field, low, high)								\
	if(++index == indexarg || (arguments[0] && !strindex(str, arguments[0]))) {		\
		if(stredit) 									\
		{										\
			crit->socket->variable = &field;						\
			if(ValidString(field))	sprintf(buf,"%s",field);			\
			else			buf[0] = '\0';					\
			string_editor(crit, buf);						\
			string_editor(crit, "@show");						\
			return 1;								\
		}										\
		if(arguments[1] && !strcasecmp(arguments[1],"?")) {				\
			olc_help(crit, str);							\
			return 1;								\
		}										\
		if(!stredit && (strlen(buf) < low || strlen(buf) > high)) {			\
			sendcritf(crit,"Syntax: %li/%s @/[text]", index, str);			\
			sendcritf(crit,"String must be greater than %li and less than %li.",	\
				low, high );							\
			return 1;								\
		}										\
		if(arguments[1] &&								\
		  (!strcasecmp(arguments[1],"clear") || !strcasecmp(arguments[1],"delete"))) {	\
			str_dup(&field, "");							\
			crit->socket->modified++;						\
			return 1;								\
		}										\
		else {										\
			str_dup(&field, buf);							\
			crit->socket->modified++;						\
			return 1;								\
		}										\
	}

#define OlcInt(str, field, low, high)								\
	if(++index == indexarg || (arguments[0] && !strindex(str, arguments[0]))) {		\
		if(arguments[1] && !strcasecmp(arguments[1],"?")) {				\
			olc_help(crit, str);							\
			return 1;								\
		}										\
		if(var < low || var > high) {							\
			sendcritf(crit,"Syntax: %li/%s [#]", index, str);			\
			sendcritf(crit,"Value must be greater than %li and less than %li.",	\
				low, high );							\
			return 1;								\
		}										\
		field = var;									\
		crit->socket->modified++;							\
		return 1;									\
	}


// string sequence, it will add or remove from the string, ie. for ->wear (head arms legs)
#define OlcStrSeq(str, field, low, high, delimeter)						\
	if(++index == indexarg || (arguments[0] && !strindex(str, arguments[0]))) {		\
		if(arguments[1] && !strcasecmp(arguments[1],"?")) {				\
			olc_help(crit, str);							\
			return 1;								\
		}										\
		if(strlen(buf) < low || strlen(buf) > high) {					\
			sendcritf(crit,"Syntax: %li/%s [text] <remove>", index, str);		\
			sendcritf(crit,"String must be greater than %li and less than %li.",	\
				low, high );							\
			return 1;								\
		}										\
		if(arguments[1] && arguments[2]							\
		&&(!strargindex("remove drop clear delete",arguments[last_arg(arguments)]))	\
		&& strstr(field, split_args(arguments,1,last_arg(arguments)-1)) != 0)		\
			str_dup(&field, str_minus(field, split_args(arguments,1,last_arg(arguments)-1), delimeter));	\
		else										\
			str_dup(&field, str_add(field, all_args(arguments,1), delimeter));	\
		crit->socket->modified++;							\
		return 1;									\
	}

#define OlcFlags(str, flags, function)							\
	if(++index == indexarg || (arguments[0] && !strindex(str, arguments[0]))) {	\
		if(arguments[1] && !strcasecmp(arguments[1],"?")) {			\
			olc_help(crit, str);						\
			return 1;							\
		}									\
		if((i = function(arguments[1])) >= 0) {					\
			if(flag_isset(flags, i))					\
				flag_remove(flags,i);					\
			else								\
				flag_set(flags, i);					\
			crit->socket->modified++;					\
			return 1;							\
		}									\
	}

#define OlcValues(str, field, table)							\
	if(++index == indexarg || (arguments[0] && !strindex(str, arguments[0]))) {	\
		if(arguments[1] && !strcasecmp(arguments[1],"?")) {			\
			olc_help(crit, str);						\
			return 1;							\
		}									\
		if(!ValidString(arguments[1]))						\
			sendcrit(crit,"Available types are:");				\
		for(i = 0; table[i]; i++) {						\
			if(!ValidString(arguments[1]))					\
				sendcritf(crit," %s",table[i]);				\
			else if(!strcmp(table[i], arguments[1])) {			\
				field = i;						\
				crit->socket->modified++;				\
				return 1;						\
			}								\
		}									\
	}

#define OlcValuesStruct(str, field, table)						\
	if(++index == indexarg || (arguments[0] && !strindex(str, arguments[0]))) {	\
		if(arguments[1] && !strcasecmp(arguments[1],"?")) {			\
			olc_help(crit, str);						\
			return 1;							\
		}									\
		if(!ValidString(arguments[1]))						\
			sendcrit(crit,"Available types are:");				\
		for(i = 0; table[i].name; i++) {					\
			if(!ValidString(arguments[1]))					\
				sendcritf(crit," %s",table[i].name);			\
			else if(!strcmp(table[i].name, arguments[1])) {			\
				field = table[i].olcvalue;				\
				crit->socket->modified++;				\
				return 1;						\
			}								\
		}									\
	}


