
void reset_editor(CREATURE *crit, char **arguments, RESET *reset)
{
	RESET *list;
	OlcInit()

	OlcInt("chance",	reset->chance,		1,100)
	OlcInt("max",		reset->max,		-1,1000)
	OlcInt("maxperm",	reset->maxperm,		-1,1000)
	OlcInt("time",		reset->time,		-1,10000)
	OlcValuesStruct("type",	reset->type,		reset_types)

	// check if this is a sub reset -- if it is, only allow obj type
	if(reset->prev_content)
		reset->type = TYPE_OBJECT;

	switch(reset->type)
	{
	case TYPE_CREATURE:
	OlcValues("position",	reset->position,	position_table)
	OlcValues("state",	reset->state,		state_table)
	break;
	case TYPE_OBJECT:
	OlcInt("dropchance",	reset->chancedrop,	-1,100)
	case TYPE_EXIT:
	OlcStr("direction",	reset->command,		1, 24)
	default: break;
	}

	sendcritf(crit,"[%s&N]",
		reset->prev_content ? "&+RSUB-RESET" : "&+BMAIN RESET",
		critp->loaded);
	OlcSendInit()
	sendcritf("    Type:      %s",
		reset->type == TYPE_CREATURE ? "&+BCreature&N" :
		reset->type == TYPE_OBJECT ? "&+GObject&N" : "&+RExit&N" );

	OlcSend("Vnum:        %li",	reset->vnum		);
	OlcSend("Loads:       %s",	get_reset_name(reset)	);
	OlcSend("Chance Load: %li",	reset->chance		);
	OlcSend("Max#:        %li",	reset->max		);
	OlcSend("Max Perm#:   %li",	reset->maxperm		);
	OlcSend("Time:        %li",	reset->time		);

	switch(reset->type)
	{
	case TYPE_CREATURE:
	OlcSend("Position:    %s",	position_table[reset->position]);
	OlcSend("State:       %s",	state_table[reset->state]);
	break;
	case TYPE_OBJECT:
	OlcSend("Drop Chance: %li",	reset->chancedrop	);
	break;
	case TYPE_EXIT:
	OlcSend("Direction:   %s",	reset->command		);
	break;
	}

	// go back to start of list to show a brief summary of this reset and it's subs
	sendcrit(crit, "All resets in this room:");
	for(list = reset; list->prev_content; list = list->prev_content)
		;

	for( ; list; list = list->next_content)
	sendcritf("%s[%s] Loads[%s] Chance[%li] Time[%li]",
		list == reset ? "&+YEditing-->&N" : "",
		list->prev_content ? "Sub-Reset" : "Main Reset",
		get_reset_name(list),
		list->chance, list->time );

	send_to(crit->socket,"\n\rOption: ");
}