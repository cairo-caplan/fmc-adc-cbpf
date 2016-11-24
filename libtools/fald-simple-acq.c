/* Copyright 2013 CERN
 * Author: Federico Vaga <federico.vaga@gmail.comZ
 * License: GPLv2
 *
 * This is a simple program to configure the FMC ADC trigger. It is not bug
 * aware because it is only a demo program to show you how you can handle the
 * trigger.
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

static void fald_help()
{
	printf("\nfald-simple-acq [OPTIONS] <DEVID>\n\n");
	printf("  <DEVID>: hexadecimal identifier (e.g.: \"0x200\")\n");
	printf("  --before|-b <num>        number of pre samples\n");
	printf("  --after|-a <num>         n. of post samples (default: 16)\n");
	printf("  --nshots|-n <num>        number of trigger shots\n");
	printf("  --delay|-d <num>         delay sample after trigger\n");
	printf("  --under-sample|-U <num>  pick 1 sample every <num>\n");
	printf("  --threshold|-t <num>     internal trigger threshold\n");
	printf("  --channel|-c <num>       channel used as trigger (0..3)\n");
	printf("  --tiemout|-T <millisec>  timeout for acquisition\n");
	printf("  --negative-edge          internal trigger is falling edge\n");
	printf("  --binary|-B <file>       save binary to <file>\n");
	printf("  --multi-binary|-M <file> save two files per shot: "
						"<file>.0000.ctrl etc\n");
	printf("  --dont-read|-N           config-only, use with zio-dump\n");

	printf("  --help|-h                show this help\n\n");
}

static int trgval[__FMCADC_CONF_LEN]; /* FIXME: this is not used */

static struct option options[] = {
	{"before",	required_argument, 0, 'b'},
	{"after",	required_argument, 0, 'a'},
	{"nshots",	required_argument, 0, 'n'},
	{"delay",	required_argument, 0, 'd'},
	{"under-sample",required_argument, 0, 'u'},
	{"threshold",	required_argument, 0, 't'},
	{"channel",	required_argument, 0, 'c'},
	{"timeout",	required_argument, 0, 'T'},
	{"negative-edge", no_argument, &trgval[FMCADC_CONF_TRG_POLARITY], 1},

	/* new options, to help stress-test */
	{"binary",	required_argument, 0, 'B'},
	{"multi-binary",required_argument, 0, 'M'},
	{"dont-read",	no_argument,       0, 'N'},

	/* backward-compatible options */
	{"pre",		required_argument, 0, 'p'},
	{"post",	required_argument, 0, 'P'},
	{"decimation",	required_argument, 0, 'D'},

	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

#define GETOPT_STRING "b:a:n:d:u:t:c:T:B:M:Np:P:D:h"

int main(int argc, char *argv[])
{
	struct fmcadc_buffer *buf;
	struct fmcadc_dev *adc;
	struct fmcadc_conf trg, acq;
	int i, c, err, opt_index, binmode = 0;
	int nshots = 1, presamples = 0, postsamples = 16;
	int timeout = -1;
	unsigned int dev_id = 0;
	char *basefile = NULL;
	char fname[PATH_MAX];
	FILE *f = NULL;

	if (argc == 1) {
		fald_help();
		exit(1);
	}

	/* reset attributes and provide defaults */
	memset(&trg, 0, sizeof(trg));
	trg.type = FMCADC_CONF_TYPE_TRG;
	fmcadc_set_conf(&trg, FMCADC_CONF_TRG_SOURCE, 1); /* external */

	memset(&acq, 0, sizeof(acq));
	acq.type = FMCADC_CONF_TYPE_ACQ;
	fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_POST_SAMP, postsamples);
	fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_N_SHOTS, nshots);

	/* Parse options */
	while ((c = getopt_long(argc, argv, GETOPT_STRING,
				options, &opt_index)) >=0) {
		switch (c) {
		case 'b': case 'p': /* before */
			presamples = atoi(optarg);
			fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_PRE_SAMP,
					presamples);
			break;
		case 'a': case 'P': /* after */
			postsamples = atoi(optarg);
			fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_POST_SAMP,
					postsamples);
			break;
		case 'n':
			nshots = atoi(optarg);
			fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_N_SHOTS,
					nshots);
			break;
		case 'd':
			fmcadc_set_conf(&trg, FMCADC_CONF_TRG_DELAY,
					atoi(optarg));
			break;
		case 'u': case 'D':
			fmcadc_set_conf(&acq, FMCADC_CONF_ACQ_DECIMATION,
					atoi(optarg));
			break;
		case 't':
			fmcadc_set_conf(&trg, FMCADC_CONF_TRG_THRESHOLD,
					atoi(optarg));
			break;
		case 'c':
			/* set internal, and then the channel */
			fmcadc_set_conf(&trg, FMCADC_CONF_TRG_SOURCE, 0);
			fmcadc_set_conf(&trg, FMCADC_CONF_TRG_SOURCE_CHAN,
					atoi(optarg));
			break;
		case 'T':
			timeout = atoi(optarg);
			break;
		case 'B':
			binmode = 1; /* do binary (default is 0) */
			basefile = optarg;
			break;
		case 'M':
			binmode = 2; /* do many binaries */
			basefile = optarg;
			break;
		case 'N':
			binmode = -1;
			break;

		case 'h': case '?':
			fald_help();
			exit(1);
			break;
		}
	}

	if (optind != argc - 1) {
		fprintf(stderr, "%s: DEVICE-ID is a mandatory argument\n",
			argv[0]);
		fald_help();
		exit(1);
	} else {
		sscanf(argv[optind], "%x", &dev_id);
	}

	/* Open the ADC */
	adc = fmcadc_open("fmc-adc-100m14b4cha", dev_id,
			  nshots * (presamples + postsamples),
			  nshots,
			  FMCADC_F_FLUSH);
	if (!adc) {
		fprintf(stderr, "%s: cannot open device: %s",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	if (strcmp(fmcadc_get_driver_type(adc), "zio")) {
		fprintf(stderr, "%s: not a zio driver, aborting\n", argv[0]);
		exit(1);
	}

	/* If we save to a a file, open it now to error out soon */
	if (binmode > 0) {
		char *s;

		s = basefile;
		if (binmode == 2) {
			sprintf(fname, "%s.000.ctrl", basefile);
			s = fname;
		}
		f = fopen(s, "a");
		if (!f) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], s,
				strerror(errno));
			exit(1);
		}
		if (binmode == 2)
			fclose(f);
	}

	/* Configure trigger (pick trigger polarity from external array) */
	fmcadc_set_conf(&trg, FMCADC_CONF_TRG_POLARITY,
			trgval[FMCADC_CONF_TRG_POLARITY]);
	err = fmcadc_apply_config(adc, 0 , &trg);
	if (err && errno != FMCADC_ENOMASK) {
		fprintf(stderr, "%s: cannot configure trigger: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Configure acquisition parameter */
	err = fmcadc_apply_config(adc, 0 , &acq);
	if (err && errno != FMCADC_ENOMASK) {
		fprintf(stderr, "%s: cannot configure acquisition: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	if (timeout < 0) {
		/* Start acquisition and wait until it completes */
		err = fmcadc_acq_start(adc, 0 , NULL);
	} else {
		/* Start acquisition and don't wait. We use acq_poll() later */
		struct timeval tv = {0, 0};

		err = fmcadc_acq_start(adc, 0 , &tv);
	}
	if (err) {
		fprintf(stderr, "%s: cannot start acquisition: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Now, if a timeout was specified, use the poll method */
	if (timeout >= 0) {
		struct timeval tv = {timeout / 1000, timeout % 1000};

		err = fmcadc_acq_poll(adc, 0 , &tv);
	}
	if (err) {
		fprintf(stderr, "%s: timeout after %i ms: %s\n", argv[0],
			timeout, strerror(errno));
		exit(1);
	}

	/* Allocate a buffer in the default way */
	buf = fmcadc_request_buffer(adc, presamples + postsamples,
				    NULL /* alloc */, 0);
	if (!buf) {
		fprintf(stderr, "Cannot allocate buffer (%s)\n",
			fmcadc_strerror(errno));
		exit(1);
	}

	/* Fill the buffer once for each shot */
	for (i = 0; i < acq.value[FMCADC_CONF_ACQ_N_SHOTS]; ++i) {
		struct zio_control *ctrl;
		int j, ch;
		int16_t *data;

		if (binmode < 0) /* no data must be acquired */
			break;

		err = fmcadc_fill_buffer(adc, buf, 0, NULL);
		if (err) {
			fprintf(stderr, "%s: shot %i/%i: cannot fill buffer:"
				" %s\n", argv[0], i + i,
				acq.value[FMCADC_CONF_ACQ_N_SHOTS],
			fmcadc_strerror(errno));
			exit(1);
		}
		ctrl = buf->metadata;
		data = buf->data;
		fprintf(stderr, "Read %d samples from shot %i/%i\n",
			ctrl->nsamples,
			i + 1, acq.value[FMCADC_CONF_ACQ_N_SHOTS]);

//DDG		

		//for (ch = 0; ch < 4; ch++) printf("%lld s %lld ticks ch %d %7i || ",(long long)ctrl->tstamp.secs, (long long)ctrl->tstamp.ticks, ch, *(data++) );
		/*for (j = 0; j < ctrl->nsamples / 4; j++)
		{
			printf("%5i ", j - acq.value[FMCADC_CONF_ACQ_PRE_SAMP]);
			for (ch = 0; ch < 4; ch++) printf("%lld s %lld ticks ch %d %7i || ",(long long)ctrl->tstamp.secs, (long long)ctrl->tstamp.ticks, ch, *(data++) );
			printf("\n");
		}*/
		

		if (binmode == 1) { /* append everything to a single file */
			if (fwrite(ctrl, sizeof(*ctrl), 1, f) != 1)
				err++;
			if (fwrite(data, ctrl->ssize, ctrl->nsamples, f)
			    != ctrl->nsamples)
				err++;
			if (err) {
				fprintf(stderr, "%s: write(%s): short write\n",
					argv[0], basefile);
				exit(1);
			}
			continue; /* next shot please */
		}

		if (binmode == 2) { /* several files */
			sprintf(fname, "%s.%03i.ctrl", basefile, i);
			f = fopen(fname, "w");
			if (!f) {
				fprintf(stderr, "%s: %s: %s\n",
					argv[0], fname, strerror(errno));
				exit(1);
			}
			if (fwrite(ctrl, sizeof(*ctrl), 1, f) != 1) {
				fprintf(stderr, "%s: write(%s): short write\n",
					argv[0], fname);
				exit(1);
			}
			fclose(f);
			sprintf(fname, "%s.%03i.data", basefile, i);
			f = fopen(fname, "w");
			if (!f) {
				fprintf(stderr, "%s: %s: %s\n",
					argv[0], fname, strerror(errno));
				exit(1);
			}
			if (fwrite(data, ctrl->ssize, ctrl->nsamples, f)
			    != ctrl->nsamples) {
				fprintf(stderr, "%s: write(%s): short write\n",
					argv[0], fname);
				exit(1);
			}
			fclose(f);

			continue;
		}

		/*
		 * Finally, binmode = 0.
		 * We lazily know samplesize is 2 bytes and chcount is 4
		 */
		for (j = 0; j < ctrl->nsamples / 4; j++) {
			printf("%5i     ", j - presamples);
			for (ch = 0; ch < 4; ch++){
			    //printf("%7i", *(data++));
			    if (ch==0) printf("%f", (*(data))*(5/32768));
			    data++;
			}
			printf("\n");
		}
	}
	if (binmode == 1)
		fclose(f);

	fmcadc_release_buffer(adc, buf, NULL);
	fmcadc_close(adc);
	exit(0);
}
