/*
 * Copyright CERN 2012
 * Author: Federico Vaga <federico.vaga@gmail.com>
 *
 * Table of register masks, used by driver functions
 */
#include "fmc-adc-100m14b4cha.h"

/* Definition of the fmc-adc registers fields: offset - mask - isbitfield */
const struct zfa_field_desc zfad_regs[] = {
	/* Control registers */
	[ZFA_CTL_FMS_CMD] =		{0x00, 0x00000003, 1},
	[ZFA_CTL_CLK_EN] =		{0x00, 0x00000004, 1},
	[ZFA_CTL_DAC_CLR_N] =		{0x00, 0x00000008, 1},
	[ZFA_CTL_BSLIP] =		{0x00, 0x00000010, 1},
	[ZFA_CTL_TEST_DATA_EN] =	{0x00, 0x00000020, 1},
	[ZFA_CTL_TRIG_LED] =		{0x00, 0x00000040, 1},
	[ZFA_CTL_ACQ_LED] =		{0x00, 0x00000080, 1},
	/* Status registers */
	[ZFA_STA_FSM] =			{0x04, 0x00000007, 1},
	[ZFA_STA_SERDES_PLL] =		{0x04, 0x00000008, 1},
	[ZFA_STA_SERDES_SYNCED] =	{0x04, 0x00000010, 1},
	/* Trigger */
		/* Config register */
	[ZFAT_CFG_HW_SEL] =		{0x08, 0x00000001, 1},
	[ZFAT_CFG_HW_POL] =		{0x08, 0x00000002, 1},
	[ZFAT_CFG_HW_EN] =		{0x08, 0x00000004, 1},
	[ZFAT_CFG_SW_EN] =		{0x08, 0x00000008, 1},
	[ZFAT_CFG_INT_SEL] =		{0x08, 0x00000030, 1},
	[ZFAT_CFG_TEST_EN] =		{0x08, 0x00000040, 1},
	[ZFAT_CFG_THRES_FILT] =		{0x08, 0x0000FF00, 1},
	[ZFAT_CFG_THRES] =		{0x08, 0xFFFF0000, 1},
		/* Delay */
	[ZFAT_DLY] =			{0x0C, 0xFFFFFFFF, 0},
		/* Software */
	[ZFAT_SW] =			{0x10, 0xFFFFFFFF, 0},
		/* Number of shots */
	[ZFAT_SHOTS_NB] =		{0x14, 0x0000FFFF, 0},
		/* Remaining shots counter */
	[ZFAT_SHOTS_REM] =		{0x18, 0x0000FFFF, 0},
		/* Sampling clock frequency */
	[ZFAT_SAMPLING_HZ] =		{0x20, 0xFFFFFFFF, 0},
		/* Sample rate */
	[ZFAT_SR_DECI] =		{0x24, 0xFFFFFFFF, 0},
		/* Position address */
	[ZFAT_POS] =			{0x1C, 0xFFFFFFFF, 0},
		/* Pre-sample */
	[ZFAT_PRE] =			{0x28, 0xFFFFFFFF, 0},
		/* Post-sample */
	[ZFAT_POST] =			{0x2C, 0xFFFFFFFF, 0},
		/* Sample counter */
	[ZFAT_CNT] =			{0x30, 0xFFFFFFFF, 0},
	/* Channel 1 */
	[ZFA_CH1_CTL_RANGE] =		{0x34, 0x00000077, 1},
	[ZFA_CH1_CTL_TERM] =		{0x34, 0x00000008, 1},
	[ZFA_CH1_STA] =			{0x38, 0x0000FFFF, 0},
	[ZFA_CH1_GAIN] =		{0x3C, 0x0000FFFF, 0},
	[ZFA_CH1_OFFSET] =		{0x40, 0x0000FFFF, 0},
	[ZFA_CH1_SAT] =			{0x44, 0x00007FFF, 0},
	/* Channel 2 */
	[ZFA_CH2_CTL_RANGE] =		{0x48, 0x00000077, 1},
	[ZFA_CH2_CTL_TERM] =		{0x48, 0x00000008, 1},
	[ZFA_CH2_STA] =			{0x4C, 0x0000FFFF, 0},
	[ZFA_CH2_GAIN] =		{0x50, 0x0000FFFF, 0},
	[ZFA_CH2_OFFSET] =		{0x54, 0x0000FFFF, 0},
	[ZFA_CH2_SAT] =			{0x58, 0x00007FFF, 0},
	/* Channel 3 */
	[ZFA_CH3_CTL_RANGE] =		{0x5C, 0x00000077, 1},
	[ZFA_CH3_CTL_TERM] =		{0x5C, 0x00000008, 1},
	[ZFA_CH3_STA] =			{0x60, 0x0000FFFF, 0},
	[ZFA_CH3_GAIN] =		{0x64, 0x0000FFFF, 0},
	[ZFA_CH3_OFFSET] =		{0x68, 0x0000FFFF, 0},
	[ZFA_CH3_SAT] =			{0x6C, 0x00007FFF, 0},
	/* Channel 4 */
	[ZFA_CH4_CTL_RANGE] =		{0x70, 0x00000077, 1},
	[ZFA_CH4_CTL_TERM] =		{0x70, 0x00000008, 1},
	[ZFA_CH4_STA] =			{0x74, 0x0000FFFF, 0},
	[ZFA_CH4_GAIN] =		{0x78, 0x0000FFFF, 0},
	[ZFA_CH4_OFFSET] =		{0x7C, 0x0000FFFF, 0},
	[ZFA_CH4_SAT] =			{0x80, 0x00007FFF, 0},
	/* IRQ */
	[ZFA_IRQ_ADC_DISABLE_MASK] =	{0x00, 0x00000003, 0},
	[ZFA_IRQ_ADC_ENABLE_MASK] =	{0x04, 0x00000003, 0},
	[ZFA_IRQ_ADC_MASK_STATUS] =	{0x08, 0x00000003, 0},
	[ZFA_IRQ_ADC_SRC] =		{0x0C, 0x00000003, 0},
	[ZFA_IRQ_VIC_CTRL] =		{0x00, 0x000FFFFF, 0},
	[ZFA_IRQ_VIC_ENABLE_MASK] =     {0x08, 0x00000003, 0},
	[ZFA_IRQ_VIC_DISABLE_MASK] =    {0x0C, 0x00000003, 0},
	[ZFA_IRQ_VIC_MASK_STATUS] =     {0x10, 0x00000003, 0},
	/* UTC */
	[ZFA_UTC_SECONDS] =		{0x00, ~0x0, 0},
	[ZFA_UTC_COARSE] =		{0x04, ~0x0, 0},
	[ZFA_UTC_TRIG_META] =		{0x08, ~0x0, 0},
	[ZFA_UTC_TRIG_SECONDS] =	{0x0C, ~0x0, 0},
	[ZFA_UTC_TRIG_COARSE] =		{0x10, ~0x0, 0},
	[ZFA_UTC_TRIG_FINE] =		{0x14, ~0x0, 0},
	[ZFA_UTC_ACQ_START_META] =	{0x18, ~0x0, 0},
	[ZFA_UTC_ACQ_START_SECONDS] =	{0x1C, ~0x0, 0},
	[ZFA_UTC_ACQ_START_COARSE] =	{0x20, ~0x0, 0},
	[ZFA_UTC_ACQ_START_FINE] =	{0x24, ~0x0, 0},
	[ZFA_UTC_ACQ_STOP_META] =	{0x28, ~0x0, 0},
	[ZFA_UTC_ACQ_STOP_SECONDS] =	{0x2C, ~0x0, 0},
	[ZFA_UTC_ACQ_STOP_COARSE] =	{0x30, ~0x0, 0},
	[ZFA_UTC_ACQ_STOP_FINE] =	{0x34, ~0x0, 0},
	[ZFA_UTC_ACQ_END_META] =	{0x38, ~0x0, 0},
	[ZFA_UTC_ACQ_END_SECONDS] =	{0x3C, ~0x0, 0},
	[ZFA_UTC_ACQ_END_COARSE] =	{0x40, ~0x0, 0},
	[ZFA_UTC_ACQ_END_FINE] =	{0x44, ~0x0, 0},
};
