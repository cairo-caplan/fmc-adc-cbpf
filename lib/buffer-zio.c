/*
 * ZIO-wide buffer management (device-independent)
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
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <linux/zio-user.h>

#include "fmcadc-lib.h"
#include "fmcadc-lib-int.h"

/* Internal function to read the control, already allocated in the buffer */
static int fmcadc_zio_read_ctrl(struct __fmcadc_dev_zio *fa,
				struct fmcadc_buffer *buf)
{
	struct zio_control *ctrl;
	int i;

	i = read(fa->fdc, buf->metadata, sizeof(struct zio_control));

	switch (i) {
	case sizeof(struct zio_control):
		return 0; /* ok */

	case -1:
		if (fa->flags & FMCADC_FLAG_VERBOSE)
			fprintf(stderr, "%s: read: %s\n", __func__,
				strerror(errno));
		return -1;
	case 0:
		if (fa->flags & FMCADC_FLAG_VERBOSE)
			fprintf(stderr, "%s: unexpected EOF\n", __func__);
		return -1;
	default:
		if (fa->flags & FMCADC_FLAG_VERBOSE)
			fprintf(stderr, "%s: read: %i bytes (expected %zi)\n",
				__func__, i, sizeof(ctrl));
		return -1;
	}
}

/* Internal function to read or map the data, already allocated in the buffer */
static int fmcadc_zio_read_data(struct __fmcadc_dev_zio *fa,
				  struct fmcadc_buffer *buf)
{
	struct zio_control *ctrl = buf->metadata;
	int datalen;
	int samplesize = buf->samplesize; /* Careful: includes n_chan */
	int i;

	/* we allocated buf->nsamples, we can have more or less */
	if (buf->nsamples < ctrl->nsamples)
		datalen = samplesize * buf->nsamples;
	else
		datalen = samplesize * ctrl->nsamples;

	if (fa->flags & FMCADC_FLAG_MMAP) {
		unsigned long mapoffset  = ctrl->mem_offset;
		unsigned long pagemask = fa->pagesize - 1;

		if (buf->mapaddr) /* unmap previous block */
			munmap(buf->mapaddr, buf->maplen);

		buf->maplen = (mapoffset & pagemask) + datalen;
		buf->mapaddr = mmap(0, buf->maplen, PROT_READ, MAP_SHARED,
				    fa->fdd, mapoffset & ~pagemask);
		if (buf->mapaddr == MAP_FAILED)
			return -1;
		buf->data = buf->mapaddr + (mapoffset & pagemask);
		return 0;
	}

	/* read */
	i = read(fa->fdd, buf->data, datalen);
	if (i == datalen)
		return 0;
	if (i > 0) {
		if (fa->flags & FMCADC_FLAG_VERBOSE)
			fprintf(stderr, "%s: read %i bytes (exp. %i)\n",
				__func__, i, datalen);
		buf->nsamples = i / fa->samplesize;
		/* short read is allowed */
		return 0;
	}
	if (i == 0) {
		if (fa->flags & FMCADC_FLAG_VERBOSE)
			fprintf(stderr, "%s: unexpected EOF\n", __func__);
		errno = ENODATA;
		return -1;
	}
	if (fa->flags & FMCADC_FLAG_VERBOSE)
		fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
	return -1;
}

/* externally-called: malloc buffer and metadata, do your best with data */
struct fmcadc_buffer *fmcadc_zio_request_buffer(struct fmcadc_dev *dev,
						int nsamples,
						void *(*alloc)(size_t),
						unsigned int flags)
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);
	struct fmcadc_buffer *buf;
	char s[16];

	/* If this is the first buffer, we need to know which kind it is */
	if ((fa->flags & (FMCADC_FLAG_MALLOC | FMCADC_FLAG_MMAP)) == 0) {
		fmcadc_get_param(dev, "cset0/current_buffer", s, NULL);
		if (!strcmp(s, "vmalloc"))
			fa->flags |= FMCADC_FLAG_MMAP;
		else
			fa->flags |= FMCADC_FLAG_MALLOC;
	}

	buf = calloc(1, sizeof(*buf));
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}
	buf->metadata = calloc(1, sizeof(struct zio_control));
	if (!buf->metadata) {
		free(buf);
		errno = ENOMEM;
		return NULL;
	}

	/* Allocate data: custom allocator, or malloc, or mmap */
	if (!alloc && fa->flags & FMCADC_FLAG_MALLOC)
		alloc = malloc;
	if (alloc) {
		buf->data = alloc(nsamples * fa->samplesize);
		if (!buf->data) {
			free(buf->metadata);
			free(buf);
			errno = ENOMEM;
			return NULL;
		}
	} else {
		/* mmap is done later */
		buf->data = NULL;
	}

	/* Copy other information */
	buf->samplesize = fa->samplesize;
	buf->nsamples = nsamples;
	buf->dev = (void *)&fa->gid;
	buf->flags = flags;
	return buf;
}

int fmcadc_zio_fill_buffer(struct fmcadc_dev *dev,
			   struct fmcadc_buffer *buf,
			   unsigned int flags,
			   struct timeval *to)
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);
	struct pollfd p;
	int to_ms, ret;

	/* So, first sample and blocking read. Wait.. */
	p.fd = fa->fdc;
	p.events = POLLIN | POLLERR;
	if (!to)
		to_ms = -1;
	else
		to_ms = to->tv_sec / 1000 + (to->tv_usec + 500) / 1000;
	ret = poll(&p, 1, to_ms);
	switch (ret) {
	case 0:
		errno = EAGAIN;
		/* fall through */
	case -1:
		return -1;
	}
	if (p.revents & POLLERR) {
		errno = FMCADC_EDISABLED;
		return -1;
	}

	ret = fmcadc_zio_read_ctrl(fa, buf);
	if (ret < 0)
		return ret;
	ret = fmcadc_zio_read_data(fa, buf);
	if (ret < 0)
		return ret;
	return 0;
}

struct fmcadc_timestamp *fmcadc_zio_tstamp_buffer(struct fmcadc_buffer *buf,
						  struct fmcadc_timestamp *ts)
{
	struct zio_control *ctrl = buf->metadata;
	if (ts) {
		memcpy(ts, &ctrl->tstamp, sizeof(*ts)); /* FIXME: endianness */
		return ts;
	}
	return (struct fmcadc_timestamp *)&ctrl->tstamp;
}

int fmcadc_zio_release_buffer(struct fmcadc_dev *dev,
				     struct fmcadc_buffer *buf,
				     void (*free_fn)(void *))
{
	struct __fmcadc_dev_zio *fa = to_dev_zio(dev);

	free(buf->metadata);
	if (!free_fn && fa->flags & FMCADC_FLAG_MALLOC)
		free_fn = free;

	if (free_fn)
		free_fn(buf->data);
	else if (buf->mapaddr && buf->mapaddr != MAP_FAILED)
			munmap(buf->mapaddr, buf->maplen);
	free(buf);
	return 0;
}
