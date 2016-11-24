/*
 * Access to 1w thermometer
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Copied from the fine-delay driver and updated with fmc-adc variable
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2 as published by the Free Software Foundation or, at your
 * option, any later version.
 */

#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "fmc-adc-100m14b4cha.h"

#define R_CSR		0x0
#define R_CDR		0x4

#define CSR_DAT_MSK	(1<<0)
#define CSR_RST_MSK	(1<<1)
#define CSR_OVD_MSK	(1<<2)
#define CSR_CYC_MSK	(1<<3)
#define CSR_PWR_MSK	(1<<4)
#define CSR_IRQ_MSK	(1<<6)
#define CSR_IEN_MSK	(1<<7)
#define CSR_SEL_OFS	8
#define CSR_SEL_MSK	(0xF<<8)
#define CSR_POWER_OFS	16
#define CSR_POWER_MSK	(0xFFFF<<16)
#define CDR_NOR_MSK	(0xFFFF<<0)
#define CDR_OVD_OFS	16
#define CDR_OVD_MSK	(0xFFFF<<16)

#define CLK_DIV_NOR	(624)
#define CLK_DIV_OVD	(124)

#define CMD_ROM_SEARCH		0xF0
#define CMD_ROM_READ		0x33
#define CMD_ROM_MATCH		0x55
#define CMD_ROM_SKIP		0xCC
#define CMD_ROM_ALARM_SEARCH	0xEC

#define CMD_CONVERT_TEMP	0x44
#define CMD_WRITE_SCRATCHPAD	0x4E
#define CMD_READ_SCRATCHPAD	0xBE
#define CMD_COPY_SCRATCHPAD	0x48
#define CMD_RECALL_EEPROM	0xB8
#define CMD_READ_POWER_SUPPLY	0xB4

#define FA_OW_PORT 0 /* what is this slow? */

static void ow_writel(struct fa_dev *fa, uint32_t val, unsigned long reg)
{
	fmc_writel(fa->fmc, val, fa->fa_ow_base + reg);
}

static uint32_t ow_readl(struct fa_dev *fa, unsigned long reg)
{
	return fmc_readl(fa->fmc, fa->fa_ow_base + reg);
}

static int ow_reset(struct fa_dev *fa, int port)
{
	uint32_t reg, data;

	data = ((port << CSR_SEL_OFS) & CSR_SEL_MSK)
		| CSR_CYC_MSK | CSR_RST_MSK;
	ow_writel(fa, data, R_CSR);
	while (ow_readl(fa, R_CSR) & CSR_CYC_MSK)
		/* FIXME: timeout */;
	reg = ow_readl(fa, R_CSR);
	return ~reg & CSR_DAT_MSK;
}

static int slot(struct fa_dev *fa, int port, int bit)
{
	uint32_t reg, data;

	data = ((port<<CSR_SEL_OFS) & CSR_SEL_MSK)
		| CSR_CYC_MSK | (bit & CSR_DAT_MSK);
	ow_writel(fa, data, R_CSR);
	while (ow_readl(fa, R_CSR) & CSR_CYC_MSK)
		/* FIXME: timeout */;
	reg = ow_readl(fa, R_CSR);
	return reg & CSR_DAT_MSK;
}

static int read_bit(struct fa_dev *fa, int port)
{
	return slot(fa, port, 0x1);
}

static int write_bit(struct fa_dev *fa, int port, int bit)
{
	return slot(fa, port, bit);
}

static int ow_read_byte(struct fa_dev *fa, int port)
{
	int byte = 0, i;

	for (i = 0; i < 8; i++)
		byte |= (read_bit(fa, port) << i);
	return byte;
}

static int ow_write_byte(struct fa_dev *fa, int port, int byte)
{
	int data = 0;
	int i;

	for (i = 0; i < 8; i++) {
		data |= write_bit(fa, port, (byte & 0x1)) << i;
		byte >>= 1;
	}
	return 0; /* success */
}

static int ow_write_block(struct fa_dev *fa, int port, uint8_t *block, int len)
{
	int i;

	for (i = 0; i < len; i++)
		ow_write_byte(fa, port, block[i]);
	return 0;
}

static int ow_read_block(struct fa_dev *fa, int port, uint8_t *block, int len)
{
	int i;
	for (i = 0; i < len; i++)
		block[i] = ow_read_byte(fa, port);
	return 0;
}

static int ds18x_read_serial(struct fa_dev *fa)
{
	if (!ow_reset(fa, 0)) {
		pr_err("%s: Failure in resetting one-wire channel\n",
		       KBUILD_MODNAME);
		return -EIO;
	}

	ow_write_byte(fa, FA_OW_PORT, CMD_ROM_READ);
	return ow_read_block(fa, FA_OW_PORT, fa->ds18_id, 8);
}

static int ds18x_access(struct fa_dev *fa)
{
	if (!ow_reset(fa, 0))
		goto out;

	if (0) {
		/* select the rom among several of them */
		if (ow_write_byte(fa, FA_OW_PORT, CMD_ROM_MATCH) < 0)
			goto out;
		return ow_write_block(fa, FA_OW_PORT, fa->ds18_id, 8);
	} else {
		/* we have one only, so skip rom */
		return ow_write_byte(fa, FA_OW_PORT, CMD_ROM_SKIP);
	}
out:
	pr_err("%s: Failure in one-wire communication\n", KBUILD_MODNAME);
	return -EIO;
}

static void __temp_command_and_next_t(struct fa_dev *fa, int cfg_reg)
{
	int ms;

	ds18x_access(fa);
	ow_write_byte(fa, FA_OW_PORT, CMD_CONVERT_TEMP);
	/* The conversion takes some time, so mark when will it be ready */
	ms = 94 * ( 1 << (cfg_reg >> 5));
	fa->next_t = jiffies + msecs_to_jiffies(ms);
}

int fa_read_temp(struct fa_dev *fa, int verbose)
{
	int i, temp;
	unsigned long j;
	uint8_t data[9];

	/* If first conversion, ask for it first */
	if (fa->next_t == 0)
		__temp_command_and_next_t(fa, 0x7f /* we ignore: max time */);

	/* Wait for it to be ready: (FIXME: we need a time policy here) */
	j = jiffies;
	if (time_before(j, fa->next_t)) {
		/* If we cannot sleep, return the previous value */
		if (in_atomic())
			return fa->temp;
		msleep(jiffies_to_msecs(fa->next_t - j));
	}

	ds18x_access(fa);
	ow_write_byte(fa, FA_OW_PORT, CMD_READ_SCRATCHPAD);
	ow_read_block(fa, FA_OW_PORT, data, 9);

	if (verbose > 1) {
		pr_info("%s: Scratchpad: ", __func__);
		for (i = 0; i < 9; i++)
			printk(KERN_CONT "%02x%c", data[i],
			       i == 8 ? '\n' : ':');
	}
	temp = ((int)data[1] << 8) | ((int)data[0]);
	if (temp & 0x1000)
		temp = -0x10000 + temp;
	fa->temp = temp;
	if (verbose) {
		pr_info("%s: Temperature 0x%x (%i bits: %i.%03i)\n", __func__,
			temp, 9 + (data[4] >> 5),
			temp / 16, (temp & 0xf) * 1000 / 16);
	}

	__temp_command_and_next_t(fa, data[4]);	/* start next conversion */
	return temp;
}

int fa_onewire_init(struct fa_dev *fa)
{
	ow_writel(fa, ((CLK_DIV_NOR & CDR_NOR_MSK)
		       | (( CLK_DIV_OVD << CDR_OVD_OFS) & CDR_OVD_MSK)),
		  R_CDR);

	if (ds18x_read_serial(fa) < 0)
		return -EIO;

	/* read the temperature once, to ensure it works, and print it */
	fa_read_temp(fa, 2);

	return 0;
}

void fa_onewire_exit(struct fa_dev *fa)
{
	/* Nothing to do */
}
