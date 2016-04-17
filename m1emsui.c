
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if __linux__
#include <termios.h>
#include <unistd.h>
#define HAVE_ICONV	(0)	// can use iconv and output UTF-8

extern int lnxdrv_apimode;
#endif

#if MACOS
#include <termios.h>
#include <unistd.h>
#define HAVE_ICONV	(0)
#endif

#if WIN32
#include <windows.h>
#include <conio.h>
#define HAVE_ICONV	(0)
#endif

#include "m1ui.h"

#if HAVE_ICONV
#include <iconv.h>
#endif

// version check
#if M1UI_VERSION != 20
#error M1UI VERSION MISMATCH!
#endif

static int max_games;
static int quit;
static char *rompath, *wavpath;


int main(int argc, char *argv[])
{
	m1snd_setoption(M1_OPT_RETRIGGER, 0);
	m1snd_setoption(M1_OPT_WAVELOG, 0);
	m1snd_setoption(M1_OPT_NORMALIZE, 1);
	m1snd_setoption(M1_OPT_LANGUAGE, M1_LANG_EN);
	m1snd_setoption(M1_OPT_RESETNORMALIZE, 0);

	quit = 0;
	info = 0;
	shownorm = 0;
	lastnorm = -1;

	m1snd_init(NULL, m1ui_message);
	max_games = m1snd_get_info_int(M1_IINF_TOTALGAMES, 0);

	printf("\nM1: arcade video and pinball sound emu by R. Belmont\n");
	printf("Core ver %s, CUI ver 2.4\n", m1snd_get_info_str(M1_SINF_COREVERSION, 0)); 
	printf("Copyright (c) 2001-2010.  All Rights Reserved.\n\n");

	cnf = fopen("m1.ini", "rt");
}
