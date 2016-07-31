/* PROGRAM:     liblohs.so
 * PURPOSE:     Declare global types, constants, and variables
 * AUTHOR:      Igor Morgado <igor@void.com.br>
 * DATE:        2003-06-03
 * REVISED:     $Log: global.h,v $
 * REVISED:     Revision 1.1.1.1  2003/10/14 18:37:21  imorgado
 * REVISED:     Initial import.
 * REVISED:
 * Copyright 2003
 * Redistributable under the terms of the GNU General Public Licence (GPL)
 */

#ifndef _GLOBAL_H

/* Good for i386, but be careful... */
typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef char int8;
typedef short int16;
typedef long int32;

/* Just to know */
#define TRUE    1
#define FALSE   0

/* POSIX compliant source */
#define _POSIX_SOURCE 1 

#define _GLOBAL_H
#endif				/* _GLOBAL_H */
