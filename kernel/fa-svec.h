/*
 * Copyright CERN 2012
 *
 * Header for the fmc-adc-100m14b spec driver
 *
 */

#ifndef __FA_SVEC_CORE_H__
#define __FA_SVEC_CORE_H__

#include <linux/irqreturn.h>

#include "fmc-adc-100m14b4cha.h"
#include "field-desc.h"

/* default spec gateware */
#define FA_GATEWARE_SVEC  "fmc/svec-fmc-adc-100m14b.bin"

/* SPEC CSR */
enum fa_spec_regs_id {
	/* DMA */
	FA_DMA_DDR_ADDR = 0,
	FA_DMA_DDR_DATA,
	/* CSR */
	FA_CAR_FMC0_PRES,
	FA_CAR_FMC1_PRES,
	FA_CAR_SYS_PLL,
	FA_CAR_DDR0_CAL,
	FA_CAR_DDR1_CAL,
	FA_CAR_FMC0_RES,
	FA_CAR_FMC1_RES,
};

/* specific carrier data */
struct fa_svec_data {
	/* DMA attributes */
	unsigned long	vme_base;
	unsigned int	fa_dma_ddr_data; /* offset */
	unsigned int	fa_dma_ddr_addr; /* offset */
	unsigned int	n_dma_err; /* statistics */
};

/* svec specific hardware registers */
extern const struct zfa_field_desc fa_svec_regfield[];
/* svec irq handler */
extern irqreturn_t fa_svec_irq_handler(int irq, void *dev_id);

/* functions exported by fa-svec-dma.c */
extern int fa_svec_dma_start(struct zio_cset *cset);
extern void fa_svec_dma_done(struct zio_cset *cset);
extern void fa_svec_dma_error(struct zio_cset *cset);

#endif /* __FA_SVEC_CORE_H__*/
