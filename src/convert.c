/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
convert.c -- converts old merc areas to this mud
*/

#include <ctype.h>
#include "stdh.h"


// converter of old merc areas..
CMD(do_convert_area)
{
	extern long total_creatures, total_objects, total_rooms,
		total_exits, total_resets, total_shops, total_extras;
	static long multi=0;
	OBJECT *objp=0;
	CREATURE *critp=0;
	EXTRA *extra=0;
	RESET *reset=0;
	RESET *rlast=0;
	ROOM *room=0;
	AREA *area=0, *tarea=0;
	EXIT *exit=0;
	SHOP *shop=0;
	FILE *fp;
	char *file;
	char buf[MAX_BUFFER];
	char abuf[MAX_BUFFER];
	char *str;
	char *p;
	char k;
	long b;
	long x;
	long y;
	long z;
	long done = 0;
	long num;
	long i, high=0, low=40000;
	long rooms=0, mobs=0, exits=0, resets=0, objs=0, shops=0, extras=0;
	long val0=0, val1=0, val2=0, val3=0;
	long theend=0;

	if( !arguments[0]
	|| (strcmp(arguments[0],"dir") && (fp = fopen(arguments[0],"r")) == 0)
	|| (!strcmp(arguments[0],"dir") && !arguments[1]))
	{
		sendcrit(crit,"Syntax: convert <[path]areaname>");
		sendcrit(crit,"Syntax: convert dir <[path]directory>");
		sendcrit(crit,"Using a directory will attempt to convert all .are files.");
		return;
	}

	if(!strcmp(arguments[0],"dir"))
	{
		sprintf(buf,"ls %s",arguments[1]);
		if((fp = popen(buf,"r")) == NULL)
			return;

		multi = 1;
		while ((file = fread_line(fp)) != (char*)EOF)
		{
			if(strstr(file,".are") == 0)
				continue;

			sprintf(buf,"convert %s/%s",arguments[1],file);
			interpret(crit,buf);
		}
		pclose(fp);
		update_resets();
		multi = 0;
		return;
	}

	area = new_area(arguments[0]);
	while(!theend)
	{
		if( (str = fread_line(fp)) == (char*)EOF)
			break;

		if( (p=strstr(str,"#AREA")) != 0 )
		{
			p += 5;
			i = 0;
			buf[0] = '\0';
			while( isspace(*p) )
				p++;
			while( *p != '~' )
				buf[i++] = *p++;
			buf[i] = '\0';
			for (z = 0; buf[z]; z++)
			{
				if (buf[z] == '}')
					break;
			}
			if (buf[z])
			{
				z+=2;
				// extrapolate area name & builders
				for (x = z; buf[x]; x++)
				{
					if (isspace(buf[x]))
						break;
					abuf[x-z] = buf[x];
				}
				abuf[x-z] = '\0';
				str_dup(&area->builders,abuf);
				for (y = 0 ; buf[x]; x++, y++)
				{
					if (isspace(buf[x]) && !done)
					{
						y--;
						continue;
					}
					done = 1;
					abuf[y] = buf[x];
				}
				abuf[y] = '\0';
			}
			else
				sprintf(abuf,"%s",buf);
			str_dup(&area->name,abuf);
			mudlog("Converting area %s...",area->name);
			continue;
		}

		if( !strcmp(str,"#MOBILES") )
		{
			while(1)
			{
				str = fread_line(fp);
				if( str[0] == '#' ) // vnum
				{
					mobs++;
					for( i = 1; i < (int)strlen(str); i++ )
						buf[i-1] = str[i];
					buf[i-1] = '\0';
					i = atoi(buf);

					if( i == 0 )
						break;

					// test to make sure area wont conflict
					for(tarea = area_list; tarea; tarea = tarea->next)
					{
						if(tarea == area) continue;

						if(tarea->low <= i && tarea->high >= i)
							break;
					}
					if(tarea)
					{
						mudlog("BAD AREA (%s) has conflicting vnums with area [%s]",
							area->name, tarea->name);
						free_area(area);
						area = 0;
						theend = 1;
						break;
					}

					if( i < low )
						low = i;
					if( i > high )
						high = i;

					critp = new_creature_proto(i);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&critp->keywords,buf);

					// add mount flag to these keyworded mobs.
					// add more as you feel necessary.
					if (strlistcmp("camel horse bull warg donkey",critp->keywords))
						flag_set(critp->flags,CFLAG_MOUNT);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&critp->name,buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&critp->long_descr,buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
						strcat(buf," ");
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&critp->description,wraptext(0,buf,70));

					critp->dexterity	= 50;
					critp->intelligence	= 50;
					critp->strength		= 50;

								fread_number(fp); // act
								fread_number(fp); // affected_by
					critp->alignment =	fread_number(fp); // alignment
								fscanf(fp,"%c",&k); // ?
								fscanf(fp,"%c",&k); // ?
								fread_number(fp); // level
								fread_number(fp); // hitroll
								fread_number(fp); // ac
					critp->hp =		fread_number(fp); // number of dice
								fscanf(fp,"%c",&k);
					num =			fread_number(fp); // size of dice

					if(critp->hp <= 0 || num <= 0)
						critp->max_hp =	randnum(25,200);
					else
						critp->max_hp =	dice(critp->hp, num);
								fscanf(fp,"%c",&k);
					critp->max_hp +=	fread_number(fp); // + hp
					critp->max_move =	critp->max_hp;

								fread_number(fp); // number of dice
								fscanf(fp,"%c",&k);
								fread_number(fp); // size of dice
								fscanf(fp,"%c",&k);
								fread_number(fp); // + damage
								fread_number(fp); // gold
								fread_number(fp); // ?
								fread_number(fp); // ?
								fread_number(fp); // ?
					critp->sex =		fread_number(fp);

					total_creatures++;
				}
			}
			continue;
		}

		if( !strcmp(str,"#OBJECTS") )
		{
			while(1)
			{
				str = fread_line(fp);
				if( str[0] == '#' ) // vnum
				{
					objs++;
					for( i = 1; i < (int)strlen(str); i++ )
						buf[i-1] = str[i];
					buf[i-1] = '\0';
					i = atoi(buf);

					if( i == 0 )
						break;

					if( i < low )
						low = i;
					if( i > high )
						high = i;

					objp = new_object_proto(i);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&objp->keywords,buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&objp->name,buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&objp->long_descr,buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
						strcat(buf," ");
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&objp->description,wraptext(0,buf,70));

					switch(fread_number(fp))
					{
					case 18: objp->objtype	= OBJ_KEY;	break;
					case 1:  objp->objtype	= OBJ_LIGHT;	break;
					case 3:  
					case 4:  
					case 5:  objp->objtype = OBJ_WEAPON;	break;
					case 9:  objp->objtype = OBJ_ARMOR;	break;
					case 10: 
					case 17: 
					case 25: objp->objtype = OBJ_DRINK;	break;
					case 26: 
					case 19: objp->objtype = OBJ_FOOD;	break;
					case 23: 
					case 24: 
					case 15: objp->objtype = OBJ_CONTAINER;	break;
					case 20: objp->objtype = OBJ_COIN;	break;
					default: objp->objtype = OBJ_TREASURE;	break;
					}

					num	= fread_number(fp); // extra_flags
					if(IsSet(num, 1))	flag_set(objp->flags, OFLAG_GLOW);
					if(IsSet(num, 2))	flag_set(objp->flags, OFLAG_HUM);

					num	= fread_number(fp); // wear flags
					buf[0]	= '\0';
					if(IsSet(num, 2))	strcat(buf,"finger ");
					if(IsSet(num, 4))	strcat(buf,"neck ");
					if(IsSet(num, 2048))	strcat(buf,"waist ");
					if(IsSet(num, 64))	strcat(buf,"feet ");
					if(IsSet(num, 4096))	strcat(buf,"wrist ");
					if(IsSet(num, 16))	strcat(buf,"head ");
					if(IsSet(num, 8))	strcat(buf,"body ");
					if(IsSet(num, 32))	strcat(buf,"legs ");
					if(IsSet(num, 256))	strcat(buf,"arms ");
					if(IsSet(num, 128))	strcat(buf,"hands ");
					if(IsSet(num, 1) || IsSet(num, 8192) || IsSet(num, 16384))
						strcat(buf,"held ");
					if(ValidString(buf))
						buf[strlen(buf)-1] = '\0'; // strip last space
					str_dup(&objp->wear, buf);

					val0 = fread_number(fp); // value[0]
					val1 = fread_number(fp); // value[1]
					val2 = fread_number(fp); // value[2]
					val3 = fread_number(fp); // value[3]
					fread_number(fp); // ?

					switch(objp->objtype)
					{
					case OBJ_WEAPON:
						// dice weapons
						objp->values[0] = val1; // min damage
						objp->values[1] = val1*val2; // max damage
						break;
					case OBJ_ARMOR:
						objp->values[0]	= val0; // armor/absorbtion266
						break;
					case OBJ_CONTAINER:
						objp->values[0] = val0; // max weight
						break;
					default:	break;
					}

					objp->weight	= fread_number(fp); 
					objp->worth	= fread_number(fp);

					// skip everything else for now
					total_objects++;
				}
			}
			continue;
		}

		if( !strcmp(str,"#ROOMS") )
		{
			while(1)
			{
				str = fread_line(fp);

				if( str[0] == '#' ) // vnum
				{
					for( i = 1; i < (int)strlen(str); i++ )
						buf[i-1] = str[i];
					buf[i-1] = '\0';
					i = atoi(buf);

					if( i == 0 )
						break;

					if( i < low )
						low = i;
					if( i > high )
						high = i;

					room = new_room(i);
					room->area = area;

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&room->name,buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
						strcat(buf," ");
					} while((p=strstr(buf,"~")) == 0);
					*p = '\0';
					str_dup(&room->description,wraptext(0,buf,70));

							fread_number(fp);	// ?
					room->light =	fread_number(fp);

					switch(fread_number(fp)) // room type
					{
					case  0: room->roomtype = ROOM_INSIDE;	break;
					case  1: room->roomtype = ROOM_CITY;	break;
					case  2: room->roomtype = ROOM_GRASSY;	break;
					case  3: room->roomtype = ROOM_FOREST;	break;
					case  4: room->roomtype = ROOM_HILLS;	break;
					case  5: room->roomtype = ROOM_MOUNTAIN; break;
					case  6: room->roomtype = ROOM_WATER;	break;
					case  7: room->roomtype = ROOM_OCEAN;	break;
					case  8:
					case  9: room->roomtype = ROOM_AIR;	break;
					case 10: room->roomtype = ROOM_DESERT;	break;
					}
					rooms++; total_rooms++;
					continue;
				}

				if ( str[0] == 'E' )
				{
					extra = new_extra(room);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(str,"~")) == 0);
					*p = '\0';
					str_dup(&extra->keywords, buf);

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(str,"~")) == 0);
					*p = '\0';
					str_dup(&extra->description, buf);

					extras++; total_extras++;
					continue;

				}

				if ( str[0] == 'D' )
				{
					exit = new_exit(room);
					for( i = 1; i < (int)strlen(str); i++ )
						buf[i-1] = str[i];
					buf[i-1] = '\0';
					i = atoi(buf);

					switch(i)
					{
					case 0: str_dup(&exit->name,"north"); break;
					case 1: str_dup(&exit->name,"east"); break;
					case 2: str_dup(&exit->name,"south"); break;
					case 3: str_dup(&exit->name,"west"); break;
					case 4: str_dup(&exit->name,"up"); break;
					case 5: str_dup(&exit->name,"down"); break;
					default: str_dup(&exit->name,"somewhere"); break;
					}

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(str,"~")) == 0);
					*p = '\0';

					buf[0] = '\0';
					do
					{
						strcat(buf,fread_line(fp));
					} while((p=strstr(str,"~")) == 0);
					*p = '\0';

					exit->door	= fread_number(fp);
					if(exit->door > 0) exit->door = 2;
					else		   exit->door = 0;

					exit->key	= fread_number(fp);
					if(exit->key > 0) exit->door = 3;
					else		  exit->key = 0;

					exit->to_vnum	= fread_number(fp);
					exits++; total_exits++;
					continue;
				}
			}
			continue;
		}

		if( !strcmp(str,"#RESETS") )
		{
			while(1)
			{
				do
				{
					k = getc(fp);
				} while(isspace(k));

				if(k == 'R' || k == '*')
				{
					fread_line(fp);
					continue;
				}
				if(k == 'S')
					break;

				if((k == 'G' || k == 'E' || k == 'P') && !rlast)
					rlast = reset;
				else if(k != 'G' && k != 'E' && k != 'P')
					rlast = 0;

				reset		= new_reset();
				reset->time	= rlast?0:randnum(5,15);
				reset->chance	= randnum(40,100);
						  fread_number(fp); // what is this?
				reset->vnum	= fread_number(fp);
				//reset->max	= fread_number(fp);
fread_number(fp);
reset->max = 5;
				if(k != 'G')
				reset->roomvnum	= fread_number(fp);

				switch(k)
				{
				case 'D':
					reset->loadtype		= TYPE_EXIT;
					reset->chance		= 100;
					reset->loaded		= reset->roomvnum+1;
					reset->roomvnum		= reset->vnum;
					reset->vnum		= 0;
					switch(reset->max)
					{
					case 0: str_dup(&reset->command,"north");	break;
					case 1: str_dup(&reset->command,"east");	break;
					case 2: str_dup(&reset->command,"south");	break;
					case 3: str_dup(&reset->command,"west");	break;
					case 4: str_dup(&reset->command,"up");		break;
					case 5: str_dup(&reset->command,"down");	break;
					default: str_dup(&reset->command,"somewhere");	break;
					}
					reset->max		= 0;
					break;
				case 'M':
					reset->loadtype		= TYPE_CREATURE;
					break;
				case 'G':
				case 'E':
					reset->loadtype		= TYPE_OBJECT;
					reset->roomvnum		= rlast->roomvnum;
					reset->max		= 0;
					reset->time		= 0;

					// set this obj to be set up as worn later with update_resets
					if(k == 'E')
						str_dup(&reset->command, "wearme");
					break;
				case 'O':
					reset->loadtype		= TYPE_OBJECT;
					break;
				case 'P':
					reset->loadtype		= TYPE_OBJECT;
					reset->roomvnum		= rlast->roomvnum;
					break;
				default:
					mudlog("CONVERT: What is type %c?",k);
					break;
				}
				fread_line(fp);

				if(reset->time)
					add_reset(reset);

				resets++; total_resets++;
			}
		}
		if( !strcmp(str,"#SHOPS") )
		{
			while(1)
			{
				num = fread_number(fp);
				if (num == 0)
					break;
				shop = new_shop(num);
				if (!shop)
				{
					for (b = 0; b < 5; b++)
						fread_number(fp);
					fread_line(fp);
					continue;
				}
				for (i = 0; i < 5; i++)
				{
					if (i == 0)
					{
						switch(fread_number(fp))
						{
							case  0: shop->item = OBJ_TREASURE;	break; // no 0
							case  1: shop->item = OBJ_LIGHT; 	break;
							case  2: shop->item = OBJ_TREASURE;	break; // scroll
							case  3:				// wand
							case  4:				// staff
							case  5: shop->item = OBJ_WEAPON;	break;
							case  8: shop->item = OBJ_TREASURE;	break; // treasure
							case  9: shop->item = OBJ_ARMOR;	break;
							case 12: shop->item = OBJ_TREASURE;	break; // furniture
							case 13: shop->item = OBJ_TREASURE;	break; // trash
							case 15: shop->item = OBJ_CONTAINER;	break;
							case 10:				// potion
							case 25:				// fountain
							case 17: shop->item = OBJ_DRINK;	break;
							case 18: shop->item = OBJ_KEY;		break;
							case 19: shop->item = OBJ_FOOD;		break;
							case 20: shop->item = OBJ_COIN;		break;
							case 22: shop->item = OBJ_TREASURE;	break; // boat
							case 23: shop->item = OBJ_TREASURE;	break; // mob corpse
							case 24: shop->item = OBJ_TREASURE;	break; // pc corpse
							case 26: shop->item = OBJ_FOOD;		break; // pill
							default: shop->item = OBJ_TREASURE;	break;
						}
					}
					else	
						fread_number(fp); // loose extraneous
				}
				shop->stype	= SHOP_STORE; 
				shop->buy	= fread_number(fp);
				shop->sell	= fread_number(fp);
				shop->open	= fread_number(fp);
				shop->close	= fread_number(fp);
				fread_line(fp); // drop comments
				shops++; total_shops++;
			}
		}
	}

	fclose(fp);

	// area had a conflict of vnums
	if(!area)
		return;

	area->low = low;
	area->high = high;

	if((mobs == 0 && rooms == 0 && exits == 0 && shops == 0) || (area->high - area->low <= 0))
	{
		mudlog("BAD AREA (%s) was an empty area (help file?)", area->name );
		delete_object(crit,area,0);
		return;
	}
	mudlog("For area: (%s):  MOBS: %li - ROOMS: %li - OBJS: %li - RESETS: %li"
		" - EXITS: %li - EXTRAS: %li - SHOPS: %li.",
		area->name,mobs,rooms,objs,resets,exits,extras,shops);
	if(!multi)
		update_resets();
}
