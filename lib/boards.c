/*
 * All the boards in the library
 *
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "fmcadc-lib.h"
#include "fmcadc-lib-int.h"

#define FMCADC_ZIO_TRG_MASK (1LL << FMCADC_CONF_TRG_SOURCE) |      \
			    (1LL << FMCADC_CONF_TRG_SOURCE_CHAN) | \
			    (1LL << FMCADC_CONF_TRG_THRESHOLD) |   \
			    (1LL << FMCADC_CONF_TRG_POLARITY) |    \
			    (1LL << FMCADC_CONF_TRG_DELAY)
#define FMCADC_ZIO_ACQ_MASK (1LL << FMCADC_CONF_ACQ_N_SHOTS) |     \
			    (1LL << FMCADC_CONF_ACQ_POST_SAMP) |   \
			    (1LL << FMCADC_CONF_ACQ_PRE_SAMP) |    \
			    (1LL << FMCADC_CONF_ACQ_DECIMATION) |  \
			    (1LL << FMCADC_CONF_ACQ_FREQ_HZ) |     \
			    (1LL << FMCADC_CONF_ACQ_N_BITS)
#define FMCADC_ZIO_CHN_MASK (1LL << FMCADC_CONF_CHN_RANGE) |       \
			    (1LL << FMCADC_CONF_CHN_TERMINATION) | \
			    (1LL << FMCADC_CONF_CHN_OFFSET)
#define FMCADC_ZIO_BRD_MASK (1LL << FMCADC_CONF_BRD_STATE_MACHINE_STATUS) | \
			    (1LL << FMCADC_CONF_BRD_N_CHAN) | \
			    (1LL << FMCADC_CONF_UTC_TIMING_BASE_S) | \
			    (1LL << FMCADC_CONF_UTC_TIMING_BASE_T)

struct fmcadc_operations fa_100ms_4ch_14bit_op = {
	.open =			fmcadc_zio_open,
	.close =		fmcadc_zio_close,

	.acq_start =		fmcadc_zio_acq_start,
	.acq_poll =		fmcadc_zio_acq_poll,
	.acq_stop =		fmcadc_zio_acq_stop,

	.apply_config =		fmcadc_zio_apply_config,
	.retrieve_config =	fmcadc_zio_retrieve_config,

	.get_param =		fmcadc_zio_get_param,
	.set_param =		fmcadc_zio_set_param,

	.request_buffer =	fmcadc_zio_request_buffer,
	.fill_buffer =		fmcadc_zio_fill_buffer,
	.tstamp_buffer =	fmcadc_zio_tstamp_buffer,
	.release_buffer =	fmcadc_zio_release_buffer,
};
struct fmcadc_board_type fmcadc_100ms_4ch_14bit = {
	.name = "fmc-adc-100m14b4cha",	/* for library open() */
	.devname = "adc-100m14b",	/* for device named in /dev/zio */
	.driver_type = "zio",
	.capabilities = {
		FMCADC_ZIO_TRG_MASK,
		FMCADC_ZIO_ACQ_MASK,
		FMCADC_ZIO_CHN_MASK,
		FMCADC_ZIO_BRD_MASK,
	},
	.fa_op = &fa_100ms_4ch_14bit_op,
};

/*
 * The following array is the main entry point into the boards
 */
static const struct fmcadc_board_type *fmcadc_board_types[] = {
	&fmcadc_100ms_4ch_14bit,
	/* add new boards here */
};

static const struct fmcadc_board_type *find_board(char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fmcadc_board_types); i++)
		if (!strcmp(name, fmcadc_board_types[i]->name))
			return fmcadc_board_types[i];
	errno = ENODEV;
	return NULL;
}


/* Open should choose the buffer type (FIXME) */
struct fmcadc_dev *fmcadc_open(char *name, unsigned int dev_id,
				      unsigned long buffersize,
				      unsigned int nbuffer,
				      unsigned long flags)
{
	const struct fmcadc_board_type *b;

	b = find_board(name);
	if (!b)
		return NULL;

	return b->fa_op->open(b, dev_id, buffersize, nbuffer, flags);
}

#define FMCADC_PATH_PATTERN "/dev/%s.%d"

/* Open by lun should lookup a database */
struct fmcadc_dev *fmcadc_open_by_lun(char *name, int lun,
				      unsigned long buffersize,
				      unsigned int nbuffer,
				      unsigned long flags)
{
	ssize_t ret;
	char dev_id_str[8];
	char path[PATH_MAX];
	int dev_id;

	ret = snprintf(path, sizeof(path), "/dev/%s.%d",
		       "adc-100m14b" /* FIXME: this must be generic */,
		       lun);
	if (ret < 0 || ret >= sizeof(path)) {
		errno = EINVAL;
		return NULL;
	}
	ret = readlink(path, dev_id_str, sizeof(dev_id_str));
	if (sscanf(dev_id_str, "%4x", &dev_id) != 1) {
		errno = ENODEV;
		return NULL;
	}
	return fmcadc_open(name, dev_id, buffersize, nbuffer, flags);

}

int fmcadc_close(struct fmcadc_dev *dev)
{
	struct fmcadc_gid *b = (void *)dev;

	return b->board->fa_op->close(dev);
}
