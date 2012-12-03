/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
main.c:
Purpose of this file:  Starts the mud!  Holds the loop in which
the mud goes on forever.  Also calls the function to check for
socket connections
*/
#include "stdh.h"
#include "io.h"
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>


/////////////////////
// LOCAL VARIABLES //
/////////////////////
time_t current_time;
extern unsigned long host;
long mud_shutdown	= 0;
long port		= 6666;
long log_all		= 0;
unsigned long tick	= 0;
MYSQL *mysql		= NULL;			// THE mysql connection of the mud

/////////////////////
// LOCAL FUNCTIONS //
/////////////////////
// catches process signals and deals with them.. checks for
// infinite looping/hanging as well.
void signal_handler(int type)
{
	static struct timeval last_signal_time;
	struct timeval time;

	get_time(&time);

	switch(type)
	{
	case SIGINT:
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
#ifndef WIN32
	case SIGTRAP:
#endif
		mudlog("Mud received bad signal(%d), aborting...",type);
		mud_exit();
		abort();
		break;
#ifndef WIN32
	case SIGVTALRM:
		// check if the mud is looping
		// this type of signal only comes every 5 seconds of _process_ usage, not real
		// time.. so if the process is using the CPU constantly for 5 seconds, we
		// assume it's looping
		if( last_signal_time.tv_sec > 0 && (time.tv_sec-last_signal_time.tv_sec) < 10 )
		{
			mudlog("Mud in infinite loop, aborting!");
			abort();
		}
		break;
#endif
	}
	get_time(&last_signal_time);
}


// game boot up stuff
void init_mud(long hotreboot)
{
	char buf[MAX_BUFFER];
	MSOCKET *socket;
	extern CREATURE *hash_creature[HASH_KEY];
	extern OBJECT *hash_object[HASH_KEY];
	extern ROOM *hash_room[HASH_KEY];
	extern char **connect_attempts;
	extern void backup_update();
#ifndef WIN32
	struct itimerval itime;
	struct timeval time;
#endif
	extern char * const months[];
	extern char * const days[];
	long x;

	for(x = 0; months[x]; x++) ;
	mudtime.months = x;
	for(x = 0; days[x]; x++) ;
	mudtime.days = x;

	// init hash tables
	memset(hash_creature,	0,sizeof(hash_creature));
	memset(hash_object,	0,sizeof(hash_object));
	memset(hash_room,	0,sizeof(hash_room));
	memset(hash_reset,	0,sizeof(hash_reset));

	// set up process signals to catch and deal with
	signal(SIGINT,		signal_handler);
	signal(SIGILL,		signal_handler);
	signal(SIGFPE,		signal_handler);

	signal(SIGSEGV,		signal_handler);
	signal(SIGPIPE,		SIG_IGN);	// ignore pipes! tx to unix socket faq for help
						// with this.  http://http://www.developerweb.net/forum/
#ifndef WIN32
	signal(SIGTRAP,		signal_handler);
	signal(SIGVTALRM,	signal_handler);

	// this code is to check for infinite looping
	time.tv_sec		= 5;
	time.tv_usec		= 0;
	itime.it_interval	= time;
	itime.it_value		= time;
	if( setitimer(ITIMER_VIRTUAL, &itime, 0) < 0 )
		mudlog("Error starting setitimer.");
#endif

	// initialize the random number generator
	init_rand();

	// connect_attempts array, to keep people from spam connecting to slow down the mud
        connect_attempts        = malloc(sizeof(char*));
        connect_attempts[0]     = 0;

	mysql = mysql_init(NULL);       
       if (!mysql)
       {
               mudlog("Error initializing MySQL: %s", mysql_error(mysql));
               abort();
       }

        // connect to MySQL database
       if (!mysql_real_connect(mysql, DB_HOST, DB_LOGIN, DB_PASSWORD, DB_NAME, 0, NULL, 0))
        {
               mudlog("Error opening mysql database: %s", mysql_error(mysql));
                abort();
        }

        if (mysql_select_db(mysql, DB_NAME))
        {
               mudlog("Error opening main database: %s",mysql_error(mysql));
               if (mysql)
                       mysql_close(mysql);
                abort();
        }

	// time of da mud !
	fread_time();

	// load areas/rooms/objects/creatures
	load_db();

	// check if hotreboot and reconnect everything
	if(hotreboot)
	{
		MSOCKET *socket=0;
		char buf[MAX_BUFFER];
		MYSQL_RES *result;
		MYSQL_ROW row;

		mysql_query(mysql, "SELECT * FROM player WHERE online='1'");
		result = mysql_store_result(mysql);

		if (mysql_num_rows(result) < 0)
		{
			mudlog("hotreboot: mysql error is: %s",mysql_error(mysql));
			exit(1);
		}

		while((row = mysql_fetch_row(result)))
		{
			strcpy(buf, row[P_NAME]);

			// check for "host" player.. holds control descriptor
			if(!strcasecmp("host", row[C_NAME]))
			{
				host = atoi(row[P_DESCRIPTOR]);
			}
			else
			{
				socket = init_socket();
				fread_player(socket, buf);

				str_dup(&socket->host,		row[P_HOST]);
				str_dup(&socket->ip,		row[P_IP]);

				socket->desc = atoi(row[P_DESCRIPTOR]);
				socket->connected = CON_PLAYING;

				trans(socket->pc, socket->pc->in_room);

				sendcrit(socket->pc, "Hotreboot completed.");
			}
		}
		mysql_free_result(result);
	}

	// make sure nobody has non-existant connections, in the event of a crash + hotreboot
	// then go through and update people who are online
	// this is also a good time to get host address, as players will be expecting lag
	mysql_query(mysql, "UPDATE player SET online='0', descriptor='0'");

	for(socket = socket_list; socket; socket = socket->next)
	{
		get_hostname(socket);
		sprintf(buf,"UPDATE player SET online='1', host=\"%s\" WHERE id='%li'",
			smash_quotes(socket->host), socket->id);
		mysql_query(mysql, buf);
	}

	// see how many players are in each area, to see if area should be 'alive'
	for(socket = socket_list; socket; socket = socket->next)
	{
		if(IsPlaying(socket->pc))
			socket->pc->in_room->area->players++;
	}

	// check if we need to backup
	backup_update();
}


// MAIN - you can check out any time you like, but you can never leave
int main(int argc, char **argv)
{
	struct timeval time;
	time_t time_slept=0;
	time_t processing_time=0, total_processing_time=0;
	long hotreboot = 0;

	if( argc > 1 )
	{
		if( !is_number(argv[1]) )
		{
			mudlog("Invalid port number!");
			exit(1);
		}
		else if( (port = atoi(argv[1])) <= 1024 )
		{
			mudlog("Port must be above 1024!");
			exit(1);
		}
		if(argc > 2 && !strcmp(argv[2], "hotreboot"))
			hotreboot = 1;
	}

// test stuff
	{
	}


	get_time(&time);
	current_time = time.tv_sec;

	if(!hotreboot)
		create_host(port);
	init_mud(hotreboot);

	while(!mud_shutdown)
	{
		// we want the mud to loop no more than (LOOPS) times a second.. but not to loop
		// (LOOPS) times in 1/10th of a second and then wait for 9/10ths... try to make it even
		if( tick % LOOPS_PER_SECOND == 0 )
		{
			if( time_slept > 1000000 )
			{
				mudlog("time_slept is over a second (%li) and total processing_time this tick (%li)", 
					time_slept, total_processing_time);
//				exit(1);
			}
			else
				mudsleep(1000000 - time_slept);

			time_slept = 0;
			total_processing_time = 0;
		}

		get_time( &time );
		processing_time = time.tv_usec;

		// do all the mud stuff here....
		check_connections();
		mud_update();
		// end mud stuff

		get_time( &time );
		if(time.tv_usec > processing_time)
			processing_time = time.tv_usec - processing_time;
		else
			processing_time = processing_time - time.tv_usec;
		total_processing_time += processing_time;
		current_time = time.tv_sec;
		time_slept += (1000000 / LOOPS_PER_SECOND)-(processing_time);
		tick++;
		mudsleep((1000000 / LOOPS_PER_SECOND)-(processing_time));
	}

	close(host);
	free_mud();
	mudlog("Mud terminated normally.");
	return 1;
}


// exit...
void mud_exit(void)
{
#ifdef WIN32
	WSACleanup( );
#endif
	mud_shutdown = 1;
}


