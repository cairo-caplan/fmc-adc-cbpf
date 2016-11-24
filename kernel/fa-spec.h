/*
 * Copyright CERN 2012
 *
 * Header for the fmc-adc-100m14b spec driver
 *
 */

#ifndef __FA_SPEC_CORE_H__
#define __FA_SPEC_CORE_H__

#include <linux/scatterlist.h>
#include <linux/irqreturn.h>

#include "fmc-adc-100m14b4cha.h"
#include "field-desc.h"

/* default spec gateware */
#define FA_GATEWARE_SPEC  "fmc/spec-fmc-adc-100m14b.bin"

/* Should be replaced by an sdb query */
#define SPEC_FA_DMA_MEM_OFF	0x01000

/*
 * fa_dma_item: The information about a DMA transfer
 * @start_addr: pointer where start to retrieve data from device memory
 * @dma_addr_l: low 32bit of the dma address on host memory
 * @dma_addr_h: high 32bit of the dma address on host memory
 * @dma_len: number of bytes to transfer from device to host
 * @next_addr_l: low 32bit of the address of the next memory area to use
 * @next_addr_h: high 32bit of the address of the next memory area to use
 * @attribute: dma information about data transferm. At the moment it is used
 *             only to provide the "last item" bit, direction is fixed to
 *             device->host
 */
struct fa_dma_item {
	uint32_t start_addr;	/* 0x00 */
	uint32_t dma_addr_l;	/* 0x04 */
	uint32_t dma_addr_h;	/* 0x08 */
	uint32_t dma_len;	/* 0x0C */
	uint32_t next_addr_l;	/* 0x10 */
	uint32_t next_addr_h;	/* 0x14 */
	uint32_t attribute;	/* 0x18 */
	uint32_t reserved;	/* ouch */
};

/* SPEC CSR */
enum fa_spec_regs_id {
	/* CSR */
	ZFA_CAR_FMC_PRES,
	ZFA_CAR_P2L_PLL,
	ZFA_CAR_SYS_PLL,
	ZFA_CAR_DDR_CAL,
	ZFA_CAR_FMC_RES,
	/* IRQ DMA: DMA spec specific irq controller */
	ZFA_IRQ_DMA_DISABLE_MASK,
	ZFA_IRQ_DMA_ENABLE_MASK,
	ZFA_IRQ_DMA_MASK_STATUS,
	ZFA_IRQ_DMA_SRC,
	/* DMA */
	ZFA_DMA_CTL_SWP,
	ZFA_DMA_CTL_ABORT,
	ZFA_DMA_CTL_START,
	ZFA_DMA_STA,
	ZFA_DMA_ADDR,
	ZFA_DMA_ADDR_L,
	ZFA_DMA_ADDR_H,
	ZFA_DMA_LEN,
	ZFA_DMA_NEXT_L,
	ZFA_DMA_NEXT_H,
	ZFA_DMA_BR_DIR,
	ZFA_DMA_BR_LAST,
};

/* SPEC ADC have to listen two IRQ sources managed by two different cores */
#define FA_SPEC_IRQ_SRC_NONE 0
#define FA_SPEC_IRQ_SRC_ACQ 1
#define FA_SPEC_IRQ_SRC_DMA 2

/* DMA spec specific IRQ values */
enum fa_spec_irq {
	FA_SPEC_IRQ_DMA_NONE =	0x0,
	FA_SPEC_IRQ_DMA_DONE =	0x1,
	FA_SPEC_IRQ_DMA_ERR =	0x2,
	FA_SPEC_IRQ_DMA_ALL =	0x3,
};

/* specific carrier data */
struct fa_spec_data {
	/* DMA attributes */
	unsigned int		fa_dma_base;
	unsigned int		fa_irq_dma_base;
	struct fa_dma_item	*items;
	dma_addr_t		dma_list_item;
	unsigned int		n_dma_err; /* statistics */
};

/* spec specific hardware registers */
extern const struct zfa_field_desc fa_spec_regs[];
/* spec irq handler */
extern irqreturn_t fa_spec_irq_handler(int irq, void *dev_id);

/* functions exported by fa-dma.c */
extern int fa_spec_dma_start(struct zio_cset *cset);
extern void fa_spec_dma_done(struct zio_cset *cset);
extern void fa_spec_dma_error(struct zio_cset *cset);

#endif /* __FA_SPEC_CORE_H__*/
