/*
 * Copyright CERN 2012
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * Table of register masks, used by driver functions
 */
#include "fa-spec.h"

/* Definition of the fa spec registers field: offset - mask - isbitfield */
const struct zfa_field_desc fa_spec_regs[] = {
	/* Carrier CSR */
	[ZFA_CAR_FMC_PRES] =	     {0x04, 0x1, 1},
	[ZFA_CAR_P2L_PLL] =	     {0x04, 0x2, 1},
	[ZFA_CAR_SYS_PLL] =	     {0x04, 0x4, 1},
	[ZFA_CAR_DDR_CAL] =	     {0x04, 0x8, 1},
	[ZFA_CAR_FMC_RES] =	     {0x0c, 0x1, 1},
	/* IRQ */
	[ZFA_IRQ_DMA_DISABLE_MASK] = {0x00, 0x00000003, 0},
	[ZFA_IRQ_DMA_ENABLE_MASK] =  {0x04, 0x00000003, 0},
	[ZFA_IRQ_DMA_MASK_STATUS] =  {0x08, 0x00000003, 0},
	[ZFA_IRQ_DMA_SRC] =	     {0x0C, 0x00000003, 0},
	/* DMA */
	[ZFA_DMA_CTL_SWP] =	     {0x00, 0x0000000C, 1},
	[ZFA_DMA_CTL_ABORT] =	     {0x00, 0x00000002, 1},
	[ZFA_DMA_CTL_START] =	     {0x00, 0x00000001, 1},
	[ZFA_DMA_STA] =		     {0x04, 0x00000007, 0},
	[ZFA_DMA_ADDR] =	     {0x08, 0xFFFFFFFF, 0},
	[ZFA_DMA_ADDR_L] =	     {0x0C, 0xFFFFFFFF, 0},
	[ZFA_DMA_ADDR_H] =	     {0x10, 0xFFFFFFFF, 0},
	[ZFA_DMA_LEN] =		     {0x14, 0xFFFFFFFF, 0},
	[ZFA_DMA_NEXT_L] =	     {0x18, 0xFFFFFFFF, 0},
	[ZFA_DMA_NEXT_H] =	     {0x1C, 0xFFFFFFFF, 0},
	[ZFA_DMA_BR_DIR] =	     {0x20, 0x00000002, 1},
	[ZFA_DMA_BR_LAST] =	     {0x20, 0x00000001, 1},
};

