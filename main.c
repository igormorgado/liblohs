#include <stdio.h>

#include "global.h"
#include "lohs.h"

int main(int argc, char **argv) {
	int fd;
	
	printf ("Opening port...\n");
	fd = open_port("/dev/ttyS0");
	printf ("OK!\n");

	printf ("Setting serial line...\n");
	set_serialline(fd, "B9600", "CRTSCTS", "CS8", "CSTOPB");
	printf ("OK!\n");

	printf("Send beep signal...\n");
	LOHS_beep(fd);
	printf ("OK!\n");
	
	printf("Reseting serial line...\n");
	reset_serialline(fd);
	printf ("OK!\n");
	
	printf("Closing port...\n");
	close_port(fd);
	printf ("OK!\n");

	return TRUE;
}
