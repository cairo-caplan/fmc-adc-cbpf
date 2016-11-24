/*
 * Routing public functions to device-specific code
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
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fmcadc-lib.h"
#include "fmcadc-lib-int.h"

int fmcadc_acq_start(struct fmcadc_dev *dev,
			     unsigned int flags,
			     struct timeval *timeout)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->acq_start(dev, flags, timeout);
}

int fmcadc_acq_poll(struct fmcadc_dev *dev, unsigned int flags,
		    struct timeval *timeout)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->acq_poll(dev, flags, timeout);
}

int fmcadc_acq_stop(struct fmcadc_dev *dev, unsigned int flags)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->acq_stop(dev, flags);
}

int fmcadc_apply_config(struct fmcadc_dev *dev, unsigned int flags,
			struct fmcadc_conf *conf)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;
	uint64_t cap_mask;

	if (!conf->mask) {
		errno = FMCADC_ENOMASK;
		return -1; /* Nothing to do */
	}
	cap_mask = b->capabilities[conf->type];
	if ((cap_mask & conf->mask) != conf->mask) {
		/* Unsupported capabilities */
		errno = FMCADC_ENOCAP;
		return -1;
	}
	return b->fa_op->apply_config(dev, flags, conf);
}

int fmcadc_retrieve_config(struct fmcadc_dev *dev, struct fmcadc_conf *conf)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;
	uint64_t cap_mask;

	if (!conf->mask) {
		errno = FMCADC_ENOMASK;
		return -1; /* Nothing to do */
	}
	cap_mask = b->capabilities[conf->type];
	if ((cap_mask & conf->mask) != conf->mask) {
		/* Unsupported capabilities */
		errno = FMCADC_ENOCAP;
		return -1;
	}
	return b->fa_op->retrieve_config(dev, conf);
}

int fmcadc_get_param(struct fmcadc_dev *dev, char *name,
		     char *sptr, int *iptr)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->get_param(dev, name, sptr, iptr);
}

int fmcadc_set_param(struct fmcadc_dev *dev, char *name,
		     char *sptr, int *iptr)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->set_param(dev, name, sptr, iptr);
}

struct fmcadc_buffer *fmcadc_request_buffer(struct fmcadc_dev *dev,
					    int nsamples,
					    void *(*alloc)(size_t),
					    unsigned int flags)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->request_buffer(dev, nsamples, alloc, flags);
}

int fmcadc_fill_buffer(struct fmcadc_dev *dev,
		       struct fmcadc_buffer *buf,
		       unsigned int flags,
		       struct timeval *timeout)
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->fill_buffer(dev, buf, flags, timeout);
}

struct fmcadc_timestamp *fmcadc_tstamp_buffer(struct fmcadc_buffer *buf,
					      struct fmcadc_timestamp *ts)
{
	struct fmcadc_gid *g = (void *)buf->dev;
	const struct fmcadc_board_type *b = g->board;

	return b->fa_op->tstamp_buffer(buf, ts);
}

int fmcadc_release_buffer(struct fmcadc_dev *dev, struct fmcadc_buffer *buf,
			  void (*free)(void *))
{
	struct fmcadc_gid *g = (void *)dev;
	const struct fmcadc_board_type *b = g->board;

	if (!buf)
		return 0;

	return b->fa_op->release_buffer(dev, buf, free);
}
