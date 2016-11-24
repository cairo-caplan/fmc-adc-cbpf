/*
 * Copyright CERN 2012
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * Table of register masks, used by driver functions
 */
#include "fa-svec.h"

/* fa svec specific registers field: offset - mask - isbitfield */
const struct zfa_field_desc fa_svec_regfield[] = {
	[FA_DMA_DDR_ADDR] =		{0x0000, 0x00FFFFFF, 0},
	[FA_DMA_DDR_DATA] =		{0x0000, 0x00FFFFFF, 0},
	/* CSR */
	[FA_CAR_FMC0_PRES] =		{0x0004, 0x00000001, 1},
	[FA_CAR_FMC1_PRES] =		{0x0004, 0x00000002, 1},
	[FA_CAR_SYS_PLL] =		{0x0004, 0x00000004, 1},
	[FA_CAR_DDR0_CAL] =		{0x0004, 0x00000008, 1},
	[FA_CAR_DDR1_CAL] =		{0x0004, 0x00000010, 1},
	[FA_CAR_FMC0_RES] =		{0x000C, 0x00000001, 1},
	[FA_CAR_FMC1_RES] =		{0x000C, 0x00000002, 1},
};


