#include <stdio.h>
#include <ctype.h> // Per la funzione toupper
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/input-event-codes.h>

int    keyfd1       = -1,                // /dev/uinput file descriptor
   keyfd2       = -1,                // /dev/input/eventX file descriptor
   keyfd        = -1;                // = (keyfd2 >= 0) ? keyfd2 : keyfd1;
extern char
  *__progname;                       // Program name (for error reporting)
char
  *progName;                         // Program name (for error reporting)

// Array di mappatura carattere -> codice del tasto
const int char_to_keycode[] = {
    [0] = 0, // Carattere nullo
    ['A'] = KEY_A,
    ['B'] = KEY_B,
    ['C'] = KEY_C,
    ['D'] = KEY_D,
    ['E'] = KEY_E,
    ['F'] = KEY_F,
    ['G'] = KEY_G,
    ['H'] = KEY_H,
    ['I'] = KEY_I,
    ['J'] = KEY_J,
    ['K'] = KEY_K,
    ['L'] = KEY_L,
    ['M'] = KEY_M,
    ['N'] = KEY_N,
    ['O'] = KEY_O,
    ['P'] = KEY_P,
    ['Q'] = KEY_Q,
    ['R'] = KEY_R,
    ['S'] = KEY_S,
    ['T'] = KEY_T,
    ['U'] = KEY_U,
    ['V'] = KEY_V,
    ['W'] = KEY_W,
    ['X'] = KEY_X,
    ['Y'] = KEY_Y,
    ['Z'] = KEY_Z,
    ['1'] = KEY_1,
    ['2'] = KEY_2,
    ['3'] = KEY_3,
    ['4'] = KEY_4,
    ['5'] = KEY_5,
    ['6'] = KEY_6,
    ['7'] = KEY_7,
    ['8'] = KEY_8,
    ['9'] = KEY_9,
    ['0'] = KEY_0,
};

// Some utility functions ------------------------------------------------
// Filter function for scandir(), identifies possible device candidates for
// simulated keypress events (distinct from actual USB keyboard(s)).
static int filter1(const struct dirent *d) {
	if(!strncmp(d->d_name, "input", 5)) { // Name usu. 'input' + #
		// Read contents of 'name' file inside this subdirectory,
		// if it matches the retrogame executable, that's probably
		// the device we want...
		char  filename[100], line[100];
		FILE *fp;
		sprintf(filename, "/sys/devices/virtual/input/%s/name",
		  d->d_name);
		memset(line, 0, sizeof(line));
		if((fp = fopen(filename, "r"))) {
			fgets(line, sizeof(line), fp);
			fclose(fp);
		}
		if(!strncmp(line, __progname, strlen(__progname))) return 1;
	}
	return 0;
}

// A second scandir() filter, checks for filename of 'event' + #
static int filter2(const struct dirent *d) {
	return !strncmp(d->d_name, "event", 5);
}

// Quick-n-dirty error reporter; print message, clean up and exit.
void err(char *msg) {
	printf("%s: %s.  Try 'sudo %s'.\n", progName, msg, progName);
	exit(1);
}


int main(int argc, char *argv[]) {
int 			i,fd;
char                   buf[50];      // For sundry filenames
struct input_event     keyEv, synEv; // uinput events

   progName = argv[0];             // For error reporting

    if (argc < 2) {
        printf("Nessuna stringa specificata.\n");
        return 1;
    }
    char *input_string = argv[1];


    for (int i = 0; input_string[i] != '\0'; ++i) {
        input_string[i] = toupper(input_string[i]);
    }


	// Set up uinput

	// Attempt to create uidev virtual keyboard

	if((keyfd1 = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) >= 0) {
		(void)ioctl(keyfd1, UI_SET_EVBIT, EV_KEY);
		(void)ioctl(keyfd1, UI_SET_KEYBIT, 36);
    		// Imposta UI_SET_KEYBIT per ogni tasto (da 1 a 255)
		for(i=0; i<255; i++) {
			(void)ioctl(keyfd1, UI_SET_KEYBIT, i);
		}


		struct uinput_user_dev uidev;
		memset(&uidev, 0, sizeof(uidev));
		snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "sendkeys");
		uidev.id.bustype = BUS_USB;
		uidev.id.vendor  = 0x1;
		uidev.id.product = 0x1;
		uidev.id.version = 1;
		if(write(keyfd1, &uidev, sizeof(uidev)) < 0)
			err("write failed");
		if(ioctl(keyfd1, UI_DEV_CREATE) < 0)
			err("DEV_CREATE failed");

// printf("%s: uidev init OK keyfd1 \n", __progname);

	}

	// SDL2 (used by some newer emulators) wants /dev/input/eventX
	// instead -- BUT -- this only exists if there's a physical USB
	// keyboard attached or if the above code has run and created a
	// virtual keyboard.  On older systems this method doesn't apply,
	// events can be sent to the keyfd1 virtual keyboard above...so,
	// this code looks for an eventX device and (if present) will use
	// that as the destination for events, else fallback on keyfd1.

	// The 'X' in eventX is a unique identifier (typically a numeric
	// digit or two) for each input device, dynamically assigned as
	// USB input devices are plugged in or disconnected (or when the
	// above code runs, creating a virtual keyboard).  As it's
	// dynamically assigned, we can't rely on a fixed number -- it
	// will vary if there's a keyboard connected at startup.

	struct dirent **namelist;
	int             n;
	char            evName[100] = "";

	if((n = scandir("/sys/devices/virtual/input",
	  &namelist, filter1, NULL)) > 0) {
		// Got a list of device(s).  In theory there should
		// be only one that makes it through the filter (name
		// matches retrogame)...if there's multiples, only
		// the first is used.  (namelist can then be freed)
		char path[100];
		sprintf(path, "/sys/devices/virtual/input/%s",
		  namelist[0]->d_name);
		for(i=0; i<n; i++) free(namelist[i]);
		free(namelist);
		// Within the given device path should be a subpath with
		// the name 'eventX' (X varies), again theoretically
		// should be only one, first in list is used.
		if((n = scandir(path, &namelist, filter2, NULL)) > 0) {
			sprintf(evName, "/dev/input/%s",
			  namelist[0]->d_name);
			for(i=0; i<n; i++) free(namelist[i]);
			free(namelist);
		}
	}

	if(!evName[0]) { // Nothing found?  Use fallback method...
		// Kinda lazy skim for last item in /dev/input/event*
		// This is NOT guaranteed to be retrogame, but if the
		// above method fails for some reason, this may be
		// adequate.  If there's a USB keyboard attached at
		// boot, it usually instantiates in /dev/input before
		// retrogame, so even if it's then removed, the index
		// assigned to retrogame stays put...thus the last
		// index mmmmight be what we need.
		struct stat st;
		for(i=99; i>=0; i--) {
			sprintf(buf, "/dev/input/event%d", i);
			if(!stat(buf, &st)) break; // last valid device
		}
		strcpy(evName, (i >= 0) ? buf : "/dev/input/event0");
		// printf("evName not found, now is : %s \n", evName);

	}

	keyfd2 = open(evName, O_WRONLY | O_NONBLOCK);
	fd = keyfd  = (keyfd2 >= 0) ? keyfd2 : keyfd1;

/*
	if (keyfd2>0)
		printf("using keyfd2 from evName : %s \n", evName);
	else
	  	printf("using keyfd1 /dev/uinput : \n");
*/

	// Initialize input event structures
	memset(&keyEv, 0, sizeof(keyEv));
	keyEv.type  = EV_KEY;
	memset(&synEv, 0, sizeof(synEv));
	synEv.type  = EV_SYN;
	synEv.code  = SYN_REPORT;

	// 'fd' is now open file descriptor for issuing uinput events

{ 
{ 

{ 



// printf("received keypress");

    for (int i = 0; input_string[i] != '\0'; ++i) {
        char character = input_string[i];
        int keycode = char_to_keycode[character];
/*
        if (keycode != 0) {
            printf("Carattere: %c, Codice del tasto: %d\n", character, keycode);
        } else {
            printf("Carattere: %c, Nessun codice del tasto valido\n", character);
        }
*/
	SendKey(keyfd,keycode );
    }


	      }
	    }



	}


    return 0;
}

SendKey(int keyfd,int key) 
{
int x;
struct input_event     keyEv, synEv; // uinput events

	// Initialize input event structures
	memset(&keyEv, 0, sizeof(keyEv));
	keyEv.type  = EV_KEY;
	memset(&synEv, 0, sizeof(synEv));
	synEv.type  = EV_SYN;
	synEv.code  = SYN_REPORT;
	keyEv.code=key;


	    		for(x=1; x>= 0; x--) { // Press, release
			      keyEv.value = x;

				// if (x==1)
	        	        //	printf("%s: key press code %d\n",__progname, key);

			      write(keyfd, &keyEv, sizeof(keyEv));
			      usleep(10000); // Be slow, else client program flakes

			     //	if (x==0)
		             //   printf("%s: key release code %d\n",__progname,  key);


			      write(keyfd, &synEv, sizeof(synEv));
			      usleep(10000);
			}

}
