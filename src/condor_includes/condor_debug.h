/***************************Copyright-DO-NOT-REMOVE-THIS-LINE**
 * CONDOR Copyright Notice
 *
 * See LICENSE.TXT for additional notices and disclaimers.
 *
 * Copyright (c)1990-1998 CONDOR Team, Computer Sciences Department, 
 * University of Wisconsin-Madison, Madison, WI.  All Rights Reserved.  
 * No use of the CONDOR Software Program Source Code is authorized 
 * without the express consent of the CONDOR Team.  For more information 
 * contact: CONDOR Team, Attention: Professor Miron Livny, 
 * 7367 Computer Sciences, 1210 W. Dayton St., Madison, WI 53706-1685, 
 * (608) 262-0856 or miron@cs.wisc.edu.
 *
 * U.S. Government Rights Restrictions: Use, duplication, or disclosure 
 * by the U.S. Government is subject to restrictions as set forth in 
 * subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer 
 * Software clause at DFARS 252.227-7013 or subparagraphs (c)(1) and 
 * (2) of Commercial Computer Software-Restricted Rights at 48 CFR 
 * 52.227-19, as applicable, CONDOR Team, Attention: Professor Miron 
 * Livny, 7367 Computer Sciences, 1210 W. Dayton St., Madison, 
 * WI 53706-1685, (608) 262-0856 or miron@cs.wisc.edu.
****************************Copyright-DO-NOT-REMOVE-THIS-LINE**/
/*
  N.B. Every file which includes this header will need to contain the
  following line:

	static char *_FileName_ = __FILE__;
*/

#ifndef DEBUG_H
#define DEBUG_H

#if !defined(__STDC__) && !defined(__cplusplus)
#define const
#endif

/*
**	Definitions for flags to pass to dprintf
**  Note: this is a little confusing, since the flags specify both
**  debug levels and dprintf options (i.e., D_NOHEADER).  The debug
**  level flags use the lower order bits while the option flag(s)
**  use the higher order bit(s).  Note that D_MAXFLAGS is 32 so we
**  can store the debug level as a integer bitmask.
*/
#define D_NUMLEVELS		22
#define D_MAXFLAGS 		32
#define D_ALWAYS 		(1<<0)
#define D_SYSCALLS		(1<<1)
#define D_CKPT			(1<<2)
#define D_XDR			(1<<3)
#define D_MALLOC		(1<<4)
#define D_LOAD			(1<<5)
#define D_EXPR			(1<<6)
#define D_PROC			(1<<7)
#define D_JOB			(1<<8)
#define D_MACHINE		(1<<9)
#define D_FULLDEBUG	 	(1<<10)
#define D_NFS			(1<<11)
#define D_UPDOWN        (1<<12)
#define D_AFS           (1<<13)
#define D_PREEMPT		(1<<14)
#define D_PROTOCOL		(1<<15)
#define D_PRIV			(1<<16)
#define D_TAPENET		(1<<17)
#define D_DAEMONCORE	(1<<18)
#define D_COMMAND		(1<<19)
#define D_BANDWIDTH		(1<<20)
#define D_SECONDS		(1<<21)
#define D_NOHEADER		(1<<(D_MAXFLAGS-1))
#define D_ALL			(~(0))

#if defined(__cplusplus)
extern "C" {
#endif

extern int DebugFlags;	/* Bits to look for in dprintf */
extern int Termlog;		/* Are we logging to a terminal? */
extern int (*DebugId)();		/* set header message */

#if defined(__STDC__) || defined(__cplusplus)
void dprintf_init ( int fd );
void dprintf ( int flags, char *fmt, ... );
void _condor_dprintf_va ( int flags, char* fmt, va_list args );
void _EXCEPT_ ( char *fmt, ... );
void Suicide();
void dprintf_config( char* subsys, int logfd );
#else
void config ();
char * param ();
void _EXCEPT_ ();
void dprintf ();
void dprintf_config ();
#endif

/*
**	Definition of exception macro
*/
#define EXCEPT \
	_EXCEPT_Line = __LINE__; \
	_EXCEPT_File = _FileName_; \
	_EXCEPT_Errno = errno; \
	_EXCEPT_

/*
**	Important external variables
*/
extern int		errno;
#if !( defined(LINUX) && defined(GLIBC) )
extern int		sys_nerr;
extern char		*sys_errlist[];
#endif

extern int	_EXCEPT_Line;			/* Line number of the exception    */
extern char	*_EXCEPT_File;			/* File name of the exception      */
extern int	_EXCEPT_Errno;			/* errno from most recent system call */
extern int (*_EXCEPT_Cleanup)();	/* Function to call to clean up (or NULL) */
extern void _EXCEPT_(char*, ...);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef ASSERT
#define ASSERT(cond) \
	if( !(cond) ) { EXCEPT("Assertion ERROR on (%s)",#cond); }
#endif

#ifndef TRACE
#define TRACE \
	fprintf( stderr, "TRACE at line %d in file \"%s\"\n",  __LINE__, __FILE__ );
#endif

#ifdef MALLOC_DEBUG
#define MALLOC(size) mymalloc(__FILE__,__LINE__,size)
#define CALLOC(nelem,size) mycalloc(__FILE__,__LINE__,nelem,size)
#define REALLOC(ptr,size) myrealloc(__FILE__,__LINE__,ptr,size)
#define FREE(ptr) myfree(__FILE__,__LINE__,ptr)
char    *mymalloc(), *myrealloc(), *mycalloc();
#else
#define MALLOC(size) malloc(size)
#define CALLOC(nelem,size) calloc(nelem,size)
#define REALLOC(ptr,size) realloc(ptr,size)
#define FREE(ptr) free(ptr)
#endif

#define D_RUSAGE( flags, ptr ) { \
        dprintf( flags, "(ptr)->ru_utime = %d.%06d\n", (ptr)->ru_utime.tv_sec,\
        (ptr)->ru_utime.tv_usec ); \
        dprintf( flags, "(ptr)->ru_stime = %d.%06d\n", (ptr)->ru_stime.tv_sec,\
        (ptr)->ru_stime.tv_usec ); \
}
