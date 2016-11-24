/*
 * Copyright CERN 2013, GNU GPL 2 or later.
 * Author: Alessandro Rubini
 */

#include "fmcadc-lib.h"

/* We currently do nothing in init/exit. We might check /proc/meminfo... */
int fmcadc_init(void)
{
	return 0;
}

void fmcadc_exit(void)
{
	return;
}
