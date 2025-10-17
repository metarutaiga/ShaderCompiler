/*
 * Copyright © 2013 Rob Clark <robdclark@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef IR3_H_
#define IR3_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
   ALIAS_TEX = 0,
   ALIAS_RT = 1,
   ALIAS_MEM = 2,
} ir3_alias_scope;

/* r0.x - r47.w are "normal" registers. r48.x - r55.w are shared registers.
 * Everything above those are non-GPR registers like a0.x and p0.x that aren't
 * assigned by RA.
 */
#define GPR_REG_SIZE (4 * 48)
#define SHARED_REG_START GPR_REG_SIZE
#define SHARED_REG_SIZE (4 * 8)
#define NONGPR_REG_START (SHARED_REG_START + SHARED_REG_SIZE)
#define NONGPR_REG_SIZE (4 * 8)

#endif /* IR3_H_ */
