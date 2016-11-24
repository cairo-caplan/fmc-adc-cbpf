/*
 * Initializing and cleaning up the fmc adc library
 *
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
 */
#include <string.h>
#include "fmcadc-lib.h"
#include "fmcadc-lib-int.h"

/* * * * * * * * * * * * * * * * Utilities * * * * * * * * * * * * * * * * */
/*
 * fmcadc_strerror
 * @dev: device for which you want to know the meaning of the error
 * @errnum: error number
 */

static struct fmcadc_errors {
	int num;
	char *str;
} fmcadc_errors[] = {
	{ FMCADC_ENOP,		"Operation not supported"},
	{ FMCADC_ENOCAP,	"Capabilities not supported"},
	{ FMCADC_ENOCFG,	"Configuration type not supported"},
	{ FMCADC_ENOGET,	"Cannot get capabilities information"},
	{ FMCADC_ENOSET,	"Cannot set capabilities information"},
	{ FMCADC_ENOCHAN,	"Invalid channel"},
	{ FMCADC_ENOMASK,	"Missing configuration mask"},
	{ FMCADC_EDISABLED,	"Trigger is disabled: I/O aborted"},
	{ 0, }
};

char *fmcadc_strerror(int errnum)
{
	struct fmcadc_errors *p;

	if (errnum < __FMCADC_ERRNO_START)
		return strerror(errnum);
	for (p = fmcadc_errors; p->num; p++)
		if (p->num == errnum)
			return p->str;
	return "Unknown error code";
}

/*
 * fmcadc_get_driver_type
 * @dev: device which want to know the driver type
 */
char *fmcadc_get_driver_type(struct fmcadc_dev *dev)
{
	struct fmcadc_gid *b = (void *)dev;

	return b->board->driver_type;
}
