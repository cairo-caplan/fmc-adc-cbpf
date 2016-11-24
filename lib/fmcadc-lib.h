/*
 * Copyright CERN 2013
 * Author: Federico Vaga <federico.vaga@gmail.com>
 */

#ifndef FMCADC_LIB_H_
#define FMCADC_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

/* Error codes start from 1024 to void conflicting with libc codes */
#define __FMCADC_ERRNO_START 1024
#define FMCADC_ENOP		1024
#define FMCADC_ENOCAP		1025
#define FMCADC_ENOCFG		1026
#define FMCADC_ENOGET		1027
#define FMCADC_ENOSET		1028
#define FMCADC_ENOCHAN		1029
#define FMCADC_ENOMASK		1030
#define FMCADC_EDISABLED	1031

struct fmcadc_dev;

enum fmcadc_supported_board {
	FMCADC_100MS_4CH_14BIT,
	__FMCADC_SUPPORTED_BOARDS_LAST_INDEX,
};

/* The buffer hosts data and metadata, plus informative fields */
struct fmcadc_buffer {
	void *data;
	void *metadata;
	int samplesize;
	int nsamples;
	struct fmcadc_dev *dev;
	void *mapaddr;
	unsigned long maplen;
	unsigned long flags; /* internal to the library */
};

/* This is exactly the zio_timestamp, there is no depency on zio here */
struct fmcadc_timestamp {
	uint64_t secs;
	uint64_t ticks;
	uint64_t bins;
};

/* The following enum can be use to se the mask of valid configurations */
enum fmcadc_configuration_trigger {
	FMCADC_CONF_TRG_SOURCE = 0,
	FMCADC_CONF_TRG_SOURCE_CHAN,
	FMCADC_CONF_TRG_THRESHOLD,
	FMCADC_CONF_TRG_POLARITY,
	FMCADC_CONF_TRG_DELAY,
	FMCADC_CONF_TRG_THRESHOLD_FILTER,
	__FMCADC_CONF_TRG_ATTRIBUTE_LAST_INDEX,
};
enum fmcadc_configuration_acquisition {
	FMCADC_CONF_ACQ_N_SHOTS = 0,
	FMCADC_CONF_ACQ_POST_SAMP,
	FMCADC_CONF_ACQ_PRE_SAMP,
	FMCADC_CONF_ACQ_DECIMATION,
	FMCADC_CONF_ACQ_FREQ_HZ,
	FMCADC_CONF_ACQ_N_BITS,
	__FMCADC_CONF_ACQ_ATTRIBUTE_LAST_INDEX,
};
enum fmcadc_configuration_channel {
	FMCADC_CONF_CHN_RANGE = 0,
	FMCADC_CONF_CHN_TERMINATION,
	FMCADC_CONF_CHN_OFFSET,
	FMCADC_CONF_CHN_SATURATION,
	__FMCADC_CONF_CHN_ATTRIBUTE_LAST_INDEX,
};
enum fmcadc_board_status {
	FMCADC_CONF_BRD_STATUS = 0,
	FMCADC_CONF_BRD_MAX_FREQ_HZ,
	FMCADC_CONF_BRD_MIN_FREQ_HZ,
	FMCADC_CONF_BRD_STATE_MACHINE_STATUS,
	FMCADC_CONF_BRD_N_CHAN,
	FMCADC_CONF_UTC_TIMING_BASE_S,
	FMCADC_CONF_UTC_TIMING_BASE_T,
	FMCADC_CONF_UTC_TIMING_BASE_B,
	__FMCADC_CONF_BRD_ATTRIBUTE_LAST_INDEX,
};
enum fmcadc_configuration_type {
	FMCADC_CONF_TYPE_TRG = 0,	/* Trigger */
	FMCADC_CONF_TYPE_ACQ,		/* Acquisition */
	FMCADC_CONF_TYPE_CHN,		/* Channel */
	FMCADC_CONT_TYPE_BRD,		/* Board */
	__FMCADC_CONF_TYPE_LAST_INDEX,
};


#define __FMCADC_CONF_LEN 64 /* number of allocated items in each structure */
struct fmcadc_conf {
	enum fmcadc_configuration_type type;
	uint32_t dev_type;
	uint32_t route_to;
	uint32_t flags; /* how to identify invalid? */
	uint64_t mask;
	uint32_t value[__FMCADC_CONF_LEN];
};


static inline void fmcadc_set_conf_mask(struct fmcadc_conf *conf,
				        unsigned int conf_index)
{
	conf->mask |= (1LL << conf_index);
}

/* assign a configuration item, and its mask */
static inline void fmcadc_set_conf(struct fmcadc_conf *conf,
				   unsigned int conf_index, uint32_t val)
{
	conf->value[conf_index] = val;
	fmcadc_set_conf_mask(conf, conf_index);
}

/* retieve a configuration item */
static inline int fmcadc_get_conf(struct fmcadc_conf *conf,
				  unsigned int conf_index,
				  uint32_t *val)
{
	if (conf->mask & (1LL << conf_index)) {
		*val = conf->value[conf_index];
		return 0;
	} else {
		return -1;
	}
}

/* Flags used in open/acq/config -- note: low-bits are used by lib-int.h */
#define FMCSDC_F_USERMASK	0xffff0000
#define FMCADC_F_FLUSH		0x00010000
#define FMCADC_F_VERBOSE	0x00020000

/*
 * Actual functions follow
 */
extern int fmcadc_init(void);
extern void fmcadc_exit(void);
extern char *fmcadc_strerror(int errnum);

extern struct fmcadc_dev *fmcadc_open(char *name, unsigned int dev_id,
				      unsigned long totalsamples,
				      unsigned int nbuffer,
				      unsigned long flags);
extern struct fmcadc_dev *fmcadc_open_by_lun(char *name, int lun,
					     unsigned long totalsamples,
					     unsigned int nbuffer,
					     unsigned long flags);
extern int fmcadc_close(struct fmcadc_dev *dev);

extern int fmcadc_acq_start(struct fmcadc_dev *dev, unsigned int flags,
			    struct timeval *timeout);
extern int fmcadc_acq_poll(struct fmcadc_dev *dev, unsigned int flags,
			    struct timeval *timeout);
extern int fmcadc_acq_stop(struct fmcadc_dev *dev, unsigned int flags);

extern int fmcadc_reset_conf(struct fmcadc_dev *dev, unsigned int flags,
			       struct fmcadc_conf *conf);
extern int fmcadc_apply_config(struct fmcadc_dev *dev, unsigned int flags,
			       struct fmcadc_conf *conf);
extern int fmcadc_retrieve_config(struct fmcadc_dev *dev,
				 struct fmcadc_conf *conf);
extern int fmcadc_get_param(struct fmcadc_dev *dev, char *name,
			    char *sptr, int *iptr);
extern int fmcadc_set_param(struct fmcadc_dev *dev, char *name,
			    char *sptr, int *iptr);

extern struct fmcadc_buffer *fmcadc_request_buffer(struct fmcadc_dev *dev,
						   int nsamples,
						   void *(*alloc_fn)(size_t),
						   unsigned int flags);
extern int fmcadc_fill_buffer(struct fmcadc_dev *dev,
			      struct fmcadc_buffer *buf,
			      unsigned int flags,
			      struct timeval *timeout);
extern struct fmcadc_timestamp *fmcadc_tstamp_buffer(struct fmcadc_buffer *buf,
						     struct fmcadc_timestamp *);
extern int fmcadc_release_buffer(struct fmcadc_dev *dev,
				 struct fmcadc_buffer *buf,
				 void (*free_fn)(void *));

extern char *fmcadc_get_driver_type(struct fmcadc_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* FMCADC_LIB_H_ */
