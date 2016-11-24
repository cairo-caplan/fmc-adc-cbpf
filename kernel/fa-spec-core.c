/*
 * core-spec fmc-adc-100m14b driver
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *		Copied from fine-delay
 *
 */

#include <linux/time.h>
#include <linux/delay.h>
#include <linux/fmc.h>
#include <linux/fmc-sdb.h>

#include "fmc-adc-100m14b4cha.h"
#include "fa-spec.h"

static char *fa_spec_get_gwname(void)
{
	return FA_GATEWARE_SPEC;
}

static int fa_spec_init(struct fa_dev *fa)
{
	struct device *msgdev = &fa->fmc->dev;
	struct fa_spec_data *cdata;
	uint32_t val;

	fa->fa_carrier_csr_base = fmc_find_sdb_device(fa->fmc->sdb, 0xce42,
						      0x603, NULL);

	cdata = kzalloc(sizeof(struct fa_spec_data), GFP_KERNEL);
	if (!cdata)
		return -ENOMEM;

	/* SDB carrier specific */
	cdata->fa_dma_base =
	    fmc_find_sdb_device(fa->fmc->sdb, 0xce42, 0x601, NULL);
	cdata->fa_irq_dma_base =
	    fmc_find_sdb_device(fa->fmc->sdb, 0xce42, 0xd5735ab4, NULL);

	dev_info(msgdev,
		"Spec Base addrs: irq_dmma:0x%x, dma_ctrl:0x%x, csr:0x%x\n",
		cdata->fa_irq_dma_base, cdata->fa_dma_base,
		fa->fa_carrier_csr_base);

	/* Wait 50ms, so device has time to calibrate */
	mdelay(50);

	/* set FMC0 in normal FMC operation */
	fa_writel(fa, fa->fa_carrier_csr_base,
			&fa_spec_regs[ZFA_CAR_FMC_RES], 1);

	/* Verify that the FMC is plugged (0 is plugged) */
	val = fa_readl(fa, fa->fa_carrier_csr_base,
		       &fa_spec_regs[ZFA_CAR_FMC_PRES]);
	if (val) {
		dev_err(msgdev, "No FCM ADC plugged\n");
		return -ENODEV;
	}
	/* Verify that system PLL is locked (1 is calibrated) */
	val = fa_readl(fa, fa->fa_carrier_csr_base,
		       &fa_spec_regs[ZFA_CAR_SYS_PLL]);
	if (!val) {
		dev_err(msgdev, "System PLL not locked\n");
		return -ENODEV;
	}
	/* Verify that DDR3 calibration is done (1 is calibrated) */
	val = fa_readl(fa, fa->fa_carrier_csr_base,
		       &fa_spec_regs[ZFA_CAR_DDR_CAL]);
	if (!val) {
		dev_err(msgdev, "DDR3 Calibration not done\n");
		return -ENODEV;
	}

	/* Set DMA to transfer data from device to host */
	fa_writel(fa, cdata->fa_dma_base,
			&fa_spec_regs[ZFA_DMA_BR_DIR], 0);

	/* register carrier data */
	fa->carrier_data = cdata;
	dev_info(msgdev, "spec::%s successfully executed\n", __func__);
	return 0;
}

static int fa_spec_reset(struct fa_dev *fa)
{
	/*struct spec_dev *spec = fa->fmc->carrier_data;*/

	dev_info(&fa->fmc->dev, "%s: resetting TDC core through Gennum.\n",
		 __func__);
	return 0;
}

static void fa_spec_exit(struct fa_dev *fa)
{
	kfree(fa->carrier_data);
}

/* Unfortunately, on the spec this is GPIO9, i.e. IRQ(1) */
static struct fmc_gpio fa_gpio_on[] = {
	{
	 .gpio = FMC_GPIO_IRQ(0),
	 .mode = GPIOF_DIR_IN,
	 .irqmode = IRQF_TRIGGER_RISING,
	 }
};

static struct fmc_gpio fa_gpio_off[] = {
	{
	 .gpio = FMC_GPIO_IRQ(0),
	 .mode = GPIOF_DIR_IN,
	 .irqmode = 0,
	 }
};

static int fa_spec_setup_irqs(struct fa_dev *fa)
{
	struct fmc_device *fmc = fa->fmc;
	struct fa_spec_data *spec_data = fa->carrier_data;
	int err;

	/* Request IRQ
	 * trick : vic needs the base address of teh core firing the irq
	 * It cannot provided throught irq_request() call therefore the trick
	 * is to set it by means of the field irq provided by the fmc device
	 */
	fmc->irq = spec_data->fa_irq_dma_base;
	err = fmc->op->irq_request(fmc, fa_spec_irq_handler,
					"fmc-adc-100m14b", 0);
	if (err) {
		dev_err(&fmc->dev, "can't request irq 0x%x (error %i)\n",
			fmc->irq, err);
		return err;
	}
	fmc->op->gpio_config(fmc, fa_gpio_on, ARRAY_SIZE(fa_gpio_on));
	dev_info(&fmc->dev, "spec::%s successfully executed\n", __func__);

	/* Add SPEC specific IRQ sources to listen */
	fa->irq_src |= FA_IRQ_SRC_DMA;

	return 0;
}

static int fa_spec_free_irqs(struct fa_dev *fa)
{
	struct fmc_device *fmc = fa->fmc;
	struct fa_spec_data *spec_data = fa->carrier_data;

	/* Release DMA IRQs */
	fmc->irq = spec_data->fa_irq_dma_base;
	fmc->op->irq_free(fmc);

	fmc->op->gpio_config(fmc, fa_gpio_off, ARRAY_SIZE(fa_gpio_off));

	return 0;
}

static int fa_spec_enable_irqs(struct fa_dev *fa)
{
	struct fa_spec_data *spec_data = fa->carrier_data;

	fa_writel(fa, spec_data->fa_irq_dma_base,
			&fa_spec_regs[ZFA_IRQ_DMA_ENABLE_MASK],
			FA_SPEC_IRQ_DMA_ALL);

	return 0;
}

static int fa_spec_disable_irqs(struct fa_dev *fa)
{
	struct fa_spec_data *spec_data = fa->carrier_data;

	fa_writel(fa, spec_data->fa_irq_dma_base,
			&fa_spec_regs[ZFA_IRQ_DMA_DISABLE_MASK],
			FA_SPEC_IRQ_DMA_NONE);

	return 0;
}

static int fa_spec_ack_irq(struct fa_dev *fa, int irq_id)
{
	return 0;
}

struct fa_carrier_op fa_spec_op = {
	.get_gwname = fa_spec_get_gwname,
	.init = fa_spec_init,
	.reset_core = fa_spec_reset,
	.exit = fa_spec_exit,
	.setup_irqs = fa_spec_setup_irqs,
	.free_irqs = fa_spec_free_irqs,
	.enable_irqs = fa_spec_enable_irqs,
	.disable_irqs = fa_spec_disable_irqs,
	.ack_irq = fa_spec_ack_irq,
	.dma_start = fa_spec_dma_start,
	.dma_done = fa_spec_dma_done,
	.dma_error = fa_spec_dma_error,
};
