/*
 * Copyright CERN 2012
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * Driver for the mezzanine ADC for the SPEC
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/delay.h>

#include <linux/fmc.h>

#include <linux/zio.h>
#include <linux/zio-buffer.h>
#include <linux/zio-trigger.h>

#include <asm/byteorder.h>

#include "fmc-adc-100m14b4cha.h"

/*
 * zio device attributes
 */
static ZIO_ATTR_DEFINE_STD(ZIO_DEV, zfad_cset_std_zattr) = {
	ZIO_ATTR(zdev, ZIO_ATTR_NBITS, ZIO_RO_PERM, ZFA_SW_R_NOADDRES_NBIT, 14),
};

/*
 * In the following table, "ATTR_EXT" are extended attributes that
 * end up in the control structure (i.e. they are metadata for the
 * acquisition), while PARAM_EXT are parameters: sysfs files that
 * don't end up in the control structure of ZIO blocks
 */

static struct zio_attribute zfad_cset_ext_zattr[] = {
	/*
	 * sample-decimation
	 * ADC acquire always at the maximum sample rate, to make "slower"
	 * acquisition you can decimate samples. 0 is a forbidden value, 1
	 * for the maximum speed.
	 */
	ZIO_ATTR_EXT("sample-decimation", ZIO_RW_PERM, ZFAT_SR_DECI, 1),

	ZIO_ATTR_EXT("ch0-offset", ZIO_RW_PERM, ZFA_CH1_OFFSET, 0),
	ZIO_ATTR_EXT("ch1-offset", ZIO_RW_PERM, ZFA_CH2_OFFSET, 0),
	ZIO_ATTR_EXT("ch2-offset", ZIO_RW_PERM, ZFA_CH3_OFFSET, 0),
	ZIO_ATTR_EXT("ch3-offset", ZIO_RW_PERM, ZFA_CH4_OFFSET, 0),

	ZIO_ATTR_EXT("ch0-vref", ZIO_RW_PERM, ZFA_CH1_CTL_RANGE, 0),
	ZIO_ATTR_EXT("ch1-vref", ZIO_RW_PERM, ZFA_CH2_CTL_RANGE, 0),
	ZIO_ATTR_EXT("ch2-vref", ZIO_RW_PERM, ZFA_CH3_CTL_RANGE, 0),
	ZIO_ATTR_EXT("ch3-vref", ZIO_RW_PERM, ZFA_CH4_CTL_RANGE, 0),

	ZIO_ATTR_EXT("ch0-saturation", ZIO_RW_PERM, ZFA_CH1_SAT, 0),
	ZIO_ATTR_EXT("ch1-saturation", ZIO_RW_PERM, ZFA_CH2_SAT, 0),
	ZIO_ATTR_EXT("ch2-saturation", ZIO_RW_PERM, ZFA_CH3_SAT, 0),
	ZIO_ATTR_EXT("ch3-saturation", ZIO_RW_PERM, ZFA_CH4_SAT, 0),

	ZIO_ATTR_EXT("ch0-50ohm-term", ZIO_RW_PERM, ZFA_CH1_CTL_TERM, 0),
	ZIO_ATTR_EXT("ch1-50ohm-term", ZIO_RW_PERM, ZFA_CH2_CTL_TERM, 0),
	ZIO_ATTR_EXT("ch2-50ohm-term", ZIO_RW_PERM, ZFA_CH3_CTL_TERM, 0),
	ZIO_ATTR_EXT("ch3-50ohm-term", ZIO_RW_PERM, ZFA_CH4_CTL_TERM, 0),

	/* last acquisition start time stamp */
	ZIO_ATTR_EXT("tstamp-acq-str-s", ZIO_RO_PERM,
			ZFA_UTC_ACQ_START_SECONDS, 0),
	ZIO_ATTR_EXT("tstamp-acq-str-t", ZIO_RO_PERM,
			ZFA_UTC_ACQ_START_COARSE, 0),
	ZIO_ATTR_EXT("tstamp-acq-str-b", ZIO_RO_PERM,
			ZFA_UTC_ACQ_START_FINE, 0),

	/* Timing base */
	ZIO_ATTR_EXT("tstamp-base-s", ZIO_RW_PERM, ZFA_UTC_SECONDS, 0),

	ZIO_ATTR_EXT("tstamp-base-t", ZIO_RW_PERM, ZFA_UTC_COARSE, 0),

	/* Parameters (not attributes) follow */

	/*
	 * State machine commands
	 * 1: start
	 * 2: stop
	 */
	ZIO_PARAM_EXT("fsm-command", ZIO_WO_PERM, ZFA_CTL_FMS_CMD, 0),
	/*
	 * Automatic start acquisition
	 * 1: enabled
	 * 0: disabled
	 */
	ZIO_PARAM_EXT("fsm-auto-start", ZIO_RW_PERM, ZFA_SW_R_NOADDERS_AUTO, 0),
	/*
	 * fsm - status of the state machine:
	 * 1: IDLE
	 * 2: PRE_TRIG
	 * 3: WAIT_TRIG
	 * 4: POST_TRIG
	 * 5: DECR_SHOT
	 * 7: Illegal
	 * */
	ZIO_PARAM_EXT("fsm-state", ZIO_RO_PERM, ZFA_STA_FSM, 0),
	/* last acquisition end time stamp */
	ZIO_PARAM_EXT("tstamp-acq-end-s", ZIO_RO_PERM,
			ZFA_UTC_ACQ_END_SECONDS, 0),
	ZIO_PARAM_EXT("tstamp-acq-end-t", ZIO_RO_PERM,
			ZFA_UTC_ACQ_END_COARSE, 0),
	ZIO_PARAM_EXT("tstamp-acq-end-b", ZIO_RO_PERM,
			ZFA_UTC_ACQ_END_FINE, 0),
	/* last acquisition stop time stamp */
	ZIO_PARAM_EXT("tstamp-acq-stp-s", ZIO_RO_PERM,
			ZFA_UTC_ACQ_STOP_SECONDS, 0),
	ZIO_PARAM_EXT("tstamp-acq-stp-t", ZIO_RO_PERM,
			ZFA_UTC_ACQ_STOP_COARSE, 0),
	ZIO_PARAM_EXT("tstamp-acq-stp-b", ZIO_RO_PERM,
			ZFA_UTC_ACQ_STOP_FINE, 0),
	/* Reset all channel offset */
	ZIO_PARAM_EXT("rst-ch-offset", ZIO_WO_PERM, ZFA_CTL_DAC_CLR_N, 1),

	ZIO_PARAM_EXT("sample-frequency", ZIO_RO_PERM, ZFAT_SAMPLING_HZ, 0),
};

#if 0 /* FIXME Unused until TLV control will be available */
static ZIO_ATTR_DEFINE_STD(ZIO_DEV, zfad_chan_std_zattr) = {
	/* the offset is complement 2 format */
	ZIO_ATTR(zdev, ZIO_ATTR_OFFSET, ZIO_RW_PERM, ZFA_CHx_OFFSET, 0),
	/*
	 * in-range
	 * 0x23 (35): 100mV range
	 * 0x11 (17): 1V range
	 * 0x45 (69): 10V range
	 * 0x00 (0): Open input
	 */
	ZIO_ATTR(zdev, ZIO_ATTR_VREFTYPE, ZIO_RW_PERM, ZFA_CHx_CTL_RANGE, 0x11),
};
#endif

static struct zio_attribute zfad_chan_ext_zattr[] = {
#if 0 /* FIXME Unused until TLV control will be available */
	ZIO_ATTR("saturation", ZIO_RW_PERM, ZFA_CHx_SAT, 0),
#endif
	/*ZIO_ATTR(zdev, "50ohm-termination", ZIO_RW_PERM, ZFA_CHx_CTL_TERM, 0x11),*/
	ZIO_PARAM_EXT("current-value", ZIO_RO_PERM, ZFA_CHx_STA, 0),
};

static struct zio_attribute zfad_dev_ext_zattr[] = {
	/* Get Mezzanine temperature from onewire */
	ZIO_PARAM_EXT("temperature", ZIO_RO_PERM, ZFA_SW_R_NOADDRES_TEMP, 0),
};

/* Temporarily, user values are the same as hardware values */
static int zfad_convert_user_range(uint32_t user_val)
{
	return zfad_convert_hw_range(user_val);
}

/*
 * zfad_conf_set
 *
 * set a value to a FMC-ADC registers
 */
static int zfad_conf_set(struct device *dev, struct zio_attribute *zattr,
			 uint32_t usr_val)
{
	struct fa_dev *fa = get_zfadc(dev);
	unsigned int baseoff = fa->fa_adc_csr_base;
	struct zio_channel *chan;
	int i, range, err, reg_index;

	reg_index = zattr->id;
	i = FA100M14B4C_NCHAN;

	if (zattr->id >= ZFA_UTC_SECONDS && zattr->id <= ZFA_UTC_ACQ_END_FINE)
		baseoff = fa->fa_utc_base;

	switch (reg_index) {
		/*
		 * Most of the following "case" statements are simply
		 * error checking and ancillary operations.  The actual
		 * programming of hardware is done at the end of the
		 * switch, in the catch-all final zfa_hardware_write()
		 */

	case ZFA_SW_R_NOADDERS_AUTO:
		fa->enable_auto_start = usr_val;
		return 0;
	/* FIXME temporary until TLV control */
	case ZFA_CH1_OFFSET:
		i--;
	case ZFA_CH2_OFFSET:
		i--;
	case ZFA_CH3_OFFSET:
		i--;
	case ZFA_CH4_OFFSET:
		i--;
		chan = to_zio_cset(dev)->chan + i;
		err = zfad_apply_user_offset(fa, chan, usr_val);
		if (err)
			return err;
		fa->user_offset[chan->index] = usr_val;
		return 0;

	case ZFA_CHx_OFFSET:
		chan = to_zio_chan(dev),
			err = zfad_apply_user_offset(fa, chan, usr_val);
		if (err)
			return err;
		fa->user_offset[chan->index] = usr_val;
		return 0;

	case ZFA_CTL_DAC_CLR_N:
		zfad_reset_offset(fa);
		return 0;
	case ZFAT_SR_DECI:
		if (usr_val == 0)
			usr_val++;
		break;
	/* FIXME temporary until TLV control */
	case ZFA_CH1_CTL_TERM:
	case ZFA_CH2_CTL_TERM:
	case ZFA_CH3_CTL_TERM:
	case ZFA_CH4_CTL_TERM:
	case ZFA_CHx_CTL_TERM:
		if (usr_val > 1)
			usr_val = 1;
		break;

	/* FIXME temporary until TLV control */
	case ZFA_CH1_CTL_RANGE:
		i--;
	case ZFA_CH2_CTL_RANGE:
		i--;
	case ZFA_CH3_CTL_RANGE:
		i--;
	case ZFA_CH4_CTL_RANGE:
		i--;
		range = zfad_convert_user_range(usr_val);
		if (range < 0)
			return range;
		return zfad_set_range(fa, &to_zio_cset(dev)->chan[i], range);

	case ZFA_CHx_CTL_RANGE:
		range = zfad_convert_user_range(usr_val);
		if (range < 0)
			return range;
		return zfad_set_range(fa, to_zio_chan(dev), range);

	case ZFA_CHx_STA:
		reg_index = zfad_get_chx_index(reg_index, to_zio_chan(dev));
		break;
	case ZFA_UTC_COARSE:
		if (usr_val >= FA100M14B4C_UTC_CLOCK_FREQ) {
			dev_err(dev,
				"ticks time must be in the range [0, %d]\n",
				FA100M14B4C_UTC_CLOCK_FREQ);
			return -EINVAL;
		}
		break;
	case ZFA_CTL_FMS_CMD:
		return zfad_fsm_command(fa, usr_val);
	}

	fa_writel(fa, baseoff, &zfad_regs[reg_index], usr_val);
	return 0;
}

/*
 * zfad_info_get
 *
 * get a register value from FMC-ADC.
 */
static int zfad_info_get(struct device *dev, struct zio_attribute *zattr,
			 uint32_t *usr_val)
{
	struct fa_dev *fa = get_zfadc(dev);
	unsigned int baseoff = fa->fa_adc_csr_base;
	int i, reg_index;

	i = FA100M14B4C_NCHAN;

	if (zattr->id >= ZFA_UTC_SECONDS && zattr->id <= ZFA_UTC_ACQ_END_FINE)
		baseoff = fa->fa_utc_base;

	switch (zattr->id) {
	/* FIXME temporary until TLV control */
	case ZFA_CH1_OFFSET:
		i--;
	case ZFA_CH2_OFFSET:
		i--;
	case ZFA_CH3_OFFSET:
		i--;
	case ZFA_CH4_OFFSET:
		i--;
		*usr_val = fa->user_offset[i];
		return 0;

	case ZFA_CHx_OFFSET:
		*usr_val = fa->user_offset[to_zio_chan(dev)->index];
		return 0;

	case ZFA_SW_R_NOADDRES_NBIT:
	case ZFA_SW_R_NOADDERS_AUTO:
		/* ZIO automatically return the attribute value */
		return 0;
	case ZFA_SW_R_NOADDRES_TEMP:
		/*
		 * Onewire returns units of 1/16 degree. We return units
		 * of 1/1000 of a degree instead.
		 */
		*usr_val = fa_read_temp(fa, 0);
		*usr_val = (*usr_val * 1000 + 8) / 16;
		return 0;
	case ZFA_CHx_SAT:
	case ZFA_CHx_CTL_TERM:
	case ZFA_CHx_CTL_RANGE:
		reg_index = zfad_get_chx_index(zattr->id, to_zio_chan(dev));
		break;

	case ZFA_CHx_STA:
		reg_index = zfad_get_chx_index(zattr->id, to_zio_chan(dev));
		*usr_val = fa_readl(fa, fa->fa_adc_csr_base,
				    &zfad_regs[reg_index]);
		i = (int16_t)(*usr_val); /* now signed integer */
		*usr_val = i;
		break;
	default:
		reg_index = zattr->id;
	}

	*usr_val = fa_readl(fa, baseoff, &zfad_regs[reg_index]);
	return 0;
}
static const struct zio_sysfs_operations zfad_s_op = {
	.conf_set = zfad_conf_set,
	.info_get = zfad_info_get,
};


/*
 * zfad_input_cset_software
 * @fa the adc instance to use
 * @cset channel set to acquire
 *
 * If the user is using the ADC trigger, then it can do a multi-shot
 * acquisition.
 * If the user is using a software trigger, it cannot do multi-shot.
 * The generic arm trigger used by software trigger returns a
 * zio_block. We must convert it into a zfad_block to perform DMA
 */
static int zfad_input_cset_software(struct fa_dev *fa, struct zio_cset *cset)
{
	struct zfad_block *tmp;
	int err;

	/* Check if device memory allows this acquisition */
	err = zfat_overflow_detection(cset->ti, ZFAT_POST, cset->ti->nsamples);
	if (err)
		return err;
	tmp = kzalloc(sizeof(struct zfad_block), GFP_ATOMIC);
	if (!tmp)
		return -ENOMEM;
	tmp->block = cset->interleave->active_block;
	cset->interleave->priv_d = tmp;
	tmp->dev_mem_off = 0; /* Always the first block */

	/* Configure post samples */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_POST],
		  cset->ti->nsamples);
	/* Start the acquisition */
	zfad_fsm_command(fa, FA100M14B4C_CMD_START);

	fa->n_shots = 1;
	/* Fire software trigger */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_SW], 1);

	return -EAGAIN;
}


/*
 * zfad_input_cset
 * @cset: channel set to acquire
 *
 * Prepare the FMC-ADC for the acquisition.
 */
static int zfad_input_cset(struct zio_cset *cset)
{
	struct fa_dev *fa = cset->zdev->priv_d;

	dev_dbg(&fa->fmc->dev, "Ready to acquire\n");
	/* ZIO should configure only the interleaved channel */
	if (!cset->interleave)
		return -EINVAL;
	/* nsamples can't be 0 */
	if (!cset->interleave->current_ctrl->nsamples) {
		dev_info(&fa->fmc->dev, "pre + post = 0: can't acquire\n");
		return -EINVAL;
	}

	/* If not the fmc-adc-trg, then is a ZIO software trigger */
	if (unlikely(cset->trig != &zfat_type))
		return zfad_input_cset_software(fa, cset);

	return -EAGAIN; /* data_done on DMA_DONE interrupt */
}

/*
 * zfad_stop_cset
 * @cset: channel set to stop
 *
 * Stop an acquisition, reset indexes and disable interrupts. This function
 * is useful only if the driver is using a software trigger.
 */
static void zfad_stop_cset(struct zio_cset *cset)
{

	struct fa_dev *fa = cset->zdev->priv_d;

	/* If the user is using a software trigger */
	if (cset->trig != &zfat_type) {
		/* Force the acquisition to stop */
		zfad_fsm_command(fa, FA100M14B4C_CMD_STOP);
		/* Release zfad_block */
		kfree(cset->interleave->priv_d);
		cset->interleave->priv_d = NULL;
		/* Clear active block */
		cset->interleave->active_block = NULL;
	}
}

/*
 * zfad_zio_probe
 * @zdev: the real zio device
 *
 * The device registration completes. Copy the calibration data from the
 * eeprom and initialize some registers
 */
static int zfad_zio_probe(struct zio_device *zdev)
{
	struct fa_dev *fa = zdev->priv_d;

	dev_dbg(&zdev->head.dev, "%s:%d\n", __func__, __LINE__);
	/* Save also the pointer to the real zio_device */
	fa->zdev = zdev;

	/* We don't have csets at this point, so don't do anything more */
	return 0;
}

/*
 * zfad_zio_remove
 * @zdev: the real zio device
 *
 * Release FMC interrupt handler
 */
static int zfad_zio_remove(struct zio_device *zdev)
{
	return 0;
}


/* Device description */
static struct zio_channel zfad_chan_tmpl = {
	.zattr_set = {
		/* FIXME usable only when TLV control is available */
		/*.std_zattr = zfad_chan_std_zattr,*/
		.ext_zattr = zfad_chan_ext_zattr,
		.n_ext_attr = ARRAY_SIZE(zfad_chan_ext_zattr),
	},
};
static struct zio_cset zfad_cset[] = {
	{
		.raw_io = zfad_input_cset,
		.stop_io = zfad_stop_cset,
		.ssize = 2,
		.n_chan = FA100M14B4C_NCHAN,
		.chan_template = &zfad_chan_tmpl,
		.flags =  ZIO_CSET_TYPE_ANALOG |	/* is analog */
			  ZIO_DIR_INPUT |	/* is input */
			  ZIO_CSET_INTERLEAVE_ONLY,/* interleave only */
		.zattr_set = {
			.std_zattr = zfad_cset_std_zattr,
			.ext_zattr = zfad_cset_ext_zattr,
			.n_ext_attr = ARRAY_SIZE(zfad_cset_ext_zattr),
		},
	}
};
static struct zio_device zfad_tmpl = {
	.owner = THIS_MODULE,
	.s_op = &zfad_s_op,
	.flags = 0,
	.cset = zfad_cset,
	.n_cset = ARRAY_SIZE(zfad_cset),
	.zattr_set = {
		.ext_zattr = zfad_dev_ext_zattr,
		.n_ext_attr = ARRAY_SIZE(zfad_dev_ext_zattr),
	},
	/* This driver prefers its own trigger */
	.preferred_trigger = "adc-100m14b",
	.preferred_buffer = "kmalloc",
};


/* List of supported boards */
static const struct zio_device_id zfad_table[] = {
	{"adc-100m14b", &zfad_tmpl},
	{},
};

static struct zio_driver fa_zdrv = {
	.driver = {
		.name = "adc-100m14b",
		.owner = THIS_MODULE,
	},
	.id_table = zfad_table,
	.probe = zfad_zio_probe,
	.remove = zfad_zio_remove,
};


/*
 * fa_zio_unregister
 *
 * It is a simple wrapper invoked by module_init to register this zio driver
 */
int fa_zio_register(void)
{
	return zio_register_driver(&fa_zdrv);
}


/*
 * fa_zio_unregister
 *
 * It is a simple wrapper invoked by module_exit to unregister this zio driver
 */
void fa_zio_unregister(void)
{
	zio_unregister_driver(&fa_zdrv);
}


/*
 * fa_zio_init
 *
 * It checks if we can register this device.  If it is possibile, the function
 * registers both device and trigger. The FMC probe invokes this function.
 */
int fa_zio_init(struct fa_dev *fa)
{
	struct device *msgdev = &fa->fmc->dev;
	int err;


	/* Allocate the hardware zio_device for registration */
	fa->hwzdev = zio_allocate_device();
	if (IS_ERR(fa->hwzdev)) {
		dev_err(msgdev, "Cannot allocate ZIO device\n");
		return PTR_ERR(fa->hwzdev);
	}

	/* Mandatory fields */
	fa->hwzdev->owner = THIS_MODULE;
	fa->hwzdev->priv_d = fa;

	/* Register the hardware zio_device */
	err = zio_register_device(fa->hwzdev, "adc-100m14b",
				  fa->fmc->device_id);
	if (err) {
		dev_err(msgdev, "Cannot register ZIO device fmc-adc-100m14b\n");
		zio_free_device(fa->hwzdev);
	}
	return err;
}

/*
 * fa_zio_exit
 *
 * It removes both device and trigger form the ZIO framework. The FMC remove
 * invokes this function.
 */
void fa_zio_exit(struct fa_dev *fa)
{
	zio_unregister_device(fa->hwzdev);
	zio_free_device(fa->hwzdev);
}
