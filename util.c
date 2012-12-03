/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
util.c:
Purpose of this file: Utilities.  Anything that is not exactly
mud related, but helps keep the mud going, add in here.  The
Random Number Generator should go in here.
*/

#include <sys/stat.h>
#include "stdh.h"

#if !defined(WIN32)
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <ctype.h>
#include <math.h>
#include <stdarg.h>


/////////////////////
// LOCAL FUNCTIONS //
/////////////////////
// pauses the mud for # of microseconds.....
// strangely enough.. windows _requires_ you have at least one fd_set
// with at least one socket in it, to use select.. linux does not
void mudsleep(long microseconds)
{
	static unsigned long sleep_socket = 0;
	fd_set fd_sleep;
	struct timeval wait_time;

	if( tick % (LOOPS_PER_SECOND*60*15) == 0 )
	{
		CREATURE *crit;
		OBJECT *obj;
		long players = 0, creatures = 0, objects = 0;

		for(crit = creature_list; crit; crit = crit->next)
		{
			if(IsPlayer(crit))	players++;
			else			creatures++;
		}
		for(obj = object_list; obj; obj = obj->next)
			objects++;

// not a true representation of a tick.. it gets called twice in a row
//		mudlog("Tick %d -- Players:%li  Creatures:%li  Objects:%li",
//			tick,
//			players, creatures, objects);
	}

	if( !sleep_socket ) // initialize
	{
		if( (sleep_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		{
			mudlog("mudsleep: Error creating sleep_socket");
			mud_exit();
			return;
		}
	}

	if( microseconds < 0 )
		microseconds = 0;

	wait_time.tv_usec = microseconds;
	wait_time.tv_sec  = 0;
	FD_ZERO(&fd_sleep);
	FD_SET(sleep_socket,&fd_sleep);

	if(select(0, 0, 0, &fd_sleep, &wait_time) < 0)
	{
#ifdef WIN32
		mudlog("select: error #%d",WSAGetLastError());
#else
		perror("mudsleep: select");
#endif
//		mud_exit();
	}
}


// game logging to std error.. now with sprintf support
void mudlog(const char *str, ...)
{
	MSOCKET *sock;
	char logstr[MAX_BUFFER*2];
	char buf[MAX_BUFFER];
	char imm[MAX_BUFFER];
	va_list ap;
	static long numlogs=0;

	if(++numlogs > MUDLOG_LIMIT)
		return;

	va_start(ap, str);
	(void)vsnprintf(buf, MAX_BUFFER, str, ap);

	sprintf(imm,"&+M[LOG] &+W");
	strcat(imm,buf);
	for(sock = socket_list; sock; sock = sock->next)
	{
		if(sock->pc && (!IsPlaying(sock->pc) || !flag_isset(sock->pc->flags, CFLAG_NOTIFY)))
			continue;
		sendcrit(sock->pc, imm);
	}

	strftime(logstr, MAX_BUFFER, "%m/%d/%Y %H:%M:%S :: ", localtime(&current_time) );
	strcat(logstr,buf);

//	fprintf(stderr, "%s\n", wraptext(23,logstr,100));
	fprintf(stderr, "%s\n", logstr);

	va_end(ap);
}


void backup_mud(void)
{
	FILE *fp;
	char buf[MAX_BUFFER];
	char timebuf[MAX_BUFFER];

	fp = fopen("mysqldump.exe","w");

	strftime(timebuf, MAX_BUFFER, "%b%d-%Y", localtime(&current_time));

	mudlog("BACKING UP SQL DATABASE [%s%s]", DB_NAME, timebuf );
	sprintf(buf,"mysqldump -c -h%s -u%s -p%s %s > %s%s.sql",
		DB_HOST, DB_LOGIN, DB_PASSWORD, DB_NAME, DB_NAME, timebuf );

	// create a file and write the mysqldump command to securely run it,
	// hiding the password from peekers on PS
	fwrite(buf,1,strlen(buf),fp);
	fclose(fp);
	chmod("mysqldump.exe",S_IRUSR|S_IXUSR|S_IWUSR);
	system("./mysqldump.exe");
	system("rm mysqldump.exe");

	sprintf(buf,"tar czhf %s%s.tgz %s%s.sql",
		DB_NAME, timebuf, DB_NAME, timebuf );
	system(buf);
	sprintf(buf,"rm %s%s.sql", DB_NAME, timebuf);
	system(buf);
	sprintf(buf,"mv %s%s.tgz ../backup", DB_NAME, timebuf);
	system(buf);
	mudtime.backup = current_time;
}


// returns the numerical location of str needle
// from the start of the string
long strstrl(char *haystack, char *needle)
{
long x;

if (!strstr(haystack,needle)) // no needle there.
	return -1;

for (x = 0; x < strlen(haystack); x++)
{
	if (needle[0] == haystack[x])
	{
		if (!strcmp(needle,str_cut(x+1,strlen(needle),haystack)))
			return x;
	}
}
return -1;
}	


// this allocates memory for a string... it first checks if the location that
// we are writing to has memory on it already, if it does, it frees it, then
// writes the string to it.  this is to keep memory from building up over time
// .. debugging stuff in it right now
void str_dup(char **destination, char *str)
{
	if(!str)
	{
		mudlog("STRDUP error -- destination=%s",*destination);
		str = "";
	}

	if(ValidString(*destination))
//	if(*destination)
	{
		DeleteObject(*destination)
	}

	*destination	= malloc(sizeof(char) * (strlen(str)+1));
	strcpy(*destination,str);
}


char *noansi(char *buf)
{
	static char newbuf[MAX_BUFFER];
	long x, i;

	newbuf[0] = '\0';
	for(x = 0, i = 0; x < strlen(buf); x++, i++)
	{
		if(buf[x] == '&')
		{
			++x;
			switch(buf[x])
			{
			case '\0':			break;
			case '+':	x += 2;		break;
			case '-':	x += 2;		break;
			case '=':	x += 3;		break;
			case 'n':
			case 'N':	x += 1;		break;
			default:			break;
			}
		}
		newbuf[i] = buf[x];
	}
	newbuf[i] = '\0';

	return newbuf;
}


// such as 2.grape .. will return 2 and replace the argument with "grape"
// if not a numbered arg, it will return 1 meaning first object
long numbered_arg(char *argument)
{
	char realarg[MAX_BUFFER];
	char cnum[8];
	long x, y;

	if(argument && isdigit(argument[0]))
	{
		for(x = 0; argument[x] != '.' && argument[x] != '\0'; x++)
		{
			if(x >= 7)
				return 0;
			cnum[x] = argument[x];
		}
		cnum[x] = '\0';

		if(argument[x] == '.') x++;
		else return 1;

		for(y = 0; argument[x] != '\0'; y++, x++)
			realarg[y] = argument[x];
		realarg[y] = '\0';

		strcpy(argument,realarg);

		return atoi(cnum);
	}
	return 1;
}


char **make_real_arguments(char *buf, char deli)
{
	char temp[MAX_BUFFER];
	char **arguments;
	char *arg=0;
	char *pbuf=0;
	long numargs=0, quotes=0;

	temp[0]		= '\0';
	arguments	= malloc(sizeof(char*));
	arguments[0]	= 0;

	if(!ValidString(buf))
		return arguments;

	while( 1 )
	{
		if(temp[0] == '\0')
			pbuf = temp;

		if (*buf == '\"')
		{
			if (quotes == 0)
				quotes = 1;
			else
				quotes = 0;
			buf++;
		}

		if((*buf == deli && *buf != '\n' && *buf != '\r' && !quotes) || *buf == '\0')
		{
			*pbuf			= '\0';
			arg			= malloc(sizeof(char)*(strlen(temp)+1));
			strcpy(arg, temp);
			temp[0]			= '\0';
			arguments		= realloc(arguments, sizeof(char*)*(numargs==0?2:numargs+2));
			arguments[numargs]	= arg;
			arguments[++numargs]	= 0;

			if(*buf == '\0')
			{
				break;
			}
			else
			{
				buf++;
				continue;
			}
		}
		*pbuf++	= *buf++;
	}
	return arguments;
}

void free_arguments(char **arguments)
{
	long i;
	for(i = 0; arguments[i]; i++)
	{
		DeleteObject(arguments[i]);
	}
	DeleteObject(arguments);
}


// str_add("borlak pip","durf") returns "borlak pip durf"
char *str_add(char *str, char *addition, char delimeter)
{
	static char buf[MAX_BUFFER];
	long x;

	if(strlen(str) + strlen(addition) > sizeof(buf)-1)
	{
		mudlog("str_add: trying to add strings bigger than max_buffer!"
		       "\n\rSTR=%s\n\rADDITION=%s", str, addition);
		return str;
	}

	buf[0] = '\0';
	if(ValidString(str))
	{
		strcpy(buf,str);
		x 		= strlen(buf);
		buf[x]		= delimeter;
		buf[x+1]	= '\0';
	}
	strcat(buf,addition);
	return buf;
}

// str_cut(3,4,"123456789") would return 3456
// if spots + start > strlen it just returns from start to end of str
char *str_cut(long start, long spots, char *str)
{
	char xbuf[MAX_BUFFER];
	static char buf[MAX_BUFFER];
	long x;

        if (start > 0)
		start--; // start position does not have to take into account 0 pos.

	if (strlen(str) < start)
	{		
		mudlog("str_cut: trying to start cutting past the string's end!");
		mudlog("START: %li - SPOTS: %li - STR: %s",start,spots,str);
		return str;
	}

	for (x = start; x < strlen(str) && x - start < spots; x++)
	{
		if (x == start)
		{
			sprintf(xbuf,"%c",str[x]);
			strcpy(buf,xbuf);
		}
		else
		{
			sprintf(xbuf,"%c",str[x]);
			strcat(buf,xbuf);
		}
	}
	return buf;
}


// str_minus("borlak pip durf","pip") returns "borlak durf";
char *str_minus(char *str, char *subtraction, char delimeter)
{
	static char buf[MAX_BUFFER];
	char **arguments = make_arguments_deli(str, delimeter);
	long x, i, found=0;

	buf[0] = '\0';
	for(x = 0; arguments[x]; x++)
	{
		if(!found && !strcasecmp(arguments[x],subtraction))
		{
			found = 1;
			continue;
		}

		strcat(buf,arguments[x]);
		i		= strlen(buf);
		buf[i]		= delimeter;
		buf[i+1]	= '\0';
	}
	buf[strlen(buf)-1] = '\0'; // chop off last delimeter
	free_arguments(arguments);
	return buf;
}


// compare one list to another, ie: "1 2 3" to "4 5 2" would return the matching
// word, or 0 if not found
char *strlistcmp(char *str1, char *str2)
{
	static char buf[MAX_BUFFER];
	char **arguments1 = make_arguments(str1);
	char **arguments2 = make_arguments(str2);
	long x, y;

	buf[0] = '\0';

	for(x = 0; arguments1[x]; x++)
		for(y = 0; arguments2[y]; y++)
			if(!strcasecmp(arguments1[x], arguments2[y]))
			{
				strcpy(buf,arguments1[x]);
				break;
			}
	free_arguments(arguments1);
	free_arguments(arguments2);
	return buf[0] == '\0' ? 0 : buf;
}


char *capitalize(char *str)
{
	static char buf[MAX_BUFFER];

	sprintf(buf,"%s",str);
	buf[0] = Upper(buf[0]);
	return buf;
}


// wraptext!  A good way to format.
// cutoff is the max amount of characters you want
// on a line, and spaces is how many spaces you want
// the next line indented.  - pip.
char *wraptext(long spaces, char *str, long cutoff)
{
	static char tempbuf[MAX_BUFFER];
	char spacebuf[MAX_BUFFER];
	char *p, *k;
	long x = 0;
	long i = 1;

	tempbuf[0] = '\0';
	spacebuf[0] = '\0';
	p = tempbuf;

	// fill in spaces before new line starts.
	strcat(spacebuf,"\n\r"); // give it a new character.
	for (x = 0; x < spaces; x++)
	{
		if (spaces != 0)
			strcat(spacebuf," ");
	}

	for (x = 0; x < strlen(str); x++)
	{
		if (str[x] == ' ')
		{
			if (x > i * cutoff)
			{
				k = spacebuf;
				while(*k != '\0')
					*p++ = *k++;

				// get rid of spaces at beginning
				// of next line

				while (str[x+1] == ' ')
					x++;
				i++;
				continue;
			}
			else
			{
				*p++ = str[x];
			}
		}
		else
		{
			*p++ = str[x];
		}
	}
	*p = '\0';
	return tempbuf;
}



// is a string a number?
long is_number(char *str)
{
	if(*str == '-')
		str++;

	for( ; *str != '\0'; str++ )
	{
		if(!isdigit(*str))
		{
			if(*str == '.')
			{
				str++;
				if(!isdigit(*str))
					return 0;
				continue;
			}
			return 0;
		}
	}
	return 1;
}


long strindex(char *haystack, char *needle)
{
	while(Lower(*haystack) == Lower(*needle))
	{
		haystack++, needle++;
		if(*needle == '\0')
			return 0;
	}
	return 1;
}


// find a word in a list of words of a single string
// ie.  borlak will be found in "borlak pip durf sock"
// abbreviations are allowed. bor will work in same list
// you may turn off abbreviations..
// returns 0 on success, finds str in compare
long strargcmp(char *haystack, char *needle)
{
	char **arguments = make_arguments(haystack);
	long x;

	for(x = 0; arguments[x]; x++)
	{
		if(!strcasecmp(arguments[x], needle))
		{
			free_arguments(arguments);
			return 0;
		}
	}
	free_arguments(arguments);
	return 1;
}
// same as above but abbreviations are allowed
long strargindex(char *haystack, char *needle)
{
	char **arguments = make_arguments(haystack);
	long x;

	for(x = 0; arguments[x]; x++)
	{
		if(!strindex(arguments[x], needle))
		{
			free_arguments(arguments);
			return 0;
		}
	}
	free_arguments(arguments);
	return 1;
}

//findinset  - This searches str1 for str2, str1 is a set of strings seperated by a delim(str3)
long findinset(char *haystack, char *needle, char delim)
{
	char **arguments = make_real_arguments(haystack,delim);
	long x;

	for (x = 0; arguments[x]; x++)
	{
		if (!strcmp(arguments[x], needle))
		{
			free_arguments(arguments);
			return 0;
		}
	}
	free_arguments(arguments);
	return 1;
}

//indexinset - same as findinset, but abbreviations are allowed.
long indexinset(char *haystack, char *needle, char delim)
{
	char **arguments = make_real_arguments(haystack,delim);
	long x;

	for (x = 0; arguments[x]; x++)
	{
		if (!strindex(arguments[x], needle))
		{
			free_arguments(arguments);
			return 0;
		}
	}
	free_arguments(arguments);
	return 1;
}

// count the number of words in a list.
long countlist(char *haystack, char *needle)
{
	char **arguments = make_arguments(haystack);
	long x, y = 0;
	
	for( x = 0; arguments[x]; x++)
	{
		if (!strcasecmp(arguments[x], needle))
			y++;
	}
	free_arguments(arguments);
	return y;
}


double wind_chill(long wind_speed, long temp)
{
#ifdef WIN32 // the linux computer i'm on doesn't have math.h!! heh
	if (4 > wind_speed)
		return (double)temp;
	else
	{
		return (((10.45 + (6.686112 * sqrt((double) wind_speed))
			- (.447041 * wind_speed)) / 22.034 * (temp - 91.4)) + 91.4);
	}
#endif
	return temp;
}


/////////////////////////////////
// The random number generator //
/////////////////////////////////

// get some /urandom goodness!
unsigned long randseed(void)
{
	FILE *fp;
	unsigned long num;
	char buf[11];
	long x;
	        
	if(!(fp = fopen("/dev/urandom","r")))
	{
		mudlog("can't open /dev/urandom!");
		return 0;
	}
	fread(buf, 11, 1, fp);
	for(x = 0; x < 12; x++)
		num += buf[x];
	for(x = 0; x < 12; x++)
		num *= Max(1,buf[x]);
	num = abs(num);
	fclose(fp);
	return num;
}

static unsigned long s1=390451501, s2=613566701, s3=858993401;  // The seeds
static unsigned mask1, mask2, mask3;
static long shft1, shft2, shft3, k1=31, k2=29, k3=28;

// use either of the following two sets of parameters
static long q1=13, q2=2, q3=3, p1=12, p2=4, p3=17;
// static long q1=3, q2=2, q3=13, p1=20, p2=16, p3=7;


// INITIALIZATION STUFF
unsigned long randgen(void)
{
	static unsigned long b;

	b  = ((s1 << q1)^s1) >> shft1;
	s1 = ((s1 & mask1) << p1) ^ b;
	b  = ((s2 << q2) ^ s2) >> shft2;
	s2 = ((s2 & mask2) << p2) ^ b;
	b  = ((s3 << q3) ^ s3) >> shft3;
	s3 = ((s3 & mask3) << p3) ^ b;

	return (s1 ^ s2 ^ s3);
}


void rand_seed (unsigned long a, unsigned long b, unsigned long c)
{
	static unsigned long x = 4294967295U;

	shft1=k1-p1;
	shft2=k2-p2;
	shft3=k3-p3;
	mask1 = x << (32-k1);
	mask2 = x << (32-k2);
	mask3 = x << (32-k3);
	if (a > (unsigned int)(1<<shft1)) s1 = a;
	if (b > (unsigned int)(1<<shft2)) s2 = b;
	if (c > (unsigned int)(1<<shft3)) s3 = c;

	randgen();
}


void init_rand()
{
//	rand_seed(current_time-s1, current_time, current_time+s3);
//	rand_seed(s1, s2, s3);
	rand_seed(randseed(),randseed(),randseed());
	mudlog("Random number generator initialized.");
}


///////////////////
// MUD FUNCTIONS //
///////////////////
long randnum(long start, long end)
{
	static long num;

	if(start<0 || end<0)
		return randneg(start,end);

	do
	{
		num = randgen() % (end+1);
	} while( num < start || num > end );

	return num;
}

// same as randnum, but allows negative numbers.
long randneg(long start, long end)
{
	long fromzero = 0;
	long neg=0;
	long test=0;

	if (start < 0 && end < 0)
		neg = 1;
	else if (start < 0)
		neg = 2;
		
	if (neg == 2)
	{
		fromzero = 0 - start;
		test = randnum(start+fromzero,end+fromzero);
	}
	else if (neg == 1)
		test = randnum(start*-1,end*-1);
	else
		test = randnum(start,end);

	return neg > 0 ? neg > 1 ? test - fromzero : test*-1 : test;
}


long dice(long howmany, long type)
{
	static long i, total;

	for( i = 0, total = 0; i < howmany; i++ )
		total += randnum(1,type);
	return total;
}


long percent(void)
{
	return randnum(1,100);
}

