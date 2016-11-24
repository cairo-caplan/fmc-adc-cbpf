/*
 * Copyright CERN 2012
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * Header for the mezzanine ADC for the SPEC
 */

#ifndef FMC_ADC_100M14B4C_H_
#define FMC_ADC_100M14B4C_H_

/*
 * Trigger Extended Attribute Enumeration
 */
enum fa100m14b4c_trg_ext_attr {
	/*
	 * The trigger extended attribute order is the same in the declaration
	 * and in the zio_control, so we can always use enumeration. But, the
	 * enumeration must start with 0 followed by only consecutive value.
	 *
	 * The parameters are not exposed to user space by zio_controle, so it
	 * is not necessary to export to user space the correspondent enum
	 */
	FA100M14B4C_TATTR_EXT = 0,
	FA100M14B4C_TATTR_POL,
	FA100M14B4C_TATTR_INT_CHAN,
	FA100M14B4C_TATTR_INT_THRES,
	FA100M14B4C_TATTR_DELAY,
#ifdef __KERNEL__
	FA100M14B4C_TATTR_SW_EN,
	FA100M14B4C_TATTR_SW_FIRE,
	FA100M14B4C_TATTR_TRG_S,
	FA100M14B4C_TATTR_TRG_C,
	FA100M14B4C_TATTR_TRG_F,
#endif
};

/*
 * Device Extended Attribute Enumeration
 */
enum fa100m14b4c_dev_ext_attr {
	/*
	 * NOTE: At the moment the only extended attributes we have in
	 * the device hierarchy are in the cset level, so we can safely
	 * start from index 0
	 */
	FA100M14B4C_DATTR_DECI = 0,
	FA100M14B4C_DATTR_CH0_OFFSET,
	FA100M14B4C_DATTR_CH1_OFFSET,
	FA100M14B4C_DATTR_CH2_OFFSET,
	FA100M14B4C_DATTR_CH3_OFFSET,
	FA100M14B4C_DATTR_CH0_VREF,
	FA100M14B4C_DATTR_CH1_VREF,
	FA100M14B4C_DATTR_CH2_VREF,
	FA100M14B4C_DATTR_CH3_VREF,
	FA100M14B4C_DATTR_CH0_50TERM,
	FA100M14B4C_DATTR_CH1_50TERM,
	FA100M14B4C_DATTR_CH2_50TERM,
	FA100M14B4C_DATTR_CH3_50TERM,
	FA100M14B4C_DATTR_ACQ_START_S,
	FA100M14B4C_DATTR_ACQ_START_C,
	FA100M14B4C_DATTR_ACQ_START_F,
};

#define FA100M14B4C_UTC_CLOCK_FREQ 125000000
#define FA100M14B4C_UTC_CLOCK_NS  8
#define FA100M14B4C_NCHAN 4 /* We have 4 of them,no way out of it */

/* ADC DDR memory */
#define FA100M14B4C_MAX_ACQ_BYTE 0x10000000 /* 256MB */
/* In Multi shot mode samples go through a dpram which has a limited size */
#define FA100M14B4C_MAX_MSHOT_ACQ_BYTE 0x3FE8 /* 2045 samples (2045*8 bytes) */

enum fa100m14b4c_input_range {
	FA100M14B4C_RANGE_10V = 0x0,
	FA100M14B4C_RANGE_1V,
	FA100M14B4C_RANGE_100mV,
	FA100M14B4C_RANGE_OPEN,		/* Channel disconnected from ADC */
};

enum fa100m14b4c_fsm_cmd {
	FA100M14B4C_CMD_NONE =	0x0,
	FA100M14B4C_CMD_START =	0x1,
	FA100M14B4C_CMD_STOP =	0x2,
};
/* All possible state of the state machine, other values are invalid*/
enum fa100m14b4c_fsm_state {
	FA100M14B4C_STATE_IDLE = 0x1,
	FA100M14B4C_STATE_PRE,
	FA100M14B4C_STATE_POST,
	FA100M14B4C_STATE_WAIT,
	FA100M14B4C_STATE_DECR,
};


#ifdef __KERNEL__ /* All the rest is only of kernel users */
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>

#include <linux/fmc.h>
#include <linux/zio.h>
#include <linux/zio-trigger.h>

#include "field-desc.h"

/*
 * ZFA_CHx_MULT : the trick which requires channel regs id grouped and ordered
 * address offset between two registers of the same type on consecutive channel
 */
#define ZFA_CHx_MULT 6

/* Device registers */
enum zfadc_dregs_enum {
	/* Device */
	/* Control registers */
	ZFA_CTL_FMS_CMD,
	ZFA_CTL_CLK_EN,
	ZFA_CTL_DAC_CLR_N,
	ZFA_CTL_BSLIP,
	ZFA_CTL_TEST_DATA_EN,
	ZFA_CTL_TRIG_LED,
	ZFA_CTL_ACQ_LED,
	/* Status registers */
	ZFA_STA_FSM,
	ZFA_STA_SERDES_PLL,
	ZFA_STA_SERDES_SYNCED,
	/* Configuration register */
	ZFAT_CFG_HW_SEL,
	ZFAT_CFG_HW_POL,
	ZFAT_CFG_HW_EN,
	ZFAT_CFG_SW_EN,
	ZFAT_CFG_INT_SEL,
	ZFAT_CFG_THRES,
	ZFAT_CFG_TEST_EN,
	ZFAT_CFG_THRES_FILT,
	/* Delay*/
	ZFAT_DLY,
	/* Software */
	ZFAT_SW,
	/* Number of shots */
	ZFAT_SHOTS_NB,
	/* Remaining shots counter */
	ZFAT_SHOTS_REM,
	/* Sample rate */
	ZFAT_SR_DECI,
	/* Sampling clock frequency */
	ZFAT_SAMPLING_HZ,
	/* Position address */
	ZFAT_POS,
	/* Pre-sample */
	ZFAT_PRE,
	/* Post-sample */
	ZFAT_POST,
	/* Sample counter */
	ZFAT_CNT,
	/* start:declaration block requiring some order */
	/* Channel 1 */
	ZFA_CH1_CTL_RANGE,
	ZFA_CH1_CTL_TERM,
	ZFA_CH1_STA,
	ZFA_CH1_GAIN,
	ZFA_CH1_OFFSET,
	ZFA_CH1_SAT,
	/* Channel 2 */
	ZFA_CH2_CTL_RANGE,
	ZFA_CH2_CTL_TERM,
	ZFA_CH2_STA,
	ZFA_CH2_GAIN,
	ZFA_CH2_OFFSET,
	ZFA_CH2_SAT,
	/* Channel 3 */
	ZFA_CH3_CTL_RANGE,
	ZFA_CH3_CTL_TERM,
	ZFA_CH3_STA,
	ZFA_CH3_GAIN,
	ZFA_CH3_OFFSET,
	ZFA_CH3_SAT,
	/* Channel 4 */
	ZFA_CH4_CTL_RANGE,
	ZFA_CH4_CTL_TERM,
	ZFA_CH4_STA,
	ZFA_CH4_GAIN,
	ZFA_CH4_OFFSET,
	ZFA_CH4_SAT,
	/*
	 * CHx__ are specifc ids used by some internal arithmetic
	 * Be carefull: the arithmetic expects
	 * that ch1 to ch4 are declared in the enum just above
	 * in the right order and grouped.
	 * Don't insert any other id in this area
	 */
	ZFA_CHx_CTL_RANGE,
	ZFA_CHx_CTL_TERM,
	ZFA_CHx_STA,
	ZFA_CHx_GAIN,
	ZFA_CHx_OFFSET,
	ZFA_CHx_SAT,
	/* end:declaration block requiring some order */
	/* two wishbone core for IRQ: VIC, ADC */
	ZFA_IRQ_ADC_DISABLE_MASK,
	ZFA_IRQ_ADC_ENABLE_MASK,
	ZFA_IRQ_ADC_MASK_STATUS,
	ZFA_IRQ_ADC_SRC,
	ZFA_IRQ_VIC_CTRL,
	ZFA_IRQ_VIC_DISABLE_MASK,
	ZFA_IRQ_VIC_ENABLE_MASK,
	ZFA_IRQ_VIC_MASK_STATUS,
	/* UTC core */
	ZFA_UTC_SECONDS,
	ZFA_UTC_COARSE,
	ZFA_UTC_TRIG_META,
	ZFA_UTC_TRIG_SECONDS,
	ZFA_UTC_TRIG_COARSE,
	ZFA_UTC_TRIG_FINE,
	ZFA_UTC_ACQ_START_META,
	ZFA_UTC_ACQ_START_SECONDS,
	ZFA_UTC_ACQ_START_COARSE,
	ZFA_UTC_ACQ_START_FINE,
	ZFA_UTC_ACQ_STOP_META,
	ZFA_UTC_ACQ_STOP_SECONDS,
	ZFA_UTC_ACQ_STOP_COARSE,
	ZFA_UTC_ACQ_STOP_FINE,
	ZFA_UTC_ACQ_END_META,
	ZFA_UTC_ACQ_END_SECONDS,
	ZFA_UTC_ACQ_END_COARSE,
	ZFA_UTC_ACQ_END_FINE,
	ZFA_HW_PARAM_COMMON_LAST,
};

/* trigger timestamp block size in bytes */
/* This block is added after the post trigger samples */
/* in the DDR and contains the trigger timestamp */
#define FA_TRIG_TIMETAG_BYTES 0x10

/*
 * ADC parameter id not mapped to Hw register
 * Id is used as zio attribute id
 */
enum fa_sw_param_id {
	/* to guarantee unique zio attr id */
	ZFA_SW_R_NOADDRES_NBIT = ZFA_HW_PARAM_COMMON_LAST,

	ZFA_SW_R_NOADDRES_TEMP,
	ZFA_SW_R_NOADDERS_AUTO,
	ZFA_SW_PARAM_COMMON_LAST,
};

/*
 * Bit pattern used in order to factorize code  between SVEC and SPEC
 * Depending of the carrier, ADC may have to listen vaious IRQ sources
 * SVEC: only ACQ irq source (end DMA irq is manged by vmebus driver)
 * SPEC: ACQ and DMA irq source
 */
enum fa_irq_src {
	FA_IRQ_SRC_ACQ = 0x1,
	FA_IRQ_SRC_DMA = 0x2,
};

/* adc IRQ values */
enum fa_irq_adc {
	FA_IRQ_ADC_NONE =	0x0,
	FA_IRQ_ADC_ACQ_END =	0x2,
};

/* Carrier-specific operations (gateware does not fully decouple
   carrier specific stuff, such as DMA or resets, from
   mezzanine-specific operations). */
struct fa_dev; /* forward declaration */
struct fa_carrier_op {
	char* (*get_gwname)(void);
	int (*init) (struct fa_dev *);
	int (*reset_core) (struct fa_dev *);
	void (*exit) (struct fa_dev *);
	int (*setup_irqs) (struct fa_dev *);
	int (*free_irqs) (struct fa_dev *);
	int (*enable_irqs) (struct fa_dev *);
	int (*disable_irqs) (struct fa_dev *);
	int (*ack_irq) (struct fa_dev *, int irq_id);
	int (*dma_start)(struct zio_cset *cset);
	void (*dma_done)(struct zio_cset *cset);
	void (*dma_error)(struct zio_cset *cset);
};

/* ADC and DAC Calibration, from  EEPROM */
struct fa_calib_stanza {
	int16_t offset[4]; /* One per channel */
	uint16_t gain[4];  /* One per channel */
	uint16_t temperature;
};

struct fa_calib {
	struct fa_calib_stanza adc[3];  /* For input, one per range */
	struct fa_calib_stanza dac[3];  /* For user offset, one per range */
};

/*
 * fa_dev: is the descriptor of the FMC ADC mezzanine
 *
 * @fmc: the pointer to the fmc_device generic structure
 * @zdev: is the pointer to the real zio_device in use
 * @hwzdev: is the pointer to the fake zio_device, used to initialize and
 *          to remove a zio_device
 *
 * @n_shots: total number of programmed shots for an acquisition
 * @n_fires: number of trigger fire occurred within an acquisition
 *
 * @n_dma_err: number of errors
 *
 */
struct fa_dev {
	/* the pointer to the fmc_device generic structure */
	struct fmc_device	*fmc;
	/* the pointer to the real zio_device in use */
	struct zio_device	*zdev;
	/* the pointer to the fake zio_device, used for init/remove */
	struct zio_device	*hwzdev;

	/* carrier common base offset addresses obtained from SDB */
	unsigned int fa_adc_csr_base;
	unsigned int fa_spi_base;
	unsigned int fa_ow_base;
	unsigned int fa_carrier_csr_base;
	unsigned int fa_irq_vic_base;
	unsigned int fa_irq_adc_base;
	unsigned int fa_utc_base;

	/* DMA description */
	struct zio_dma_sg *zdma;

	/* carrier specific functions (init/exit/reset/readout/irq handling) */
	struct fa_carrier_op *carrier_op;
	/* carrier private data */
	void *carrier_data;
	int irq_src; /* list of irq sources to listen */
	struct work_struct irq_work;
	/*
	 * keep last core having fired an IRQ
	 * Used to check irq sequence: ACQ followed by DMA
	 */
	int last_irq_core_src;

	/* Acquisition */
	unsigned int		n_shots;
	unsigned int		n_fires;

	/* Statistic informations */
	unsigned int		n_dma_err;

	/* Configuration */
	int			user_offset[4]; /* one per channel */

	/* one-wire */
	uint8_t ds18_id[8];
	unsigned long		next_t;
	int			temp;	/* temperature: scaled by 4 bits */

	/* Calibration Data */
	struct fa_calib calib;

	/* flag  */
	int enable_auto_start;
};

/*
 * zfad_block
 * @block is zio_block which contains data and metadata from a single shot
 * @dev_mem_off is the offset in ADC internal memory. It points to the first
 *              sample of the stored shot
 * @first_nent is the index of the first nent used for this block
 */
struct zfad_block {
	struct zio_block *block;
	uint32_t	dev_mem_off;
	unsigned int first_nent;
};

/*
 * Channel signal transmission delay
 * Trigger and channel signals are not going through the
 * same path on the board and trigger is faster.
 * Trying to sample the trigger itself by connecting
 * it to a channel, one can see a delay of 30ns between trigger and
 * its sampling. This constant is added to the trigger delay to
 * conpensate the channel signal transmission delay.
 * Expressed in tick count 3*10ns = 30ns
 */
#define FA_CH_TX_DELAY		3
#define FA_CAL_OFFSET		0x0100 /* Offset in EEPROM */

#define FA_CAL_NO_OFFSET	((int16_t)0x0000)
#define FA_CAL_NO_GAIN		((uint16_t)0x8000)

/* SPI Slave Select lines (as defined in spec_top_fmc_adc_100Ms.vhd) */
#define FA_SPI_SS_ADC		0
#define FA_SPI_SS_DAC(ch)	((ch) + 1)

/* Global variable exported by fa-zio-trg.c */
extern struct zio_trigger_type zfat_type;

static inline int zfat_overflow_detection(struct zio_ti *ti, unsigned int addr,
					  uint32_t val)
{
	struct zio_attribute *ti_zattr = ti->zattr_set.std_zattr;
	uint32_t pre_t, post_t, nshot_t;
	size_t shot_size;

	if (!addr)
		return 0;

	pre_t = addr == ZFAT_PRE ? val :
			ti_zattr[ZIO_ATTR_TRIG_PRE_SAMP].value;
	post_t = addr == ZFAT_POST ? val :
			ti_zattr[ZIO_ATTR_TRIG_POST_SAMP].value;
	if (ti->cset->trig != &zfat_type)
		nshot_t = 1; /* with any other trigger work in one-shot mode */
	else
		nshot_t = addr == ZFAT_SHOTS_NB ? val :
			  ti_zattr[ZIO_ATTR_TRIG_N_SHOTS].value;

	/*
	 * +1 because of the trigger samples, which is not counted as
	 * post-sample by the ADC
	 */
	shot_size = ((pre_t + post_t + 1) * ti->cset->ssize) * FA100M14B4C_NCHAN;
	if ( (shot_size * nshot_t) >= FA100M14B4C_MAX_ACQ_BYTE ) {
		dev_err(&ti->head.dev, "Cannot acquire, dev memory overflow\n");
		return -ENOMEM;
	}
	/* in case of multi shot, each shot cannot exceed the dpram size */
	if ( (nshot_t > 1) &&
	     (shot_size >= FA100M14B4C_MAX_MSHOT_ACQ_BYTE) ) {
		dev_err(&ti->head.dev, "Cannot acquire such amount of samples "
				"(shot_size: %d pre-samp:%d post-samp:%d) in multi shot mode."
				"dev memory overflow\n",
				(int)shot_size, pre_t, post_t);
		return -ENOMEM;
	}
	return 0;
}

static inline struct fa_dev *get_zfadc(struct device *dev)
{
	switch (to_zio_head(dev)->zobj_type) {
		case ZIO_DEV:
			return to_zio_dev(dev)->priv_d;
		case ZIO_CSET:
			return to_zio_cset(dev)->zdev->priv_d;
		case ZIO_CHAN:
			return to_zio_chan(dev)->cset->zdev->priv_d;
		case ZIO_TI:
			return to_zio_ti(dev)->cset->zdev->priv_d;
		default:
			return NULL;
	}
	return NULL;
}

static inline uint32_t fa_readl(struct fa_dev *fa,
				unsigned int base_off,
				const struct zfa_field_desc *field)
{
	uint32_t cur;

	cur = fmc_readl(fa->fmc, base_off+field->offset);
	if (field->is_bitfield) {
		/* apply mask and shift right accordlying to the mask */
		cur &= field->mask;
		cur /= (field->mask & -(field->mask));
	} else {
		cur &= field->mask; /* bitwise and with the mask */
	}
	return cur;
}

static inline void fa_writel(struct fa_dev *fa,
				unsigned int base_off,
				const struct zfa_field_desc *field,
				uint32_t usr_val)
{
	uint32_t cur, val;

	val = usr_val;
	/* Read current register value first if it's a bitfield */
	if (field->is_bitfield) {
		cur = fmc_readl(fa->fmc, base_off+field->offset);
		/* */
		cur &= ~field->mask; /* clear bits according to the mask */
		val = usr_val * (field->mask & -(field->mask));
		if (val & ~field->mask)
			dev_warn(fa->fmc->hwdev,
				"addr 0x%lx: value 0x%x doesn't fit mask 0x%x\n",
				base_off+field->offset, val, field->mask);
		val &= field->mask;
		val |= cur;
	}
	fmc_writel(fa->fmc, val, base_off+field->offset);
}

/* Global variable exported by fa-core.c */
extern struct workqueue_struct *fa_workqueue;

/* Global variable exported by fa-spec.c */
extern struct fa_carrier_op fa_spec_op;

/* Global variable exported by fa-svec.c */
extern struct fa_carrier_op fa_svec_op;

/* Global variable exported by fa-regfield.c */
extern const struct zfa_field_desc zfad_regs[];

/* Functions exported by fa-core.c */
extern int zfad_fsm_command(struct fa_dev *fa, uint32_t command);
extern int zfad_apply_user_offset(struct fa_dev *fa, struct zio_channel *chan,
				  uint32_t usr_val);
extern void zfad_reset_offset(struct fa_dev *fa);
extern int zfad_convert_hw_range(uint32_t bitmask);
extern int zfad_set_range(struct fa_dev *fa, struct zio_channel *chan,
			  int range);
extern int zfad_get_chx_index(unsigned long addr, struct zio_channel *chan);

/* Functions exported by fa-zio-drv.c */
extern int fa_zio_register(void);
extern void fa_zio_unregister(void);
extern int fa_zio_init(struct fa_dev *fa);
extern void fa_zio_exit(struct fa_dev *fa);

/* Functions exported by fa-zio-trg.c */
extern int fa_trig_init(void);
extern void fa_trig_exit(void);

/* Functions exported by fa-irq.c */
extern int zfad_dma_start(struct zio_cset *cset);
extern void zfad_dma_done(struct zio_cset *cset);
extern void zfad_dma_error(struct zio_cset *cset);
extern void zfat_irq_trg_fire(struct zio_cset *cset);
extern void zfat_irq_acq_end(struct zio_cset *cset);
extern int fa_setup_irqs(struct fa_dev *fa);
extern int fa_free_irqs(struct fa_dev *fa);
extern int fa_enable_irqs(struct fa_dev *fa);
extern int fa_disable_irqs(struct fa_dev *fa);

/* Functions exported by onewire.c */
extern int fa_onewire_init(struct fa_dev *fa);
extern void fa_onewire_exit(struct fa_dev *fa);
extern int fa_read_temp(struct fa_dev *fa, int verbose);

/* functions exported by spi.c */
extern int fa_spi_xfer(struct fa_dev *fa, int cs, int num_bits,
		       uint32_t tx, uint32_t *rx);
extern int fa_spi_init(struct fa_dev *fd);
extern void fa_spi_exit(struct fa_dev *fd);

/* fmc extended function */
signed long fmc_find_sdb_device_ext(struct sdb_array *tree,
					uint64_t vid, uint32_t did, int index,
					unsigned long *sz);

/* function exporetd by fa-calibration.c */
extern void fa_read_eeprom_calib(struct fa_dev *fa);

#endif /* __KERNEL__ */
#endif /*  FMC_ADC_H_ */
