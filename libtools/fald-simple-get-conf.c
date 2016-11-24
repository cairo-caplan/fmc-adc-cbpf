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
#include <errno.h>
#include <linux/zio-user.h>
#include <fmcadc-lib.h>

static void fald_help()
{
	printf("\nfald-simple-get-conf [OPTIONS] 0x<DEVICE ID>\n\n");
	printf("  <DEVICE>: hexadecimal string which represent the device "
	       "identificator of an fmc-adc\n");
	printf("  --help|-h: show this help\n\n");
}

int main(int argc, char *argv[])
{
	struct fmcadc_dev *adc;
	struct fmcadc_conf trg, acq, brd, chn;
	static struct option options[] = {
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int opt_index = 0, err = 0, i;
	unsigned int dev_id = 0;
	char c;

	if (argc == 1) {
		fald_help();
		exit(1);
	}

	/* reset attribute's mask */
	trg.mask = 0;
	acq.mask = 0;

	/* Parse options */
	while((c = getopt_long(argc, argv, "h",	options, &opt_index)) >= 0) {
		switch (c) {
		case 'h':
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
		sscanf(argv[optind], "0x%x", &dev_id);
	}

	printf("Open ADC fmcadc_100MS_4ch_14bit dev_id 0x%04x ...\n", dev_id);
	/* Open the ADC */
	adc = fmcadc_open("fmcadc_100MS_4ch_14bit", dev_id,
			  /* no buffers expexted */ 0, 0,
			  /* no flush either */ 0);
	if (!adc) {
		fprintf(stderr, "%s: cannot open device: %s",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	printf("Get Trigger Configuration ...\n");
	/* Configure Trigger parameter to retrieve */
	trg.type = FMCADC_CONF_TYPE_TRG;
	fmcadc_set_conf_mask(&trg, FMCADC_CONF_TRG_SOURCE);
	fmcadc_set_conf_mask(&trg, FMCADC_CONF_TRG_SOURCE_CHAN);
	fmcadc_set_conf_mask(&trg, FMCADC_CONF_TRG_THRESHOLD);
	fmcadc_set_conf_mask(&trg, FMCADC_CONF_TRG_POLARITY);
	fmcadc_set_conf_mask(&trg, FMCADC_CONF_TRG_DELAY);
	err = fmcadc_retrieve_config(adc, &trg);
	if (err) {
		fprintf(stderr, "%s: cannot get trigger config: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	printf("    source: %s\n",
		trg.value[FMCADC_CONF_TRG_SOURCE] ? "external" : "internal");
	printf("    channel: %d\n", trg.value[FMCADC_CONF_TRG_SOURCE_CHAN]);
	printf("    threshold: %d\n", trg.value[FMCADC_CONF_TRG_THRESHOLD]);
	printf("    polarity: %d\n", trg.value[FMCADC_CONF_TRG_POLARITY]);
	printf("    delay: %d\n", trg.value[FMCADC_CONF_TRG_DELAY]);


	printf("Get Acquisition Configuration ...\n");
	/* Configure acquisition parameter */
	acq.type = FMCADC_CONF_TYPE_ACQ;
	fmcadc_set_conf_mask(&acq, FMCADC_CONF_ACQ_N_SHOTS);
	fmcadc_set_conf_mask(&acq, FMCADC_CONF_ACQ_POST_SAMP);
	fmcadc_set_conf_mask(&acq, FMCADC_CONF_ACQ_PRE_SAMP);
	fmcadc_set_conf_mask(&acq, FMCADC_CONF_ACQ_DECIMATION);
	fmcadc_set_conf_mask(&acq, FMCADC_CONF_ACQ_FREQ_HZ);
	fmcadc_set_conf_mask(&acq, FMCADC_CONF_ACQ_N_BITS);
	err = fmcadc_retrieve_config(adc, &acq);
	if (err) {
		fprintf(stderr, "%s: cannot get acquisition config: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	printf("    n-shots: %d\n", acq.value[FMCADC_CONF_ACQ_N_SHOTS]);
	printf("    post-sample: %d\n", acq.value[FMCADC_CONF_ACQ_POST_SAMP]);
	printf("    pre-sample: %d\n", acq.value[FMCADC_CONF_ACQ_PRE_SAMP]);
	printf("    decimation: %d\n", acq.value[FMCADC_CONF_ACQ_DECIMATION]);
	printf("    frequency: %dHz\n", acq.value[FMCADC_CONF_ACQ_FREQ_HZ]);
	printf("    n-bits: %d\n", acq.value[FMCADC_CONF_ACQ_N_BITS]);


	printf("Get Board Configuration ...\n");
	/* Configure acquisition parameter */
	brd.type = FMCADC_CONT_TYPE_BRD;
	fmcadc_set_conf_mask(&brd, FMCADC_CONF_BRD_STATE_MACHINE_STATUS);
	fmcadc_set_conf_mask(&brd, FMCADC_CONF_BRD_N_CHAN);
	err = fmcadc_retrieve_config(adc, &brd);
	if (err) {
		fprintf(stderr, "%s: cannot get board config: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	printf("    n-chan: %d\n", brd.value[FMCADC_CONF_BRD_N_CHAN]);
	printf("    State Machine: %d\n",
		brd.value[FMCADC_CONF_BRD_STATE_MACHINE_STATUS]);


	for (i = 0; i < brd.value[FMCADC_CONF_BRD_N_CHAN]; ++i) {
		printf("Get Channel %d Configuration ...\n", i);
		/* Configure acquisition parameter */
		chn.type = FMCADC_CONF_TYPE_CHN;
		chn.route_to = i;
		fmcadc_set_conf_mask(&chn, FMCADC_CONF_CHN_RANGE);
		fmcadc_set_conf_mask(&chn, FMCADC_CONF_CHN_TERMINATION);
		fmcadc_set_conf_mask(&chn, FMCADC_CONF_CHN_OFFSET);
		err = fmcadc_retrieve_config(adc, &chn);
		if (err) {
			fprintf(stderr, "%s: cannot get channel config: %s\n",
				argv[0], fmcadc_strerror(errno));
			exit(1);
		}
		printf("    range: %d\n", chn.value[FMCADC_CONF_CHN_RANGE]);
		printf("    50Ohm Termination: %s\n",
			chn.value[FMCADC_CONF_CHN_TERMINATION] ? "yes" : "no");
		printf("    offset: %d\n", chn.value[FMCADC_CONF_CHN_OFFSET]);
	}

	fmcadc_close(adc);
	exit(0);
}
