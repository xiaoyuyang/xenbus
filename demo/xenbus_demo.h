#ifndef XENBUS_DEMO_H
#define XENBUS_DEMO_H

#include<asm/ioctl.h>

//from svm.h
#define SVM_GET_DOMID  _IO(101, 0)
#define SVM_ALLOC_PAGE _IO(101, 1)
#define SVM_GRANT_RING _IOW(101, 2, int)

//from xa.h
#define XA_DOMAIN_NUMBER _IOR(100, 0, int)
#define XA_WATCH		 _IO(100, 1)
#define XA_MAP_RING		 _IO(100, 2)

const int SERVER_PORT(12345);
const int BACKLOG(30);

#endif
