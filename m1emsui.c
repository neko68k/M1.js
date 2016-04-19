
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
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

#include <emscripten.h>

// version check
#if M1UI_VERSION != 20
#error M1UI VERSION MISMATCH!
#endif

static int max_games;
char *rompath;
int curgame;

// find_rompath: returns if the file can be found in the ROM search path
// alters "fn" to be the full path if found so make sure it's big
static int find_rompath(char *fn)
{
	static char curpath[16384];
	char *p;
	FILE *f;
	int i;

	p = rompath;
	while (*p != '\0')
	{
		// copy the path
		i = 0;
		while (*p != ';' && *p != '\0')
		{
			curpath[i++] = *p++;
		}
		curpath[i] = '\0';

		// p now points to the semicolon, skip that for next iteration
		p++;

		strcat(curpath, "/");	// path separator
		strcat(curpath, fn);

		f = fopen(curpath, "rb");
		if (f != (FILE *)NULL)
		{
			// got it!
			fclose(f);
			m1snd_set_info_str(M1_SINF_SET_ROMPATH, curpath, 0, 0, 0);
			return 1;
		}
	}	

	return 0;
}

int findgame(char *romname){
	int gotone = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < max_games; i++)
	{
		if (!strcasecmp(romname, m1snd_get_info_str(M1_SINF_ROMNAME, i)))
		{
			printf("\nThis is a list of the ROMs required for the game \"%s\"\n", m1snd_get_info_str(M1_SINF_ROMNAME, i));
			printf("Name              Size       Checksum\n");

			for (j = 0; j < m1snd_get_info_int(M1_IINF_ROMNUM, i); j++)
			{
				printf("%-12s  %7d bytes  %08x\n",
					m1snd_get_info_str(M1_SINF_ROMFNAME, i|(j<<16)),
					m1snd_get_info_int(M1_IINF_ROMSIZE, i|(j<<16)),
					m1snd_get_info_int(M1_IINF_ROMCRC, i|(j<<16)));
			}

			//m1snd_run(M1_CMD_GAMEJMP, i);
			gotone = 1;
			break;
		}
	}

	if (!gotone)
	{
		fprintf(stderr, "Unknown/unsupported game %s\n", romname);
		return(-1);
	} else {
		return(i);
	}
}

void gamejmp(int i){
	m1snd_run(M1_CMD_GAMEJMP, i);
}


// callbacks from the core of interest to the user interface
static int m1ui_message(void *this, int message, char *txt, int iparm)
{
	int curgame;

	switch (message)
	{
		// called when switching to a new game
		case M1_MSG_SWITCHING:
			printf("\nSwitching to game %s\n", txt);
			break;

		// called to show the current game's name
		case M1_MSG_GAMENAME:
			curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			EM_ASM_({document.getElementById("romtitle").innerHTML = 'Game: '+Pointer_stringify($0);}, txt);
			//printf("Game selected: %s (%s, %s)\n", txt, m1snd_get_info_str(M1_SINF_MAKER, curgame), m1snd_get_info_str(M1_SINF_YEAR, curgame));
			break;

		// called to show the driver's name
		case M1_MSG_DRIVERNAME:
			EM_ASM_({document.getElementById("romdriver").innerHTML = 'Driver: '+Pointer_stringify($0);}, txt);				
			//printf("Driver: %s\n", txt);
			break;

		// called to show the hardware description
		case M1_MSG_HARDWAREDESC:
			EM_ASM_({document.getElementById("romhardware").innerHTML = 'Hardware: '+Pointer_stringify($0);}, txt);
			//printf("Hardware: %s\n", txt);
			break;

		// called when ROM loading fails for a game
		case M1_MSG_ROMLOADERR:
			EM_ASM_({document.getElementById("romtitle").innerHTML = 'Game: '+Pointer_stringify($0);}, txt);
			//printf("ROM load error, bailing\n");
			exit(-1);
			break;

		// called when a song is (re)triggered
		case M1_MSG_STARTINGSONG:
			curgame = m1snd_get_info_int(M1_IINF_CURGAME, 0);
			if (m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | curgame) == (char *)NULL)
			{
				EM_ASM_({document.getElementById("trktitle").innerHTML = 'Track: '+ $0;}, iparm);
				//printf("Starting song #%d\n", iparm);
			}
			else
			{
				int i;
				char *rawname, transname[512];

				#if HAVE_ICONV
				rawname = m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | curgame);
//				printf("rawname = [%s]\n", rawname);
				memset(transname, 0, 512);
				char_conv(rawname, transname, 512);
				#else	// pass-through
				rawname = m1snd_get_info_str(M1_SINF_TRKNAME, (iparm<<16) | curgame);
				strcpy(transname, rawname);
				#endif

				//printf("Starting song #%d: %s\n", iparm, transname);
				EM_ASM_({document.getElementById("trktitle").innerHTML = 'Track: '+ $0 +' - ' + Pointer_stringify($1);}, iparm, transname);

				if (m1snd_get_info_int(M1_IINF_NUMEXTRAS, (iparm<<16) | curgame) > 0)
				{
					for (i = 0; i < m1snd_get_info_int(M1_IINF_NUMEXTRAS, (iparm<<16) | curgame); i++)
					{
						#if HAVE_ICONV
						rawname = m1snd_get_info_str_ex(M1_SINF_EX_EXTRA, curgame, iparm, i);
						memset(transname, 0, 512);
						char_conv(rawname, transname, 512);
						//printf("%s\n", transname);
						//#else
						//printf("%s\n", m1snd_get_info_str_ex(M1_SINF_EX_EXTRA, curgame, iparm, i));
						#endif
					}
					printf("\n");
				}
			}
			break;

		// called if a hardware error occurs
		case M1_MSG_HARDWAREERROR:
			m1snd_shutdown();

			//free(rompath);
			//free(wavpath);

			exit(-1);
			break;

		// called when the hardware begins booting
		case M1_MSG_BOOTING:
			printf("\nBooting hardware, please wait...");
			break;

		// called when the hardware is finished booting and is ready to play
		case M1_MSG_BOOTFINISHED:
			//printf("ready!\n\n");
			//printf("Press ESC to exit, + for next song, - for previous, 0 to restart current,\n");
			//printf("                   * for next game, / for previous, space to pause/unpause\n\n");

			#if 0
			if (1)
			{
				int numst, st, ch;

				numst = m1snd_get_info_int(M1_IINF_NUMSTREAMS, 0);
				printf("%d streams\n", numst);

				for (st = 0; st < numst; st++)
				{
					for (ch = 0; ch < m1snd_get_info_int(M1_IINF_NUMCHANS, st); ch++)
					{
						printf("st %02d ch %02d: [%s] [vol %d] [pan %d]\n", st, ch, 
							m1snd_get_info_str(M1_SINF_CHANNAME, st<<16|ch),
							m1snd_get_info_int(M1_IINF_CHANLEVEL, st<<16|ch),
							m1snd_get_info_int(M1_IINF_CHANPAN, st<<16|ch));
					}
				}
			}
			#endif
			break;

		// called when there's been at least 2 seconds of silence
		case M1_MSG_SILENCE:
			break;

		// called to let the UI do vu meters/spectrum analyzers/etc
		// txt = pointer to the frame's interleaved 16-bit stereo wave data
		// iparm = number of samples (bytes / 4)
		case M1_MSG_WAVEDATA:
			break;

		case M1_MSG_MATCHPATH:
			return find_rompath(txt);
			break;

		case M1_MSG_GETWAVPATH:
			{
				int song = m1snd_get_info_int(M1_IINF_CURCMD, 0);
				int game = m1snd_get_info_int(M1_IINF_CURGAME, 0); 

				sprintf(txt, "%s%s%04d.wav", "/wav/", m1snd_get_info_str(M1_SINF_ROMNAME, game), song);
			}
			break;

		case M1_MSG_ERROR:
			printf("%s\n", txt);
			break;
	}

	return 0;
}

void mainLoop(){
	int current = m1snd_get_info_int(M1_IINF_CURCMD, 0);
	int trklen = m1snd_get_info_int(M1_IINF_TRKLENGTH, (current<<16) | curgame);
	int curtime = m1snd_get_info_int(M1_IINF_CURTIME, 0);

	if(trklen == -1)
		trklen = 18000;	// five minutes
	
	if (curtime >= trklen){
		EM_ASM(_nextTrack());
	}
	EM_ASM_({
			var tmp;
			var seconds = $0/60;
			var minutes = seconds / 60;
			var trkmins = parseInt($1/60/60, 10);
			var trksecs = parseInt($1%60,10);

			minutes = parseInt(minutes, 10);
			seconds = parseInt(seconds - (minutes*60),10);

			if($1 % 60 < 10){
				tmp = ':0';
			} else {
				tmp = ':';
			}

			if(seconds < 10)
			{
				document.getElementById('time').innerHTML = 'Time: ' + minutes + ':0' + seconds + '/' + trkmins+tmp+trksecs;
			} else {
				document.getElementById('time').innerHTML = 'Time: ' + minutes + ':' + seconds + '/' + trkmins+tmp+trksecs;
			}
		}, curtime, trklen);
	m1snd_run(M1_CMD_IDLE, 0);
//	printf("Hit main loop.\n");
}

/*{
int seconds = curtime / 60;
int minutes = seconds / 60;
seconds -= minutes * 60;

if (trklen % 60 < 10)
	tmp = ":0";
else
	tmp = ":";
if (seconds < 10) {

	playTime.setText("Time: " + minutes + ":0"
			+ seconds + "/" + NDKBridge.songLen
			/ 60 + tmp + NDKBridge.songLen % 60);
} else
	playTime.setText("Time: " + minutes + ":"
			+ seconds + "/" + NDKBridge.songLen
			/ 60 + tmp + NDKBridge.songLen % 60);
}*/

int main(int argc, char *argv[])
{
	m1snd_setoption(M1_OPT_RETRIGGER, 0);
	m1snd_setoption(M1_OPT_WAVELOG, 0);
	m1snd_setoption(M1_OPT_NORMALIZE, 1);
	m1snd_setoption(M1_OPT_LANGUAGE, M1_LANG_EN);
	m1snd_setoption(M1_OPT_RESETNORMALIZE, 0);
	m1snd_setoption(M1_OPT_INTERNALSND, 1);

	m1snd_init(NULL, m1ui_message);
	max_games = m1snd_get_info_int(M1_IINF_TOTALGAMES, 0);

	printf("\nM1: arcade video and pinball sound emu by R. Belmont\n");
	printf("Core ver %s, CUI ver 2.4\n", m1snd_get_info_str(M1_SINF_COREVERSION, 0)); 
	printf("Copyright (c) 2001-2010.  All Rights Reserved.\n\n");

	//printf("No config file found, using defaults\n");
	rompath = (char *)malloc(512);
	strcpy(rompath, "roms;");	// default rompath
	//wavpath = (char *)malloc(512);
	//strcpy(wavpath, "wave;");

	m1snd_set_info_str(M1_SINF_SET_ROMPATH, rompath, 0, 0, 0);

	curgame = findgame(argv[1]);
	gamejmp(curgame);

	emscripten_set_main_loop(mainLoop, 0, true);

}

void nextTrack(){
	int current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	if (current < m1snd_get_info_int(M1_IINF_MAXSONG, 0))
	{
		m1snd_run(M1_CMD_SONGJMP, current+1);
	}
}

void prevTrack(){
	int current = m1snd_get_info_int(M1_IINF_CURSONG, 0);
	if (current > m1snd_get_info_int(M1_IINF_MINSONG, 0))
	{
		m1snd_run(M1_CMD_SONGJMP, current-1);
	}
}

void setLang(int lang){
	m1snd_setoption(M1_OPT_LANGUAGE, lang);
}






