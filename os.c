/****************************************************************************
* AC Mud written by Michael "borlaK" Morrison and Jason "Pip" Wallace       *
*                   acmud@borlak.org              piptastic+acmud@gmail.com *
*                                                                           *
* Read ../doc/licence.txt for the terms of use.  One of the terms of use is *
* not to remove these headers.                                              *
****************************************************************************/

/*
os.c - operating system dependant functions
*/

#include "stdh.h"


//////////////////////////////////////////////////////////////////////
// Windows doesn't have gettimeofday, so we have to do it this way. //
//////////////////////////////////////////////////////////////////////
void get_time(struct timeval *time)
{
#ifdef WIN32
	static struct _timeb timebuffer;
	_ftime (& timebuffer);
	time->tv_sec = timebuffer.time;
	time->tv_usec = timebuffer.millitm * 1000;
#else
	gettimeofday( time, 0 );
#endif
}



////////////////////////////
// STRCASECMP for windows //
////////////////////////////
#ifdef WIN32
static const u_char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
	'\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
	'\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
	'\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int strcasecmp(const char *s1, const char *s2)
{
	register const u_char *cm = charmap,
			*us1 = (const u_char *)s1,
			*us2 = (const u_char *)s2;

	while (cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return (0);
	return (cm[*us1] - cm[*--us2]);
}
#endif


//////////////////////////////////////////////////////////////////
// CRYPT for windows, using it for unix as well so no conflicts //
//////////////////////////////////////////////////////////////////
/* @(#) $Revision: 66.2 $ */
/*LINTLIBRARY*/
/*
 * This program implements the
 * Proposed Federal Information Processing
 *  Data Encryption Standard.
 * See Federal Register, March 17, 1975 (40FR12134)
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define setkey _setkey
#define crypt _crypt
#define encrypt _encrypt
#endif

/*
 * Initial permutation,
 */
static char ICRYPTP[] = {
	58,50,42,34,26,18,10, 2,
	60,52,44,36,28,20,12, 4,
	62,54,46,38,30,22,14, 6,
	64,56,48,40,32,24,16, 8,
	57,49,41,33,25,17, 9, 1,
	59,51,43,35,27,19,11, 3,
	61,53,45,37,29,21,13, 5,
	63,55,47,39,31,23,15, 7,
};

/*
 * Final permutation, FP = IP^(-1)
 */
static char FCRYPTP[] = {
	40, 8,48,16,56,24,64,32,
	39, 7,47,15,55,23,63,31,
	38, 6,46,14,54,22,62,30,
	37, 5,45,13,53,21,61,29,
	36, 4,44,12,52,20,60,28,
	35, 3,43,11,51,19,59,27,
	34, 2,42,10,50,18,58,26,
	33, 1,41, 9,49,17,57,25,
};

/*
 * Permuted-choice 1 from the key bits
 * to yield C and D.
 * Note that bits 8,16... are left out:
 * They are intended for a parity check.
 */
static char PC1_CRYPTC[] = {
	57,49,41,33,25,17, 9,
	 1,58,50,42,34,26,18,
	10, 2,59,51,43,35,27,
	19,11, 3,60,52,44,36,
};

static char PC1_CRYPTD[] = {
	63,55,47,39,31,23,15,
	 7,62,54,46,38,30,22,
	14, 6,61,53,45,37,29,
	21,13, 5,28,20,12, 4,
};

/*
 * Sequence of shifts used for the key schedule.
 */
static char shifts[] = { 1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1, };

/*
 * Permuted-choice 2, to pick out the bits from
 * the CD array that generate the key schedule.
 */
static char PC2_CRYPTC[] = {
	14,17,11,24, 1, 5,
	 3,28,15, 6,21,10,
	23,19,12, 4,26, 8,
	16, 7,27,20,13, 2,
};

static char PC2_CRYPTD[] = {
	41,52,31,37,47,55,
	30,40,51,45,33,48,
	44,49,39,56,34,53,
	46,42,50,36,29,32,
};

/*
 * The C and D arrays used to calculate the key schedule.
 */

static char CRYPTC[28];
static char CRYPTD[28];
/*
 * The key schedule.
 * Generated from the key.
 */
static char KCRYPTS[16][48];

/*
 * The E bit-selection table.
 */
static char CRYPTE[48];
static char e2[] = {
	32, 1, 2, 3, 4, 5,
	 4, 5, 6, 7, 8, 9,
	 8, 9,10,11,12,13,
	12,13,14,15,16,17,
	16,17,18,19,20,21,
	20,21,22,23,24,25,
	24,25,26,27,28,29,
	28,29,30,31,32, 1,
};

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setkey
#pragma _HP_SECONDARY_DEF _setkey setkey
#define setkey _setkey
#endif

/*
 * Set up the key schedule from the key.
 */

void
setkey(key)
char	*key;
{
	register long i, j, k;
	int	t;

	/*
	 * First, generate C and D by permuting
	 * the key.  The low order bit of each
	 * 8-bit char is not used, so C and D are only 28
	 * bits apiece.
	 */
	for(i=0; i < 28; i++) {
		CRYPTC[i] = key[PC1_CRYPTC[i]-1];
		CRYPTD[i] = key[PC1_CRYPTD[i]-1];
	}
	/*
	 * To generate Ki, rotate C and D according
	 * to schedule and pick up a permutation
	 * using PC2.
	 */
	for(i=0; i < 16; i++) {
		/*
		 * rotate.
		 */
		for(k=0; k < shifts[i]; k++) {
			t = CRYPTC[0];
			for(j=0; j < 28-1; j++)
				CRYPTC[j] = CRYPTC[j+1];
			CRYPTC[27] = t;
			t = CRYPTD[0];
			for(j=0; j < 28-1; j++)
				CRYPTD[j] = CRYPTD[j+1];
			CRYPTD[27] = t;
		}
		/*
		 * get Ki. Note C and D are concatenated.
		 */
		for(j=0; j < 24; j++) {
			KCRYPTS[i][j] = CRYPTC[PC2_CRYPTC[j]-1];
			KCRYPTS[i][j+24] = CRYPTD[PC2_CRYPTD[j]-28-1];
		}
	}

	for(i=0; i < 48; i++)
		CRYPTE[i] = e2[i];
}

/*
 * The 8 selection functions.
 * For some reason, they give a 0-origin
 * index, unlike everything else.
 */
static char CRYPTS[8][64] =
	{ 14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	 0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	 4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13,

	15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	 3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	 0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9,

	10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	 1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12,

	 7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	 3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14,

	 2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	 4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3,

	12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	 9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	 4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13,

	 4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	 1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	 6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12,

	13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	 1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	 7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	 2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11,
};

/*
 * P is a permutation on the selected combination
 * of the current L and key.
 */
static char CRYPTP[] = {
	16, 7,20,21,
	29,12,28,17,
	 1,15,23,26,
	 5,18,31,10,
	 2, 8,24,14,
	32,27, 3, 9,
	19,13,30, 6,
	22,11, 4,25,
};

/*
 * The current block, divided into 2 halves.
 */
static char CRYPTL[32], CRYPTR[32];
static char tempCRYPTL[32];
static char CRYPTf[32];

/*
 * The combination of the key and the input, before selection.
 */
static char preCRYPTS[48];

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef encrypt
#pragma _HP_SECONDARY_DEF _encrypt encrypt
#define encrypt _encrypt
#endif

/*
 * The payoff: encrypt a block.
 */

void
encrypt(block, edflag)
char	*block;
int	edflag;
{
	int	i, ii;
	register int t, j, k;

	/*
	 * First, permute the bits in the input
	 */
	for(j=0; j < 64; j++)
		CRYPTL[j] = block[ICRYPTP[j]-1];
	/*
	 * Perform an encryption operation 16 times.
	 */
	for(ii=0; ii < 16; ii++) {
		i = ii;
		/*
		 * Save the R array,
		 * which will be the new L.
		 */
		for(j=0; j < 32; j++)
			tempCRYPTL[j] = CRYPTR[j];
		/*
		 * Expand R to 48 bits using the E selector;
		 * exclusive-or with the current key bits.
		 */
		for(j=0; j < 48; j++)
			preCRYPTS[j] = CRYPTR[CRYPTE[j]-1] ^ KCRYPTS[i][j];
		/*
		 * The pre-select bits are now considered
		 * in 8 groups of 6 bits each.
		 * The 8 selection functions map these
		 * 6-bit quantities into 4-bit quantities
		 * and the results permuted
		 * to make an f(R, K).
		 * The indexing into the selection functions
		 * is peculiar; it could be simplified by
		 * rewriting the tables.
		 */
		for(j=0; j < 8; j++) {
			t = 6*j;
			k = CRYPTS[j][(preCRYPTS[t+0]<<5)+
				(preCRYPTS[t+1]<<3)+
				(preCRYPTS[t+2]<<2)+
				(preCRYPTS[t+3]<<1)+
				(preCRYPTS[t+4]<<0)+
				(preCRYPTS[t+5]<<4)];
			t = 4*j;
			CRYPTf[t+0] = (k>>3)&01;
			CRYPTf[t+1] = (k>>2)&01;
			CRYPTf[t+2] = (k>>1)&01;
			CRYPTf[t+3] = (k>>0)&01;
		}
		/*
		 * The new R is L ^ f(R, K).
		 * The f here has to be permuted first, though.
		 */
		for(j=0; j < 32; j++)
			CRYPTR[j] = CRYPTL[j] ^ CRYPTf[CRYPTP[j]-1];
		/*
		 * Finally, the new L (the original R)
		 * is copied back.
		 */
		for(j=0; j < 32; j++)
			CRYPTL[j] = tempCRYPTL[j];
	}
	/*
	 * The output L and R are reversed.
	 */
	for(j=0; j < 32; j++) {
		t = CRYPTL[j];
		CRYPTL[j] = CRYPTR[j];
		CRYPTR[j] = t;
	}
	/*
	 * The final output
	 * gets the inverse permutation of the very original.
	 */
	for(j=0; j < 64; j++)
		block[j] = CRYPTL[FCRYPTP[j]-1];
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef crypt
#pragma _HP_SECONDARY_DEF _crypt crypt
#define crypt _crypt
#endif

char *
crypt(pw, salt)
const char	*pw, *salt;
{
	register int i, j, c;
	int	temp;
	static char block[66], iobuCRYPTf[16];

	for(i=0; i < 66; i++)
		block[i] = 0;
	for(i=0; (c= *pw) && i < 64; pw++) {
		for(j=0; j < 7; j++, i++)
			block[i] = (c>>(6-j)) & 01;
		i++;
	}

	setkey(block);

	for(i=0; i < 66; i++)
		block[i] = 0;

	for(i=0; i < 2; i++) {
		c = *salt++;
		iobuCRYPTf[i] = c;
		if(c > 'Z')
			c -= 6;
		if(c > '9')
			c -= 7;
		c -= '.';
		for(j=0; j < 6; j++) {
			if((c>>j) & 01) {
				temp = CRYPTE[6*i+j];
				CRYPTE[6*i+j] = CRYPTE[6*i+j+24];
				CRYPTE[6*i+j+24] = temp;
			}
		}
	}

	for(i=0; i < 25; i++)
		encrypt(block, 0);

	for(i=0; i < 11; i++) {
		c = 0;
		for(j=0; j < 6; j++) {
			c <<= 1;
			c |= block[6*i+j];
		}
		c += '.';
		if(c > '9')
			c += 7;
		if(c > 'Z')
			c += 6;
		iobuCRYPTf[i+2] = c;
	}
	iobuCRYPTf[i+2] = 0;
	if(iobuCRYPTf[1] == 0)
		iobuCRYPTf[1] = iobuCRYPTf[0];
	return(iobuCRYPTf);
}
