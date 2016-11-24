/*
 * Copyright CERN 2014
 * Author: Federico Vaga <federico.vaga@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>

#include <linux/zio-user.h>
#include <fmcadc-lib.h>
#include <fmc-adc-100m14b4cha.h>

/* Subtract the `struct timespec' values X and Y,
storing the result in RESULT.
Return 1 if the difference is negative, otherwise 0. */

int timespec_subtract (result, x, y)
  struct timespec *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_nsec < y->tv_nsec) {
		int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
		y->tv_nsec -= 1000000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_nsec - y->tv_nsec > 1000000000) {
		int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
		y->tv_nsec += 1000000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	  tv_nsec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

static void fald_help()
{
	printf("\nfald-bad-clock [OPTIONS] <devid>\n\n");
	printf("  -i <seconds>      observation interval\n\n");
	printf("  -h                show this help\n\n");
	exit(1);
}

int main (int argc, char *argv[])
{
	struct fmcadc_dev *adc;
	struct fmcadc_conf brd_cfg;
	struct timespec sys_start, sys_cur, adc_cur, dlt_ts, dlt_dlt_ts = {0, 0};
	int err, devid, interval = 360;
	uint32_t adc_sec, adc_ticks;
	char c;

	/* Prepare the board timing base configuration */
	memset(&brd_cfg, 0, sizeof(brd_cfg));
	brd_cfg.type = FMCADC_CONT_TYPE_BRD;

	while ((c = getopt(argc, argv, "i:h")) >= 0) {
		switch (c) {
		case 'i':
			err = sscanf(optarg, "%d", &interval);
			if (err != 1)
				fald_help();
			break;
		case '?':
		case 'h':
			fald_help();
			break;
		}
	}

	sscanf(argv[argc-1], "%x", &devid);

	adc = fmcadc_open("fmc-adc-100m14b4cha", devid, 0, 0, FMCADC_F_FLUSH);
	if (!adc) {
		fprintf(stderr, "%s: cannot open device: %s",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	do {
		err = clock_gettime(CLOCK_REALTIME, &sys_start);
		if (err) {
			fprintf(stderr, "%s: cannot get real time: %s",
				argv[0], strerror(errno));
			exit(1);
		}
	} while (sys_start.tv_nsec > 1000);

	/* Configure ADC internal clock */
	adc_sec = sys_start.tv_sec;
	adc_ticks = sys_start.tv_nsec / FA100M14B4C_UTC_CLOCK_NS;
	fmcadc_set_conf(&brd_cfg, FMCADC_CONF_UTC_TIMING_BASE_T, adc_ticks);
	fmcadc_set_conf(&brd_cfg, FMCADC_CONF_UTC_TIMING_BASE_S, adc_sec);
	err = fmcadc_apply_config(adc, 0 , &brd_cfg);
	if (err && errno != FMCADC_ENOMASK) {
		fprintf(stderr, "%s: cannot configure board %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	fprintf(stdout,
		"ADC clock configured:  %010li s  %010li ns ( %i %ins ticks)\n",
		sys_start.tv_sec, sys_start.tv_nsec,
		adc_ticks, FA100M14B4C_UTC_CLOCK_NS);


	/* Measure how clock diverge */
	while (interval--) {
		/* Get the system clock */
		err = clock_gettime(CLOCK_REALTIME, &sys_cur);
		if (err) {
			fprintf(stderr, "%s: cannot get real time: %s",
				argv[0], strerror(errno));
			exit(1);
		}

		/* Get the ADC clock */
		err = fmcadc_retrieve_config(adc, &brd_cfg);
		if (err) {
			fprintf(stderr, "%s: cannot get trigger config: %s\n",
				argv[0], fmcadc_strerror(errno));
			exit(1);
		}
		fmcadc_get_conf(&brd_cfg, FMCADC_CONF_UTC_TIMING_BASE_S,
				&adc_sec);
		fmcadc_get_conf(&brd_cfg, FMCADC_CONF_UTC_TIMING_BASE_T,
				&adc_ticks);
		adc_cur.tv_sec = adc_sec;
		adc_cur.tv_nsec = adc_ticks * FA100M14B4C_UTC_CLOCK_NS;

		/* Get the difference between system and ADC clock */
		timespec_subtract(&dlt_ts, &sys_cur, &adc_cur);
		/* How bad is? */
		timespec_subtract(&dlt_dlt_ts, &dlt_ts, &dlt_dlt_ts);

		/* Show time stamps and delta */
		printf(" sys   %ld.%.9ld s\n", sys_cur.tv_sec,
			sys_cur.tv_nsec);
		printf(" adc   %ld.%.9ld s\n", adc_cur.tv_sec,
			adc_cur.tv_nsec);
		printf("|dlt|  %ld.%.9ld s (%ld.%.9ld s)\n",
			dlt_ts.tv_sec, dlt_ts.tv_nsec,
			dlt_dlt_ts.tv_sec, dlt_dlt_ts.tv_nsec);
		dlt_dlt_ts = dlt_ts;

		if (dlt_ts.tv_sec) {
			timespec_subtract(&dlt_dlt_ts, &sys_cur, &sys_start);
			printf("Clock diverged of %ld.%.9ld s in %ld.%.9ld s\n",
				dlt_ts.tv_sec, dlt_ts.tv_nsec,
				dlt_dlt_ts.tv_sec, dlt_dlt_ts.tv_nsec);
			break;
		}
		printf("          sleep  1 s\n");
		sleep(1);
	}

	exit(0);
}
