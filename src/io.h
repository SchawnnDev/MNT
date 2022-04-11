#ifndef __IO_H__
#define __IO_H__

#include "type.h"

mnt *mnt_read(char *fname);
void mnt_write(mnt *m, FILE *f);
void mnt_write_lakes(mnt *m, mnt *d, FILE *f);
void mnt_compare(mnt* expected, mnt* result);

#endif
