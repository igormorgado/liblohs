/* PROGRAM:	liblohs.so
 * PURPOSE:	Specify all functions in lohs protocol
 * AUTHOR:	Igor Morgado <igor@void.com.br>
 * DATE:	2003-07-18
 * REVISED:	$Log: lohs.c,v $
 * REVISED:	Revision 1.2  2003/10/17 16:34:28  imorgado
 * REVISED:	printf changed to fprintf
 * REVISED:	
 * REVISED:	Revision 1.1.1.1  2003/10/14 18:37:21  imorgado
 * REVISED:	Initial import.
 * REVISED:	
 * Copyright 2003
 * Redistributable under the terms of the GNU General Public Licence (GPL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "global.h"

/* ASCII Signals */
#define 	ACK	6
#define 	NACK	21
#define		DLE	16
#define		STX	2
#define		ETX	3

/* LOHS COMMANDS */
#define		LOHS_CMD_RESET			"\x06\x00"
#define		LOHS_CMD_START_READ		"\x06\x01"
#define		LOHS_CMD_BEEP			"\x06\x03"
#define		LOHS_CMD_ENABLE_COMM_TEST	"\x06\x04"
#define		LOHS_CMD_DISABLE_COMM_TEST	"\x06\x05"
#define		LOHS_CMD_CLOCK			"\x07\x08"
#define		LOHS_CMD_SHOW_VERSION		"\x06\x0b"
#define		LOHS_CMD_BUFFER1		"\x07\x02\x01"
#define		LOHS_CMD_BUFFER2		"\x07\x02\x02"
#define		LOHS_CMD_READ_TIME1		"\x07\x06\x01"
#define		LOHS_CMD_READ_TIME2		"\x07\x06\x02"
#define		LOHS_CMD_XOR_CHECK		"\x07\x07\x00"
#define		LOHS_CMD_DISPLAY_TEXT		"\x07\x09\x40"
#define		LOHS_CMD_CRC_CHECK		"\x07\x07\x01"
#define		LOHS_CMD_XONXOFF		"\x07\x0a\x00"
#define		LOHS_CMD_RTSCTS			"\x07\x0a\x01"

/* LOHS STATUS */
#define		LOHS_STATUS_RAM_ERROR		0x10
#define		LOHS_STATUS_WAITING		0x11
#define		LOHS_STATUS_WRONG_READ		0x12
#define		LOHS_STATUS_WRONG_CLOCK		0x13
#define		LOHS_STATUS_OK			0x14
#define		LOHS_STATUS_ERROR		0x15

/************************************* 
 * 
 * External declarations 
 *
 ************************************/
extern uint16 BlockCRC16(char *block, uint16 len);


/* Global variables 
 * TODO: Expurge all global variables
 * 	and substitute everything for
 * 	professional programming
 */
struct termios oldtio;

/*************************************
 * 
 * Support functions 
 *
 * open_port: Open a serial port
 * set_serialline: Set serial line port
 * strTX: Transmits data to other peer
 * 	add CRC16 Cypher
 * strRX: Read N bytes from the peer 
 * verify_CRC16: Check CRC16 in string
 * 	based in LOHS Protocol
 *
 ************************************/

int open_port(const char *port)
{
	int fd; 
	
	fd = open (port, O_RDWR | O_NOCTTY);

	if(errno != 0)
		perror ("open port: Unable to open device"), exit(EXIT_FAILURE);
	else fcntl (fd, F_SETFL, 0);
	
	return (fd);
}

int close_port(int fd) 
{
	if(fd)
		close(fd);

	return TRUE;
}

int set_serialline(int fd,
		const char *bauds, 
		const char *flowcontrol, 
		const char *databits,
		const char *stopbits)
{
/* As defined in termios.h
 * BAUDRATE: B0, B2400, B4800, B9600, B19200, B38400, B57600
 * FLOWCONTROL: CRTSCTS (unset bit to XONXOFF)
 * DATABITS: CS7, CS8
 * STOPBITS: CSTOPB (unset bit to one stop bits)
 * PARENB: Parity enable (unset bit to no parity)
 */
	struct termios newtio;

	tcgetattr (fd, &oldtio);
        memset(&newtio, '0', sizeof(newtio));
	
	/* OLD STYLE
	newtio.c_cflag = B9600 | CRTSCTS | CS8 | CSTOPB | CLOCAL | CREAD;
	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0; */

	newtio.c_cflag = B9600 | CRTSCTS | CS8 | CSTOPB | CLOCAL | CREAD;
	newtio.c_iflag = oldtio.c_iflag;
	newtio.c_oflag = oldtio.c_oflag;
	newtio.c_lflag = oldtio.c_lflag;
	
	/* Clean the line and activate settings */
	tcsetattr(fd, TCSANOW, &newtio);
	tcflush(fd, TCIFLUSH);

	return TRUE;
}

int reset_serialline(int fd) 
{
	tcsetattr(fd, TCSANOW, &oldtio);

	return TRUE;
}

int strTX(int fd, const char *mesg)
{
	uint16 		blk;
	int 		mesg_size;
	char 		*blocked_mesg;

	mesg_size = strlen(mesg);
	
	if((blocked_mesg = (char *)malloc(mesg_size + 3)) == NULL) {
		perror("strTX: malloc()");
		return (FALSE);
	}

	strncpy(blocked_mesg, mesg, mesg_size);
	
	/* We have to add CRC16 block to the message */
	blk = BlockCRC16((char *)mesg, mesg_size);
	blocked_mesg[mesg_size]  =(byte) (blk >> 8) & 0xff;
	blocked_mesg[mesg_size+1]=(byte)  blk       & 0xff;
	blocked_mesg[mesg_size+2]='\0';
	
	if(write (fd, blocked_mesg, mesg_size + 3) < 0) {
		perror ("write() failed\n");
		free(blocked_mesg);
		return (FALSE);
	}

	free(blocked_mesg);
	return TRUE;
}

char *strRX(int fd, int nbytes)
{
	struct 	timeval tv;
	fd_set 	fds;
	char 	*mesg;
	int 	retval;
	
	if((mesg = (char *)malloc(nbytes)) == NULL) {
		perror("strRX: malloc()");
		return (NULL);
	}

	FD_ZERO(&fds);
        FD_SET(fd, &fds);
                                                                                                    
        tv.tv_sec = 5;
        tv.tv_usec = 0;
                                                                                                    
        retval = select(fd+1, &fds, NULL, NULL, &tv);
                                                                                                    
        if (retval == -1) {
                perror("strRX: select()");
		free(mesg);
		return(NULL);
	}
        else if (retval) {
		if(read (fd, mesg, nbytes) < 0) {
			perror("strRX: read()");
			free(mesg);
			return(NULL);
		}
		return mesg;
	}
        else {
		perror("strRX: read(): Timed Out\n");
		free(mesg);
		return(NULL);
        }                                                                                      
}

int verify_CRC16(const char *mesg) 
{
	uint16 	blk, mesg_size;
	char	*raw_mesg, *blocked_mesg;

	if((raw_mesg = (char *)malloc(strlen(mesg))) == NULL) {
		perror("verify_CRC16: malloc()");
		return (FALSE);
	}
	if((blocked_mesg = (char *)malloc(strlen(mesg+3))) == NULL) {
		perror("verify_CRC16: malloc()");
		return (FALSE);
	}
	
	strncpy(raw_mesg, mesg, strlen(mesg)-2);

	mesg_size=strlen(raw_mesg);
	blk=BlockCRC16(raw_mesg, mesg_size);
        blocked_mesg[mesg_size]  =(byte) (blk >> 8) & 0xff;
        blocked_mesg[mesg_size+1]=(byte)  blk       & 0xff;
        blocked_mesg[mesg_size+2]='\0';

	if (strcmp(blocked_mesg, mesg)) {
		free(blocked_mesg); free(raw_mesg);
		return TRUE;
	} else {
		free(blocked_mesg); free(raw_mesg);
		return FALSE;
	}
}

/*********************************
 * 
 * Protocol commands 
 *
 ********************************/

int LOHS_cancel_wait(int fd)
{
	strTX(fd, '\0');
	return TRUE;
}

int LOHS_set_default(int fd)
{
	LOHS_reset(fd);
	LOHS_verify_type(fd, 1); 	// CRC16 Check
	LOHS_read_times(fd, 1); 	// Single read
	LOHS_flow_control(fd, 1);	// Hardware control
	LOHS_clocks_number(fd, 0); 	// Disable clocks verify

	return TRUE;
}

int LOHS_reset(int fd)
{
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_RESET, ETX);

	if(strTX(fd, cmd) == FALSE) {
		perror("LOHS_reset: strTX");
		return (FALSE);
	}

	if((p = strRX(fd, 1)) == NULL)
		return FALSE;
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}

	free(p);
	sleep (1);
	return TRUE;
}

int LOHS_start_read(int fd)
{
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_START_READ, ETX);
	
	if(strTX(fd, cmd) == FALSE) {
		perror("LOHS_start_read: strTX");
		return (FALSE);
	}

	if((p = strRX(fd, 1)) == NULL) 
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}
	
	free(p);
	return TRUE;
}

int LOHS_buffer_tx(int fd, int bnumber)
{
	char	cmd[32], *p;
	
	memset(cmd, 0, sizeof(cmd));
	
	switch(bnumber) {
		default:
		case 1:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_BUFFER1, ETX);
			strTX(fd, cmd);
			break;
		case 2:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_BUFFER2, ETX);
			strTX(fd, cmd);
			break;
	}

	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK ) {
		free(p); return (FALSE);
	}

	free(p);
	return TRUE;
}

int LOHS_beep(int fd)
{
	char	cmd[32], *p;
	
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_BEEP, ETX);

	if(strTX(fd, cmd) == FALSE) {
		perror("LOHS_beep: strTX");
		return (FALSE);
	}
	
	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return(FALSE);
	} 
	
	free(p);
	return TRUE;
}

/* 0, enable
 * 1, disable
*/
int LOHS_com_test(int fd, int value)
{
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));

	switch(value) {
		default:
		case 0:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_ENABLE_COMM_TEST, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
		case 1:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_DISABLE_COMM_TEST, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
	}

	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}
	
	free(p);
	return (TRUE);
}

/* 
 * 1, single read
 * 2, double read
*/

int LOHS_read_times(int fd, int ntimes)
{
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));

	switch(ntimes) {
		default:
		case 1:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_READ_TIME1, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
		case 2:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_READ_TIME2, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
	}

	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}

	free(p);
	return TRUE;
}

/*
 * 0, XOR check.
 * 1, CRC check.
*/

int LOHS_verify_type(int fd, int vtype)
{
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));

	switch(vtype) {
		case 0:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_XOR_CHECK, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
		case 1:
		default:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_CRC_CHECK, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
	}
	
	if((p = strRX(fd, 1)) == NULL) 
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return(FALSE);
	}

	free(p);
	return TRUE;
}

int LOHS_clocks_number(int fd, int nclock)
{
	char 	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));
	
	snprintf(cmd, sizeof(cmd), "%02x%s%02x%02x", STX, LOHS_CMD_CLOCK, nclock, ETX);

	if(strTX(fd, cmd) == FALSE)
		return (FALSE);

	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}

	free(p);
	return TRUE;
}

int LOHS_read_mask(int fd, char *mask)
{
	/* Frame format 
	 * Origin: Computer
	 * <stx><08><09><MASK1><MASK2><etx><CRC1><CRC2>
	 * Response: ACK/NACK
	 * Mask 1: 7  6  5  4  3  2  1  0
	 * Mask 2: X  X  X  X 11 10  9  8
	 */
	printf ("read_mask: Not implemented\n");

	return FALSE;
}

/*
 * 0, RTS/CTS
 * 1, XON/XOFF
*/

int LOHS_flow_control(int fd, int fcontrol)
{
	char	cmd[32], *p;
	
	memset(cmd, 0, sizeof(cmd));
	
	switch(fcontrol) {
		case 0:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_XONXOFF, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
		default:
		case 1:
			snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_RTSCTS, ETX);
			if(strTX(fd, cmd) == FALSE)
				return (FALSE);
			break;
	}
		
	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}
		
	free(p);
	return TRUE;
}

int LOHS_status(int fd)
{
	/* Reader status response:
	 * Origin: Reader 
	 * Frame format:
	 * <stx><07><F0><status><etx><CRC1><CRC2>
	 * Computer response:
	 * ACK or NACK
	 *
	 * Reader status table
	 * Bit 0: Ram error
	 * Bit 1: Not Used
	 * Bit 2: Activated, but waiting card
	 * Bit 3: Wrong read (in double read)
	 * Bit 4: Wrong clocks
	 * Bit 5: Not Used
	 * Bit 6: Not Used
	 * Bit 7: Not Used
	 */ 
	char	*p;

	if((p = strRX(fd, 7)) == NULL)
		return (LOHS_STATUS_ERROR);
	
	if(verify_CRC16(p) == FALSE) {
		if(strTX(fd, (char *)NACK) == FALSE)
			return (LOHS_STATUS_ERROR);
	} else {
		if(strTX(fd, (char *)ACK) == FALSE)
			return (LOHS_STATUS_ERROR);
	}
	
	if(p[3] & 0x00) {
		free(p); return (LOHS_STATUS_RAM_ERROR);
	} else if(p[3] & 0x02) {
		free(p); return (LOHS_STATUS_WAITING);
	} else if(p[3] & 0x03) {
		free(p); return (LOHS_STATUS_WAITING);
	} else if(p[3] & 0x04) {
		free(p); return (LOHS_STATUS_WAITING);
	} else { 
		free(p); return (LOHS_STATUS_OK);
	}

	free(p);
	return (LOHS_STATUS_ERROR);
}

char * LOHS_text_block(int fd)
{
	/* Card data from reader
	 * Origin: Reader 
	 * Frame format:
	 * <stx><size><F1><text><etx><CRC1><CRC2>
	 * Computer response:
	 * ACK or NACK
	 *
	 * Text field:
	 * Each byte transports the card block, as in:
	 * 
	 * 	FIRST CARD LINE
	 * 7  6  5  4  3  2  1  0  x  x  x  11 10
	 * 	SECOND CARD LINE
	 * 9  8  7  6  5  4  3  2  1  0	
	 */ 
	 printf ("text_block: Not implemented\n");

	 return FALSE;
}

int LOHS_display_text(int fd, const char *mesg)
{
	/* Frame format:
	 * <stx><size><09><"mesg"><etx><CRC1><CRC2>
	 * Where: size== strlen(mesg)+6 
	 */
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_DISPLAY_TEXT, ETX);

	if(strTX(fd, cmd) == FALSE)
		return (FALSE);

	if((p = strRX(fd, 1)) == NULL)
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}

	free(p);
	return (TRUE);
}

int LOHS_show_version(int fd)
{
	char	cmd[32], *p;

	memset(cmd, 0, sizeof(cmd));

	snprintf(cmd, sizeof(cmd), "%02x%s%02x", STX, LOHS_CMD_SHOW_VERSION, ETX);
	if(strTX(fd, cmd) == FALSE)
		return (FALSE);

	if((p = strRX(fd, 1)) == NULL) 
		return (FALSE);
	else if(atoi(p) != ACK) {
		free(p); return (FALSE);
	}
	strTX(fd, "\x02\x06\x0b\x03");

	free(p);
	return TRUE;
}

int LOHS_version_info(int fd)
{
	/* Read model, firmware and hardware version info
	 * Origin: Reader
	 * Frame format:
	 * <stx><09><F2><MODEL><FIRM><HARD><etx><CRC1><CRC2>
	 * Response:
	 * ACK or NACK
	 * Model: 	\x01 == 3m
	 * 		\x02 == 4
	 * Firmware:	\x34 == 3.4
	 * 		\x35 == 3.5
	 * Hardware:	\x00 == Standard
	 * 		\x01 == Double read support 
	 */
	unsigned char	*p;

	if((p = (unsigned char *)strRX(fd, 9)) == NULL)
		return (FALSE);

        if ( verify_CRC16(p) == FALSE ) {
		if(strTX(fd, (char *)NACK) == FALSE) {
			free(p); return (FALSE);
		}
        } else {
		if(strTX(fd, (char *)ACK) == FALSE) {
			free(p); return (FALSE);
		}
	}

	fprintf(stdout, "Model: ");
	if(p[3] == 0x01) fprintf(stdout, "3m\n");
	else if(p[3] == 0x02) fprintf(stdout, "4\n");
	else fprintf(stdout, "unknown\n");

	fprintf(stdout, "Firmware: ");
	if(p[4] == 0x34) fprintf(stdout, "3.4\n");
	else if(p[4] == 0x35) fprintf(stdout, "3.5\n");
	else fprintf(stdout, "unknown\n");

	fprintf(stdout, "Hardware: ");
	if(p[5] == 0x00) fprintf(stdout, "Standard\n");
	else if(p[5] == 0x01) fprintf(stdout, "Double Read Support\n");
	else fprintf(stdout, "unknown\n");
	
	printf("\n");
	
	free(p);
	return (TRUE);
}

