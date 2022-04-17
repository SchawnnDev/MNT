#ifndef __DARBOUX_H__
#define __DARBOUX_H__

#include "type.h"

#define EPSILON .01

// Acceder aux variables du main.c
extern int rank, size;

mnt *darboux(const mnt *restrict m);

#endif
