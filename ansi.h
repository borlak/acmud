/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

// Ansi color table and data structure ////////////////////////////////////////

struct ansi_color
{
	const char *symbol;
	const char *fg_code;
	const char *bg_code;
} color_table[] = {

	{ "L", "30", "40" },	// Black
	{ "R", "31", "41" },	// Red
	{ "G", "32", "42" },	// Green
	{ "Y", "33", "43" },	// Yellow
	{ "B", "34", "44" },	// Blue
	{ "M", "35", "45" },	// Magenta
	{ "C", "36", "46" },	// Cyan
	{ "W", "37", "47" },	// White
	{ 0, 0, 0 }		
};

long find_color_entry(long c)
{
	long i = 0;
	char s;

	s = Upper(c);

	while ((color_table[i].symbol != 0) && (*color_table[i].symbol != s))
		i++;

	if(s == 'K') // Random color code
		return randnum(1,i-1);

	return i;
}
