/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
config.h:
Purpose of this file:  Variables that will need to be changed by the user
depending on their setup
*/

//////////////////////////////
// MySQL Database variables //
//////////////////////////////
#define DB_HOST		"localhost"
#define DB_NAME		"acm"
#define DB_LOGIN	"acm"
#define DB_PASSWORD	"acm"
#define BACKUP_HOURS	72		// how many hours until you backup the db?

////////////////////////////////////////////////////
// How do you want time to work in your universe? //
////////////////////////////////////////////////////
#define SECONDS_MINUTES		1	// how many RL seconds make up a mud-minute
#define MINUTES_HOURS		60	// how many mud minutes in a mud hour
#define HOURS_DAYS		24	// how many mud hours in a mud day
#define STARTING_YEAR		1	// starting year of mud
// see info.c to configure how many days in a month, months in a year
#define SECONDS_DAY		(SECONDS_MINUTES*MINUTES_HOURS*HOURS_DAYS)
#define NEW_TIME_LOGS		0	// do you want a new time log to be made each time the mud boots up?
					// this may be useful for statistical purposes (longest mud has been
					// up, etc.)

///////////////////
// Miscellaneous //
///////////////////
#define SOCKET_RECONNECT	30	// how long before a player can connect to the mud again (seconds)
#define SOCKET_TRIES		2	// how many tries do they get before getting forced to wait for reconnect
#define BAN_MESSAGE		"You have been banned from playing here."
#define MUDLOG_LIMIT		660000	// how many mudlogs until it stops logging (fixes huge logs
					// due to inf.loops) reboot will reset this
