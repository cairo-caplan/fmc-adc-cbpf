/*
 * Copyright 2012 CERN
 * Author: Federico Vaga <federico.vaga@gmail.comZ
 * License: GPLv2
 *
 * This is a simple program which calculate the acquisition time.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define buf_len 50
/* user will edit by adding the device name */
char basepath[40] = "/sys/bus/zio/devices/";

enum fau_attribute {
	FAU_UTR_STR_S,
	FAU_UTR_STR_T,
	FAU_UTR_STR_B,
	FAU_UTR_END_S,
	FAU_UTR_END_T,
	FAU_UTR_END_B,
	FAU_UTR_STP_S,
	FAU_UTR_STP_T,
	FAU_UTR_STP_B,
	FAU_UTR_TRG_S,
	FAU_UTR_TRG_T,
	FAU_UTR_TRG_B,
	FAU_UTC_NUM_ATTR,
};

const char *attribute[] = {
	[FAU_UTR_STR_S] = "/cset0/tstamp-acq-str-s",
	[FAU_UTR_STR_T] = "/cset0/tstamp-acq-str-t",
	[FAU_UTR_STR_B] = "/cset0/tstamp-acq-str-b",
	[FAU_UTR_END_S] = "/cset0/tstamp-acq-end-s",
	[FAU_UTR_END_T] = "/cset0/tstamp-acq-end-t",
	[FAU_UTR_END_B] = "/cset0/tstamp-acq-end-b",
	[FAU_UTR_STP_S] = "/cset0/tstamp-acq-stp-s",
	[FAU_UTR_STP_T] = "/cset0/tstamp-acq-stp-t",
	[FAU_UTR_STP_B] = "/cset0/tstamp-acq-stp-b",
	[FAU_UTR_TRG_S] = "/cset0/trigger/tstamp-trg-lst-s",
	[FAU_UTR_TRG_T] = "/cset0/trigger/tstamp-trg-lst-t",
	[FAU_UTR_TRG_B] = "/cset0/trigger/tstamp-trg-lst-b",
};

/* Write a sysfs attribute */
int fau_read_attribute(enum fau_attribute attr, long *val)
{
	char fullpath[200];
	FILE *f;

	sprintf(fullpath, "%s%s", basepath, attribute[attr]);
	f = fopen(fullpath, "r");
	if (!f)
		return -1;
	if (fscanf(f, "%li", val) != 1) {
		fclose(f);
		errno = EINVAL;
		return -1;
	}
	fclose(f);
	return 0;
}

int fau_read_time(long time[3], enum fau_attribute first_attr)
{
	int i, attr = first_attr, err = 0;

	for(i = 0; i < 3; ++i, ++attr) {
		err += (fau_read_attribute(attr, &time[i]) * (-1));
		if (i == 1)
			time[i] *= 8; /* convert ticks (125Mhz) */

	}
	return err;
}

void fau_print_time(long time1[3], long time2[3])
{
	long result[3];

	result[0] = time2[0] - time1[0];
	if (time2[1] >= time1[1]) {
		result[1] = time2[1] - time1[1];
	} else {
		result[0]--;
		result[1] = (1000000000)-(time1[1] - time1[2]);
	}

	printf("Acquisition time: %li.%09li\n\n",
		result[0], result[1]);
}

static void fau_help()
{
	printf("\nfau-acq-time [OPTIONS] <DEVICE>\n\n");
	printf("  <DEVICE>: ZIO name of the device to use\n");
	printf("  --last|-l : time between the last trigger and the acquisition end\n\n");
	printf("  --full|-f : time between the acquisition start and the acquisition end\n\n");
	printf("  --help|-h: show this help\n\n");
}

int main(int argc, char *argv[])
{
	/* getop attribute */
	static int last = 0, full = 0;
	static struct option options[] = {
		{"last",no_argument, &last, 1},
		{"full",no_argument, &full, 1},
		{"help",no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int opt_index = 0, err = 0;
	char c;
	long time1[3], time2[3];

	if (argc == 1) {
		fau_help();
		exit(1);
	}

	while( (c = getopt_long(argc, argv, "lfh", options, &opt_index)) >=0 ){
		if (c == 'h') {
			fau_help();
			exit(1);
			break;
		}
	}

	if (optind != argc - 1 ) {
		fprintf(stderr, "%s: DEVICE-ID is a mandatory argument\n",
			argv[0]);
		fau_help();
		exit(1);
	}

	strcat(basepath, argv[argc-1]);
	printf("Sysfs path to device is: %s\n", basepath);

	if (last) {
		err = fau_read_time(time1, FAU_UTR_TRG_S);
		if (err)
			exit(1);
		printf("Last Trigger fired at %li.%09li\n",
			time1[0], time1[1]);
		err = fau_read_time(time2, FAU_UTR_END_S);
		if (err)
			exit(1);
		printf("Last Acquisition end at %li.%09li\n",
			time2[0], time2[1]);
		fau_print_time(time1, time2);
	}
	if (full) {
		err = fau_read_time(time1, FAU_UTR_STR_S);
		if (err)
			exit(1);
		printf("Last Acquisition start at %li.%09li\n",
			time1[0], time1[1]);
		err = fau_read_time(time2, FAU_UTR_END_S);
		if (err)
			exit(1);
		printf("Last Acquisition end at %li.%09li\n",
			time2[0], time2[1]);
		fau_print_time(time1, time2);
	}

	exit(0);
}
