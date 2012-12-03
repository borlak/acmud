#include "stdh.h"

typedef struct search_t SEARCH;
typedef struct path_t PATH;
typedef struct map_t MAP;

long shortest = 100;
long total = 0;
long goal = -1;
long start = -1;

SEARCH *search_list=0;
SEARCH *search_free=0;	

PATH *path_list=0;
PATH *path_free=0;

struct map_t
{
	long x;
	long y;
	char k;
	char color;
};

MAP map[256][256];

struct path_t
{
	PATH *next;
	PATH *prev;

	EXIT *startexit; // this is the starting exit for the path.

	long open; //1 = open 2 = closed 3 = found target
	long distance;
	long startvnum;

	long mapx;
	long mapy;
};

struct search_t
{
	SEARCH	*next;
	SEARCH	*prev;

	long	distance; // distance from start
	long	room; // room vnum
};


///// borlak -- im bored
void draw_map(CREATURE *crit, long distance)
{
	char buf[MAX_BUFFER*2];
	long x,y;
	long drew=0;

mudlog("DISTANCE IS %li",distance);
	buf[0] = '\0';

	for(y = 0; y < distance; y++)
	{
		for(x = 0; x < distance; x++)
		{
			if(map[x][y].color != 'n')
			{
				drew = 1;
				sprintf(buf+strlen(buf),"&+%c%c&N", map[x][y].color, map[x][y].k);
			}
			else
				sprintf(buf+strlen(buf),"%c", map[x][y].k);
		}
		if(!drew)
			buf[strlen(buf)-distance] = '\0';
		else
			sprintf(buf+strlen(buf),"\n\r");
	}

	sendcrit(crit,buf);
}


long curcolor=0;
char choose_color(void)
{
	switch(curcolor % 10)
	{
	case 0: return 'r';
	case 1: return 'b';
	case 2: return 'g';
	case 3: return 'm';
	case 4: return 'c';
	case 5: return 'R';
	case 6: return 'G';
	case 7: return 'B';
	case 8: return 'M';
	case 9: return 'C';
	}
	return 'n';
}

void modmap(PATH *path, EXIT *exit)
{
	if(!strcasecmp(exit->name,"north"))
	{
		path->mapy--;
		map[path->mapx][path->mapy].k = '|';
		map[path->mapx][path->mapy].color = choose_color();
		path->mapy--;
		map[path->mapx][path->mapy].k = 'x';
		map[path->mapx][path->mapy].color = choose_color();
	}
	else if(!strcasecmp(exit->name,"east"))
	{
		path->mapx++;
		map[path->mapx][path->mapy].k = '-';
		map[path->mapx][path->mapy].color = choose_color();
		path->mapx++;
		map[path->mapx][path->mapy].k = 'x';
		map[path->mapx][path->mapy].color = choose_color();
	}
	else if(!strcasecmp(exit->name,"south"))
	{
		path->mapy++;
		map[path->mapx][path->mapy].k = '|';
		map[path->mapx][path->mapy].color = choose_color();
		path->mapy++;
		map[path->mapx][path->mapy].k = 'x';
		map[path->mapx][path->mapy].color = choose_color();
	}
	else if(!strcasecmp(exit->name,"west"))
	{
		path->mapx--;
		map[path->mapx][path->mapy].k = '-';
		map[path->mapx][path->mapy].color = choose_color();
		path->mapx--;
		map[path->mapx][path->mapy].k = 'x';
		map[path->mapx][path->mapy].color = choose_color();
	}
}


// this picks the exit and creates paths as needed
EXIT *pick_exit(ROOM *room, PATH *current, long depth)
{
	SEARCH *search; 
	EXIT *exit = 0;
	PATH *path = 0;
	PATH *dummy;
	EXIT *picked = 0;
	long skip = 0;

	if (!room) // for exits that don't lead anywhere
	{
		current->open = 2;
		return 0;
	}

	if (room->vnum == goal)
		return 0;

	for (exit = room->exits; exit; exit = exit->next)
	{
		if (exit->to_vnum == goal)
		{
			current->open = 3;
			map[current->mapx][current->mapy].k = 'F';
			map[current->mapx][current->mapy].color = 'W';
			return exit;
		}

		if (start == exit->to_vnum)
			continue;			

		for (search = search_list; search; search = search->next)
		{
			skip = 0;
			if (search->room == exit->to_vnum)
			{
				if (search->distance <= (current->distance + 1))
				{
					skip = 1;	
					break;
				}
				else
					break;
			}
		}

		if (skip)
			continue;

		// now to pick the exits if they made it through.	
	 	if (!picked) // current path already picked mark all new exits with new paths
		{
			NewObject(path_free, path)
			AddToListEnd(path_list, path, dummy)
			path->distance = current->distance + 1;
			path->startvnum = exit->to_vnum;
			path->mapx = current->mapx;
			path->mapy = current->mapy;
			modmap(path, exit);

			if (!current->startexit)
				path->startexit = picked;
			else
				path->startexit = current->startexit;
			total++;
			path->open = 1;
		}
		else // continue on current path and add to route.
			picked = exit;
	}
	// close path and move on.
	if (!picked)
		current->open = 2;
	return picked;
}


// depth = max # of rooms to search through from starting point
char *find_path(void *fromobj, void *toobj, long depth)
{
	static char buf[MAX_BUFFER];
	CREATURE *crit=0;
	OBJECT *obj=0;
	SEARCH *search=0;
	SEARCH *searchc=0;
	ROOM *from=0;
	ROOM *to=0;
	ROOM *room=0;
	PATH *path=0;
	PATH *pathc=0;
	PATH *pdummy=0;
	EXIT *picked = 0;
	long x,y;

	long roomdepth = 21; //

	if(roomdepth % 2 == 0) // keept roomdepth odd to make the map drawing easier (codewise)
		roomdepth++;

//	map = malloc(sizeof(MAP)*(roomdepth*2*2+2));
//	memset(map,0,sizeof(MAP)*(roomdepth*2*2+2));
	for(x = 0; x < roomdepth*2*2+2; x++)
	for(y = 0; y < roomdepth*2*2+2; y++)
	{
		map[x][y].k = ' ';
		map[x][y].color = 'n';
	}

	buf[0] = '\0';

	switch(*(unsigned int*)fromobj)
	{
	case TYPE_CREATURE:
		crit = (CREATURE*)fromobj;
		from = ((CREATURE*)fromobj)->in_room;
		break;
	case TYPE_OBJECT:
		obj = (OBJECT*)fromobj;
		if(obj->in_obj)
			obj = obj->in_obj;
		if(obj->held_by)
			from = obj->held_by->in_room;
		else	from = obj->in_room;
		break;
	case TYPE_ROOM:
		from = (ROOM*)fromobj;
		break;
	}
	switch(*(unsigned int*)toobj)
	{
	case TYPE_CREATURE:
		to = ((CREATURE*)toobj)->in_room;
		break;
	case TYPE_OBJECT:
		obj = (OBJECT*)toobj;
		if(obj->in_obj)
			obj = obj->in_obj;
		if(obj->held_by)
			to = obj->held_by->in_room;
		else	to = obj->in_room;
		break;
	case TYPE_ROOM:
		to = (ROOM*)toobj;
		break;
	}

	if(from == to)
	{
		strcpy(buf,"right here");
		return buf;
	}

	// FIX see if from and to already have a shortest path in MySQL

	goal = to->vnum;
	start = from->vnum;

	// add first path
	NewObject(path_free, path)
	AddToListEnd(path_list, path, pdummy)
	path->distance = 0;
	path->startvnum = from->vnum;
	path->open = 1;
	path->startexit = 0;
	path->mapx = roomdepth;
	path->mapy = roomdepth;
	map[roomdepth][roomdepth].k = 'S';
	total++;

	// add first search
	NewObject(search_free, search)
	AddToList(search_list, search)
	search->distance = 0;
	search->room = from->vnum;

	// traverse paths
	for (pathc = path_list; pathc; pathc = pathc->next)
	{
		if (total >= depth)
			break;
		if (pathc->open != 1)
			continue;
		if (pathc->startvnum == from->vnum)
			room = from;
		else
			room = hashfind_room(pathc->startvnum);

		sprintf(buf,"{%li}: (%s)",pathc->startvnum,pathc->startexit->name);

		while ((picked = pick_exit(room,pathc,depth)) != 0)
		{	
			if (!pathc->startexit)
				pathc->startexit = picked;

			if (room == to)
			{
				pathc->open = 2; // this is for paths started from goal room
				break;
			}

			strcat(buf,picked->name);
			strcat(buf," ");

			if (!(room = hashfind_room(picked->to_vnum)))
			{
				pathc->open = 2;
				curcolor++;
				break;
			}
			pathc->distance++;

			if (room == to)
			{
				if (shortest > pathc->distance)
					shortest = pathc->distance;
				pathc->open = 3;
				break;
			}

			if (pathc->distance >= shortest || pathc->distance > roomdepth)
			{
				pathc->open = 2;
			}	

			if (pathc->open != 1)
			{
				break;
			}

			for (searchc = search_list; searchc; searchc = searchc->next)
			{
				if (searchc->room == room->vnum) // found room on list already
				{
					if (searchc->distance <= pathc->distance)
						break;
					else
					{
						searchc->distance = pathc->distance;
						break;
					}
				}
			}

			if (!searchc)
			{
				NewObject(search_free, searchc)
				AddToList(search_list, searchc)
				searchc->distance = pathc->distance;
				searchc->room = room->vnum;
			}
		}
	}

	for (path = path_list; path; path = path->next)
	{
		if ((path->open == 3) && (path->distance == shortest))
			break;
	}

	if (path)
		sprintf(buf,"shortest distance from %li to %li is %li\n\r[%s]",
			from->vnum, to->vnum, shortest, 
			path->startexit->name);
	else
		sprintf(buf,"NOT FOUND :(");

	while((search = search_list))
	{	
		RemoveFromList(search_list, search)
		AddToList(search_free, search)
	}

	while((path = path_list))
	{
		RemoveFromList(path_list, path)
		AddToList(path_free, path)
	}

	shortest = 100; // reset shortest path
	total = 0;

	if(crit)
	{
		map[roomdepth][roomdepth].k = 'S';
		map[roomdepth][roomdepth].color = 'W';
		draw_map(crit, roomdepth*2*2+2);
	}

	return buf[0] == '\0' ? "Nowhere" : buf;
}


	




