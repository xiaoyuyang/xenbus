#ifndef SVM_H
#define SVM_H

#include<asm/ioctl.h>

#define SVM_GET_DOMID  _IO(101, 0)
#define SVM_ALLOC_PAGE _IO(101, 1)
#define SVM_GRANT_RING _IOW(101, 2, int)

#endif
