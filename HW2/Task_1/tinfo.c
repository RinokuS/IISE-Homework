#include <stdio.h>
#include <stdlib.h>
#include "tinfo.h"

// Allocates memory for a tinfo struct.
//
tinfo *init_tinfo() {
    tinfo *T = (tinfo *)malloc(sizeof(tinfo));
    T->in = NULL;
    T->out = NULL;
    T->section = 0;
    T->divide = 0;
    return T;
}
