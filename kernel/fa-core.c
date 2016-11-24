/*
 * core fmc-adc-100m14b driver
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *		Copied from fine-delay fd-core.c
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>

#include <linux/fmc.h>
#include <linux/fmc-sdb.h>

#include "fmc-adc-100m14b4cha.h"

/* Module parameters */
static struct fmc_driver fa_dev_drv;
FMC_PARAM_BUSID(fa_dev_drv);
FMC_PARAM_GATEWARE(fa_dev_drv);

static int fa_show_sdb;
module_param_named(show_sdb, fa_show_sdb, int, 0444);
MODULE_PARM_DESC(show_sdb, "Print a dump of the gateware's SDB tree.");

static int fa_enable_test_data;
module_param_named(enable_test_data, fa_enable_test_data, int, 0444);

static int fa_internal_trig_test = 0;
module_param_named(internal_trig_test, fa_internal_trig_test, int, 0444);

static const int zfad_hw_range[] = {
	[FA100M14B4C_RANGE_10V]   = 0x45,
	[FA100M14B4C_RANGE_1V]    = 0x11,
	[FA100M14B4C_RANGE_100mV] = 0x23,
	[FA100M14B4C_RANGE_OPEN]  = 0x00,
};

/* fmc-adc specific workqueue */
struct workqueue_struct *fa_workqueue;

/*
 * zfad_convert_hw_range
 * @usr_val: range value
 *
 * return the enum associated to the range value
 */
int zfad_convert_hw_range(uint32_t bitmask)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(zfad_hw_range); i++)
		if (zfad_hw_range[i] == bitmask)
			return i;
	return -EINVAL;
}

/* Calculate correct index in fa_regfield array for channel from CHx indexes */
int zfad_get_chx_index(unsigned long addr, struct zio_channel *chan)
{
	int offset;

	offset = ZFA_CHx_MULT  * (FA100M14B4C_NCHAN - chan->index);

	return addr - offset;
}

/*
 * zfad_apply_user_offset
 * @fa: the fmc-adc descriptor
 * @chan: the channel where apply offset
 * @usr_val: the offset value to apply, expressed as millivolts (-5000..5000)
 *
 * Apply user offset to the channel input. Before apply the user offset it must
 * be corrected with offset and gain calibration value. An open input does not
 * need any correction.
 */
int zfad_apply_user_offset(struct fa_dev *fa, struct zio_channel *chan,
				  uint32_t usr_val)
{
	uint32_t range_reg;
	int32_t uval =  (int32_t)usr_val;
	int offset, gain, hwval, i, range;

	if (uval < -5000 || uval > 5000)
		return -EINVAL;

	i = zfad_get_chx_index(ZFA_CHx_CTL_RANGE, chan);
	range_reg = fa_readl(fa, fa->fa_adc_csr_base, &zfad_regs[i]);

	range = zfad_convert_hw_range(range_reg);
	if (range < 0)
		return range;

	if (range == FA100M14B4C_RANGE_OPEN) {
		offset = FA_CAL_NO_OFFSET;
		gain = FA_CAL_NO_GAIN;
	} else {
		offset = fa->calib.dac[range].offset[chan->index];
		gain = fa->calib.dac[range].gain[chan->index];
	}

	hwval = uval * 0x8000 / 5000;
	if (hwval == 0x8000)
		hwval = 0x7fff; /* -32768 .. 32767 */

	hwval = ((hwval + offset) * gain) >> 15; /* signed */
	hwval += 0x8000; /* offset binary */
	if (hwval < 0)
		hwval = 0;
	if (hwval > 0xffff)
		hwval = 0xffff;

	/* Apply calibrated offset to DAC */
	return fa_spi_xfer(fa, FA_SPI_SS_DAC(chan->index), 16, hwval, NULL);
}

/*
 * zfad_reset_offset
 * @fa: the fmc-adc descriptor
 *
 * Reset channel's offsets
 */
void zfad_reset_offset(struct fa_dev *fa)
{
	int i;

	for (i = 0; i < FA100M14B4C_NCHAN; ++i)
		zfad_apply_user_offset(fa, &fa->zdev->cset->chan[i], 0);
}

/*
 * zfad_init_saturation
 * @fa: the fmc-adc descriptor
 *
 * Initialize all saturation registers to the maximum value
 */
void zfad_init_saturation(struct fa_dev *fa)
{
	int idx, i;

	for (i = 0, idx = ZFA_CH1_SAT; i < FA100M14B4C_NCHAN; ++i, idx += ZFA_CHx_MULT)
		fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[idx], 0x7fff);
}

/*
 * zfad_set_range
 * @fa: the fmc-adc descriptor
 * @chan: the channel to calibrate
 * @usr_val: the volt range to set and calibrate
 *
 * When the input range changes, we must write new fixup values
 */
int zfad_set_range(struct fa_dev *fa, struct zio_channel *chan,
			  int range)
{
	int i, offset, gain;

	/* Actually set the range */
	i = zfad_get_chx_index(ZFA_CHx_CTL_RANGE, chan);
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[i], zfad_hw_range[range]);

	if (range == FA100M14B4C_RANGE_OPEN) {
		offset = FA_CAL_NO_OFFSET;
		gain = FA_CAL_NO_GAIN;
	} else {
		if (range < 0 || range > ARRAY_SIZE(fa->calib.adc)) {
			dev_info(&fa->fmc->dev, "Invalid range %i or ch %i\n",
				 range, chan->index);
			return -EINVAL;
		}
		offset = fa->calib.adc[range].offset[chan->index];
		gain = fa->calib.adc[range].gain[chan->index];
	}

	i = zfad_get_chx_index(ZFA_CHx_OFFSET, chan);
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[i],
		  offset & 0xffff /* prevent warning */);
	i = zfad_get_chx_index(ZFA_CHx_GAIN, chan);
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[i], gain);

	/* recalculate user offset for the new range */
	zfad_apply_user_offset(fa, chan, fa->user_offset[chan->index]);

	return 0;
}

/*
 * zfad_fsm_command
 * @fa: the fmc-adc descriptor
 * @command: the command to apply to FSM
 *
 * This function checks if the command can be done and performs some
 * preliminary operation beforehand
 */
int zfad_fsm_command(struct fa_dev *fa, uint32_t command)
{
	struct device *dev = &fa->fmc->dev;
	struct zio_cset *cset = fa->zdev->cset;
	uint32_t val;

	if (command != FA100M14B4C_CMD_START &&
	    command != FA100M14B4C_CMD_STOP) {
		dev_info(dev, "Invalid command %i\n", command);
		return -EINVAL;
	}

	/*
	 * When any command occurs we are ready to start a new acquisition, so
	 * we must abort any previous one. If it is STOP, we abort because we
	 * abort an acquisition. If it is START, we abort because if there was
	 * a previous start but the acquisition end interrupt doesn't occurs,
	 * START mean RESTART. If it is a clean START, the abort has not
	 * effects.
	 *
	 * This is done only if ADC is using its own trigger, otherwise it is
	 * not necessary.
	 *
	 * The case of fmc-adc-trg is optimized because is the most common
	 * case
	 */
	if (likely(cset->trig == &zfat_type || command == FA100M14B4C_CMD_STOP))
		zio_trigger_abort_disable(cset, 0);

	/* Reset counters */
	fa->n_shots = 0;
	fa->n_fires = 0;

	/* If START, check if we can start */
	if (command == FA100M14B4C_CMD_START) {
		/* Verify that SerDes PLL is lockes */
		val = fa_readl(fa, fa->fa_adc_csr_base,
			       &zfad_regs[ZFA_STA_SERDES_PLL]);
		if (!val) {
			dev_info(dev, "Cannot start acquisition: "
				 "SerDes PLL not locked\n");
			return -EBUSY;
		}
		/* Verify that SerDes is synched */
		val = fa_readl(fa, fa->fa_adc_csr_base,
			       &zfad_regs[ZFA_STA_SERDES_SYNCED]);
		if (!val) {
			dev_info(dev, "Cannot start acquisition: "
				 "SerDes not synchronized\n");
			return -EBUSY;
		}

		/* Now we can arm the trigger for the incoming acquisition */
		zio_arm_trigger(cset->ti);
		/*
		 *  FIXME maybe zio_arm_trigger() can return an error when it
		 * is not able to arm a trigger.
		 *
		 * It returns -EPERM, but the error can be -ENOMEM or -EINVAL
		 * from zfat_arm_trigger() or zfad_input_cset()
		 */
		if (!(cset->ti->flags & ZIO_TI_ARMED)) {
			dev_info(dev, "Cannot start acquisition: "
				 "Trigger refuses to arm\n");
			return -EIO;
		}

		dev_dbg(dev, "FSM START Command, Enable interrupts\n");
		fa_enable_irqs(fa);
	} else {
		dev_dbg(dev, "FSM STOP Command, Disable interrupts\n");
		fa->enable_auto_start = 0;
		fa_disable_irqs(fa);
	}

	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFA_CTL_FMS_CMD],
		  command);
	return 0;
}

/* Extract from SDB the base address of the core components */
/* which are not carrier specific */
static int __fa_sdb_get_device(struct fa_dev *fa)
{
	struct fmc_device *fmc = fa->fmc;
	struct device *dev = fmc->hwdev;
	int ret;

	ret = fmc_scan_sdb_tree(fmc, 0);
	if (ret == -EBUSY) {
		/* Not a problem, it's already there. We assume that
		   it's the correct one */
		ret = 0;
	}
	if (ret < 0) {
		dev_err(dev,
			"%s: no SDB in the bitstream."
			"Are you sure you've provided the correct one?\n",
			KBUILD_MODNAME);
		return ret;
	}

	/* FIXME: this is obsoleted by fmc-bus internal parameters */
	if (fa_show_sdb)
		fmc_show_sdb_tree(fmc);

	/* Now use SDB to find the base addresses */
	fa->fa_irq_vic_base = fmc_find_sdb_device(fmc->sdb, 0xce42,
						  0x13, NULL);
	fa->fa_adc_csr_base = fmc_find_sdb_device_ext(fmc->sdb, 0xce42,
						      0x608,
						      fmc->slot_id, NULL);
	fa->fa_irq_adc_base = fmc_find_sdb_device_ext(fmc->sdb, 0xce42,
						      0x26ec6086,
						      fmc->slot_id, NULL);
	fa->fa_utc_base = fmc_find_sdb_device_ext(fmc->sdb, 0xce42,
						  0x604, fmc->slot_id, NULL);
	fa->fa_spi_base = fmc_find_sdb_device_ext(fmc->sdb, 0xce42, 0xe503947e,
							fmc->slot_id, NULL);
	fa->fa_ow_base = fmc_find_sdb_device_ext(fmc->sdb, 0xce42, 0x779c5443,
							fmc->slot_id, NULL);

	return ret;
}

/*
 * Specific check and init
 */
static int __fa_init(struct fa_dev *fa)
{
	struct device *hwdev = fa->fmc->hwdev;
	struct device *msgdev = &fa->fmc->dev;
	struct zio_device *zdev = fa->zdev;
	int i, addr;

	/* Check if hardware supports 64-bit DMA */
	if (dma_set_mask(hwdev, DMA_BIT_MASK(64))) {
		/* Check if hardware supports 32-bit DMA */
		if (dma_set_mask(hwdev, DMA_BIT_MASK(32))) {
			dev_err(msgdev, "32-bit DMA addressing not available\n");
			return -EINVAL;
		}
	}

	/* Retrieve calibration from the eeprom and validate*/
	fa_read_eeprom_calib(fa);

	/* Force stop FSM to prevent early trigger fire */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFA_CTL_FMS_CMD],
		   FA100M14B4C_CMD_STOP);
	/* Initialize channels to use 1V range */
	for (i = 0; i < 4; ++i) {
		addr = zfad_get_chx_index(ZFA_CHx_CTL_RANGE,
						&zdev->cset->chan[i]);
		fa_writel(fa,  fa->fa_adc_csr_base, &zfad_regs[addr],
			  FA100M14B4C_RANGE_1V);
		zfad_set_range(fa, &zdev->cset->chan[i], FA100M14B4C_RANGE_1V);
	}
	zfad_reset_offset(fa);

	/* Enable mezzanine clock */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFA_CTL_CLK_EN], 1);
	/* Set decimation to minimum */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_SR_DECI], 1);
	/* Set test data register */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFA_CTL_TEST_DATA_EN],
		  fa_enable_test_data);
	/* Set internal trigger test mode */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_CFG_TEST_EN],
		  fa_internal_trig_test);

	/* Set to single shot mode by default */
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_SHOTS_NB], 1);
	if (zdev->cset->ti->cset->trig == &zfat_type) {
		/* Select external trigger (index 0) */
		fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_CFG_HW_SEL],
			  1);
		zdev->cset->ti->zattr_set.ext_zattr[FA100M14B4C_TATTR_EXT].value = 1;
	} else {
		/* Enable Software trigger*/
		fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_CFG_SW_EN],
			  1);
		/* Disable Hardware trigger*/
		fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFAT_CFG_HW_EN],
			  0);
	}

	/* Zero offsets and release the DAC clear */
	zfad_reset_offset(fa);
	fa_writel(fa, fa->fa_adc_csr_base, &zfad_regs[ZFA_CTL_DAC_CLR_N], 1);

	/* Initialize channel saturation values */
	zfad_init_saturation(fa);

	/* Set UTC seconds from the kernel seconds */
	fa_writel(fa, fa->fa_utc_base, &zfad_regs[ZFA_UTC_SECONDS],
		  get_seconds());

	/*
	 * Set Trigger delay in order to compensate
	 * the channel signal transmission delay
	 */
	fa_writel(fa, fa->fa_utc_base, &zfad_regs[ZFAT_DLY], FA_CH_TX_DELAY);

	/* disable auto_start */
	fa->enable_auto_start = 0;
	return 0;
}

/* This structure lists the various subsystems */
struct fa_modlist {
	char *name;
	int (*init)(struct fa_dev *);
	void (*exit)(struct fa_dev *);
};

static struct fa_modlist mods[] = {
	{"spi", fa_spi_init, fa_spi_exit},
	{"onewire", fa_onewire_init, fa_onewire_exit},
	{"zio", fa_zio_init, fa_zio_exit},
};

/* probe and remove are called by fa-spec.c */
int fa_probe(struct fmc_device *fmc)
{
	struct fa_modlist *m = NULL;
	struct fa_dev *fa;
	int err, i = 0;
	char *fwname;

	/* Validate the new FMC device */
	i = fmc->op->validate(fmc, &fa_dev_drv);
	if (i < 0) {
		dev_info(&fmc->dev, "not using \"%s\" according to "
			 "modparam\n", KBUILD_MODNAME);
		return -ENODEV;
	}

	/* Driver data */
	fa = devm_kzalloc(&fmc->dev, sizeof(struct fa_dev), GFP_KERNEL);
	if (!fa)
		return -ENOMEM;
	fmc_set_drvdata(fmc, fa);
	fa->fmc = fmc;

	/* apply carrier-specific hacks and workarounds */
	fa->carrier_op = NULL;
	if (!strcmp(fmc->carrier_name, "SPEC")) {
		fa->carrier_op = &fa_spec_op;
	} else if (!strcmp(fmc->carrier_name, "SVEC")) {
#ifdef CONFIG_FMC_ADC_SVEC
		fa->carrier_op = &fa_svec_op;
#endif
	}

	/*
	 * Check if carrier operations exists. Otherwise it means that the
	 * driver was compiled without enable any carrier, so it cannot work
	 */
	if (!fa->carrier_op) {
		dev_err(fmc->hwdev,
			"This binary doesn't support the '%s' carrier\n",
			fmc->carrier_name);
		return -ENODEV;
	}

	if (fa_dev_drv.gw_n)
		fwname = "";	/* reprogram will pick from module parameter */
	else
		fwname = fa->carrier_op->get_gwname();
	dev_info(fmc->hwdev, "Gateware (%s)\n", fwname);
	/* We first write a new binary (and lm32) within the carrier */
	err = fmc->op->reprogram(fmc, &fa_dev_drv, fwname);
	if (err) {
		dev_err(fmc->hwdev, "write firmware \"%s\": error %i\n",
				fwname, err);
		goto out;
	}
	dev_info(fmc->hwdev, "Gateware successfully loaded\n");

	/* Extract whisbone core base address fron SDB */
	err = __fa_sdb_get_device(fa);
	if (err < 0)
		goto out;

	err = fa->carrier_op->init(fa);
	if (err < 0)
		goto out;

	err = fa->carrier_op->reset_core(fa);
	if (err < 0)
		goto out;

	/* init all subsystems */
	for (i = 0, m = mods; i < ARRAY_SIZE(mods); i++, m++) {
		dev_dbg(&fmc->dev, "Calling init for \"%s\"\n", m->name);
		err = m->init(fa);
		if (err) {
			dev_err(&fmc->dev, "error initializing %s\n", m->name);
			goto out;
		}
	}

	/* time to execute specific driver init */
	err = __fa_init(fa);
	if (err < 0)
		goto out;

	err = fa_setup_irqs(fa);
	if (err < 0)
		goto out;

	return 0;

out:
	while (--m, --i >= 0)
		if (m->exit)
			m->exit(fa);
	return err;
}

int fa_remove(struct fmc_device *fmc)
{
	struct fa_dev *fa = fmc_get_drvdata(fmc);
	struct fa_modlist *m;
	int i = ARRAY_SIZE(mods);

	fa_free_irqs(fa);
	flush_workqueue(fa_workqueue);

	while (--i >= 0) {
		m = mods + i;
		if (m->exit)
			m->exit(fa);
	}

	fa->carrier_op->exit(fa);

	return 0;
}

static struct fmc_fru_id fa_fru_id[] = {
	{
		.product_name = "FmcAdc100m14b4cha",
	},
};

static struct fmc_driver fa_dev_drv = {
	.version = FMC_VERSION,
	.driver.name = KBUILD_MODNAME,
	.probe = fa_probe,
	.remove = fa_remove,
	.id_table = {
		.fru_id = fa_fru_id,
		.fru_id_nr = ARRAY_SIZE(fa_fru_id),
	},
};

static int fa_init(void)
{
	int ret;

	/*fa_workqueue = alloc_workqueue(fa_dev_drv.driver.name,
					WQ_NON_REENTRANT | WQ_UNBOUND |
					WQ_MEM_RECLAIM, 1);*/
	fa_workqueue = alloc_workqueue(fa_dev_drv.driver.name, WQ_UNBOUND |
					WQ_MEM_RECLAIM, 1);
	if (fa_workqueue == NULL)
		return -ENOMEM;

	/* First trigger and zio driver */
	ret = fa_trig_init();
	if (ret)
		goto out1;

	ret = fa_zio_register();
	if (ret)
		goto out2;

	/* Finally the fmc driver, whose probe instantiates zio devices */
	ret = fmc_driver_register(&fa_dev_drv);
	if (ret)
		goto out3;

	return ret;

out3:
	fa_zio_unregister();
out2:
	fa_trig_exit();
out1:
	destroy_workqueue(fa_workqueue);

	return ret;
}

static void fa_exit(void)
{
	fmc_driver_unregister(&fa_dev_drv);
	fa_zio_unregister();
	fa_trig_exit();
	if (fa_workqueue != NULL)
		destroy_workqueue(fa_workqueue);
}

module_init(fa_init);
module_exit(fa_exit);

MODULE_AUTHOR("Federico Vaga");
MODULE_DESCRIPTION("FMC-ADC-100MS-14b Linux Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(GIT_VERSION);

CERN_SUPER_MODULE;
