/* PROGRAM:	liblohs.so
 * PURPOSE:	Specify all functions in lohs protocol
 * AUTHOR:	Igor Morgado <igor@void.com.br>
 * DATE:	2003-10-13
 * REVISED:	$Log: lohs.h,v $
 * REVISED:	Revision 1.1.1.1  2003/10/14 18:37:21  imorgado
 * REVISED:	Initial import.
 * REVISED:	
 * Copyright 2003
 * Redistributable under the terms of the GNU General Public Licence (GPL)
 */

extern int open_port(const char *port);
extern int close_port(int fd);
extern int set_serialline(int fd,
		const char *bauds, 
		const char *flowcontrol, 
		const char *databits,
		const char *stopbits);
extern int reset_serialline(int fd);

extern int LOHS_cancel_wait(int fd);
extern int LOHS_set_default(int fd);
extern int LOHS_reset(int fd);
extern int LOHS_start_read(int fd);
extern int LOHS_buffer_tx(int fd, int bnumber);
extern int LOHS_beep(int fd);
extern int LOHS_com_test(int fd, int value);
extern int LOHS_read_times(int fd, int ntimes);
extern int LOHS_verify_type(int fd, int vtype);
extern int LOHS_clocks_number(int fd, int nclock);
extern int LOHS_read_mask(int fd, char *mask);
extern int LOHS_flow_control(int fd, int fcontrol);
extern int LOHS_status(int fd);
extern char * LOHS_text_block(int fd);
extern int LOHS_display_text(int fd, const char *mesg);
extern int LOHS_show_version(int fd);
extern int LOHS_version_info(int fd);

