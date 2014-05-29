#ifndef XA_H
#define XA_H

#include<asm/ioctl.h>

#define XA_DOMAIN_NUMBER _IOR(100, 0, int)
#define XA_WATCH		 _IO(100, 1)
#define XA_MAP_RING		 _IO(100, 2)
#define XA_SET_EVTCHN	 _IOR(100, 3, int)

#endif
