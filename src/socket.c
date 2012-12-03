/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
socket.c:
Purpose of this file:  Holds the socket code for the mud
which connects players and finds their addresses.
*/

#include "stdh.h"
#include "ansi.h"
#include "io.h"
#include <errno.h>
#if !defined(WIN32)
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

//////////////////
// DECLARATIONS //
//////////////////
long banned(MSOCKET *socket);
long check_name(char *argument);
long get_address(MSOCKET *socket);
long non_block(long fd);
void prompt(CREATURE *crit, bool snoop);


/////////////////////
// LOCAL VARIABLES //
/////////////////////
MSOCKET		*socket_list;
MSOCKET		*socket_free;
unsigned long	host = 0;
fd_set		fd_read;
fd_set		fd_write;
fd_set		fd_exc;
//fd_set		fd_hold;
char		**connect_attempts;

char *welcome_message =
"\n\r\n\rACM (A C Mud)\n\r\n\r"
"\n\r\n\r\n\r\n\r\n\r\n\r\n\r";

///////////////
// FUNCTIONS //
///////////////
void reset_socket(MSOCKET *sock)
{
	sock->id		= 0; // reset for mysql saving

	sock->inbuf[0]		= '\0';
	sock->last_command[0]	= '\0';
	sock->outbuf[0]		= '\0';
	sock->connected		= CON_GET_NAME;
	sock->pc		= 0;
	sock->editing		= 0;
	sock->stringbuf		= 0;
	sock->string		= 0;
	sock->variable		= 0;
	sock->lines		= 24;
	sock->pause		= 0;
	sock->repeat		= 0;
	sock->doprompt		= 1;

	str_dup(&sock->host,		"");
	str_dup(&sock->ip,		"");
	str_dup(&sock->last_ip,		"");
	str_dup(&sock->password,	"");
	str_dup(&sock->prompt,		"");
	str_dup(&sock->title,		"");
	str_dup(&sock->who_name,	"");
	str_dup(&sock->reply,		"");
	str_dup(&sock->snoop,		"");
}


// initiate a socket
MSOCKET *init_socket()
{
	MSOCKET *sock, *dummy;
	struct sockaddr_in *sockad = malloc(sizeof(struct sockaddr_in));

	memset(sockad, 0, sizeof(struct sockaddr_in));

	NewObject(socket_free,sock)
 	AddToListEnd(socket_list,sock,dummy)

	reset_socket(sock);
	sock->pc		= new_creature(1);
	sock->pc->socket	= sock;
	sock->pc->level		= 0;
	sock->sockad		= sockad;

	flag_set(sock->pc->flags, CFLAG_PLAYER);

	return sock;
}


long new_socket(void)
{
	MSOCKET		*sock = 0;
	unsigned int	desc = sizeof(struct sockaddr_in);
	unsigned int	len = sizeof(struct sockaddr_in);

	sock = init_socket();

	getsockname(host, (struct sockaddr*)sock->sockad, &desc);

	non_block(desc);

	if((desc = accept(host, (struct sockaddr*)sock->sockad, &len)) < 0)
	{
		perror("new_socket: accept");
		free_socket(sock);
		return 0;
	}

#ifndef WIN32
	if(fcntl(desc, F_SETFL, FNDELAY) == -1)
	{
		free_socket(sock);
		return 0;
	}
#endif
	sock->desc	= desc;

	if(!get_address(sock))
	{
		free_socket(sock);
		return 0;
	}

	if(banned(sock))
	{
		free_socket(sock);
		return 0;
	}

	send_to(sock, welcome_message);
	send_to(sock, "What is your name? ");

	return 1;
}


void free_socket(MSOCKET *sock)
{
	char buf[MAX_BUFFER];

	if(sock->desc > 0)
		close(sock->desc);
	sock->desc = -1;
	RemoveFromList(socket_list,sock)

	mudlog("%s@%s(%s) disconnected.",
		sock->pc ? sock->pc->name : "Someone",
		ValidString(sock->host) ? sock->host : "Unknown",
		sock->ip);

	if(sock->pc)
	{
		if (ValidString(sock->password))
			fwrite_player(sock->pc);
		if(IsEditing(sock->pc))
		{
			save_editing(sock->pc);
			stop_editing(sock->pc,1);
		}
		if(sock->connected == CON_PLAYING)
		{
			message("$n loses $s link.",sock->pc,sock->pc->in_room,0);
			sprintf(buf,"UPDATE player SET online='0' WHERE id='%li'",sock->id);
			mysql_query(mysql,buf);
			sock->connected = CON_LINKDEAD;
			return; // don't lose socket
		}
		else //if (sock->connected != CON_LINKDEAD)
			free_creature(sock->pc); 

		sock->pc->socket = 0;
	}

	DeleteObject(sock->host)
	DeleteObject(sock->ip)
	DeleteObject(sock->last_ip)
	DeleteObject(sock->password)
	DeleteObject(sock->prompt)
	DeleteObject(sock->title)
	DeleteObject(sock->who_name)
	DeleteObject(sock->snoop)

	free(sock->sockad);

	memset(sock, 0, sizeof(MSOCKET));
	AddToList(socket_free,sock)
}


// write a string to a socket!
void send_to(MSOCKET *sock, char *message)
{
	if(strlen(message) + strlen(sock->outbuf) >= sizeof(sock->outbuf))
			return;

	if(strlen(message) > sizeof(sock->outbuf))
		message[sizeof(sock->outbuf)-1] = '\0';

	strcat(sock->outbuf,message);
}

// write a string to a socket, not waiting for next mud cycle
void send_immediately(MSOCKET *sock, char *message)
{
	send_to(sock, message);
	process_output(sock,0,0);
}


// ip address of socket
long get_address(MSOCKET *socket)
{
	MSOCKET *sock = (MSOCKET*)socket;
	char buf[MAX_BUFFER];
	char *ip;
	long addr;
	long x;
	long tries=0;
	unsigned int size = sizeof(struct sockaddr_in);
	struct sockaddr_in *sockad = sock->sockad;

	getpeername(sock->desc, (struct sockaddr*)sockad, &size);

	addr = ntohl(sockad->sin_addr.s_addr);
	sprintf( buf, "%li.%li.%li.%li",
		(addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
		(addr >> 8) & 0xFF, (addr) & 0xFF );
	str_dup(&sock->ip,buf);

	// connection overload handling (people connecting really fast to lag mud)
	ip = malloc(sizeof(char) * (strlen(sock->ip)+1));
	strcpy(ip, sock->ip);

	for(x = 0; connect_attempts[x]; x++)
	{
		if(!strcmp(connect_attempts[x],ip))
		{
			if(++tries >= SOCKET_TRIES)
			{
				send_immediately(sock,"Too many login attempts, wait a minute and try again.");
				mudlog("Socket denied from %s", sock->ip);
				free(ip);
				return 0;
			}
		}
	}

	connect_attempts	= realloc(connect_attempts, sizeof(char*) * (x==0?2:x+2));
	connect_attempts[x]	= ip;
	connect_attempts[x+1]	= 0;

	mudlog("Socket connected from %s", sock->ip);
	return 1;
}


void get_hostname(MSOCKET *sock)
{
	struct hostent *from;
	struct sockaddr_in *sockad = sock->sockad;

	from = gethostbyaddr((char*)&sockad->sin_addr,sizeof(sockad->sin_addr), AF_INET);

	if(from)
		str_dup(&sock->host,from->h_name);
}

// the traditional "nanny" takes care of initial input from a socket,
// logs them in...
void nanny(MSOCKET *sock, char *argument)
{
	char buf[MAX_BUFFER];
	CREATURE *xcrit;
	long i=0;

	if(!ValidString(argument) && sock->connected != CON_MENU)
		return;

	switch(sock->connected)
	{
	case CON_CONNECTING:
		break;
	case CON_GET_NAME:

		argument[0] = Upper(argument[0]);

		if( !check_name(argument) )
		{
			send_immediately(sock,"Illegal name, try another: ");
			break;
		}

		if( (i=fread_player(sock,argument)) )
		{
			if(i==2) // a deleted player, not removed from player table
			{
				send_immediately(sock,"That name is not available, try another: ");
				break;
			}
			if(banned(sock))
			{
				free_socket(sock);
				break;
			}
			send_immediately(sock,"Welcome back, what is your password? ");
			sock->connected = CON_CHECK_PASSWORD;
			break;
		}

		str_dup(&sock->pc->name, argument);
		sprintf(buf,"You want your name to be '%s'? ",argument);
		send_immediately(sock,buf);
		sock->connected = CON_CONFIRM_NAME;
		break;
	case CON_CONFIRM_NAME:
		switch(argument[0])
		{
		case 'y': case 'Y':
			send_immediately(sock,"What password? ");
			sock->connected = CON_GET_PASSWORD;
			break;
		default:
			send_immediately(sock,"What is your name then? ");
			sock->connected = CON_GET_NAME;
			break;
		}
		break;
	case CON_GET_PASSWORD:
		str_dup(&sock->password, crypt(argument,sock->pc->name));

		send_immediately(sock,"Retype password: ");
		sock->connected = CON_CONFIRM_PASSWORD;
		break;
	case CON_CONFIRM_PASSWORD:
		if(strcmp(crypt(argument,sock->password), sock->password))
		{
			send_immediately(sock, "Passwords don't match, try again: ");
			sock->connected = CON_GET_PASSWORD;
		}

		// defaults for new players
		str_dup(&sock->prompt,"Type [&+Whelp prompt&n] for prompt info> ");		// set default prompt
		flag_set(sock->pc->flags,CFLAG_BLANK);						// turn blank on 
		flag_set(sock->pc->flags,CFLAG_ANSI);  						// set ansi on
		sock->lines = 40;								// 40 lines default
		flag_set(sock->pc->flags,CFLAG_LOG);	// TEMPORARY set logging on


		fwrite_player(sock->pc);
		sock->connected = CON_MENU;
		nanny(sock,"");
		break;
	case CON_CHECK_PASSWORD:
		if(sock->password[0] != '\0' && strcmp(crypt(argument,sock->pc->name), sock->password))
		{
			send_immediately(sock,"Wrong password...");
			free_socket(sock);
			break;
		}
		// first off... does the player exist already?
        	// disconnect the old connection if they do.
		// then check for linkdead, then for nomenu.
		for(xcrit = creature_list; xcrit; xcrit = xcrit->next)
		{
                	if(xcrit->id == sock->pc->id && xcrit->socket != sock)
			{
				if (xcrit->socket->connected != CON_LINKDEAD)
				{
					send_immediately(xcrit->socket,"SOMEONE HAS TAKEN CONTROL OF YOUR CHARACTER.  (hopefully you)\n\r");
					send_to(sock,"Previous connection closed... usurping player.\n\r");
				}
				else
				{
					sock->connected = CON_LINKDEAD;
					send_to(sock,"You steal back your unused body.\n\r");
				}
				xcrit->socket->pc = 0;
				free_socket(xcrit->socket);
				xcrit->socket = sock;
				free_creature(sock->pc);
				sock->pc = xcrit;
				break;
			}
		}
		if (sock->connected == CON_LINKDEAD)
		{
			sock->connected = CON_PLAYING;
			message("$n $v reconnected.",sock->pc,0,0);
			interpret(sock->pc,"look");
			break;
		}
		else if(!xcrit && !flag_isset(sock->pc->flags, CFLAG_NOMENU))
		{
			sock->connected = CON_MENU;
			nanny(sock,"");
			break;
		}
		else
		{
			sock->connected = CON_PLAYING;
			trans(sock->pc, sock->pc->in_room);
			interpret(sock->pc,"help motd");
			sock->pc->in_room->area->players++;
			message_all("&+W&*Notice->&N$p has entered the game.",sock->pc,sock->pc->name,1);
			interpret(sock->pc,"look");
			nanny(sock,"");
			break;
		}
	case CON_MENU:
		switch(argument[0])
		{
		case '1':
			sock->connected = CON_PLAYING;
			trans(sock->pc, sock->pc->in_room);
			interpret(sock->pc,"help motd");
			sock->pc->in_room->area->players++;
			message_all("&+W&*Notice->&N$p has entered the game.",sock->pc,sock->pc->name,1);
			interpret(sock->pc,"look");
			break;
		case '2':
		send_to(sock,"I have a tale to tell\n\rSometimes it gets so hard to hide it well\n\rI was not ready for the fall\n\rToo blind to see the writing on the wall\n\r\n\rChorus:\n\r\n\rA man can tell a thousand lies\n\rI've learned my lesson well\n\rHope I live to tell\n\rThe secret I have learned, 'till then\n\rIt will burn inside of me\n\r\n\r\n\rI know where beauty lives\n\rI've seen it once, I know the warm she gives\n\rThe light that you could never see\n\rIt shines inside, you can't take that from me\n\r\n\r(chorus)\n\r\n\r2nd Chorus:\n\r\n\rThe truth is never far behind\n\rYou kept it hidden well\n\rIf I live to tell\n\rThe secret I knew then\n\rWill I ever have the chance again\n\r\n\rIf I ran away, I'd never have the strength\n\rTo go very far\n\rHow would they hear the beating of my heart\n\rWill it grow cold\n\rThe secret that I hide, will I grow old\n\rHow will they hear\n\rWhen will they learn\n\rHow will they know\n\r\n\r(chorus)\n\r(2nd chorus)\n\r\n\r");
		send_to(sock,"Hit [enter] to continue.\n\r\n\r");
		sock->connected = CON_MENU;
		break;
		case '3':
			free_socket(sock);
			break;
// FIX TODO TEMPORARY OMG!
case '4':
if (sock->pc->level < LEVEL_BUILDER)
	sock->pc->level = LEVEL_BUILDER;
send_to(sock,"You can build stuff now.  HELP OLC\n\r");
break;
		default:
		sprintf(buf,		"-= ACM =-\n\r");
		sprintf(buf+strlen(buf),"1) Enter the game.\n\r");
		sprintf(buf+strlen(buf),"2) Print Madonna lyrics.\n\r");
		sprintf(buf+strlen(buf),"3) Quit\n\r");
		sprintf(buf+strlen(buf),"4) Make yourself a builder!\n\r");
		sprintf(buf+strlen(buf),"\n\r:: ");
		send_to(sock,buf);
		break;
		}
		break;
	default:
		break;
	}
}


// create the listening socket for people to connect to
long create_host(long port)
{
	struct sockaddr_in	sa;
	int		reuse = 1;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 1, 1 );

	if( (WSAStartup(wVersionRequested, &wsaData)) != 0 )
	{
		mudlog("error starting winsock");
		mud_exit();
	}

	if ( LOBYTE( wsaData.wVersion ) != 1 ||
	     HIBYTE( wsaData.wVersion ) != 1 )
	{
		mudlog("No suitable winsock DLL found, aborting.");
		mud_exit();
		return 0;
	}
#endif

	if((host = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("create_host: failed to create host socket");
		mud_exit();
		return 0;
	}

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family		= AF_INET;
	sa.sin_port		= htons((short)port);
	sa.sin_addr.s_addr	= htonl(INADDR_ANY);

	if( setsockopt(host, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(reuse)) < 0 )
		mudlog("create_host: can't set SO_REUSEADDR");
	if( setsockopt(host, SOL_SOCKET, SO_KEEPALIVE, (void*)&reuse, sizeof(reuse)) < 0 )
		mudlog("create_host: can't set SO_KEEPALIVE");

	if(bind(host, (struct sockaddr*)&sa, sizeof(sa)) < 0)
	{
		perror("create_host: bind failed");
		mud_exit();
		exit(1);
		return 0;
	}

	if( non_block(host) < 0 )
		mudlog("create_host: can't set NONBLOCKING");

	if(listen(host, 5) < 0)
	{
		perror("create_host: listen failed");
		mud_exit();
		return 0;
	}

	mudlog("Host socket listening on port %li, file descriptor %li.",port,host);
	return 1;
}


// this function connects new sockets and calls the functions to process all commands,
// as well as kick out sockets with errors or that are quitting the game.
// windows requires a timeval with a value of {0,0} to be used in select if you want
// it to return immediately.. linux does not..
long check_connections(void)
{
	MSOCKET		*sock;
	MSOCKET		*sock_next;
	unsigned long 	high = host;
	struct timeval	null_time;
	long		x=0;

	FD_ZERO(&fd_read);
	FD_ZERO(&fd_write);
	FD_ZERO(&fd_exc);
//	FD_ZERO(&fd_hold);
	FD_SET(host, &fd_read);
	null_time.tv_sec = 0;
	null_time.tv_usec = 0;

	for(sock = socket_list; sock; sock = sock->next)
	{
		if(sock->desc > high)
			high = sock->desc;
		if (!sock->desc)	
			break;

		FD_SET(sock->desc, &fd_read);
		FD_SET(sock->desc, &fd_write);
		FD_SET(sock->desc, &fd_exc);
	}

	if(select(high+1, &fd_read, &fd_write, &fd_exc, &null_time) < 0)
		perror("check_connections: select");

 	if(FD_ISSET(host, &fd_read))
		new_socket();

	// clear connect_attempts every 30 seconds
	if(current_time % SOCKET_RECONNECT == 0)
	{
		for(x = 0; connect_attempts[x]; x++)
			free(connect_attempts[x]);
		free(connect_attempts);
		connect_attempts	= malloc(sizeof(char*));
		connect_attempts[0]	= 0;
	}

	for(sock = socket_list; sock; sock = sock_next)
   	{
		sock_next = sock->next;

		if(sock->connected != CON_PLAYING
		&& ++sock->pause == LOOPS_PER_SECOND*30)
		{
			send_to(sock,"You have been idle for too long.\n\r");
			free_socket(sock);
			continue;
		}

		if(sock->repeat > 0)
			--sock->repeat;

		if (sock->doprompt != 2)
			sock->doprompt = 0;

		if(FD_ISSET(sock->desc, &fd_exc))
		{
			free_socket(sock);
			continue;
		}

		if(FD_ISSET(sock->desc, &fd_write))
		{
			if(!process_output(sock,1,0))
			{
				free_socket(sock);
				continue;
			}
		}

		if(FD_ISSET(sock->desc, &fd_read))
		{
			if(!process_input(sock))
			{
				free_socket(sock);
				continue;
			}
		}

		if(sock->doprompt  && sock->pc)
			prompt(sock->pc,0);
	}
	return 1;
}


// write data to a socket
long process_output(MSOCKET *sock, bool input, bool snoop)
{
	char mainbuf[MAX_BUFFER+25];
	char *buf = sock->outbuf;
	unsigned long total = 0;
	long i = 0;
	long block;
	long j, k;
	long lines = 0;
	bool bg, bold, blink, flag = 0;
	char prev;

  	if(sock->outbuf[0] == '\0' || (IsPlaying(sock->pc) && sock->pause))
	{
		return 1;
	}

	if( sock->pc )
	{
		CREATURE *crit=0;
		char snoopbuf[MAX_BUFFER];
		long x=0;	
		char **snoopers;
		flag = flag_isset(sock->pc->flags, CFLAG_ANSI);

	        if (sock->snoop && ValidString(sock->snoop))
		{
			// show snoopers the output first!
			snoopers = make_arguments(sock->snoop);
			sprintf(snoopbuf,"\n\r&+W%%%s>&N ",sock->pc->name);
			for (x = 0; snoopers[x]; x++)
			{
				crit = find_crit(sock->pc, snoopers[x], CRIT_WORLD|PLAYER_ONLY);
				if (crit)
				{
					send_to(crit->socket,snoopbuf);
					send_to(crit->socket,sock->outbuf);
					send_to(crit->socket,"&+W%%&N");
					process_output(crit->socket,1,1);
				}
			}
			free_arguments(snoopers);
		}
	}

	if(sock->stringbuf)
		buf = sock->stringbuf;
	else
		buf = sock->outbuf;

	sock->stringbuf	= 0;
	mainbuf[0]	= '\0';
	j		= 0;

	if(input && !sock->string)
	{
		strcat(mainbuf,"\n\r");
		j += 2;
	}

	for( ; *buf != '\0' && (j < (MAX_BUFFER-1)) && !sock->stringbuf; buf++ )
	{
		if( *buf == '&' )
		{
			buf++;
			if( *buf == '\0' )
				continue;

			switch(*buf)
			{
			case '&':
				mainbuf[j++] = '&';
				break;
			case '*':
				sprintf(&mainbuf[j], "%c", 007 );
				j += 1;
				break;
			case 'N':
			case 'n':
				if(flag)
				{
					sprintf(&mainbuf[j], "\033[0m");
					j += 4;
				}
				break;

			case '+':
			case '-':
				bg = (*buf == '-');
				buf++;
				if(*buf == '\0')
					continue;

				bold = bg ? 0 : (isupper(*buf)) ? 1 : 0;
				blink = !bg ? 0 : (isupper(*buf)) ? 1 : 0;
				k = find_color_entry(*buf);
				if( color_table[k].symbol != 0 )
				{
					if( flag )
					{
						sprintf(&mainbuf[j], "\033[%s%s%sm",
							bold ? "1;" : "",
							blink ? "5;" : "",
							(bg ? color_table[k].bg_code : color_table[k].fg_code));
						j += (5 + (bold ? 2 : 0) + (blink ? 2 : 0));
					}
				}
				else
				{
					sprintf(&mainbuf[j], "&%c%c",
						(bg ? '-' : '+'),
						*buf );
					j += 3;

				}
				break;

			case '=':
				buf++;
				if( *buf == '\0' )
					continue;

				blink = (isupper(*buf) ? 1 : 0);
				bg = find_color_entry(*buf);
				prev = *buf;
				buf++;
				if( *buf == '\0' )
					continue;

				bold = (isupper(*buf) ? 1 : 0);
				k = find_color_entry(*buf);
				if( (color_table[k].symbol != 0)
				&&  (color_table[bg].symbol != 0) )
				{
					if( flag )
					{
						sprintf(&mainbuf[j], "\033[%s%s%s;%sm",
							bold ? "1;" : "",
							blink ? "5;" : "",
							color_table[bg].bg_code,
							color_table[k].fg_code );
						j += (8 + (bold ? 2 : 0) + (blink ? 2 : 0));
					}
				}
				else
				{
					sprintf(&mainbuf[j], "&=%c%c",
						prev,
						*buf );
					j += 4;
				}
				break;

			default:
				sprintf(&mainbuf[j], "&%c", *buf );
				j += 2;
				break;
			}
		}
		else if(*buf == '\n')
		{
			if( flag )
			{
				sprintf(&mainbuf[j], "\033[0m");
				j += 4;
			}
			mainbuf[j++] = '\n';
		}
		else if(*buf == '\r' && !IsEditing(sock->pc) && !snoop)
		{
			if(++lines == sock->lines)
			{
				char *cont = "[Press Enter to continue]";
				mainbuf[j++] = *buf;
				sock->stringbuf = (buf+1);

				if(strlen(cont) + j >= MAX_BUFFER-2)
					sprintf(&mainbuf[(MAX_BUFFER-2)-strlen(cont)], "%s", cont);
				else
					sprintf(&mainbuf[j], "%s", cont);

				j += strlen(cont);
				sock->pause = 1;
				sock->doprompt = 2;
			}
			else
			{
				mainbuf[j++] = *buf;
			}
		}
		else
		{
			mainbuf[j++] = *buf;
		}
	}

	mainbuf[j] = '\0';

	while(total < strlen(mainbuf))
	{
		block = strlen(mainbuf) - total;
#ifndef WIN32
		if((i = write(sock->desc, &mainbuf[total], block)) < 0)
			return 0;
#else
//		if((i = send(sock->desc, &mainbuf[total], block,0)) < 0)
		if((i = write(sock->desc, &mainbuf[total], block)) < 0)
			return 0;
#endif
		total += i;
	}

	if(!sock->stringbuf)
		sock->outbuf[0] = '\0';
	mainbuf[0]	= '\0';
	if (sock->doprompt < 1)
		sock->doprompt = 1;

	return 1;
}


// read data from a socket
long process_input(MSOCKET *sock)
{
	char buf[MAX_BUFFER];
	char checkrecv[MAX_BUFFER];
	char snoopbuf[MAX_BUFFER];
	unsigned long len = 0;
	unsigned long inbuf_len = 0;
	unsigned long result;
	char **snoopers;
	CREATURE *crit=0;
	long x=0;
	long recvlen=0;
	while(1)
	{
		checkrecv[0] = '\0';
		inbuf_len = strlen(sock->inbuf);
		len = recv(sock->desc, checkrecv, sizeof(checkrecv) - 1, MSG_PEEK|MSG_NOSIGNAL);
		if (errno && len < 0)
		{
			mudlog("Errno: %d - %s",errno,strerror(errno));
			if (errno == EWOULDBLOCK || errno == EINTR)
				return 1;
			return 0;
		}
		else if (len == 0)
		{
			mudlog("%s disconnected on process_input.",sock->pc->name);
			return 0;
		}
		else if (len < 0)
		{
			mudlog("Error encountered on recv");
			return 0;
		}
		checkrecv[len] = '\0';
//mudlog("%d, strstr for \\n|\\r: %li|%li",strlen(checkrecv),strstrl(checkrecv,"\n"),strstrl(checkrecv,"\r"));
//mudlog("\"%s\" - %d, strstr for \\n|\\r: %li|%li",checkrecv,strlen(checkrecv),strstrl(checkrecv,"\n"),strstrl(checkrecv,"\r"));
//		recvlen = Max(strstrl(checkrecv,"\n"),strstrl(checkrecv,"\r"));
		recvlen = strstrl(checkrecv,"\n");
//mudlog("recvlen: %li",recvlen);
		if (recvlen < 0)
			return 1;  // not ready to process command
if (sock->pc && flag_isset(sock->pc->flags,CFLAG_LOG))
		mudlog("%li: %d-%d",strlen(checkrecv),strlen(checkrecv) > 1 ? checkrecv[strlen(checkrecv)-2] : -1,checkrecv[strlen(checkrecv)-1]);
		len = recv(sock->desc, sock->inbuf + inbuf_len, recvlen + 1, 0);
		
		if(len >= 1)
		{
			sock->inbuf[inbuf_len+len] = '\0';
			if(sock->inbuf[inbuf_len+len-1] == '\n' 
			|| sock->inbuf[inbuf_len+len-1] == '\r')
			{

				if(sock->doprompt < 1)
					sock->doprompt = 1;

				sock->command_ready = 1;
 				break;
			}
			break;
		}
		else if (errno) 
		{
			mudlog("Errno: %d - %s",errno,strerror(errno));
			if (errno == EWOULDBLOCK || errno == EINTR)
				return 1;
			return 0;
		}
		else if (len == 0)
		{
			mudlog("%s disconnected on process_input.",sock->pc->name);
			return 0;
		}
		else
		{
			mudlog("Error encountered on recv");
			return 0;
		}
	}
	if( IsPlaying(sock->pc) && sock->pause)
	{
		sock->inbuf[0] = '\0';
		sock->command_ready = 0;
	}
	else if( sock->command_ready)
	{
		if(ValidString(sock->inbuf))
			sock->repeat += 3;
		if(!IsEditing(sock->pc) && sock->repeat >= 40)
		{
			send_to(sock,"* *  *   *    *     * S P A M *     *    *   *  * *\n\r");
			mudlog("%s has been kicked for spamming.",sock->pc ? sock->pc->name : "someone");
			return 0;
		}

		if(sock->inbuf[0] != '!')
		{
			// strip control/esc codes
			for(result=0, len=0; result < strlen(sock->inbuf); result++)
			{
				if(iscntrl(sock->inbuf[result]) && sock->inbuf[result] != '\0')
					continue;

				buf[len++] = sock->inbuf[result];
			}
			buf[len] = '\0';
			strcpy(sock->inbuf, buf);

			if(IsPlaying(sock->pc)) // have to check this, or else will save pw
				strcpy(sock->last_command, buf);
		}
		else
			strcpy(sock->inbuf, sock->last_command);
		if (sock->snoop && ValidString(sock->snoop))
		{
			// show snoopers the output first!
			snoopers = make_arguments(sock->snoop);
			sprintf(snoopbuf,"\n\r%%%s> ",sock->pc->name);
			for (x = 0; snoopers[x]; x++)
			{
				crit = find_crit(sock->pc, snoopers[x], CRIT_WORLD|PLAYER_ONLY);
				if (crit)
				{
					send_to(crit->socket,snoopbuf);
					send_to(crit->socket,sock->inbuf);
					send_to(crit->socket,"\n\r");
				}
			}
			free_arguments(snoopers);
		}

		if( sock->connected == CON_PLAYING )
			interpret(sock->pc, sock->inbuf);
		else
			nanny(sock,sock->inbuf);

		sock->inbuf[0] = '\0';
		sock->command_ready = 0;
	}

	sock->pause	= 0;

	return 1;
}


long non_block(long fd)
{
#ifndef WIN32
	return fcntl(fd, F_SETFL, O_NONBLOCK);
#else
	unsigned long i = 1;
	return ioctlsocket(fd, FIONBIO, &i);
#endif
}


long banned(MSOCKET *sock)
{
	BAN *ban;
	char buf[MAX_BUFFER];

	for(ban = ban_list; ban; ban = ban->next)
	{
		if((ban->bantype == BAN_IP && !strindex(ban->ip, sock->ip))
		|| (ban->bantype == BAN_PLAYER && !strcasecmp(sock->pc->name,ban->name)) )
		{
			sprintf(buf,"%s:\n\r%s\n\r",
				BAN_MESSAGE, ban->message);
			send_immediately(sock,buf);
			return 1;
		}
	}
	return 0;
}


// make sure a name is legal for the mud!
long check_name(char *argument)
{
	extern CREATURE *hash_creature[HASH_KEY];
	CREATURE *crit;
	long i = 0;

	if(!ValidString(argument))
		return 0;

	if(strstr(argument, "it the host imm immortal we you him her he she someone god who") != 0)
		return 0;

	if(strlen(argument) > 12 || strlen(argument) < 2)
		return 0;

	for( i = 0; i < (int)strlen(argument); i++ )
	{
		if( !isalpha(argument[i]) )
			return 0;
	}

	for(i = 0; i < HASH_KEY; i++)
	{
		for(crit = hash_creature[i%HASH_KEY]; crit; crit = crit->next_hash)
		{
			if(strlistcmp(argument, crit->keywords) != 0)
				return 0;
		}
	}

	return 1;
}


void prompt(CREATURE *crit, bool snoop)
{
	char buf[MAX_BUFFER];
	char temp[MAX_BUFFER];
	char *pbuf=buf;
	char *tbuf=temp;
	char *prompt;

	if(!IsPlaying(crit) || crit->socket->string
	|| crit->socket->stringbuf || crit->socket->pause)
		return;

	if (crit->socket->doprompt > 1)
	{	
		crit->socket->doprompt = 1;
		return;
	}

	buf[0] = '\0';
	prompt = crit->socket->prompt;

	for(; *prompt != '\0'; prompt++)
	{
		switch(*prompt)
		{
		case '%':
			prompt++;
			switch(*prompt)
			{
			case 'a':
			case 'A':
				sprintf(temp,"%s", crit->in_room->area->name);
				break;
			case 'b':
			case 'B':
				if (crit->level >= LEVEL_BUILDER)
					sprintf(temp,"%s", crit->in_room->area->builders);
				break;
			case 'e':
				sprintf(temp,"%s",exit_names(crit->in_room,0));
				break;
			case 'E':
				if (crit->level >= LEVEL_BUILDER)
					sprintf(temp,"%s",exit_names(crit->in_room,1));
				break;
			case 'h':
				sprintf(temp,"%li",crit->hp);
				break;
			case 'H':
				sprintf(temp,"%li",crit->max_hp);
				break;
			case 'i':
			case 'I':
				sprintf(temp,"%s",
					flag_isset(crit->flags,CFLAG_WIZINVIS) ? "&+W(I)&N" : "");
				break;
			case 'm':
				sprintf(temp,"%li",crit->move);
				break;
			case 'M':
				sprintf(temp,"%li",crit->max_move);
				break;
			case 'n':
			case 'N':
				sprintf(temp,"\n\r");
				break;
			case 'r':
				if(crit->level >= LEVEL_BUILDER)
					sprintf(temp,"%li",crit->in_room->vnum);
				else
					sprintf(temp,"%s",crit->in_room->name);				
				break;
			case 'R':
				sprintf(temp,"%s",crit->in_room->name);
				break;
			case 's':
				if(crit->level >= LEVEL_BUILDER)
					sprintf(temp,"%s",roomtype_table[crit->in_room->roomtype]);
				break;
			default:
				temp[0] = *prompt;
				temp[1] = '\0';
				break;
			}
			tbuf = temp;

			while(*tbuf != '\0')
				*pbuf++ = *tbuf++;
			break;
		default:
			*pbuf++ = *prompt;
			break;
		}
	}
	*pbuf = '\0';
	temp[0] = '\0';
	if (flag_isset(crit->flags, CFLAG_BLANK) && !snoop)
		sprintf(temp,"\n%s",buf);
	else
		sprintf(temp,"%s",buf);

	if(IsEditing(crit))
		sprintf(temp+strlen(temp),"&+Y(editor)&N ");

	if(crit->state > STATE_NORMAL)
		sprintf(temp+strlen(temp),"&+Y(%s)&N ",state_table[crit->state]);
	if(crit->position > POS_STANDING)
		sprintf(temp+strlen(temp),"&+Y(%s)&N ",position_table[crit->position]);

	send_immediately(crit->socket, temp);
}
