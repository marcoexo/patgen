/* Demo of how to emulate key presses 
(C) P.J.Onion 2014
No Warantee of any kind.  
Running this may cause the destruction
of the universe!  You have been warned !
*/



#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>


bool pressKeys( void )
{
    static int keyboardFd = -1;
    int rd,n;
    bool ret = false;

    DIR *dirp;
    struct dirent *dp;
    regex_t kbd;

    char fullPath[1024];
    static char *dirName = "/dev/input/by-id";
    
    int result;
    struct input_event forcedKey,event;


    // Send ls<ret>
    uint16_t  // keys[] = {KEY_L,KEY_S,KEY_ENTER,0}; 

keys[] = {0x6a,0x6a,0};

    int index;

    /* Find the device with a name ending in "event-kbd" */

    if(regcomp(&kbd,"event-kbd",0)!=0)
    {
        printf("regcomp for kbd failed\n");
        return false;

    }
    if ((dirp = opendir(dirName)) == NULL) {
        perror("couldn't open '/dev/input/by-id'");
        return false;
    }


    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) 
        {
            printf("readdir (%s)\n",dp->d_name);
            if(regexec (&kbd, dp->d_name, 0, NULL, 0) == 0)
            {
                printf("match for the kbd = %s\n",dp->d_name);
                sprintf(fullPath,"%s/%s",dirName,dp->d_name);
                keyboardFd = open(fullPath,O_WRONLY | O_NONBLOCK);
                printf("%s Fd = %d\n",fullPath,keyboardFd);
                printf("Getting exclusive access: ");
                result = ioctl(keyboardFd, EVIOCGRAB, 1);
                printf("%s\n", (result == 0) ? "SUCCESS" : "FAILURE");

            }


        }
    } while (dp != NULL);

    closedir(dirp);


    regfree(&kbd);


    /* Now write some key press and key release events to the device */


    index = 0;
    while(keys[index] != 0)
    {
    
        forcedKey.type = EV_KEY;
        forcedKey.value = 1;    // Press
        forcedKey.code = keys[index];
        gettimeofday(&forcedKey.time,NULL);
            
        n = write(keyboardFd,&forcedKey,sizeof(struct input_event));
        printf("n=%d\n",n);

        forcedKey.type = EV_KEY;
        forcedKey.value = 0 ;   // Release
        forcedKey.code = keys[index];
        gettimeofday(&forcedKey.time,NULL);       
             
        n = write(keyboardFd,&forcedKey,sizeof(struct input_event));
        printf("n=%d\n",n);

        // Send synchronization event (to indicate end of key events)
        event.type = EV_SYN;
        event.code = SYN_REPORT;
        event.value = 0;

        // Send the synchronization event
        if (write(keyboardFd, &event, sizeof(event)) == -1) {
            perror("Error sending synchronization event");
            close(keyboardFd);
            return(false);
        } 

    
        index += 1;
    }


    close(keyboardFd);

    return(true);

}




int main(int argc,char **argv)
{
    pressKeys();
    return(1);
}
