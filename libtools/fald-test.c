/* Copyright 2013 CERN
 * Author: Federico Vaga <federico.vaga@gmail.com>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <linux/zio-user.h>
#include <fmcadc-lib.h>

/* Global configuration*/
#define N_CHAN		4
#define CARD_NAME	"fmc-adc-100m14b4cha"

unsigned nshots = 1;
unsigned presamples = 10;
unsigned postsamples = 10;
int print_data = 1;

void print_buffer_content(struct fmcadc_buffer *buf);
int read_with_one_buffer(struct fmcadc_dev *dev);
int read_with_n_buffer(struct fmcadc_dev *dev);

int main(int argc, char *argv[])
{
	unsigned long totalsamples = nshots * (presamples + postsamples);
	unsigned int dev_id, n_buffers = nshots;
	struct fmcadc_conf acq, trg;
	struct fmcadc_dev *dev;
	int test = 0, err;

	if (argc != 3) {
		fprintf(stderr, "%s: Use \"%s <dev_id> <testnr>\n",
			argv[0], argv[1]);
		exit(1);
	}
	sscanf(argv[1], "%x", &dev_id);
	sscanf(argv[2], "%i", &test);

	/* Change parameters from environment */
	if (getenv("FALD_TEST_NSHOTS"))
		nshots = atoi(getenv("FALD_TEST_NSHOTS"));
	if (getenv("FALD_TEST_PRE_S"))
		presamples = atoi(getenv("FALD_TEST_PRE_S"));
	if (getenv("FALD_TEST_POST_S"))
		postsamples = atoi(getenv("FALD_TEST_POST_S"));
	if (getenv("FALD_TEST_NOPRINT"))
		print_data = 0;

	fmcadc_init();

	dev = fmcadc_open(CARD_NAME, dev_id, totalsamples, n_buffers,
		FMCADC_F_FLUSH);
	if (!dev) {
		fprintf(stderr, "%s: fmcadc_open(%s, 0x%x): %s\n", argv[0],
			CARD_NAME, dev_id, strerror(errno));
		exit(1);
	}
	if (strcmp(fmcadc_get_driver_type(dev), "zio")) {
		fprintf(stderr, "%s: not a zio driver, aborting\n", argv[0]);
		exit(1);
	}

	if (0) { /* We can't change buffer, because chardevs are open */
		err = fmcadc_set_param(dev, "cset0/current_buffer", argv[3], 0);
		if (err) {
			fprintf(stderr, "%s: cannot set '%s' as buffer: %s\n",
				argv[0], argv[3], fmcadc_strerror(errno));
			exit(1);
		}
	}

	/* FIXME: use nshots to set  cset0/chani/buffer/max-buffer-len */
	/* FIXME: use maxsize to set cset0/chani/buffer/max-buffer-kb  */

	/* configure acquisition parameters */
	memset(&acq, 0, sizeof(acq));
	acq.type = FMCADC_CONF_TYPE_ACQ;
	fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_N_SHOTS, nshots);
	fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_PRE_SAMP, presamples);
	fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_POST_SAMP, postsamples);

	err = fmcadc_apply_config(dev, 0, &acq);
	if (err) {
		fprintf(stderr, "%s: apply_config(acq): %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* configure trigger parameters */
	memset(&trg, 0, sizeof(trg));
	trg.type = FMCADC_CONF_TYPE_TRG;
	fmcadc_set_conf(&trg, FMCADC_CONF_TRG_SOURCE, 1); /* external */

	err = fmcadc_apply_config(dev, 0, &trg);
	if (err) {
		fprintf(stderr, "%s: apply_config(trigger): %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Start the acquisition */
	err = fmcadc_acq_start(dev, 0 , NULL);
	if (err) {
		fprintf(stderr, "%s: cannot start acquisition: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	switch (test) {
	case 0:
		err = read_with_one_buffer(dev);
		break;
	case 1:
		err = read_with_n_buffer(dev);
		break;
	}
	if (err) {
		fprintf(stderr, "%s: problem with read buffer: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Stop acquisition, close device and exit from library */
	fmcadc_acq_stop(dev, 0);
	fmcadc_close(dev);
	fmcadc_exit();

	exit(0);
}

/* Read all shots with a single buffer and then release it */
int read_with_one_buffer(struct fmcadc_dev *dev)
{
	struct fmcadc_buffer *buf;
	int err = 0, i;

	buf = fmcadc_request_buffer(dev, presamples + postsamples, NULL, 0);
	for (i = 0; i < nshots; ++i) {
		err = fmcadc_fill_buffer(dev, buf, 0, NULL);
		if (err)
			break;
		print_buffer_content(buf);
	}
	fmcadc_release_buffer(dev, buf, NULL);

	return err;
}

/* Read all shots with a multiple buffer and then release them
 * request, fill and release are separated in different loop to show that
 * are not strictly consecutive operations
 */
int read_with_n_buffer(struct fmcadc_dev *dev)
{
	struct fmcadc_buffer *buf[nshots] ;
	int err = 0, i;

	/* Allocate all buffers before the use */
	for (i = 0; i < nshots; ++i)
		buf[i] = fmcadc_request_buffer(dev,
				presamples + postsamples, NULL, 0);

	/* Fill all buffers */
	for (i = 0; i < nshots; ++i) {
		err = fmcadc_fill_buffer(dev, buf[i], 0, NULL);
		if (err)
			break;
		print_buffer_content(buf[i]);
	}

	/* Release all buffers */
	for (i = 0; i < nshots; ++i)
		fmcadc_release_buffer(dev, buf[i], NULL);

	return err;
}


void print_buffer_content(struct fmcadc_buffer *buf)
{
	int16_t *data = buf->data;		  /* get data */
	struct fmcadc_timestamp *ts;
	int i, ch;

	ts = fmcadc_tstamp_buffer(buf, NULL);
	printf("timestamp %lli:%lli:%lli\n",
	       (long long)ts->secs,
	       (long long)ts->ticks,
	       (long long)ts->bins);

	if (!print_data)
		return;

	for (i = 0; i < presamples + postsamples; i++) {
		printf("%5i     ", i - presamples);
		for (ch = 0; ch < N_CHAN; ch++)
			printf("%7i", *(data++));
		printf("\n");
	}
}
