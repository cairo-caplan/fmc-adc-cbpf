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
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/zio-user.h>
#include <fmcadc-lib.h>
#include <fmc-adc-100m14b4cha.h>
#include <signal.h>

#ifdef DEBUG
#define fald_print_debug(format, ...) \
	fprintf(stdout, "%s(%d)" format, __func__, __LINE__, ##__VA_ARGS__)

#else
#define fald_print_debug(format, ...)
#endif

void stop_adc(char *called_from);

static void fald_help()
{
	printf("\nfald-simple-acq [OPTIONS] <LUN>\n\n");
	printf("  <LUN>: LUN identifier  (e.g.: \"0\")\n");
	printf("  --before|-b <num>        number of pre samples\n");
	printf("  --after|-a <num>         n. of post samples (default: 16)\n");
	printf("  --nshots|-n <num>        number of trigger shots\n");
	printf("  --delay|-d <num>         delay sample after trigger\n");
	printf("  --under-sample|-u|-D <num>  pick 1 sample every <num>\n");
	printf("  --external|-e            use external trigger\n");
	printf("  --threshold|-t <num>     internal trigger threshold\n");
	printf("  --channel|-c <num>       channel used as trigger (1..4)\n");
	printf("  --range|-r <num>         channel input range: "
						"100(100mv) 1(1v) 10(10v)\n");
	printf("  --tiemout|-T <millisec>  timeout for acquisition\n");
	printf("  --negative-edge          internal trigger is falling edge\n");
	printf("  --binary|-B <file>       save binary to <file>\n");
	printf("  --multi-binary|-M <file> save two files per shot: "
						"<file>.0000.ctrl etc\n");
	printf("  --dont-read|-N           config-only, use with zio-dump\n");
	printf("  --loop|-l <num>          number of loop before exiting\n");
	printf("  --show-data|-s <num>     how many data to display: "
						">0 from head, <0 from tail\n");
	printf("  --graph|-g <chnum>       plot the desired channel\n");
	printf("  --X11|-X                 Gnuplot will use X connection\n");
	printf("  --help|-h                show this help\n\n");
}

static int trg_cfgval[__FMCADC_CONF_LEN]; /* FIXME: this is not used */
static struct option options[] = {
	{"before",	required_argument, 0, 'b'},
	{"after",	required_argument, 0, 'a'},
	{"nshots",	required_argument, 0, 'n'},
	{"delay",	required_argument, 0, 'd'},
	{"under-sample",required_argument, 0, 'u'},
	{"external", no_argument,
			&trg_cfgval[FMCADC_CONF_TRG_SOURCE], 1},
	{"threshold",	required_argument, 0, 't'},
	{"channel",	required_argument, 0, 'c'},
	{"timeout",	required_argument, 0, 'T'},
	{"negative-edge", no_argument,
			&trg_cfgval[FMCADC_CONF_TRG_POLARITY], 1},

	/* new options, to help stress-test */
	{"binary",	required_argument, 0, 'B'},
	{"multi-binary",required_argument, 0, 'M'},
	{"dont-read",	no_argument,       0, 'N'},
	{"loop",	required_argument, 0, 'l'},
	{"show-data",	required_argument, 0, 's'},
	{"graph",	required_argument, 0, 'g'},
	{"X11 display",	no_argument,       0, 'X'},
	{"input-range",	required_argument, 0, 'r'},

	/* backward-compatible options */
	{"pre",		required_argument, 0, 'p'},
	{"post",	required_argument, 0, 'P'},
	{"decimation",	required_argument, 0, 'D'},

	/* loop for stess test */
	{"loop",	required_argument, 0, 'l'},

	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

#define GETOPT_STRING "b:a:n:d:u:t:c:T:B:M:N:l:s:r:g:X:p:P:D:he"

/* variables shared between threads */
static struct fmcadc_dev *adc;
unsigned int devid = 0;
static pthread_mutex_t mtx;
static pthread_cond_t readyToPollCondVar;
static pthread_cond_t readyToReadOrCfgCondVar;
static int adc_wait_thread_ready;
static int adc_state;
static int poll_state;
static int new_config = 0;
static struct fmcadc_conf trg_cfg, acq_cfg, ch_cfg;
static int show_ndata = INT_MAX; /* by default all values are displayed */
static int plot_chno = -1;
static int x_display;
static int binmode;
static int timeout = -1;
static int loop = 1;
static char *basefile;
#define MAX_BUF 512
static char buf_fifo[MAX_BUF];
static char *_argv[16];
static int _argc;
#define ADC_STATE_START_ACQ (1 << 0)
#define ADC_STATE_CHANGE_CFG (1 << 1)
#define ADC_STATE_FAILURE (1 << 2)
#define START_POLL 1

/* default is 1 V*/
static double bit_scale = 0.5/(1<<15);
static int write_file(const char *filename, int ch, int16_t *pdata,
		      unsigned int n_sample)
{
	FILE *filp;
	int i;
	double val;

	int mask = umask(0); /* set umask to have a file with 0666 mode */
	filp = fopen(filename, "w");
	umask(mask); /* restore previous umask */
	if (filp == NULL) {
		printf("fopen %s failed: %s", filename, strerror(errno));
		return -1;
	}
	for (i = 0, pdata += (ch - 1); i < n_sample; ++i, pdata += 4) {
		val = (*pdata)*bit_scale;
		fprintf(filp, "%d %g\n", i, val);
	}
	if (fclose(filp) < 0)
		return -1;
	return 0;
}

void parse_args(int argc, char *argv[])
{
	int c, opt_index, val;

	optind = 1; /* set to 1 to make getopt_long happy */
	/* Parse options */
	while ((c = getopt_long(argc, argv, GETOPT_STRING, options, &opt_index))
	       >= 0 ) {
		switch (c) {
		case 'b': case 'p': /* before */
			fprintf(stdout, "FMCADC_CONF_ACQ_PRE_SAMP: %d\n",
				atoi(optarg));
			fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_PRE_SAMP,
					atoi(optarg));
			break;
		case 'a': case 'P': /* after */
			fprintf(stdout, "FMCADC_CONF_ACQ_POST_SAMP: %d\n",
				atoi(optarg));
			fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_POST_SAMP,
					atoi(optarg));
			break;
		case 'n':
			fprintf(stdout, "FMCADC_CONF_ACQ_N_SHOTS: %d\n",
				atoi(optarg));
			fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_N_SHOTS,
					atoi(optarg));
			break;
		case 'd':
			fprintf(stdout, "FMCADC_CONF_TRG_DELAY: %d\n",
				atoi(optarg));
			fmcadc_set_conf(&trg_cfg, FMCADC_CONF_TRG_DELAY,
					atoi(optarg));
			break;
		case 'u': case 'D':
			fprintf(stdout, "FMCADC_CONF_ACQ_DECIMATION: %d\n",
				atoi(optarg));
			fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_DECIMATION,
					atoi(optarg));
			break;
		case 't':
			fprintf(stdout, "FMCADC_CONF_TRG_THRESHOLD: %d\n",
				atoi(optarg));
			fmcadc_set_conf(&trg_cfg, FMCADC_CONF_TRG_THRESHOLD,
					atoi(optarg));
			break;
		/*
		 * in-range
		 * 0x23 (35): 100mV range
		 * 0x11 (17): 1V range
		 * 0x45 (69): 10V range
		 * 0x00 (0): Open input
		 */
		case 'r':
			val = atoi(optarg);
			switch (val) {
			case 100:
				val = 0x23;
				bit_scale = 0.05/(1<<15);
				break;
			case 1:
				val = 0x11;
				bit_scale = 0.5/(1<<15);
				break;
			case 10:
				val = 0x45;
				bit_scale = 5.0/(1<<15);
				break;
			}
			fprintf(stdout, "FMCADC_CONF_CHN_RANGE: %d\n", val);
			fmcadc_set_conf(&ch_cfg, FMCADC_CONF_CHN_RANGE,
					val);
			break;
		case 'c':
			val = atoi(optarg);
			if (val < 1 || val > 4) {
				fprintf(stderr, "Invalid channel %d\n", val);
				fald_help();
				exit(1);
			}
			fprintf(stdout, "FMCADC_CONF_TRG_SOURCE_CHAN: %d\n",
				atoi(optarg));
			/* set internal, and then the channel */
			trg_cfgval[FMCADC_CONF_TRG_SOURCE] = 0; /* set later */
			fmcadc_set_conf(&trg_cfg, FMCADC_CONF_TRG_SOURCE_CHAN,
					val - 1);
			break;
		case 'e':
			trg_cfgval[FMCADC_CONF_TRG_SOURCE] = 1;
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
		case 'l':
			loop = atoi(optarg);
			break;
		case 's':
			show_ndata = atoi(optarg);
			break;
		case 'g':
			plot_chno = atoi(optarg);
			if (plot_chno < 1 || plot_chno > 4) {
				fprintf(stderr, "Invalid channel %d\n",
					plot_chno);
				fald_help();
				exit(1);
			}
			fprintf(stdout, "Plot channel %d\n", plot_chno);
			break;
		case 'X':
			x_display = 1;
			fprintf(stdout, "Gnuplot will use X display\n");
			break;

		case 'h': case '?':
			fald_help();
			exit(1);
			break;
		}
	}
	/* Configure trigger (pick trigger polarity from external array) */
	fmcadc_set_conf(&trg_cfg, FMCADC_CONF_TRG_POLARITY,
			trg_cfgval[FMCADC_CONF_TRG_POLARITY]);
	fmcadc_set_conf(&trg_cfg, FMCADC_CONF_TRG_SOURCE,
			trg_cfgval[FMCADC_CONF_TRG_SOURCE]);
}

void apply_config()
{
	int err;

	err = fmcadc_apply_config(adc, 0 , &trg_cfg);
	if (err && errno != FMCADC_ENOMASK) {
		fprintf(stderr, "%s: cannot configure trigger: %s\n",
			_argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Configure acquisition parameter */
	err = fmcadc_apply_config(adc, 0 , &acq_cfg);
	if (err && errno != FMCADC_ENOMASK) {
		fprintf(stderr, "%s: cannot configure acquisition: %s\n",
			_argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Configure channel parameter */
	err = fmcadc_apply_config(adc, 0 , &ch_cfg);
	if (err && errno != FMCADC_ENOMASK) {
		fprintf(stderr, "%s: cannot configure channel0: %s\n",
			_argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	// raise new_config flag
	pthread_mutex_lock(&mtx);
	new_config = 1;
	pthread_mutex_unlock(&mtx);
}

void start_adc(char *called_from, int flag)
{
	int try = 5, err;
	struct timeval tv = {0, 0};

	fald_print_debug("%s : call fmcadc_acq_start with %s\n",
			 called_from, ((flag) ? "flush" : "no flush"));
	while (try) {
		err = fmcadc_acq_start(adc, flag, &tv);
		if (!err)
			break;

		/* Cannot start acquisition right now */
		fprintf(stderr, "%s: cannot start acquisition: %s\n(Retry)\n",
			_argv[0], fmcadc_strerror(errno));
		/* Instead of leaving try another stop/start sequence */
		stop_adc("start_adc");
		/* give a chance to breath in case the error persists */
		sleep(1);
		try--;
	}

	/*
	 * If also the last try fails, then set FAILURE state
	 * Otherwise, start polling
	 */
	if (!try) {
		fald_print_debug("%s: Cannot start acquisition. Exit\n");
		pthread_mutex_lock(&mtx);
		adc_state |= ADC_STATE_FAILURE;
		pthread_mutex_unlock(&mtx);
	} else {
		/* Start the poll */
		pthread_mutex_lock(&mtx);
		poll_state = START_POLL; /* adc has been flushed and started */
		pthread_mutex_unlock(&mtx);
		pthread_cond_signal(&readyToPollCondVar);
		fald_print_debug("%s send signal to readyToPollCondVar\n", called_from);
	}
}

void stop_adc(char *called_from)
{
	int try = 5, err;

	/* stop any pending acquisition */
	fald_print_debug("%s: call fmcadc_acq_stop\n", called_from);
	while (try) {
		err = fmcadc_acq_stop(adc, 0);
		if (!err)
			break;

		fprintf(stderr, "%s: cannot stop acquisition: %s\n(Retry)\n",
			_argv[0], fmcadc_strerror(errno));

		try--;
	}

	/* If also the last try fails, then set FAILURE state */
	if (!try) {
		fald_print_debug("%s: Cannot stop acquisition. Exit\n");
		pthread_mutex_lock(&mtx);
		adc_state |= ADC_STATE_FAILURE;
		pthread_mutex_unlock(&mtx);
	}
}
void *adc_wait_thread(void *arg)
{
	int err;

	for (;;) {
		pthread_mutex_lock(&mtx);
		while (!poll_state) {
			fald_print_debug("cond_wait readyToPollCondVar\n");
			adc_wait_thread_ready = 1;
			pthread_cond_wait(&readyToPollCondVar, &mtx);
		}
		poll_state = 0;
		pthread_mutex_unlock(&mtx);
		fald_print_debug("It's time to call fmcadc_acq_poll\n");
		err = fmcadc_acq_poll(adc, 0 , NULL);
		if (err) {
			if (errno == FMCADC_EDISABLED) {
				fprintf(stderr, "fmcadc_acq_poll has been aborted due to a triiger stop:  err:%d errno:%s(%d)\n",
					err, fmcadc_strerror(errno), errno);
				continue;
			} else {
				fprintf(stderr, "fmcadc_acq_poll failed:  err:%d errno:%s(%d)\n",
					err, strerror(errno), errno);
				exit(-1);
			}
		}
		fald_print_debug("fmc-adc_poll ends normally, send signal to readyToReadOrCfgCondVar requesting to read data\n");
		pthread_mutex_lock(&mtx);
		adc_state |= ADC_STATE_START_ACQ; /* means start acquisition */
		pthread_mutex_unlock(&mtx);
		// Wake up the change config thread
		pthread_cond_signal(&readyToReadOrCfgCondVar);
	}
}

void *change_config_thread(void *arg)
{
	int fd, ret;
	char adcfifo[128];
	char *s, *t;
	char buf[MAX_BUF];

	if (access(adcfifo, F_OK) == -1) {
		sprintf(adcfifo, "/tmp/adcfifo-%04x", devid);
		/* create the FIFO (named pipe) */
		mkfifo(adcfifo, 0666);
	}
	/* open, read, and display the message from the FIFO */
	fd = open(adcfifo, O_RDONLY);
	for (;;) {
		memset(buf, 0, MAX_BUF);
		ret = read(fd, buf, MAX_BUF);
		if (ret > 0) {
			_argc = 1;
			memcpy(buf_fifo, buf, MAX_BUF); /* for future parsing */
			s = buf_fifo;
			while ((t = strtok(s, " ")) != NULL) {
				s = NULL;
				_argv[_argc++] = t;
			}

			/* async way of changing trig config */
			stop_adc("change_config");
			parse_args(_argc, _argv);
			apply_config();
			fprintf(stdout, "mainThread: Change trig config ................. done\n");

			start_adc("change_config", FMCADC_F_FLUSH) ;

		} else {
			fprintf(stdout, "read returns %d\n", ret);
			/* writer close the fifo. Colse the reader side */
			close(fd);
			/* should block until the writer is back */
			fd = open(adcfifo, O_RDONLY);
			continue;
		}
	}
}

void create_thread()
{
	/* Config thread */
	pthread_attr_t thread_attr;
	pthread_t tid;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	int res = pthread_create(&tid, &thread_attr, change_config_thread, NULL);

	if (res)
		fprintf(stderr, "Cannot create 'change_config_thread' (%d)\n",
			res);

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	res = pthread_create(&tid, &thread_attr, adc_wait_thread, NULL);
	if (res)
		fprintf(stderr, "Cannot create 'adc_wait_thread' (%d)\n", res);

	// initialize condition variables
	pthread_condattr_t condVarAttr;
	pthread_condattr_init(&condVarAttr);
	pthread_cond_init(&readyToPollCondVar, &condVarAttr);
	pthread_condattr_destroy(&condVarAttr);
	pthread_condattr_init(&condVarAttr);
	pthread_cond_init(&readyToReadOrCfgCondVar, &condVarAttr);
	pthread_condattr_destroy(&condVarAttr);
	// initialize mutexes
	pthread_mutexattr_t mtxAttr;
	pthread_mutexattr_init(&mtxAttr);
	pthread_mutex_init(&mtx, &mtxAttr);
	pthread_mutexattr_destroy(&mtxAttr);
	/* Give a chance to newly created thread to be up and running */
//	sleep(1);
}

int main(int argc, char *argv[])
{
	struct fmcadc_buffer *buf;
	int i, err;
	FILE *f = NULL;
	char fname[PATH_MAX];
	char cmd[256];
	struct zio_control *ctrl;
	
	pid_t pid;

	if ((pid = fork()!=0)){
		//father process
		//puts("Im child\n");
			usleep(100000);
			//puts("Too late\n");
			kill(pid, SIGKILL);
			return 1;
	}

	if (argc == 1) {
		fprintf(stderr, "%s: DEVICE-ID is a mandatory argument\n",
			argv[0]);
		fald_help();
		exit(1);
	}
	/* set local _argv[0] with  pg name */
	_argv[0] = argv[0];
	/* devid is the last arg */
	sscanf(argv[argc-1], "%x", &devid);

	/* Open the ADC */
	adc = fmcadc_open("fmc-adc-100m14b4cha", devid,
		/* nshots * (presamples + postsamples) */
		/*
		acq.value[FMCADC_CONF_ACQ_N_SHOTS] *
		( acq.value[FMCADC_CONF_ACQ_PRE_SAMP] +
		acq.value[FMCADC_CONF_ACQ_POST_SAMP] )*/ 0,
		/*acq.value[FMCADC_CONF_ACQ_N_SHOTS]*/ 0,
		FMCADC_F_FLUSH /*0*/);
	if (!adc) {
		fprintf(stderr, "%s: cannot open device: %s",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* Before parsing args : */
	/* First retrieve current config in case the program */
	/* is launched with a subset of options */
	memset(&trg_cfg, 0, sizeof(trg_cfg));
	trg_cfg.type = FMCADC_CONF_TYPE_TRG;
	fmcadc_set_conf_mask(&trg_cfg, FMCADC_CONF_TRG_SOURCE);
	fmcadc_set_conf_mask(&trg_cfg, FMCADC_CONF_TRG_SOURCE_CHAN);
	fmcadc_set_conf_mask(&trg_cfg, FMCADC_CONF_TRG_THRESHOLD);
	fmcadc_set_conf_mask(&trg_cfg, FMCADC_CONF_TRG_POLARITY);
	fmcadc_set_conf_mask(&trg_cfg, FMCADC_CONF_TRG_DELAY);
	err = fmcadc_retrieve_config(adc, &trg_cfg);
	if (err) {
		fprintf(stderr, "%s: cannot get trigger config: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	memset(&acq_cfg, 0, sizeof(acq_cfg));
	acq_cfg.type = FMCADC_CONF_TYPE_ACQ;
	fmcadc_set_conf_mask(&acq_cfg, FMCADC_CONF_ACQ_N_SHOTS);
	fmcadc_set_conf_mask(&acq_cfg, FMCADC_CONF_ACQ_POST_SAMP);
	fmcadc_set_conf_mask(&acq_cfg, FMCADC_CONF_ACQ_PRE_SAMP);
	fmcadc_set_conf_mask(&acq_cfg, FMCADC_CONF_ACQ_DECIMATION);
	err = fmcadc_retrieve_config(adc, &acq_cfg);
	if (err) {
		fprintf(stderr, "%s: cannot get acquisition config: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}
	memset(&ch_cfg, 0, sizeof(ch_cfg));
	ch_cfg.type = FMCADC_CONF_TYPE_CHN;
	ch_cfg.route_to = 0; /* channel 0 */
	fmcadc_set_conf_mask(&ch_cfg, FMCADC_CONF_CHN_RANGE);
	fmcadc_set_conf_mask(&ch_cfg, FMCADC_CONF_CHN_TERMINATION);
	fmcadc_set_conf_mask(&ch_cfg, FMCADC_CONF_CHN_OFFSET);
	err = fmcadc_retrieve_config(adc, &ch_cfg);
	if (err) {
		fprintf(stderr, "%s: cannot get channel config: %s\n",
			argv[0], fmcadc_strerror(errno));
		exit(1);
	}

	/* get the new given trigger and acq config */
	/* Only the ones provided will override the current ones */
	parse_args(argc, argv);


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
	/* create the various thread and sync mechanism */
	create_thread();

	while (!adc_wait_thread_ready)
		nanosleep((struct timespec[]){{0, 10000}}, NULL);
	fprintf(stdout, "adc_wait_thread_ready\n");

	/* configure adc */
	stop_adc("main");
	apply_config();
	/* Allocate a first buffer in the default way */
	buf = fmcadc_request_buffer(adc,
		acq_cfg.value[FMCADC_CONF_ACQ_PRE_SAMP] +
		acq_cfg.value[FMCADC_CONF_ACQ_POST_SAMP],
		NULL /* alloc */, 0);
	if (!buf) {
		fprintf(stderr, "Cannot allocate buffer (%s)\n",
			fmcadc_strerror(errno));
		exit(1);
	}
	start_adc("main", FMCADC_F_FLUSH);
	while (loop > 0) {
		pthread_mutex_lock(&mtx);
		while (!adc_state) {
			fald_print_debug("mainThread: cond_wait readyToReadOrCfgCondVar\n");
			pthread_cond_wait(&readyToReadOrCfgCondVar, &mtx);
			fald_print_debug("mainThread:  waked up adc_state:%d\n", adc_state);
		}

		fald_print_debug("mainThread: wakeup readyToReadOrCfgCondVar with adc_state: %s\n",
				 ((adc_state==1)?"Read data":"Change trigger config"));

		/* If the system fails stop the loop and close the program */
		if (adc_state & ADC_STATE_FAILURE) {
			pthread_mutex_unlock(&mtx);
			break;
		}

		/* time to acquire data */
		adc_state &= ~ADC_STATE_START_ACQ; /* ack time to acquire data */
		if (new_config && buf != NULL) {
			/* first release previous buffer */
			fmcadc_release_buffer(adc, buf, NULL);
			buf = NULL;
			new_config = 0;
		}
		pthread_mutex_unlock(&mtx);

		if (buf == NULL) { /* buf has been released due to a change of trig config */
			/* Allocate a buffer in the default way */
			buf = fmcadc_request_buffer(adc,
				acq_cfg.value[FMCADC_CONF_ACQ_PRE_SAMP] +
				acq_cfg.value[FMCADC_CONF_ACQ_POST_SAMP],
				NULL /* alloc */, 0);
			if (!buf) {
				fprintf(stderr, "Cannot allocate buffer (%s)\n",
					fmcadc_strerror(errno));
				exit(1);
			}
		}
		/* Fill the buffer once for each shot */
		for (i = 0; i < acq_cfg.value[FMCADC_CONF_ACQ_N_SHOTS]; ++i) {
			int j, ch;
			int16_t *data;

			if (binmode < 0) /* no data must be acquired */
				break;

			err = fmcadc_fill_buffer(adc, buf, 0, NULL);
			if (err) {
				if (errno == FMCADC_EDISABLED) {
					fprintf(stdout, "mainThread: leaves fmcadc_fill_buffer with errno=FMCADC_EDISABLED\n");
					break;
				}
				fprintf(stderr, "%s: shot %i/%i: cannot fill buffer:"
					" %s\n", argv[0], i + i,
					acq_cfg.value[FMCADC_CONF_ACQ_N_SHOTS],
				fmcadc_strerror(errno));
				exit(1);
			}
			ctrl = buf->metadata;
			data = buf->data;
			/* FIXME adc-lib should provide enums to retrive
			 * attributes values */
			fprintf(stderr, "Acquisition started at secs:%u ticks:%u\n",
				ctrl->attr_channel.ext_val[FA100M14B4C_DATTR_ACQ_START_S],
				ctrl->attr_channel.ext_val[FA100M14B4C_DATTR_ACQ_START_C]);
			fprintf(stderr, "Read %d samples from shot %i/%i secs:%lld ticks:%lld (loop: %d)\n",
				ctrl->nsamples,
				i + 1, acq_cfg.value[FMCADC_CONF_ACQ_N_SHOTS],
				(long long)ctrl->tstamp.secs, (long long)ctrl->tstamp.ticks, loop);

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
			if (show_ndata != 0) {
				if (ctrl->nsamples / 4 != buf->nsamples) {
					fprintf(stdout, "discrepancy between ctrl->nsamples: %d and buf->nsamples: %d\n", ctrl->nsamples, buf->nsamples);
				} else {
					for (j = 0; j < ctrl->nsamples / 4; j++) {
						if ( (show_ndata > 0 && j < show_ndata) ||
							(show_ndata < 0 && (ctrl->nsamples / 4 - j) <= (-show_ndata)) ) {
							printf("%5i     ", j - acq_cfg.value[FMCADC_CONF_ACQ_PRE_SAMP]);
							for (ch = 0; ch < 4; ch++)
								printf("%7i", *(data++));
							printf("\n");
						}
						else
							data += 4;
					}
				}
			}
		}
		if (binmode == 1)
			fclose(f);

		--loop;
		if (loop > 0)
			start_adc("End of data processing", 0);
		else if (plot_chno != -1) {
			if ((plot_chno>=1) && (plot_chno <= 4) ) {
				snprintf(fname, sizeof(fname), "/tmp/fmcadc.0x%04x.ch%d.dat", devid, plot_chno);
				if (write_file(fname, plot_chno, (int16_t *)buf->data,
						(ctrl->nsamples)/4) < 0) {
					printf("Write data into file %s failed\n", fname);
					return -1;
				}
				snprintf(cmd, sizeof(cmd), "echo \"%s plot '%s' with lines "
						"\" | gnuplot -persist",
					(x_display==1) ? "set term x11; " : "set term dumb; ",
					fname);
				cmd[sizeof(cmd) - 1] = '\0';
				system(cmd);
			} else {
				printf("illegal ch number.\n");
			}
		}
	}
	fmcadc_close(adc);
	exit(0);
}
