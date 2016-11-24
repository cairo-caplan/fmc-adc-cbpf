/*
 * ZIO-specific configuration (mostly device-independent)
 *
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fmcadc-lib.h"
#include "fmcadc-lib-int.h"

#define FMCADC_CONF_GET 0
#define FMCADC_CONF_SET 1

/*
 * Internal functions to read and write a string.
 * Trailing newlines are added/removed as needed
 */
static int __fa_zio_sysfs_set(struct __fmcadc_dev_zio *fa, char *name,
			      char *val, int maxlen)
{
	char pathname[128];
	char newval[maxlen + 1];
	int fd, ret, len;

	snprintf(pathname, sizeof(pathname), "%s/%s", fa->sysbase, name);
	len = sprintf(newval, "%s\n", val);

	fd = open(pathname, O_WRONLY);
	if (fd < 0)
		return -1;
	ret = write(fd, newval, len);
	close(fd);
	if (ret < 0)
		return -1;
	if (ret == len)
		return 0;
	errno = EIO; /* short write */
	return -1;
}

static int __fa_zio_sysfs_get(struct __fmcadc_dev_zio *fa, char *name,
			      char *val /* no maxlen: reader knows */ )
{
	char pathname[128];
	int fd, ret;

	snprintf(pathname, sizeof(pathname), "%s/%s", fa->sysbase, name);
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		return -1;
	ret = read(fd, val, 128 /* well... user knows... */);
	close(fd);
	if (ret < 0)
		return -1;
	if (val[ret - 1] == '\n')
		val[ret - 1] = '\0';
	return 0;
}

/*
 * Public functions (through ops and ./route.c).
 * They manage both strings and integers
 */
int fmcadc_zio_set_param(struct fmcadc_dev *dev, char *name,
			 char *sptr, int *iptr)
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);
	char istr[12];
	int len;

	if (sptr)
		return __fa_zio_sysfs_set(fa, name, sptr, strlen(sptr + 2));
	len = sprintf(istr, "%i", *iptr);
	return __fa_zio_sysfs_set(fa, name, istr, len + 2);
}

int fmcadc_zio_get_param(struct fmcadc_dev *dev, char *name,
			 char *sptr, int *iptr)
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);
	char istr[12];
	int ret;

	if (sptr)
		return __fa_zio_sysfs_get(fa, name, sptr);

	ret = __fa_zio_sysfs_get(fa, name, istr);
	if (ret < 0)
		return ret;
	if (sscanf(istr, "%i", iptr) == 1)
		return 0;
	errno = EINVAL;
	return -1;
}

/*
 * Previous functions, now based on the public ones above
 * Note: they are swapped: get, then set (above is set then get)
 * FIXME: code using these must be refactored using data structures
 */
static int fa_zio_sysfs_get(struct __fmcadc_dev_zio *fa, char *name,
		uint32_t *resp)
{
	struct fmcadc_dev *dev = (void *)&fa->gid; /* hack: back and forth.. */
	int ret;
	int val;

	ret = fmcadc_zio_get_param(dev, name, NULL, &val);
	if (!ret) {
		*resp = val; /* different type */
		return 0;
	}
	if (!(fa->flags & FMCADC_FLAG_VERBOSE))
		return ret;
	/* verbose tail */
	if (ret)
		fprintf(stderr, "lib-fmcadc: Error reading %s (%s)\n",
			name, strerror(errno));
	else
		fprintf(stderr, "lib-fmcadc: %08x %5i <- %s\n",
			(int)*resp, (int)*resp, name);
	return ret;
}

int fa_zio_sysfs_set(struct __fmcadc_dev_zio *fa, char *name,
		uint32_t *value)
{
	struct fmcadc_dev *dev = (void *)&fa->gid; /* hack: back and forth.. */
	int ret;
	int val = *value; /* different type */

	ret = fmcadc_zio_set_param(dev, name, NULL, &val);
	if (!ret)
		return 0;
	if (!(fa->flags & FMCADC_FLAG_VERBOSE))
		return ret;
	/* verbose tail */
	if (ret)
		fprintf(stderr, "lib-fmcadc: Error writing %s (%s)\n",
			name, strerror(errno));
	else
		fprintf(stderr, "lib-fmcadc: %08x %5i -> %s\n",
			(int)*value, (int)*value, name);
	return ret;
}

static int fmcadc_zio_config_trg(struct __fmcadc_dev_zio *fa,
		unsigned int index, uint32_t *value, unsigned int direction)
{
	switch (index) {
	case FMCADC_CONF_TRG_SOURCE:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/external",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/external",
					value);
		break;
	case FMCADC_CONF_TRG_SOURCE_CHAN:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/int-channel",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/int-channel",
					value);
		break;
	case FMCADC_CONF_TRG_THRESHOLD:
		if (direction)
			return fa_zio_sysfs_set(fa,
					"cset0/trigger/int-threshold", value);
		else
			return fa_zio_sysfs_get(fa,
					"cset0/trigger/int-threshold", value);
		break;
	case FMCADC_CONF_TRG_POLARITY:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/polarity",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/polarity",
					value);
		break;
	case FMCADC_CONF_TRG_DELAY:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/delay",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/delay",
					value);
		break;
	case FMCADC_CONF_TRG_THRESHOLD_FILTER:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/int-threshold-filter",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/int-threshold-filter",
					value);
		break;
	default:
		errno = FMCADC_ENOCAP;
		return -1;
	}
}
static int fmcadc_zio_config_acq(struct __fmcadc_dev_zio *fa,
		unsigned int index, uint32_t *value, unsigned int direction)
{
	switch (index) {
	case FMCADC_CONF_ACQ_N_SHOTS:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/nshots",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/nshots",
					value);
		break;
	case FMCADC_CONF_ACQ_POST_SAMP:
		if (direction)
			return fa_zio_sysfs_set(fa,
					"cset0/trigger/post-samples", value);
		else
			return fa_zio_sysfs_get(fa,
					"cset0/trigger/post-samples", value);
		break;
	case FMCADC_CONF_ACQ_PRE_SAMP:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/trigger/pre-samples",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/trigger/pre-samples",
					value);
		break;
	case FMCADC_CONF_ACQ_DECIMATION:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/sample-decimation",
					value);
		else
			return fa_zio_sysfs_get(fa, "cset0/sample-decimation",
					value);
		break;
	case FMCADC_CONF_ACQ_FREQ_HZ:
		if (direction) {
			errno = FMCADC_ENOSET;
			return -1;
		} else {
			*value = 100000000; /* 100Mhz */
			return 0;
		}
		break;
	case FMCADC_CONF_ACQ_N_BITS:
		if (direction) {
			errno = FMCADC_ENOSET;
			return -1;
		} else {
			*value = 14;
			return 0;
		}
		break;
	default:
		errno = FMCADC_ENOCAP;
		return -1;
	}
}
static int fmcadc_zio_config_chn(struct __fmcadc_dev_zio *fa, unsigned int ch,
		unsigned int index, uint32_t *value, unsigned int direction)
{
	char path[128];

	switch (index) {
	case FMCADC_CONF_CHN_RANGE:
		sprintf(path, "cset%d/ch%d-vref", fa->cset, ch);
		if (direction)
			return fa_zio_sysfs_set(fa, path, value);
		else
			return fa_zio_sysfs_get(fa, path, value);
		break;
	case FMCADC_CONF_CHN_TERMINATION:
		sprintf(path, "cset%d/ch%d-50ohm-term", fa->cset, ch);
		if (direction)
			return fa_zio_sysfs_set(fa, path, value);
		else
			return fa_zio_sysfs_get(fa, path, value);
		break;
	case FMCADC_CONF_CHN_OFFSET:
		sprintf(path, "cset%d/ch%d-offset", fa->cset, ch);
		if (direction)
			return fa_zio_sysfs_set(fa, path, value);
		else
			return fa_zio_sysfs_get(fa, path, value);
		break;
	case FMCADC_CONF_CHN_SATURATION:
		sprintf(path, "cset%d/ch%d-saturation", fa->cset, ch);
		if (direction)
			return fa_zio_sysfs_set(fa, path, value);
		else
			return fa_zio_sysfs_get(fa, path, value);
		break;
	default:
		errno = FMCADC_ENOCAP;
		return -1;
	}

	return 0;
}
static int fmcadc_zio_config_brd(struct __fmcadc_dev_zio *fa,
		unsigned int index, uint32_t *value, unsigned int direction)
{
	switch (index) {
	case FMCADC_CONF_UTC_TIMING_BASE_S:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/tstamp-base-s",
						value);
		else
			return fa_zio_sysfs_get(fa, "cset0/tstamp-base-s",
						value);
		break;
	case FMCADC_CONF_UTC_TIMING_BASE_T:
		if (direction)
			return fa_zio_sysfs_set(fa, "cset0/tstamp-base-t",
						value);
		else
			return fa_zio_sysfs_get(fa, "cset0/tstamp-base-t",
						value);
		break;
	case FMCADC_CONF_BRD_STATE_MACHINE_STATUS:
		if (!direction)
			return fa_zio_sysfs_get(fa, "cset0/fsm-state",
						value);
		errno = EINVAL;
		return -1;
	case FMCADC_CONF_BRD_N_CHAN:
		if (!direction) {
			*value = 4;
			return 0;
		}
		errno = EINVAL;
		return -1;
	default:
		errno = FMCADC_ENOCAP;
		return -1;
	}
}

static int fmcadc_zio_config(struct __fmcadc_dev_zio *fa, unsigned int flags,
		struct fmcadc_conf *conf, unsigned int direction)
{

	int err = 0, i;
	uint32_t enabled;

	/* Disabling the trigger before changing configuration */
	if (direction) {
		err = fa_zio_sysfs_get(fa, "cset0/trigger/enable", &enabled);
		if (err)
			return err;
		if (enabled) {
			enabled = 0;
			err = fa_zio_sysfs_set(fa, "cset0/trigger/enable", &enabled);
			/* restore the initial value */
			enabled = 1;
		}
	}

	for (i = 0; i < __FMCADC_CONF_LEN; ++i) {
		if (!(conf->mask & (1LL << i)))
			continue;

		/* Parameter to configure */
		switch (conf->type) {
		case FMCADC_CONF_TYPE_TRG:
			err = fmcadc_zio_config_trg(fa, i, &conf->value[i],
					direction);
			break;
		case FMCADC_CONF_TYPE_ACQ:
			err = fmcadc_zio_config_acq(fa, i, &conf->value[i],
					direction);
			break;
		case FMCADC_CONF_TYPE_CHN:
			if (conf->route_to > 3) {
				errno = FMCADC_ENOCHAN;
				return -1;
			}
			err = fmcadc_zio_config_chn(fa, conf->route_to,
					i, &conf->value[i],
					direction);
			break;
		case FMCADC_CONT_TYPE_BRD:
			err = fmcadc_zio_config_brd(fa, i, &conf->value[i],
					direction);
			break;
		default:
			errno = FMCADC_ENOCFG;
			return -1;
		}
		if (err)
			break; /* stop the config process: an error occurs */
	}

	/* if the trigger was enabled restore it */
	if (direction && enabled)
		err = fa_zio_sysfs_set(fa, "cset0/trigger/enable", &enabled);
	return err;
}

int fmcadc_zio_apply_config(struct fmcadc_dev *dev, unsigned int flags,
		struct fmcadc_conf *conf)
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);

	return fmcadc_zio_config(fa, flags, conf, FMCADC_CONF_SET);
}

int fmcadc_zio_retrieve_config(struct fmcadc_dev *dev,
		struct fmcadc_conf *conf)
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);

	return fmcadc_zio_config(fa, 0, conf, FMCADC_CONF_GET);
}
