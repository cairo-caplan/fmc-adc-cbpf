/*
 * Copyright 2012 Federico Vaga
 *
 * GNU GPLv2 or later
 */


#ifndef _FIELD_DESC_H_
#define _FIELD_DESC_H_

#include <linux/types.h>

/*
 * zfa_field_desc is a field register descriptor. By using address, mask
 * and shift the driver can describe every fields in registers.
 */
struct zfa_field_desc {
	unsigned long offset; /* related to its component base */
	uint32_t mask; /* bit mask a register field */
	int is_bitfield; /* whether it maps  full register or a field */
};

#endif /* _FIELD_DESC_H_ */
