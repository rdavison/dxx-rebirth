/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines to display title screens...
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: titles.c,v 1.2 2006/03/18 23:08:13 michaelstather Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "timer.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "joy.h"
#include "gamefont.h"
#include "cfile.h"
#include "error.h"
#include "polyobj.h"
#include "textures.h"
#include "screens.h"
#include "multi.h"
#include "player.h"
#include "digi.h"
#include "text.h"
#include "kmatrix.h"
#include "piggy.h"
#include "songs.h"
#include "newmenu.h"
#include "state.h"
#include "menu.h"
#include "mouse.h"
#include "console.h"

void title_save_game();
void set_briefing_fontcolor ();

#define MAX_BRIEFING_COLORS     7
#define DEFAULT_BRIEFING_BKG		"brief03.pcx"

int	Briefing_text_colors[MAX_BRIEFING_COLORS];
int	Current_color = 0;
int	Erase_color;

// added by Jan Bobrowski for variable-size menu screen
static int rescale_x(int x)
{
	return x * GWIDTH / 320;
}

static int rescale_y(int y)
{
	return y * GHEIGHT / 200;
}

typedef struct title_screen
{
	grs_bitmap title_bm;
	fix timer;
	int allow_keys;
} title_screen;

int title_handler(window *wind, d_event *event, title_screen *ts)
{
	switch (event->type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
			if (mouse_get_button(event) != 0)
				return 0;
			else if (ts->allow_keys)
			{
				window_close(wind);
				return 1;
			}
			break;
			
		case EVENT_KEY_COMMAND:
			switch (((d_event_keycommand *)event)->keycode)
			{
				case KEY_PRINT_SCREEN:
					save_screen_shot(0);
					return 1;
					
				default:
					if (ts->allow_keys)
						window_close(wind);
					return 1;
			}
			break;
			
		case EVENT_IDLE:
			timer_delay2(50);

			if (timer_get_fixed_seconds() > ts->timer)
			{
				window_close(wind);
				return 1;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			show_fullscr(&ts->title_bm);
			break;

		case EVENT_WINDOW_CLOSE:
			gr_free_bitmap_data (&ts->title_bm);
			d_free(ts);
			break;
			
		default:
			break;
	}
	
	return 0;
}

int show_title_screen( char * filename, int allow_keys, int from_hog_only )
{
	title_screen *ts;
	window *wind;
	int pcx_error;
	char new_filename[FILENAME_LEN+1] = "";
	
	MALLOC(ts, title_screen, 1);
	if (!ts)
		return 0;
	
	ts->allow_keys = allow_keys;

#ifdef RELEASE
	if (from_hog_only)
		strcpy(new_filename,"\x01");	//only read from hog file
#endif

	strcat(new_filename,filename);
	filename = new_filename;

	gr_init_bitmap_data (&ts->title_bm);

	if ((pcx_error=pcx_read_bitmap( filename, &ts->title_bm, BM_LINEAR, gr_palette ))!=PCX_ERROR_NONE)	{
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",filename, pcx_errormsg(pcx_error), pcx_error);
	}

	gr_set_current_canvas( NULL );

	ts->timer = timer_get_fixed_seconds() + i2f(3);

	gr_palette_load( gr_palette );

	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))title_handler, ts);
	if (!wind)
	{
		gr_free_bitmap_data (&ts->title_bm);
		d_free(ts);
		return 0;
	}
	
	while (window_exists(wind))
		event_process();

	return 0;
}

void show_titles(void)
{
	char    publisher[16];
	
	if (GameArg.SysNoTitles)
		return;

	strcpy(publisher, "macplay.pcx");	// Mac Shareware
	if (!PHYSFS_exists(publisher))
		strcpy(publisher, "mplaycd.pcx");	// Mac Registered
	if (!PHYSFS_exists(publisher))
		strcpy(publisher, "iplogo1.pcx");	// PC. Only down here because it's lowres ;-)
	
	show_title_screen( publisher, 1, 1 );
	show_title_screen( (((SWIDTH>=640&&SHEIGHT>=480) && cfexist("logoh.pcx"))?"logoh.pcx":"logo.pcx"), 1, 1 );
	show_title_screen( (((SWIDTH>=640&&SHEIGHT>=480) && cfexist("descenth.pcx"))?"descenth.pcx":"descent.pcx"), 1, 1 );
}

void show_order_form()
{
	char    exit_screen[16];
	
	strcpy(exit_screen, "warning.pcx");	// D1 Registered
	if (! cfexist(exit_screen))
		strcpy(exit_screen, "apple.pcx");	// D1 Mac OEM Demo
	if (! cfexist(exit_screen))
		strcpy(exit_screen, "order01.pcx"); // D1 Demo
	show_title_screen(exit_screen, 1, 1);
}


//-----------------------------------------------------------------------------
typedef struct {
	char    bs_name[16];                //  filename, eg merc01.  Assumes .lbm suffix.
	sbyte   level_num;
	sbyte   message_num;
	short   text_ulx, text_uly;         //  upper left x,y of text window
	short   text_width, text_height;    //  width and height of text window
} briefing_screen;

#define BRIEFING_SECRET_NUM 31          //  This must correspond to the first secret level which must come at the end of the list.
#define BRIEFING_OFFSET_NUM 4           // This must correspond to the first level screen (ie, past the bald guy briefing screens)

#define	ENDING_LEVEL_NUM_OEMSHARE 0x7f
#define	ENDING_LEVEL_NUM_REGISTER 0x7e
#define Briefing_screens_LH ((SWIDTH >= 640 && SHEIGHT >= 480 && cfexist( "brief01h.pcx"))?Briefing_screens_h:Briefing_screens)

briefing_screen Briefing_screens[] = {
	{ "brief01.pcx",   0,  1,  13, 140, 290,  59 },
	{ "brief02.pcx",   0,  2,  27,  34, 257, 177 },
	{ "brief03.pcx",   0,  3,  20,  22, 257, 177 },
	{ "brief02.pcx",   0,  4,  27,  34, 257, 177 },

	{ "moon01.pcx",    1,  5,  10,  10, 300, 170 }, // level 1
	{ "moon01.pcx",    2,  6,  10,  10, 300, 170 }, // level 2
	{ "moon01.pcx",    3,  7,  10,  10, 300, 170 }, // level 3

	{ "venus01.pcx",   4,  8,  15, 15, 300,  200 }, // level 4
	{ "venus01.pcx",   5,  9,  15, 15, 300,  200 }, // level 5

	{ "brief03.pcx",   6, 10,  20,  22, 257, 177 },
	{ "merc01.pcx",    6, 11,  10, 15, 300, 200 },  // level 6
	{ "merc01.pcx",    7, 12,  10, 15, 300, 200 },  // level 7

	{ "brief03.pcx",   8, 13,  20,  22, 257, 177 },
	{ "mars01.pcx",    8, 14,  10, 100, 300,  200 }, // level 8
	{ "mars01.pcx",    9, 15,  10, 100, 300,  200 }, // level 9
	{ "brief03.pcx",  10, 16,  20,  22, 257, 177 },
	{ "mars01.pcx",   10, 17,  10, 100, 300,  200 }, // level 10

	{ "jup01.pcx",    11, 18,  10, 40, 300,  200 }, // level 11
	{ "jup01.pcx",    12, 19,  10, 40, 300,  200 }, // level 12
	{ "brief03.pcx",  13, 20,  20,  22, 257, 177 },
	{ "jup01.pcx",    13, 21,  10, 40, 300,  200 }, // level 13
	{ "jup01.pcx",    14, 22,  10, 40, 300,  200 }, // level 14

	{ "saturn01.pcx", 15, 23,  10, 40, 300,  200 }, // level 15
	{ "brief03.pcx",  16, 24,  20,  22, 257, 177 },
	{ "saturn01.pcx", 16, 25,  10, 40, 300,  200 }, // level 16
	{ "brief03.pcx",  17, 26,  20,  22, 257, 177 },
	{ "saturn01.pcx", 17, 27,  10, 40, 300,  200 }, // level 17

	{ "uranus01.pcx", 18, 28,  100, 100, 300,  200 }, // level 18
	{ "uranus01.pcx", 19, 29,  100, 100, 300,  200 }, // level 19
	{ "uranus01.pcx", 20, 30,  100, 100, 300,  200 }, // level 20
	{ "uranus01.pcx", 21, 31,  100, 100, 300,  200 }, // level 21

	{ "neptun01.pcx", 22, 32,  10, 20, 300,  200 }, // level 22
	{ "neptun01.pcx", 23, 33,  10, 20, 300,  200 }, // level 23
	{ "neptun01.pcx", 24, 34,  10, 20, 300,  200 }, // level 24

	{ "pluto01.pcx",  25, 35,  10, 20, 300,  200 }, // level 25
	{ "pluto01.pcx",  26, 36,  10, 20, 300,  200 }, // level 26
	{ "pluto01.pcx",  27, 37,  10, 20, 300,  200 }, // level 27

	{ "aster01.pcx",  -1, 38,  10, 90, 300,  200 }, // secret level -1
	{ "aster01.pcx",  -2, 39,  10, 90, 300,  200 }, // secret level -2
	{ "aster01.pcx",  -3, 40,  10, 90, 300,  200 }, // secret level -3

	{ "end01.pcx",   ENDING_LEVEL_NUM_OEMSHARE,  1,  23, 40, 320, 200 },   //  OEM and shareware end
	{ "end02.pcx",   ENDING_LEVEL_NUM_REGISTER,  1,  5, 5, 300, 200 },    // registered end
	{ "end01.pcx",   ENDING_LEVEL_NUM_REGISTER,  2,  23, 40, 320, 200 },  // registered end
	{ "end03.pcx",   ENDING_LEVEL_NUM_REGISTER,  3,  5, 5, 300, 200 },    // registered end

};

briefing_screen Briefing_screens_h[] = { // hires screens
	{ "brief01h.pcx",   0,  1,  13, 140, 290,  59 },
	{ "brief02h.pcx",   0,  2,  27,  34, 257, 177 },
	{ "brief03h.pcx",   0,  3,  20,  22, 257, 177 },
	{ "brief02h.pcx",   0,  4,  27,  34, 257, 177 },

	{ "moon01h.pcx",    1,  5,  10,  10, 300, 170 },	// level 1
	{ "moon01h.pcx",    2,  6,  10,  10, 300, 170 },	// level 2
	{ "moon01h.pcx",    3,  7,  10,  10, 300, 170 },	// level 3

	{ "venus01h.pcx",   4,  8,  15,  15, 300, 200 },	// level 4
	{ "venus01h.pcx",   5,  9,  15,  15, 300, 200 },	// level 5

	{ "brief03h.pcx",   6, 10,  20,  22, 257, 177 },
	{ "merc01h.pcx",    6, 11,  10,  15, 300, 200 },	// level 6
	{ "merc01h.pcx",    7, 12,  10,  15, 300, 200 },	// level 7

	{ "brief03h.pcx",   8, 13,  20,  22, 257, 177 },
	{ "mars01h.pcx",    8, 14,  10, 100, 300, 200 },	// level 8
	{ "mars01h.pcx",    9, 15,  10, 100, 300, 200 },	// level 9
	{ "brief03h.pcx",  10, 16,  20,  22, 257, 177 },
	{ "mars01h.pcx",   10, 17,  10, 100, 300, 200 },	// level 10

	{ "jup01h.pcx",    11, 18,  10,  40, 300, 200 },	// level 11
	{ "jup01h.pcx",    12, 19,  10,  40, 300, 200 },	// level 12
	{ "brief03h.pcx",  13, 20,  20,  22, 257, 177 },
	{ "jup01h.pcx",    13, 21,  10,  40, 300, 200 },	// level 13
	{ "jup01h.pcx",    14, 22,  10,  40, 300, 200 },	// level 14

	{ "saturn01h.pcx", 15, 23,  10,  40, 300, 200 },	// level 15
	{ "brief03h.pcx",  16, 24,  20,  22, 257, 177 },
	{ "saturn01h.pcx", 16, 25,  10,  40, 300, 200 },	// level 16
	{ "brief03h.pcx",  17, 26,  20,  22, 257, 177 },
	{ "saturn01h.pcx", 17, 27,  10,  40, 300, 200 },	// level 17

	{ "uranus01h.pcx", 18, 28, 100, 100, 300, 200 },	// level 18
	{ "uranus01h.pcx", 19, 29, 100, 100, 300, 200 },	// level 19
	{ "uranus01h.pcx", 20, 30, 100, 100, 300, 200 },	// level 20
	{ "uranus01h.pcx", 21, 31, 100, 100, 300, 200 },	// level 21

	{ "neptun01h.pcx", 22, 32,  10,  20, 300, 200 },	// level 22
	{ "neptun01h.pcx", 23, 33,  10,  20, 300, 200 },	// level 23
	{ "neptun01h.pcx", 24, 34,  10,  20, 300, 200 },	// level 24

	{ "pluto01h.pcx",  25, 35,  10,  20, 300, 200 },	// level 25
	{ "pluto01h.pcx",  26, 36,  10,  20, 300, 200 },	// level 26
	{ "pluto01h.pcx",  27, 37,  10,  20, 300, 200 },	// level 27

	{ "aster01h.pcx",  -1, 38,  10, 90, 300,  200 },	// secret level -1
	{ "aster01h.pcx",  -2, 39,  10, 90, 300,  200 },	// secret level -2
	{ "aster01h.pcx",  -3, 40,  10, 90, 300,  200 }, 	// secret level -3

	{ "end01h.pcx",   ENDING_LEVEL_NUM_OEMSHARE,  1,  23, 40, 320, 200 },   //  OEM and shareware end
	{ "end02h.pcx",   ENDING_LEVEL_NUM_REGISTER,  1,  5, 5, 300, 200 },    // registered end
	{ "end01h.pcx",   ENDING_LEVEL_NUM_REGISTER,  2,  23, 40, 320, 200 },  // registered end
	{ "end03h.pcx",   ENDING_LEVEL_NUM_REGISTER,  3,  5, 5, 300, 200 },    // registered end

};

#define	MAX_BRIEFING_SCREEN (sizeof(Briefing_screens) / sizeof(Briefing_screens[0]))

typedef struct msgstream
{
	int x;
	int y;
	int color;
	int ch;
} __pack__ msgstream;

typedef struct briefing
{
	short	level_num;
	short	cur_screen;
	briefing_screen	*screen;
	grs_bitmap background;
	char	background_name[16];
	char	*text;
	char	*message;
	int		text_x, text_y;
	msgstream messagestream[2048];
	int		streamcount;
	short	tab_stop;
	ubyte	flashing_cursor;
	ubyte	new_page;
	int		new_screen;
	fix		start_time;
	fix		delay_count;
	int		robot_num;
	grs_canvas	*robot_canv;
	vms_angvec	robot_angles;
	char    bitmap_name[32];
	grs_bitmap  guy_bitmap;
	sbyte	guy_bitmap_show;
	sbyte   door_dir, door_div_count, animating_bitmap_type;
	sbyte	prev_ch;
} briefing;

void briefing_init(briefing *br, short level_num)
{
	br->level_num = level_num;
	if (br->level_num == 1)
		br->level_num = 0;	// for start of game stuff

	br->cur_screen = 0;
	br->screen = NULL;
	gr_init_bitmap_data (&br->background);
	strncpy(br->background_name, DEFAULT_BRIEFING_BKG, sizeof(br->background_name));
	br->robot_canv = NULL;
	br->bitmap_name[0] = '\0';
	br->door_dir = 1;
	br->door_div_count = 0;
	br->animating_bitmap_type = 0;
}

//-----------------------------------------------------------------------------
//	Load Descent briefing text.
int load_screen_text(char *filename, char **buf)
{
	CFILE *tfile;
	CFILE *ifile;
	int	len;
	int	have_binary = 0;
	
	if ((tfile = cfopen(filename,"rb")) == NULL) {
		char nfilename[30], *ptr;
		
		strcpy(nfilename, filename);
		if ((ptr = strrchr(nfilename, '.')))
			*ptr = '\0';
		strcat(nfilename, ".txb");
		if ((ifile = cfopen(nfilename, "rb")) == NULL) {
			return (0);
			//Error("Cannot open file %s or %s", filename, nfilename);
		}
		
		have_binary = 1;
		
		len = cfilelength(ifile);
		MALLOC(*buf, char, len+1);
		cfread(*buf, 1, len, ifile);
		cfclose(ifile);
	} else {
		len = cfilelength(tfile);
		MALLOC(*buf, char, len+1);
		cfread(*buf, 1, len, tfile);
		cfclose(tfile);
	}
	
	if (have_binary)
		decode_text(*buf, len);
	
	*(*buf+len)='\0';
	
	return (1);
}

int get_message_num(char **message)
{
	int	num=0;
	
	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;
	
	while (strlen(*message) > 0 && (**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}
	
	while (strlen(*message) > 0 && *(*message)++ != 10)		//	Get and drop eoln
		;
	
	return num;
}

void get_message_name(char **message, char *result)
{
	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;
	
	while (strlen(*message) > 0 && (**message != ' ') && (**message != 10)) {
		if (**message != '\n')
			*result++ = **message;
		(*message)++;
	}
	
	if (**message != 10)
		while (strlen(*message) > 0 && *(*message)++ != 10)		//	Get and drop eoln
			;
	
	*result = 0;
}

// Return a pointer to the start of text for screen #screen_num.
char * get_briefing_message(briefing *br, int screen_num)
{
	char	*tptr = br->text;
	int	cur_screen=0;
	int	ch;
	
	Assert(screen_num >= 0);
	
	while ( (*tptr != 0 ) && (screen_num != cur_screen)) {
		ch = *tptr++;
		if (ch == '$') {
			ch = *tptr++;
			if (ch == 'S')
				cur_screen = get_message_num(&tptr);
		}
	}
	
	if (screen_num!=cur_screen)
		return (NULL);
	
	return tptr;
}

void init_char_pos(briefing *br, int x, int y)
{
	br->text_x = x;
	br->text_y = y;
}

// Make sure the text stays on the screen
// Return 1 if new page required
// 0 otherwise
int check_text_pos(briefing *br)
{
	if (br->text_x > br->screen->text_ulx + br->screen->text_width)
	{
		br->text_x = br->screen->text_ulx;
		br->text_y += br->screen->text_uly;
	}
	
	if (br->text_y > br->screen->text_uly + br->screen->text_height)
	{
		br->new_page = 1;
		return 1;
	}
	
	return 0;
}

void put_char_delay(briefing *br, int ch)
{
	char str[] = { ch, '\0' };
	int	w, h, aw;
	
	if (br->delay_count && (timer_get_fixed_seconds() < br->start_time + br->delay_count))
	{
		br->message--;		// Go back to same character
		return;
	}
	
	br->messagestream[br->streamcount].x = br->text_x;
	br->messagestream[br->streamcount].y = br->text_y;
	br->messagestream[br->streamcount].color = Briefing_text_colors[Current_color];
	br->messagestream[br->streamcount].ch = ch;
	br->streamcount++;
	
	br->prev_ch = ch;
	gr_get_string_size(str, &w, &h, &aw );
	br->text_x += w;

	br->start_time = timer_get_fixed_seconds();
}

void init_spinning_robot(briefing *br);
int load_briefing_screen(briefing *br, char *fname);

// Process a character for the briefing,
// including special characters preceded by a '$'.
// Return 1 when page is finished, 0 otherwise
int briefing_process_char(briefing *br)
{
	int	ch;
	
	ch = *br->message++;
	if (ch == '$') {
		ch = *br->message++;
		if (ch == 'C') {
			Current_color = get_message_num(&br->message)-1;
			Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
			br->prev_ch = 10;
		} else if (ch == 'F') {     // toggle flashing cursor
			br->flashing_cursor = !br->flashing_cursor;
			br->prev_ch = 10;
			while (*br->message++ != 10)
				;
		} else if (ch == 'T') {
			br->tab_stop = get_message_num(&br->message);
			br->tab_stop*=(1+HIRESMODE);
			br->prev_ch = 10;							//	read to eoln
		} else if (ch == 'R') {
			if (br->robot_canv != NULL)
			{
				d_free(br->robot_canv);
				br->robot_canv=NULL;
			}
			
			init_spinning_robot(br);
			br->robot_num = get_message_num(&br->message);
			while (*br->message++ != 10)
				;
			br->prev_ch = 10;                           // read to eoln
		} else if (ch == 'N') {
			if (br->robot_canv != NULL)
			{
				d_free(br->robot_canv);
				br->robot_canv=NULL;
			}
			
			get_message_name(&br->message, br->bitmap_name);
			strcat(br->bitmap_name, "#0");
			br->animating_bitmap_type = 0;
			br->prev_ch = 10;
		} else if (ch == 'O') {
			if (br->robot_canv != NULL)
			{
				d_free(br->robot_canv);
				br->robot_canv=NULL;
			}
			
			get_message_name(&br->message, br->bitmap_name);
			strcat(br->bitmap_name, "#0");
			br->animating_bitmap_type = 1;
			br->prev_ch = 10;
		} else if (ch == 'B') {
			char		bitmap_name[32];
			ubyte		temp_palette[768];
			int		iff_error;
			
			if (br->robot_canv != NULL)
			{
				d_free(br->robot_canv);
				br->robot_canv=NULL;
			}
			
			get_message_name(&br->message, bitmap_name);
			strcat(bitmap_name, ".bbm");
			gr_init_bitmap_data (&br->guy_bitmap);
			iff_error = iff_read_bitmap(bitmap_name, &br->guy_bitmap, BM_LINEAR, temp_palette);
			Assert(iff_error == IFF_NO_ERROR);
			gr_remap_bitmap_good( &br->guy_bitmap, temp_palette, -1, -1 );
			
			br->guy_bitmap_show=1;
			br->prev_ch = 10;
		} else if (ch == 'S') {
			br->new_screen = 1;
			return 1;
		} else if (ch == 'P') {		//	New page.
			br->new_page = 1;
			
			while (*br->message != 10) {
				br->message++;	//	drop carriage return after special escape sequence
			}
			br->message++;
			br->prev_ch = 10;
			
			return 1;
		} else if (ch == '$' || ch == ';') // Print a $/;
			put_char_delay(br, ch);
	} else if (ch == '\t') {		//	Tab
		if (br->text_x - br->screen->text_ulx < br->tab_stop)
			br->text_x = br->screen->text_ulx + br->tab_stop;
	} else if ((ch == ';') && (br->prev_ch == 10)) {
		while (*br->message++ != 10)
			;
		br->prev_ch = 10;
	} else if (ch == '\\') {
		br->prev_ch = ch;
	} else if (ch == 10) {
		if (br->prev_ch != '\\') {
			br->prev_ch = ch;
			br->text_y += FSPACY(5)+FSPACY(5)*3/5;
			br->text_x = br->screen->text_ulx;
			if (br->text_y > br->screen->text_uly + br->screen->text_height) {
				load_briefing_screen(br, Briefing_screens[br->cur_screen].bs_name);
				br->text_x = br->screen->text_ulx;
				br->text_y = br->screen->text_uly;
			}
		} else {
			if (ch == 13)		//Can this happen? Above says ch==10
				Int3();
			br->prev_ch = ch;
		}
	} else
		put_char_delay(br, ch);
	
	return 0;
}

void set_briefing_fontcolor ()
{
	Briefing_text_colors[0] = gr_find_closest_color_current( 0, 40, 0);
	Briefing_text_colors[1] = gr_find_closest_color_current( 40, 33, 35);
	Briefing_text_colors[2] = gr_find_closest_color_current( 8, 31, 54);
	
	//green
	Briefing_text_colors[0] = gr_find_closest_color_current( 0, 54, 0);
	//white
	Briefing_text_colors[1] = gr_find_closest_color_current( 42, 38, 32);
	
	//Begin D1X addition
	//red
	Briefing_text_colors[2] = gr_find_closest_color_current( 63, 0, 0);
	
	//blue
	Briefing_text_colors[3] = gr_find_closest_color_current( 0, 0, 54);
	//gray
	Briefing_text_colors[4] = gr_find_closest_color_current( 14, 14, 14);
	//yellow
	Briefing_text_colors[5] = gr_find_closest_color_current( 54, 54, 0);
	//purple
	Briefing_text_colors[6] = gr_find_closest_color_current( 0, 54, 54);
	//End D1X addition
	
	Erase_color = gr_find_closest_color_current(0, 0, 0);
}

void redraw_messagestream(msgstream *stream, int count)
{
	char msgbuf[2];
	int i;
	
	for (i=0; i<count; i++) {
		msgbuf[0] = stream[i].ch;
		msgbuf[1] = 0;
		if (stream[i-1].color != stream[i].color)
			gr_set_fontcolor(stream[i].color,-1);
		gr_printf(stream[i].x+1,stream[i].y,"%s",msgbuf);
	}
}

void flash_cursor(briefing *br, int cursor_flag)
{
	if (cursor_flag == 0)
		return;
	
	if ((timer_get_fixed_seconds() % (F1_0/2) ) > (F1_0/4))
		gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
	else
		gr_set_fontcolor(Erase_color, -1);
	
	gr_printf(br->text_x, br->text_y, "_" );
}

#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      // Adam: This is the number of frames in your new animating thing.
#define DOOR_DIV_INIT   6

//-----------------------------------------------------------------------------
void show_animated_bitmap(briefing *br)
{
	grs_canvas  *curcanv_save, *bitmap_canv=0;
	grs_bitmap	*bitmap_ptr;

	// Only plot every nth frame.
	if (br->door_div_count) {
		if (br->bitmap_name[0] != 0) {
			bitmap_index bi;
			bi = piggy_find_bitmap(br->bitmap_name);
			bitmap_ptr = &GameBitmaps[bi.index];
			PIGGY_PAGE_IN( bi );
#ifdef OGL
			ogl_ubitmapm_cs(rescale_x(220), rescale_y(45),(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
#else
			gr_bitmapm(rescale_x(220), rescale_y(45), bitmap_ptr);
#endif
		}
		br->door_div_count--;
		return;
	}

	br->door_div_count = DOOR_DIV_INIT;

	if (br->bitmap_name[0] != 0) {
		char		*pound_signp;
		int		num, dig1, dig2;
		bitmap_index bi;

		switch (br->animating_bitmap_type) {
			case 0:		bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(45), 64, 64);	break;
			case 1:		bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(45), 94, 94);	break; // Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
			default:	Int3(); // Impossible, illegal value for br->animating_bitmap_type
		}

		curcanv_save = grd_curcanv;
		grd_curcanv = bitmap_canv;

		pound_signp = strchr(br->bitmap_name, '#');
		Assert(pound_signp != NULL);

		dig1 = *(pound_signp+1);
		dig2 = *(pound_signp+2);
		if (dig2 == 0)
			num = dig1-'0';
		else
			num = (dig1-'0')*10 + (dig2-'0');

		switch (br->animating_bitmap_type) {
			case 0:
				num += br->door_dir;
				if (num > EXIT_DOOR_MAX) {
					num = EXIT_DOOR_MAX;
					br->door_dir = -1;
				} else if (num < 0) {
					num = 0;
					br->door_dir = 1;
				}
				break;
			case 1:
				num++;
				if (num > OTHER_THING_MAX)
					num = 0;
				break;
		}

		Assert(num < 100);
		if (num >= 10) {
			*(pound_signp+1) = (num / 10) + '0';
			*(pound_signp+2) = (num % 10) + '0';
			*(pound_signp+3) = 0;
		} else {
			*(pound_signp+1) = (num % 10) + '0';
			*(pound_signp+2) = 0;
		}

		bi = piggy_find_bitmap(br->bitmap_name);
		bitmap_ptr = &GameBitmaps[bi.index];
		PIGGY_PAGE_IN( bi );
#ifdef OGL
		ogl_ubitmapm_cs(0,0,(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
#else
		gr_bitmapm(0, 0, bitmap_ptr);
#endif
		grd_curcanv = curcanv_save;
		d_free(bitmap_canv);

		switch (br->animating_bitmap_type) {
			case 0:
				if (num == EXIT_DOOR_MAX) {
					br->door_dir = -1;
					br->door_div_count = 64;
				} else if (num == 0) {
					br->door_dir = 1;
					br->door_div_count = 64;
				}
				break;
			case 1:
				break;
		}
	}
}

//-----------------------------------------------------------------------------
void show_briefing_bitmap(grs_bitmap *bmp)
{
	grs_canvas	*curcanv_save, *bitmap_canv;

	bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(55), (bmp->bm_w*(SWIDTH/(HIRESMODE ? 640 : 320))),(bmp->bm_h*(SHEIGHT/(HIRESMODE ? 480 : 200))));
	curcanv_save = grd_curcanv;
	gr_set_current_canvas(bitmap_canv);
#ifdef OGL
	ogl_ubitmapm_cs(0,0,(bmp->bm_w*(SWIDTH/(HIRESMODE ? 640 : 320))),(bmp->bm_h*(SHEIGHT/(HIRESMODE ? 480 : 200))),bmp,255,F1_0);
#else
	gr_bitmapm(0, 0, bmp);
#endif
	gr_set_current_canvas(curcanv_save);

	d_free(bitmap_canv);
}

//-----------------------------------------------------------------------------
void init_spinning_robot(briefing *br) //(int x,int y,int w,int h)
{
	int x = rescale_x(138);
	int y = rescale_y(55);
	int w = rescale_x(166);
	int h = rescale_y(138);

	br->robot_canv = gr_create_sub_canvas(grd_curcanv, x, y, w, h);
}

void show_spinning_robot_frame(briefing *br, int robot_num)
{
	grs_canvas	*curcanv_save;

	if (robot_num != -1) {
		br->robot_angles.h += 150;

		curcanv_save = grd_curcanv;
		grd_curcanv = br->robot_canv;
		Assert(Robot_info[robot_num].model_num != -1);
		draw_model_picture(Robot_info[robot_num].model_num, &br->robot_angles);
		grd_curcanv = curcanv_save;
	}

}

//-----------------------------------------------------------------------------
#define KEY_DELAY_DEFAULT       ((F1_0*20)/1000)

void init_new_page(briefing *br)
{
	br->new_page = 0;
	br->robot_num = -1;
	
	load_briefing_screen(br, br->background_name);
	br->text_x = br->screen->text_ulx;
	br->text_y = br->screen->text_uly;
	
	br->streamcount=0;
	if (br->guy_bitmap_show) {
		gr_free_bitmap_data (&br->guy_bitmap);
		br->guy_bitmap_show=0;
	}
	
	br->start_time = 0;
	br->delay_count = KEY_DELAY_DEFAULT;
}

//	-----------------------------------------------------------------------------
ubyte baldguy_cheat = 0;
static ubyte bald_guy_cheat_index = 0;
char new_baldguy_pcx[] = "btexture.xxx";

#define	BALD_GUY_CHEAT_SIZE	7
#define NEW_END_GUY1	1
#define NEW_END_GUY2	3

ubyte	bald_guy_cheat_1[BALD_GUY_CHEAT_SIZE] = { KEY_B ^ 0xF0 ^ 0xab, 
	KEY_A ^ 0xE0 ^ 0xab, 
	KEY_L ^ 0xD0 ^ 0xab, 
	KEY_D ^ 0xC0 ^ 0xab, 
	KEY_G ^ 0xB0 ^ 0xab, 
	KEY_U ^ 0xA0 ^ 0xab,
	KEY_Y ^ 0x90 ^ 0xab };

void bald_guy_cheat(int key)
{
	if (key == (bald_guy_cheat_1[bald_guy_cheat_index] ^ (0xf0 - (bald_guy_cheat_index << 4)) ^ 0xab)) {
		bald_guy_cheat_index++;
		if (bald_guy_cheat_index == BALD_GUY_CHEAT_SIZE)	{
			baldguy_cheat = 1;
			bald_guy_cheat_index = 0;
		}
	} else
		bald_guy_cheat_index = 0;
}

void free_briefing_screen(briefing *br);

//	loads a briefing screen
int load_briefing_screen(briefing *br, char *fname)
{
	int pcx_error;

	free_briefing_screen(br);

	gr_init_bitmap_data(&br->background);
	if (stricmp(br->background_name, fname))
		strncpy (br->background_name,fname, sizeof(br->background_name));

	if (!stricmp(fname, "brief02h.pcx") && baldguy_cheat)
		if ( bald_guy_load(new_baldguy_pcx, &br->background, BM_LINEAR, gr_palette) == 0)
			return 0;

	if ((pcx_error = pcx_read_bitmap(fname, &br->background, BM_LINEAR, gr_palette))!=PCX_ERROR_NONE)
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",fname, pcx_errormsg(pcx_error), pcx_error);

	show_fullscr(&br->background);

	gr_palette_load(gr_palette);

	set_briefing_fontcolor();

	MALLOC(br->screen, briefing_screen, 1);
	if (!br->screen)
		return 0;
	
	memcpy(br->screen, &Briefing_screens[br->cur_screen], sizeof(briefing_screen));
	br->screen->text_ulx = rescale_x(br->screen->text_ulx);
	br->screen->text_uly = rescale_y(br->screen->text_uly);
	br->screen->text_width = rescale_x(br->screen->text_width);
	br->screen->text_height = rescale_y(br->screen->text_height);
	init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
	
	return 1;
}

void free_briefing_screen(briefing *br)
{
	if (br->robot_canv != NULL)
		d_free(br->robot_canv);
	
	if (br->screen != NULL)
		d_free(br->screen);
	
	if (br->background.bm_data != NULL)
		gr_free_bitmap_data (&br->background);
}



int new_briefing_screen(briefing *br, int first)
{
	br->new_screen = 0;
	
	if (!first)
		br->cur_screen++;
	
	while ((br->cur_screen < MAX_BRIEFING_SCREEN) && (Briefing_screens[br->cur_screen].level_num != br->level_num))
	{
		br->cur_screen++;
		if ((br->cur_screen == MAX_BRIEFING_SCREEN) && (br->level_num == 0))
		{
			// Showed the pre-game briefing, now show level 1 briefing
			br->level_num++;
			br->cur_screen = 0;
		}
	}
	
	if (br->cur_screen == MAX_BRIEFING_SCREEN)
		return 0;		// finished

	if (!load_briefing_screen(br, Briefing_screens_LH[br->cur_screen].bs_name))
		return 0;

	br->message = get_briefing_message(br, Briefing_screens[br->cur_screen].message_num);
	
	if (br->message==NULL)
		return 0;
	
	Current_color = 0;
	br->streamcount = 0;
	br->tab_stop = 0;
	br->flashing_cursor = 0;
	br->new_page = 0;
	br->start_time = 0;
	br->delay_count = KEY_DELAY_DEFAULT;
	br->robot_num = -1;
	br->bitmap_name[0] = 0;
	br->guy_bitmap_show = 0;
	br->prev_ch = -1;
	
	return 1;
}


//-----------------------------------------------------------------------------
void title_save_game()
{
	grs_canvas * save_canv;
	grs_canvas * save_canv_data;
	grs_font * save_font;
	ubyte palette[768];
	
	if ( Next_level_num == 0 ) return;
	
	save_canv = grd_curcanv;
	save_font = grd_curcanv->cv_font;
	
	save_canv_data = gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h );
	gr_set_current_canvas(save_canv_data);
	gr_ubitmap(0,0,&save_canv->cv_bitmap);
	gr_set_current_canvas(save_canv);
	gr_clear_canvas(gr_find_closest_color_current( 0, 0, 0));
	gr_palette_read( palette );
	gr_palette_load( gr_palette );
#ifndef SHAREWARE
	state_save_all(1, 0);
#endif
	
	gr_set_current_canvas(save_canv);
	gr_ubitmap(0,0,&save_canv_data->cv_bitmap);
	gr_palette_load( palette );
	gr_set_curfont(save_font);
}

//-----------------------------------------------------------------------------
int briefing_handler(window *wind, d_event *event, briefing *br)
{
	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
		case EVENT_WINDOW_DEACTIVATED:
			key_flush();
			break;
			
		case EVENT_MOUSE_BUTTON_DOWN:
			if (mouse_get_button(event) == 0)
			{
				if (br->new_screen)
				{
					if (!new_briefing_screen(br, 0))
					{
						window_close(wind);
						return 1;
					}
				}
				else if (br->new_page)
					init_new_page(br);
				else
					br->delay_count = 0;

				return 1;
			}
			break;
			
		case EVENT_KEY_COMMAND:
		{
			int key = ((d_event_keycommand *)event)->keycode;
			
			switch (key)
			{
				case KEY_ALTED+KEY_F2:
					title_save_game();
					return 1;
					
				case KEY_PRINT_SCREEN:
					save_screen_shot(0);
					return 1;
					
#ifndef NDEBUG
				case KEY_BACKSP:
					Int3();
					return 1;
#endif

				case KEY_ESC:
					window_close(wind);
					return 1;
					
				case KEY_SPACEBAR:
				case KEY_ENTER:
					br->delay_count = 0;
					// fall through
					
				default:
					if (br->new_screen)
					{
						if (!new_briefing_screen(br, 0))
						{
							window_close(wind);
							return 1;
						}
					}
					else if (br->new_page)
						init_new_page(br);
					break;
			}
			break;
		}

		case EVENT_IDLE:
			timer_delay2(50);
			
			if (!(br->new_screen || br->new_page))
				while (!briefing_process_char(br) && !br->delay_count)
				{
					check_text_pos(br);
					if (br->new_page)
						break;
				}
			check_text_pos(br);
			break;

		case EVENT_WINDOW_DRAW:
			if (br->background.bm_data)
				show_fullscr(&br->background);

			if (br->guy_bitmap_show)
				show_briefing_bitmap(&br->guy_bitmap);
			if (br->bitmap_name[0] != 0)
				show_animated_bitmap(br);
			if (br->robot_num != -1)
				show_spinning_robot_frame(br, br->robot_num);

			gr_set_curfont( GAME_FONT );
			
			Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
			gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
			redraw_messagestream(br->messagestream, br->streamcount);
			
			if (br->new_page || br->new_screen)
				flash_cursor(br, br->flashing_cursor);
			else if (br->flashing_cursor)
				gr_printf(br->text_x, br->text_y, "_");
			break;

		case EVENT_WINDOW_CLOSE:
			free_briefing_screen(br);
			d_free(br->text);
			d_free(br);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void do_briefing_screens(char *filename, int level_num)
{
	briefing *br;
	window *wind;

	if (!filename || !*filename)
		return;

	MALLOC(br, briefing, 1);
	if (!br)
		return;

	briefing_init(br, level_num);
	
	if (!load_screen_text(filename, &br->text))
	{
		d_free(br);
		return;
	}
	
	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))briefing_handler, br);
	if (!wind)
	{
		d_free(br->text);
		d_free(br);
		return;
	}

	songs_play_song( SONG_BRIEFING, 1 );

	set_screen_mode( SCREEN_MENU );
	gr_set_current_canvas(NULL);

	if (!new_briefing_screen(br, 1))
	{
		window_close(wind);
		return;
	}

	// Stay where we are in the stack frame until briefing done
	// Too complicated otherwise
	while (window_exists(wind))
		event_process();
}

void do_end_briefing_screens(char *filename)
{
	int level_num_screen = Current_level_num, showorder = 0;
	
	if (!strlen(filename))
		return; // no filename, no ending

	if (stricmp(filename, BIMD1_ENDING_FILE_OEM) == 0)
	{
		songs_play_song( SONG_ENDGAME, 0 );
		level_num_screen = ENDING_LEVEL_NUM_OEMSHARE;
		showorder = 1;
	}
	else if (stricmp(filename, BIMD1_ENDING_FILE_SHARE) == 0)
	{
		songs_play_song( SONG_BRIEFING, 1 );
		level_num_screen = ENDING_LEVEL_NUM_OEMSHARE;
		showorder = 1;
	}
	else
	{
		songs_play_song( SONG_ENDGAME, 0 );
		level_num_screen = ENDING_LEVEL_NUM_REGISTER;
	}

	do_briefing_screens(filename, level_num_screen);
	if (showorder)
		show_order_form();
}
