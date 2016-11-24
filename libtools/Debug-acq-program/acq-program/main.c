/** Copyright 2013 CERN
 * Author: Federico Vaga <federico.vaga@gmail.com>
 * License: GPLv2
 *
 * This is a simple program to configure the FMC ADC trigger. It is not bug
 * aware because it is only a demo program to show you how you can handle the
 * trigger.
 *
 * Modified by Cairo Caplan <cairo@cbpf.br> in 2015
 * To be used during the ISOTDAQ schools lab 8, based on the fald-simple-acq
 * example.
 *
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
#include "../../../zio/include/linux/zio-user.h"
#include "../../../lib/fmcadc-lib.h"
static void fald_help()
{
    printf("\nacq-program [OPTIONS] <LUN>\n\n");
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

static struct fmcadc_dev *adc;
unsigned int devid = 0;

struct fmcadc_conf trg_cfg, acq_cfg, ch_cfg;
static int show_ndata = INT_MAX; /* by default all values are displayed */

static int plot_chno = -1;
static int binmode;
static int timeout = -1;
static int loop = 1;
static char *basefile = NULL;

//static char buf_fifo[MAX_BUF];
static char *_argv[16];

/* default is 1 V */
static double bit_scale = 0.5/(1<<15);


void parse_args(int argc, char *argv[])
{
    int c, opt_index, val;

    optind = 1; /* set to 1 to make getopt_long happy */
    /* Parse options */
    while ((c = getopt_long(argc, argv, GETOPT_STRING, options, &opt_index))
           >= 0 ) {
        switch (c) {
        case 'b': case 'p': /* before */
            fprintf(stderr, "FMCADC_CONF_ACQ_PRE_SAMP: %d\n",
                atoi(optarg));
            fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_PRE_SAMP,
                    atoi(optarg));
            break;
        case 'a': case 'P': /* after */
            fprintf(stderr, "FMCADC_CONF_ACQ_POST_SAMP: %d\n",
                atoi(optarg));
            fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_POST_SAMP,
                    atoi(optarg));
            break;
        case 'n':
            fprintf(stderr, "FMCADC_CONF_ACQ_N_SHOTS: %d\n",
                atoi(optarg));
            fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_N_SHOTS,
                    atoi(optarg));
            break;
        case 'd':
            fprintf(stderr, "FMCADC_CONF_TRG_DELAY: %d\n",
                atoi(optarg));
            fmcadc_set_conf(&trg_cfg, FMCADC_CONF_TRG_DELAY,
                    atoi(optarg));
            break;
        case 'u': case 'D':
            fprintf(stderr, "FMCADC_CONF_ACQ_DECIMATION: %d\n",
                atoi(optarg));
            fmcadc_set_conf(&acq_cfg, FMCADC_CONF_ACQ_DECIMATION,
                    atoi(optarg));
            break;
        case 't':
            fprintf(stderr, "FMCADC_CONF_TRG_THRESHOLD: %d\n",
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
            fprintf(stderr, "FMCADC_CONF_CHN_RANGE: %d\n", val);
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
            fprintf(stderr, "FMCADC_CONF_TRG_SOURCE_CHAN: %d\n",
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
            fprintf(stderr, "Plot channel %d\n", plot_chno);
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



int main(int argc, char *argv[])
{

    struct fmcadc_buffer *buf = NULL;;
    int i, err, binmode = 0;//  c, opt_index
    //int nshots = 1, presamples = 0, postsamples = 16;
    char fname[PATH_MAX];
    FILE *f = NULL;

    if (argc == 1) {
        fald_help();
        exit(1);
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

    /**
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
    */
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

    /* Configure channel parameter */
    err = fmcadc_apply_config(adc, 0 , &ch_cfg);
    if (err && errno != FMCADC_ENOMASK) {
        fprintf(stderr, "%s: cannot configure channel0: %s\n",
            _argv[0], fmcadc_strerror(errno));
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
        puts("TIMEOUT USED");
        err = fmcadc_acq_poll(adc, 0 , &tv);
    }
    if (err) {
        puts("TIMEOUT ERROR");
        fprintf(stderr, "%s: timeout after %i ms: %s\n", argv[0],
            timeout, strerror(errno));
        exit(1);
    }

    /* Allocate a buffer in the default way */

    //buf = fmcadc_request_buffer(adc, presamples + postsamples,
    //			    NULL /* alloc */, 0);
    //if (!buf) {
    //	fprintf(stderr, "Cannot allocate buffer (%s)\n",
    //		fmcadc_strerror(errno));
    //	exit(1);
    //}

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
        struct zio_control *ctrl;
        int j, ch;
        int16_t *data;

        if (binmode < 0) /* no data must be acquired */
            break;

        err = fmcadc_fill_buffer(adc, buf, 0, NULL);
        if (err) {
            fprintf(stderr, "%s: shot %i/%i: cannot fill buffer:"
                " %s\n", argv[0], i + i,
                acq_cfg.value[FMCADC_CONF_ACQ_N_SHOTS],
            fmcadc_strerror(errno));
            exit(1);
        }
        ctrl = buf->metadata;
        data = buf->data;
        fprintf(stderr, "Read %d samples from shot %i/%i\n",
            ctrl->nsamples,
            i + 1, acq_cfg.value[FMCADC_CONF_ACQ_N_SHOTS]);



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
        /*
        for (j = 0; j < ctrl->nsamples / 4; j++) {
            printf("%5i     ", j - presamples);
            for (ch = 0; ch < 4; ch++){
                printf("%7i", *(data++));

                //if (ch==0) printf("%f", (*(data))*(5/32768));
                //data++;
            }
            printf("\n");
        }
        */

        if (show_ndata != 0) {
                if (ctrl->nsamples / 4 != buf->nsamples) {
                    fprintf(stderr, "discrepancy between ctrl->nsamples: %d and buf->nsamples: %d\n", ctrl->nsamples, buf->nsamples);
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

    fmcadc_release_buffer(adc, buf, NULL);
    fmcadc_close(adc);
    exit(0);
}
