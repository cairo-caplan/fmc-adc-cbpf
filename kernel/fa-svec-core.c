/*
 * core-spec fmc-adc-100m14b driver
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *		Copied from fine-delay
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fmc.h>
#include <linux/fmc-sdb.h>

#include <svec.h>
#include "fmc-adc-100m14b4cha.h"
#include "fa-svec.h"

static char *fa_svec_get_gwname(void)
{
	return FA_GATEWARE_SVEC;
}

static int fa_svec_init(struct fa_dev *fa)
{
	struct fmc_device *fmc = fa->fmc;
	struct fa_svec_data *cdata;
	struct svec_dev *svec = fmc->carrier_data;

	cdata = kzalloc(sizeof(struct fa_svec_data), GFP_KERNEL);
	if (!cdata)
		return -ENOMEM;

	cdata->vme_base = svec->cfg_cur.vme_base;
	fa->fa_carrier_csr_base = fmc_find_sdb_device(fmc->sdb, 0xce42,
						      0x6603, NULL);
	cdata->fa_dma_ddr_addr = fmc_find_sdb_device_ext(fmc->sdb, 0xce42,
							 0x10006611,
							fmc->slot_id, NULL);
	cdata->fa_dma_ddr_data = fmc_find_sdb_device_ext(fmc->sdb, 0xce42,
							 0x10006610,
							fmc->slot_id, NULL);

	if (fmc->slot_id == 0)
		/* set FMC0 in normal FMC operation */
		fa_writel(fa, fa->fa_carrier_csr_base,
			&fa_svec_regfield[FA_CAR_FMC0_RES], 1);
	else if (fmc->slot_id == 1)
		/* set FMC1 in normal FMC operation */
		fa_writel(fa, fa->fa_carrier_csr_base,
			&fa_svec_regfield[FA_CAR_FMC1_RES], 1);

	/* register carrier data */
	fa->carrier_data = cdata;
	return 0;
}

static int fa_svec_reset(struct fa_dev *fa)
{
	return 0;
}

static void fa_svec_exit(struct fa_dev *fa)
{
	kfree(fa->carrier_data);
}

struct fa_carrier_op fa_svec_op = {
	.get_gwname = fa_svec_get_gwname,
	.init = fa_svec_init,
	.reset_core = fa_svec_reset,
	.exit = fa_svec_exit,
	.dma_start = fa_svec_dma_start,
	.dma_done = fa_svec_dma_done,
	.dma_error = fa_svec_dma_error,
};
