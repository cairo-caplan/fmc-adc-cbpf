/*
 * Copyright CERN 2013
 * Author: Federico Vaga <federico.vaga@gmail.com>
 */
#ifndef FMCADC_LIB_INT_H_
#define FMCADC_LIB_INT_H_

/*
 * offsetof and container_of come from kernel.h header file
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = ((void *)ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#define to_dev_zio(dev) (container_of(dev, struct __fmcadc_dev_zio, gid))

/* ->open takes different args than open(), so fa a fun to use tpyeof */
struct fmcadc_board_type;
struct fmcadc_dev *fmcadc_internal_open(const struct fmcadc_board_type *b,
					unsigned int dev_id,
					unsigned long totalsamples,
					unsigned int nbuffer,
					unsigned long flags);

/*
 * The operations structure is the device-specific backend of the library
 */
struct fmcadc_operations {
	typeof(fmcadc_internal_open)	*open;
	typeof(fmcadc_close)		*close;

	typeof(fmcadc_acq_start)	*acq_start;
	typeof(fmcadc_acq_poll)		*acq_poll;
	typeof(fmcadc_acq_stop)		*acq_stop;

	typeof(fmcadc_apply_config)	*apply_config;
	typeof(fmcadc_retrieve_config)	*retrieve_config;
	typeof(fmcadc_get_param)	*get_param;
	typeof(fmcadc_set_param)	*set_param;

	typeof(fmcadc_request_buffer)	*request_buffer;
	typeof(fmcadc_fill_buffer)	*fill_buffer;
	typeof(fmcadc_tstamp_buffer)	*tstamp_buffer;
	typeof(fmcadc_release_buffer)	*release_buffer;
};
/*
 * This structure describes the board supported by the library
 * @name name of the board type, for example "fmc-adc-100MS"
 * @devname name of the device in Linux
 * @driver_type: the kind of driver that hanlde this kind of board (e.g. ZIO)
 * @capabilities bitmask of device capabilities for trigger, channel
 *               acquisition
 * @fa_op pointer to a set of operations
 */
struct fmcadc_board_type {
	char			*name;
	char			*devname;
	char			*driver_type;
	uint32_t		capabilities[__FMCADC_CONF_TYPE_LAST_INDEX];
	struct fmcadc_operations *fa_op;
};

/*
 * Generic Instance Descriptor
 */
struct fmcadc_gid {
	const struct fmcadc_board_type *board;
};

/* Definition of board types */
extern struct fmcadc_board_type fmcadc_100ms_4ch_14bit;

/* Internal structure (ZIO specific, for ZIO drivers only) */
struct __fmcadc_dev_zio {
	unsigned int cset;
	int fdc;
	int fdd;
	uint32_t dev_id;
	unsigned long flags;
	char *devbase;
	char *sysbase;
	unsigned long samplesize;
	unsigned long pagesize;
	/* Mandatory field */
	struct fmcadc_gid gid;
};
/* Note: bit 16 and up are passed by users, see fmcadc-lib.h */
#define FMCADC_FLAG_VERBOSE 0x00000001
#define FMCADC_FLAG_MALLOC  0x00000002 /* allocate data */
#define FMCADC_FLAG_MMAP    0x00000004 /* mmap data */

/* The board-specific functions are defined in fmc-adc-100m14b4cha.c */
struct fmcadc_dev *fmcadc_zio_open(const struct fmcadc_board_type *b,
				   unsigned int dev_id,
				   unsigned long totalsamples,
				   unsigned int nbuffer,
				   unsigned long flags);
int fmcadc_zio_close(struct fmcadc_dev *dev);

int fmcadc_zio_acq_start(struct fmcadc_dev *dev,
			 unsigned int flags, struct timeval *timeout);
int fmcadc_zio_acq_poll(struct fmcadc_dev *dev, unsigned int flags,
			struct timeval *timeout);
int fmcadc_zio_acq_stop(struct fmcadc_dev *dev,
			unsigned int flags);
struct fmcadc_buffer *fmcadc_zio_request_buffer(struct fmcadc_dev *dev,
						int nsamples,
						void *(*alloc)(size_t),
						unsigned int flags);
int fmcadc_zio_fill_buffer(struct fmcadc_dev *dev,
			   struct fmcadc_buffer *buf,
			   unsigned int flags,
			   struct timeval *timeout);
struct fmcadc_timestamp *fmcadc_zio_tstamp_buffer(struct fmcadc_buffer *buf,
						  struct fmcadc_timestamp *);
int fmcadc_zio_release_buffer(struct fmcadc_dev *dev,
			      struct fmcadc_buffer *buf,
			      void (*free_fn)(void *));

/* The following functions are in config-zio.c */
int fmcadc_zio_apply_config(struct fmcadc_dev *dev, unsigned int flags,
			    struct fmcadc_conf *conf);
int fmcadc_zio_retrieve_config(struct fmcadc_dev *dev,
			       struct fmcadc_conf *conf);
int fmcadc_zio_set_param(struct fmcadc_dev *dev, char *name,
			 char *sptr, int *iptr);
int fmcadc_zio_get_param(struct fmcadc_dev *dev, char *name,
			 char *sptr, int *iptr);


int fa_zio_sysfs_set(struct __fmcadc_dev_zio *fa, char *name,
		     uint32_t *value);

#endif /* FMCADC_LIB_INT_H_ */
