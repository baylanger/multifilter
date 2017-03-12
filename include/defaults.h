#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__


#define DEFAULTLOGTIME 300
#define DEFAULTCONFIGFILE "./multifilter.conf"
#define DEFAULTPIDFILE "../../log/multifilter.pid"
#define DEFAULTLOGFILE "../../log/multifilter.log"

/*
 * Highwind's default timeout to drop the external spam-filtering program
 */
#ifdef HIGHWIND
#define SERVERTIMEOUT 60
#endif

/*
 * Joe Greco's patch for Diablo (http://www.nntp.sol.net/patches/diablo/)
 */
#ifdef DIABLO
#define SERVERTIMEOUT 300
#endif


#define ERROR -1
#define OK 1
#define TRUE 1
#define FALSE 0
#define GOODBYE 0


#define ACCEPT  "335\r\n"
#define REFUSE  "435\r\n"
#define RESPONSE_LENGTH 5


#define EOA     "\r\n.\r\n"    /* End of Article */

#define EOAL    5              /* EOA Length     */

#define EOH     "\r\n\r\n"     /* End of Header  */

#define EOHL    4              /* EOA Length     */

#endif
