/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
command.h -- the commands for the mud, and the structure which holds them
*/

// alphabetical order bitch!
CMD(do_areas);
CMD(do_at);
CMD(do_backup);
CMD(do_ban);
CMD(do_buildtalk);
CMD(do_bug);
CMD(do_buy);
CMD(do_chat);
CMD(do_clear);
CMD(do_close);
CMD(do_coins);
CMD(do_commands);
CMD(do_config);
CMD(do_convert_area);
CMD(do_cough);
CMD(do_credits);
CMD(do_debug);
CMD(do_deposit);
CMD(do_dig);
CMD(do_disconnect);
CMD(do_dismount);
CMD(do_drop);
CMD(do_eat);
CMD(do_echo);
CMD(do_edit);
CMD(do_emote);
CMD(do_equipment);
CMD(do_findvnum);
CMD(do_flex);
CMD(do_force);
CMD(do_give);
CMD(do_goto);
CMD(do_heal);
CMD(do_help);
CMD(do_hunt);
CMD(do_hurt);
CMD(do_idea);
CMD(do_immtalk);
CMD(do_input);
CMD(do_inventory);
CMD(do_lie);
CMD(do_list);
CMD(do_lock);
CMD(do_log);
CMD(do_look);
CMD(do_load);
CMD(do_open);
CMD(do_memory);
CMD(do_move);
CMD(do_mount);
CMD(do_music);
CMD(do_note);
CMD(do_password);
CMD(do_playerpurge);
CMD(do_prompt);
CMD(do_purge);
CMD(do_put);
CMD(do_quit);
CMD(do_reboot);
CMD(do_remove);
CMD(do_repeat);
CMD(do_reply);
CMD(do_resets);
CMD(do_rest);
CMD(do_restore);
CMD(do_retrieve);
CMD(do_sahove);
CMD(do_save);
CMD(do_say);
CMD(do_score);
CMD(do_sell);
CMD(do_shutdown);
CMD(do_slay);
CMD(do_sleep);
CMD(do_snoop);
CMD(do_socials);
CMD(do_stand);
CMD(do_stat);
CMD(do_store);
CMD(do_take);
CMD(do_tell);
CMD(do_test);
CMD(do_time);
CMD(do_title);
CMD(do_top);
CMD(do_transfer);
CMD(do_typo);
CMD(do_unlock);
CMD(do_users);
CMD(do_value);
CMD(do_wake);
CMD(do_wear);
CMD(do_who);
CMD(do_withdraw);
CMD(do_wizhelp);
CMD(do_wizlist);

// OLC THINGS
CMD(do_oedit);
CMD(do_redit);
CMD(do_sedit);
CMD(do_cedit);
CMD(do_xedit);
CMD(do_hedit);

struct command_t
{
	char	*command;
	void	(*function)(CREATURE *crit, char **arguments, OBJECT *obj, CREATURE *xcrit);
	long	level;
	long	log;
	long	position;
	long	state;
	long	arguments;
	long	extra;
};


///////////////////////////////////////////////////////////////////////////////
// COMMAND TABLE //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static const struct command_t command_table[] = {
/*{ "up",		do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "north",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "south",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "east",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "west",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "down",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "northwest",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "northeast",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "southwest",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0},
{ "southeast",	do_move,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	0, 0}, */
{ "look",	do_look,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	0, OBJ_POSSIBLE|CRIT_POSSIBLE|OBJ_GROUND|OBJ_HELD|OBJ_EQUIPMENT},
{ "areas",	do_areas,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "buy",	do_buy,		0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	1, CRIT_POSSIBLE},
{ "bug",	do_bug,		0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	1, 0},
{ "chat",	do_chat,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ ".",		do_chat,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "close",	do_close,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, 0},
{ "clear",	do_clear,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "commands",	do_commands,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "config",	do_config,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "cough",	do_cough,	0,	LOG_NO,	POS_PRONE,	STATE_NORMAL,
	0, 0},
{ "credits",	do_credits,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
/**/{ "debug", do_debug, 0, LOG_NO, 0 },/**/
{ "deposit",	do_deposit,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	2, OBJ_REQUIRED|OBJ_HELD},
{ "dismount",	do_dismount,	0,	LOG_NO,	POS_SITTING,	STATE_FIGHTING,
	0, 0},
{ "drop",	do_drop,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, OBJ_POSSIBLE|OBJ_HELD},
{ "eat",	do_eat,		0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, OBJ_REQUIRED|OBJ_HELD},
{ "edit",	do_edit,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	2, 0},
{ "emote",	do_emote,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, CRIT_POSSIBLE|OBJ_POSSIBLE|OBJ_HELD|OBJ_GROUND},
{ "equipment",	do_equipment,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	0, CRIT_POSSIBLE},
{ "flex",	do_flex,	0,	LOG_NO,	POS_SITTING,	STATE_NORMAL,
	0, CRIT_POSSIBLE},
{ "get",	do_take,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, 0},
{ "give",	do_give,	0,	LOG_NO,	POS_SITTING,	STATE_RESTING,
	2, 0},
{ "help",	do_help,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "hold",	do_wear,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, OBJ_POSSIBLE|OBJ_HELD},
{ "inventory",	do_inventory,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	0, 0},
{ "idea",	do_idea,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	1, 0},
{ "lie",	do_lie,		0,	LOG_NO,	POS_SITTING,	STATE_RESTING,
	0, 0},
{ "list",	do_list,	0,	LOG_NO,	POS_STANDING,	STATE_RESTING,
	0, CRIT_POSSIBLE},
{ "lock",	do_lock,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, 0},
{ "open",	do_open,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, 0},
{ "mount",	do_mount,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, CRIT_REQUIRED},
{ "music",	do_music,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "note",	do_note,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	1, 0},
{ "password",	do_password,	0,	LOG_NEVER, POS_PRONE,	STATE_DEAD,
	3, 0},
{ "prompt",	do_prompt,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "put",	do_put,		0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	2, 0},
{ "quit",	do_quit,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "remove",	do_remove,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, OBJ_POSSIBLE|OBJ_EQUIPMENT},
{ "reply",	do_reply,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	1, 0},
{ "rest",	do_rest,	0,	LOG_NO,	POS_PRONE,	STATE_NORMAL,
	0, 0},
{ "retrieve",	do_retrieve,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	1, 0},
{ "say",	do_say,		0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, 0},
{ "save",	do_save,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "sahove",	do_sahove,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "'",		do_say,		0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, 0},
{ "score",	do_score,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "sell",	do_sell,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	1, OBJ_REQUIRED|OBJ_HELD|CRIT_POSSIBLE},
{ "sit",	do_rest,	0,	LOG_NO,	POS_PRONE,	STATE_NORMAL,
	0, 0},
{ "sleep",	do_sleep,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	0, 0},
{ "socials",	do_socials,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "stand",	do_stand,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	0, 0},
{ "store",	do_store,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	1, OBJ_REQUIRED|OBJ_HELD|CRIT_REQUIRED},
{ "take",	do_take,	0,	LOG_NO,	POS_PRONE,	STATE_RESTING,
	1, OBJ_POSSIBLE|OBJ_GROUND|OBJ_HELD|OBJ_EQUIPMENT},
{ "tell",	do_tell,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	2, CRIT_REQUIRED|CRIT_WORLD|PLAYER_ONLY },
{ "time",	do_time,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "title",	do_title,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	1, 0},
{ "typo",	do_typo,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	1, 0},
{ "unlock",	do_unlock,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, 0},
{ "value",	do_value,	0,	LOG_NO,	POS_SITTING,	STATE_RESTING,
	1, OBJ_REQUIRED|OBJ_HELD|CRIT_POSSIBLE},
{ "wake",	do_wake,	0,	LOG_NO,	POS_PRONE,	STATE_SLEEPING,
	0, 0},
{ "wear",	do_wear,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, OBJ_POSSIBLE|OBJ_HELD},
{ "wield",	do_wear,	0,	LOG_NO,	POS_STANDING,	STATE_FIGHTING,
	1, OBJ_POSSIBLE|OBJ_HELD},
{ "who",	do_who,		0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
{ "withdraw",	do_withdraw,	0,	LOG_NO,	POS_STANDING,	STATE_NORMAL,
	2, CRIT_POSSIBLE},
{ "wizlist",	do_wizlist,	0,	LOG_NO,	POS_PRONE,	STATE_DEAD,
	0, 0},
// IMMORTAL commands

// IMP
{ "backup",	do_backup,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, 0 },
{ "ban",	do_ban,		LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, CRIT_POSSIBLE|CRIT_WORLD|PLAYER_ONLY },
{ "coins",	do_coins,	LEVEL_BUILDER,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, 0},
{ "convert",	do_convert_area, LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, 0},
{ "disconnect",	do_disconnect,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, CRIT_REQUIRED|CRIT_WORLD|PLAYER_ONLY},
{ "heal",	do_heal,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, CRIT_REQUIRED|CRIT_WORLD},
{ "hunt",	do_hunt,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, CRIT_REQUIRED|CRIT_WORLD},
{ "hurt",	do_hurt,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, CRIT_REQUIRED|CRIT_WORLD},
{ "log",	do_log,		LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, CRIT_POSSIBLE|PLAYER_ONLY},
{ "memory",	do_memory,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, 0},
{ "playerpurge",do_playerpurge,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, 0},
{ "reboot",	do_reboot,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, 0},
{ "restore",	do_restore,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, CRIT_POSSIBLE|CRIT_WORLD},
{ "shutdown",	do_shutdown,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, 0},
{ "slay",	do_slay,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, CRIT_REQUIRED},
{ "snoop",	do_snoop,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, CRIT_POSSIBLE|CRIT_WORLD|PLAYER_ONLY},
{ "test",	do_test,	LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, 0},
{ "top",	do_top,		LEVEL_IMP,	LOG_YES,	POS_PRONE, STATE_DEAD,
	0, 0},

// Builder
{ "at",		do_at,		LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	2, CRIT_POSSIBLE|CRIT_WORLD},
{ "buildtalk",	do_buildtalk,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ "dig",	do_dig,		LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},
{ "input",	do_input,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ "load",	do_load,	LEVEL_BUILDER,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, 0},
{ "purge",	do_purge,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, CRIT_POSSIBLE|OBJ_POSSIBLE|OBJ_GROUND},
{ "repeat",	do_repeat,	LEVEL_BUILDER,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, 0},
{ "resets",	do_resets,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ "findvnum",	do_findvnum,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},
// Builder OLC Specific commands
{ "cedit",	do_cedit,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},
{ "hedit",	do_hedit,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},
{ "oedit",	do_oedit,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},
{ "redit",	do_redit,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ "sedit",	do_sedit,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},
{ "xedit",	do_xedit,	LEVEL_BUILDER,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, 0},

// Immortal
{ "echo",	do_echo,	LEVEL_IMM,	LOG_YES,	POS_PRONE, STATE_DEAD,
	1, 0},
{ "force",	do_force,	LEVEL_IMM,	LOG_YES,	POS_PRONE, STATE_DEAD,
	2, CRIT_POSSIBLE|CRIT_WORLD},
{ "goto", 	do_goto,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, CRIT_POSSIBLE|OBJ_POSSIBLE|CRIT_WORLD|OBJ_WORLD|OBJ_GROUND},
{ "immtalk",	do_immtalk,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ ":",		do_immtalk,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ "users",	do_users,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},
{ "stat",	do_stat,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, CRIT_POSSIBLE|OBJ_POSSIBLE|OBJ_HELD},
{ "transfer",	do_transfer,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	1, CRIT_POSSIBLE|OBJ_POSSIBLE|ONE_REQUIRED|CRIT_WORLD|OBJ_WORLD|OBJ_GROUND|OBJ_HELD|OBJ_INOBJ},
{ "wizhelp",	do_wizhelp,	LEVEL_IMM,	LOG_NO,		POS_PRONE, STATE_DEAD,
	0, 0},

{0,0,LEVEL_IMP+1,LOG_NO,POS_PRONE,STATE_DEAD,0,0}

};
