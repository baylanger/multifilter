#ifndef __SIGNAL_H__
#define __SIGNAL_H__

/* Prototypes */
void *InAlarm(void);
void *Signal(void *);

static void Sig_Reload(void);
static void Sig_Term(void);

#endif
