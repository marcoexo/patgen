
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/inotify.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/i2c-dev.h>
#include <bcm_host.h>

#define INPUT_EVENT_PATH "/dev/uinput" // Replace X with the event number of your keyboard

// Mapping table for common characters to their key codes
const int KEYCODE_MAP[256] = {
    [0x61] = KEY_A, [0x62] = KEY_B, [0x63] = KEY_C, [0x64] = KEY_D, [0x65] = KEY_E,
    [0x66] = KEY_F, [0x67] = KEY_G, [0x68] = KEY_H, [0x69] = KEY_I, [0x6a] = KEY_J,
    [0x6b] = KEY_K, [0x6c] = KEY_L, [0x6d] = KEY_M, [0x6e] = KEY_N, [0x6f] = KEY_O,
    [0x70] = KEY_P, [0x71] = KEY_Q, [0x72] = KEY_R, [0x73] = KEY_S, [0x74] = KEY_T,
    [0x75] = KEY_U, [0x76] = KEY_V, [0x77] = KEY_W, [0x78] = KEY_X, [0x79] = KEY_Y,
    [0x7a] = KEY_Z,
    [0x30] = KEY_0, [0x31] = KEY_1, [0x32] = KEY_2, [0x33] = KEY_3, [0x34] = KEY_4,
    [0x35] = KEY_5, [0x36] = KEY_6, [0x37] = KEY_7, [0x38] = KEY_8, [0x39] = KEY_9,
    [0x20] = KEY_SPACE, [0x0A] = KEY_ENTER
};

extern char
  *__progname;                      // Program name (for error reporting)


// Filter function for scandir(), identifies possible device candidates for
// simulated keypress events (distinct from actual USB keyboard(s)).
static int filter1(const struct dirent *d) {
	if(!strncmp(d->d_name, "input", 5)) { // Name usu. 'input' + #
		// Read contents of 'name' file inside this subdirectory,
		// if it matches the program executable, that's probably
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


// Function to send keystrokes
void send_keystrokes(const char *keystrokes) {
    struct input_event event;
    int kbd_fd, keyfd1, keyfd2;
    int i;
    char buf[50];

        // Attempt to create uidev virtual keyboard
        if((keyfd1  = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) >= 0) {
                (void)ioctl(keyfd1 , UI_SET_EVBIT, EV_KEY);
 
                struct uinput_user_dev uidev;
                memset(&uidev, 0, sizeof(uidev));
                snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "testsend");
                uidev.id.bustype = BUS_USB;
                uidev.id.vendor  = 0x1;
                uidev.id.product = 0x1;
                uidev.id.version = 1;
                if(write(keyfd1 , &uidev, sizeof(uidev)) < 0)
                        perror("write failed");
                if(ioctl(keyfd1 , UI_DEV_CREATE) < 0)
                        perror("DEV_CREATE failed");
 
		printf("device created ok\n");
        }

	struct dirent **namelist;
	int             n;
	char            evName[100] = "";

	if((n = scandir("/sys/devices/virtual/input",
	  &namelist, filter1, NULL)) > 0) {
		// Got a list of device(s).  In theory there should
		// be only one that makes it through the filter (name
		// matches program name)...if there's multiples, only
		// the first is used.  (namelist can then be freed)
		char path[100];
		sprintf(path, "/sys/devices/virtual/input/%s",
		  namelist[0]->d_name);

printf("%s\n",path);

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
		// This is NOT guaranteed to be the program name, but if the
		// above method fails for some reason, this may be
		// adequate.  If there's a USB keyboard attached at
		// boot, it usually instantiates in /dev/input before
		// program, so even if it's then removed, the index
		// assigned to this program  stays put...thus the last
		// index mmmmight be what we need.
		struct stat st;
		for(i=99; i>=0; i--) {
			sprintf(buf, "/dev/input/event%d", i);
			if(!stat(buf, &st)) break; // last valid device
		}
		strcpy(evName, (i >= 0) ? buf : "/dev/input/event0");
	}

	printf("evname : %s\n",evName);

	keyfd2 = open(evName, O_WRONLY | O_NONBLOCK);
	kbd_fd = (keyfd2 >= 0) ? keyfd2 : keyfd1;


    // Set up the events for key presses and releases
    memset(&event, 0, sizeof(event));

    // Get the current time for the events
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Send key presses for each character in the keystrokes string
    for (i = 0; i < strlen(keystrokes); i++) {
    
        char ch = keystrokes[i];
        int keycode = KEYCODE_MAP[ch];
        if (keycode == 0) {
            fprintf(stderr, "Unsupported character: %c\n", ch);
            continue;
        }



    // Initialize the event structure
        event.type = EV_KEY;
        event.code = keycode;
        event.value = 1; // Key press event

        // Set the timestamp for the event
        event.time.tv_sec = tv.tv_sec;
        event.time.tv_usec = tv.tv_usec;

        // Send the key press event
        if (write(kbd_fd, &event, sizeof(event)) == -1) {
            perror("Error sending key press event");
            close(kbd_fd);
            exit(EXIT_FAILURE);
        }

 


        // Initialize the event structure for key release
        event.type = EV_KEY;
        event.code = keycode;
        event.value = 0; // Key release event

        // Set the timestamp for the release event
        event.time.tv_sec = tv.tv_sec;
        event.time.tv_usec = tv.tv_usec;

        // Send the key release event
        if (write(kbd_fd, &event, sizeof(event)) == -1) {
            perror("Error sending key release event");
            close(kbd_fd);
            exit(EXIT_FAILURE);
        }

        // Send synchronization event (to indicate end of key events)
        event.type = EV_SYN;
        event.code = SYN_REPORT;
        event.value = 0;

        // Send the synchronization event
        if (write(kbd_fd, &event, sizeof(event)) == -1) {
            perror("Error sending synchronization event");
            close(kbd_fd);
            exit(EXIT_FAILURE);
        } 

printf("done %d --\n",keystrokes[i]);
    }

    // Close the keyboard device
    close(kbd_fd);
}


int main() {
    // Example usage of send_keystrokes function
    send_keystrokes("1g\n");

    return 0;
}

