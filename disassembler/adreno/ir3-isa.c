/* Copyright (C) 2020 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "ir3-isa.h"

#include <stdint.h>
#include "bitset.h"

#define BITMASK_WORDS BITSET_WORDS(64)

typedef struct {
    BITSET_WORD bitset[BITMASK_WORDS];
} bitmask_t;


#define BITSET_FORMAT "08x%08x"
#define BITSET_VALUE(v) v[1], v[0]

static inline void
next_instruction(bitmask_t *instr, BITSET_WORD *start)
{
    instr->bitset[0] = *(start + 0);
    instr->bitset[1] = *(start + 1);
}

static inline uint64_t
bitmask_to_uint64_t(bitmask_t mask)
{
    return ((uint64_t)mask.bitset[1] << 32) | mask.bitset[0];
}

static inline bitmask_t
uint64_t_to_bitmask(uint64_t val)
{
    bitmask_t mask = {
        .bitset[0] = val & 0xffffffff,
        .bitset[1] = (val >> 32) & 0xffffffff,
    };

    return mask;
}

#include "isaspec_decode_decl.h"

static uint64_t
isa_decode_field(struct decode_scope *scope, const char *field_name);

/*
 * enum tables, these don't have any link back to other tables so just
 * dump them up front before the bitset tables
 */

static const struct isa_enum enum___rptn = {
    .num_values = 8,
    .values = {
        { .val = 0, .display = "" },
        { .val = 1, .display = "(rpt1)" },
        { .val = 2, .display = "(rpt2)" },
        { .val = 3, .display = "(rpt3)" },
        { .val = 4, .display = "(rpt4)" },
        { .val = 5, .display = "(rpt5)" },
        { .val = 6, .display = "(rpt6)" },
        { .val = 7, .display = "(rpt7)" },
    },
};
static const struct isa_enum enum___cond = {
    .num_values = 6,
    .values = {
        { .val = 0, .display = "lt" },
        { .val = 1, .display = "le" },
        { .val = 2, .display = "gt" },
        { .val = 3, .display = "ge" },
        { .val = 4, .display = "eq" },
        { .val = 5, .display = "ne" },
    },
};
static const struct isa_enum enum___swiz = {
    .num_values = 4,
    .values = {
        { .val = 0, .display = "x" },
        { .val = 1, .display = "y" },
        { .val = 2, .display = "z" },
        { .val = 3, .display = "w" },
    },
};
static const struct isa_enum enum___type = {
    .num_values = 8,
    .values = {
        { .val = 0, .display = "f16" },
        { .val = 1, .display = "f32" },
        { .val = 2, .display = "u16" },
        { .val = 3, .display = "u32" },
        { .val = 4, .display = "s16" },
        { .val = 5, .display = "s32" },
        { .val = 6, .display = "u8" },
        { .val = 7, .display = "u8_32" },
    },
};
static const struct isa_enum enum___type_atomic = {
    .num_values = 8,
    .values = {
        { .val = 0, .display = "f16" },
        { .val = 1, .display = "f32" },
        { .val = 2, .display = "u16" },
        { .val = 3, .display = "u32" },
        { .val = 4, .display = "s16" },
        { .val = 5, .display = "s32" },
        { .val = 6, .display = "u64" },
        { .val = 7, .display = "u8_32" },
    },
};
static const struct isa_enum enum___absneg = {
    .num_values = 4,
    .values = {
        { .val = 0, .display = "" },
        { .val = 1, .display = "(neg)" },
        { .val = 2, .display = "(abs)" },
        { .val = 3, .display = "(absneg)" },
    },
};
static const struct isa_enum enum___flut = {
    .num_values = 12,
    .values = {
        { .val = 0, .display = "(0.0)" },
        { .val = 1, .display = "(0.5)" },
        { .val = 2, .display = "(1.0)" },
        { .val = 3, .display = "(2.0)" },
        { .val = 4, .display = "(e)" },
        { .val = 5, .display = "(pi)" },
        { .val = 6, .display = "(1/pi)" },
        { .val = 7, .display = "(1/log2(e))" },
        { .val = 8, .display = "(log2(e))" },
        { .val = 9, .display = "(1/log2(10))" },
        { .val = 10, .display = "(log2(10))" },
        { .val = 11, .display = "(4.0)" },
    },
};
static const struct isa_enum enum___wrmask = {
    .num_values = 16,
    .values = {
        { .val = 0, .display = "" },
        { .val = 1, .display = "x" },
        { .val = 2, .display = "y" },
        { .val = 3, .display = "xy" },
        { .val = 4, .display = "z" },
        { .val = 5, .display = "xz" },
        { .val = 6, .display = "yz" },
        { .val = 7, .display = "xyz" },
        { .val = 8, .display = "w" },
        { .val = 9, .display = "xw" },
        { .val = 10, .display = "yw" },
        { .val = 11, .display = "xyw" },
        { .val = 12, .display = "zw" },
        { .val = 13, .display = "xzw" },
        { .val = 14, .display = "yzw" },
        { .val = 15, .display = "xyzw" },
    },
};
static const struct isa_enum enum___round = {
    .num_values = 4,
    .values = {
        { .val = 0, .display = "" },
        { .val = 1, .display = "(even)" },
        { .val = 2, .display = "(pos_infinity)" },
        { .val = 3, .display = "(neg_infinity)" },
    },
};
static const struct isa_enum enum___signedness = {
    .num_values = 2,
    .values = {
        { .val = 0, .display = ".unsigned" },
        { .val = 1, .display = ".mixed" },
    },
};
static const struct isa_enum enum___8bitvec2pack = {
    .num_values = 2,
    .values = {
        { .val = 0, .display = ".low" },
        { .val = 1, .display = ".high" },
    },
};
static const struct isa_enum enum___cat5_s2en_bindless_desc_mode = {
    .num_values = 8,
    .values = {
        { .val = 0, .display = "CAT5_UNIFORM" },
        { .val = 1, .display = "CAT5_BINDLESS_A1_UNIFORM" },
        { .val = 2, .display = "CAT5_BINDLESS_NONUNIFORM" },
        { .val = 3, .display = "CAT5_BINDLESS_A1_NONUNIFORM" },
        { .val = 4, .display = "CAT5_NONUNIFORM" },
        { .val = 5, .display = "CAT5_BINDLESS_UNIFORM" },
        { .val = 6, .display = "CAT5_BINDLESS_IMM" },
        { .val = 7, .display = "CAT5_BINDLESS_A1_IMM" },
    },
};
static const struct isa_enum enum___cat6_shfl_mode = {
    .num_values = 5,
    .values = {
        { .val = 1, .display = "xor" },
        { .val = 2, .display = "up" },
        { .val = 3, .display = "down" },
        { .val = 6, .display = "rup" },
        { .val = 7, .display = "rdown" },
    },
};
static const struct isa_enum enum___cat6_src_mode = {
    .num_values = 3,
    .values = {
        { .val = 0, .display = "imm" },
        { .val = 1, .display = "uniform" },
        { .val = 2, .display = "nonuniform" },
    },
};
static const struct isa_enum enum___dccln_type = {
    .num_values = 2,
    .values = {
        { .val = 0, .display = ".shr" },
        { .val = 1, .display = ".all" },
    },
};
static const struct isa_enum enum___sleep_duration = {
    .num_values = 2,
    .values = {
        { .val = 0, .display = ".s" },
        { .val = 1, .display = ".l" },
    },
};
static const struct isa_enum enum___alias_scope = {
    .num_values = 4,
    .values = {
        { .val = 0, .display = "tex" },
        { .val = 1, .display = "rt" },
        { .val = 2, .display = "mem" },
        { .val = 3, .display = "mem" },
    },
};
static const struct isa_enum enum___alias_type_size = {
    .num_values = 2,
    .values = {
        { .val = 0, .display = "16" },
        { .val = 1, .display = "32" },
    },
};
static const struct isa_enum enum___alias_type = {
    .num_values = 2,
    .values = {
        { .val = 0, .display = "f" },
        { .val = 1, .display = "b" },
    },
};
static const struct isa_enum enum___alias_src_reg_type = {
    .num_values = 3,
    .values = {
        { .val = 0, .display = "GPR" },
        { .val = 1, .display = "CONST" },
        { .val = 2, .display = "IMMED" },
    },
};

/*
 * generated expression functions, can be linked from bitset tables, so
 * also dump them up front
 */

static uint64_t
expr___cat2_cat3_nop_encoding(struct decode_scope *scope)
{
    int64_t REPEAT = isa_decode_field(scope, "REPEAT");
    int64_t SRC1_R = isa_decode_field(scope, "SRC1_R");
    int64_t SRC2_R = isa_decode_field(scope, "SRC2_R");
    return ((SRC1_R != 0) || (SRC2_R != 0)) && (REPEAT == 0);
}
static uint64_t
expr___cat2_cat3_nop_value(struct decode_scope *scope)
{
    int64_t SRC1_R = isa_decode_field(scope, "SRC1_R");
    int64_t SRC2_R = isa_decode_field(scope, "SRC2_R");
    return SRC1_R | (SRC2_R << 1);
}
static uint64_t
expr___reg_gpr_a0(struct decode_scope *scope)
{
    int64_t GPR = isa_decode_field(scope, "GPR");
    return GPR == 61 /* a0.* */;
}
static uint64_t
expr___reg_gpr_p0(struct decode_scope *scope)
{
    int64_t GPR = isa_decode_field(scope, "GPR");
    return GPR == 62 /* p0.x */;
}
static uint64_t
expr___offset_zero(struct decode_scope *scope)
{
    int64_t OFFSET = isa_decode_field(scope, "OFFSET");
    return OFFSET == 0;
}
static uint64_t
expr___multisrc_half(struct decode_scope *scope)
{
    int64_t FULL = isa_decode_field(scope, "FULL");
    return !FULL;
}
static uint64_t
expr___dest_half(struct decode_scope *scope)
{
    int64_t DST = isa_decode_field(scope, "DST");
    int64_t DST_CONV = isa_decode_field(scope, "DST_CONV");
    int64_t FULL = isa_decode_field(scope, "FULL");
    return (FULL == DST_CONV) && (DST <= 0xf7 /* p0.x */);
}
static uint64_t
expr___true(struct decode_scope *scope)
{
    return 1;
}
static uint64_t
expr___false(struct decode_scope *scope)
{
    return 0;
}
static uint64_t
expr___zero(struct decode_scope *scope)
{
    return 0;
}
static uint64_t
expr___one(struct decode_scope *scope)
{
    return 1;
}
static uint64_t
expr___two(struct decode_scope *scope)
{
    return 2;
}
static uint64_t
expr___type_half(struct decode_scope *scope)
{
    int64_t TYPE = isa_decode_field(scope, "TYPE");
    return (TYPE == 0) /* f16 */ ||
	(TYPE == 2) /* u16 */ ||
	(TYPE == 4) /* s16 */ ||
	(TYPE == 6) /* u8 */;
}
static uint64_t
expr_anon_0(struct decode_scope *scope)
{
    int64_t W = isa_decode_field(scope, "W");
    return 2ULL << W;
}
static uint64_t
expr_anon_1(struct decode_scope *scope)
{
    int64_t DST_REL = isa_decode_field(scope, "DST_REL");
    int64_t OFFSET = isa_decode_field(scope, "OFFSET");
    return (OFFSET == 0) && DST_REL;
}
static uint64_t
expr_anon_2(struct decode_scope *scope)
{
    int64_t DST_REL = isa_decode_field(scope, "DST_REL");
    return DST_REL;
}
static uint64_t
expr_anon_3(struct decode_scope *scope)
{
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return (SRC_TYPE == 0) /* f16 */ ||
			(SRC_TYPE == 2) /* u16 */ ||
			(SRC_TYPE == 4) /* s16 */ ||
			(SRC_TYPE == 6) /* u8 */  ||
			(SRC_TYPE == 7) /* s8 */;
}
static uint64_t
expr_anon_4(struct decode_scope *scope)
{
    int64_t DST_TYPE = isa_decode_field(scope, "DST_TYPE");
    return (DST_TYPE == 0) /* f16 */ ||
			(DST_TYPE == 2) /* u16 */ ||
			(DST_TYPE == 4) /* s16 */ ||
			(DST_TYPE == 6) /* u8 */  ||
			(DST_TYPE == 7) /* s8 */;
}
static uint64_t
expr_anon_5(struct decode_scope *scope)
{
    int64_t DST = isa_decode_field(scope, "DST");
    int64_t DST_TYPE = isa_decode_field(scope, "DST_TYPE");
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return (DST == 0xf4 /* a0.x */) && (SRC_TYPE == 4 /* s16 */) && (DST_TYPE == 4);
}
static uint64_t
expr_anon_6(struct decode_scope *scope)
{
    int64_t DST = isa_decode_field(scope, "DST");
    int64_t DST_TYPE = isa_decode_field(scope, "DST_TYPE");
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return (DST == 0xf5 /* a0.y */) && (SRC_TYPE == 2 /* u16 */) && (DST_TYPE == 2);
}
static uint64_t
expr_anon_7(struct decode_scope *scope)
{
    int64_t DST_TYPE = isa_decode_field(scope, "DST_TYPE");
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return SRC_TYPE != DST_TYPE;
}
static uint64_t
expr_anon_8(struct decode_scope *scope)
{
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return SRC_TYPE == 0 /* f16 */;
}
static uint64_t
expr_anon_9(struct decode_scope *scope)
{
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return SRC_TYPE == 1 /* f32 */;
}
static uint64_t
expr_anon_10(struct decode_scope *scope)
{
    int64_t IMMED = isa_decode_field(scope, "IMMED");
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return (SRC_TYPE == 3 /* u32 */) && (IMMED > 0x1000);
}
static uint64_t
expr_anon_11(struct decode_scope *scope)
{
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return SRC_TYPE == 4 /* s16 */;
}
static uint64_t
expr_anon_12(struct decode_scope *scope)
{
    int64_t SRC_TYPE = isa_decode_field(scope, "SRC_TYPE");
    return SRC_TYPE == 5 /* s32 */;
}
static uint64_t
expr_anon_13(struct decode_scope *scope)
{
    int64_t REPEAT = isa_decode_field(scope, "REPEAT");
    return (REPEAT + 1) * 32;
}
static uint64_t
expr___wmm_dest_half(struct decode_scope *scope)
{
    int64_t DST_FULL = isa_decode_field(scope, "DST_FULL");
    return (!DST_FULL);
}
static uint64_t
expr_anon_14(struct decode_scope *scope)
{
    int64_t IMMED_ENCODING = isa_decode_field(scope, "IMMED_ENCODING");
    return IMMED_ENCODING;
}
static uint64_t
expr___cat5_s2enb_is_indirect(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return DESC_MODE < 6  /* CAT5_BINDLESS_IMM */;
}
static uint64_t
expr___cat5_s2enb_is_bindless(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return (DESC_MODE == 1) /* CAT5_BINDLESS_A1_UNIFORM */ ||
	(DESC_MODE == 2) /* CAT5_BINDLESS_NONUNIFORM */ ||
	(DESC_MODE == 3) /* CAT5_BINDLESS_A1_NONUNIFORM */ ||
	(DESC_MODE == 5) /* CAT5_BINDLESS_UNIFORM */ ||
	(DESC_MODE == 6) /* CAT5_BINDLESS_IMM */ ||
	(DESC_MODE == 7) /* CAT5_BINDLESS_A1_IMM */;
}
static uint64_t
expr___cat5_s2enb_uses_a1(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return (DESC_MODE == 1) /* CAT5_BINDLESS_A1_UNIFORM */ ||
	(DESC_MODE == 3) /* CAT5_BINDLESS_A1_NONUNIFORM */ ||
	(DESC_MODE == 7) /* CAT5_BINDLESS_A1_IMM */;
}
static uint64_t
expr___cat5_s2enb_uses_a1_gen6(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return ISA_GPU_ID() >= 600 && ISA_GPU_ID() < 700 &&
	((DESC_MODE == 1) /* CAT5_BINDLESS_A1_UNIFORM */ ||
	 (DESC_MODE == 3) /* CAT5_BINDLESS_A1_NONUNIFORM */ ||
	 (DESC_MODE == 7))/* CAT5_BINDLESS_A1_IMM */;
}
static uint64_t
expr___cat5_s2enb_uses_a1_gen7(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return ISA_GPU_ID() >= 700 &&
	((DESC_MODE == 1) /* CAT5_BINDLESS_A1_UNIFORM */ ||
	 (DESC_MODE == 3) /* CAT5_BINDLESS_A1_NONUNIFORM */ ||
	 (DESC_MODE == 7))/* CAT5_BINDLESS_A1_IMM */;
}
static uint64_t
expr___cat5_s2enb_is_uniform(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return (DESC_MODE == 0) /* CAT5_UNIFORM */ ||
	(DESC_MODE == 1) /* CAT5_BINDLESS_A1_UNIFORM */ ||
	(DESC_MODE == 5) /* CAT5_BINDLESS_UNIFORM */;
}
static uint64_t
expr___cat5_s2enb_is_nonuniform(struct decode_scope *scope)
{
    int64_t DESC_MODE = isa_decode_field(scope, "DESC_MODE");
    return (DESC_MODE == 2) /* CAT5_BINDLESS_NONUNIFORM */ ||
	(DESC_MODE == 3) /* CAT5_BINDLESS_A1_NONUNIFORM */ ||
	(DESC_MODE == 4) /* CAT5_NONUNIFORM */;
}
static uint64_t
expr_anon_15(struct decode_scope *scope)
{
    int64_t BASE_HI = isa_decode_field(scope, "BASE_HI");
    int64_t BASE_LO = isa_decode_field(scope, "BASE_LO");
    return (BASE_HI * 2) | BASE_LO;
}
static uint64_t
expr_anon_16(struct decode_scope *scope)
{
    int64_t BINDLESS = isa_decode_field(scope, "BINDLESS");
    return BINDLESS;
}
static uint64_t
expr_anon_17(struct decode_scope *scope)
{
    int64_t S2EN_BINDLESS = isa_decode_field(scope, "S2EN_BINDLESS");
    return S2EN_BINDLESS;
}
static uint64_t
expr_anon_18(struct decode_scope *scope)
{
    int64_t W = isa_decode_field(scope, "W");
    return 2 << W;
}
static uint64_t
expr_anon_19(struct decode_scope *scope)
{
    int64_t NUM_SRC = isa_decode_field(scope, "NUM_SRC");
    return NUM_SRC > 0;
}
static uint64_t
expr_anon_20(struct decode_scope *scope)
{
    int64_t NUM_SRC = isa_decode_field(scope, "NUM_SRC");
    int64_t O = isa_decode_field(scope, "O");
    return O || (NUM_SRC > 1);
}
static uint64_t
expr_anon_21(struct decode_scope *scope)
{
    int64_t SRC2_IMM_OFFSET = isa_decode_field(scope, "SRC2_IMM_OFFSET");
    return SRC2_IMM_OFFSET;
}
static uint64_t
expr_anon_22(struct decode_scope *scope)
{
    int64_t HAS_SAMP = isa_decode_field(scope, "HAS_SAMP");
    return HAS_SAMP;
}
static uint64_t
expr_anon_23(struct decode_scope *scope)
{
    int64_t HAS_SAMP = isa_decode_field(scope, "HAS_SAMP");
    return HAS_SAMP;
}
static uint64_t
expr_anon_24(struct decode_scope *scope)
{
    int64_t HAS_TEX = isa_decode_field(scope, "HAS_TEX");
    return HAS_TEX;
}
static uint64_t
expr_anon_25(struct decode_scope *scope)
{
    int64_t HAS_TEX = isa_decode_field(scope, "HAS_TEX");
    return HAS_TEX;
}
static uint64_t
expr_anon_26(struct decode_scope *scope)
{
    int64_t HAS_TEX = isa_decode_field(scope, "HAS_TEX");
    return HAS_TEX;
}
static uint64_t
expr_anon_27(struct decode_scope *scope)
{
    int64_t HAS_TYPE = isa_decode_field(scope, "HAS_TYPE");
    return HAS_TYPE;
}
static uint64_t
expr_anon_28(struct decode_scope *scope)
{
    int64_t BINDLESS = isa_decode_field(scope, "BINDLESS");
    return !BINDLESS;
}
static uint64_t
expr___cat6_type_shift(struct decode_scope *scope)
{
    int64_t TYPE = isa_decode_field(scope, "TYPE");
    int64_t TYPE_HALF = isa_decode_field(scope, "TYPE_HALF");
    return TYPE >= 6 ? 0 /* u8 */ : (TYPE_HALF ? 1 : 2);;
}
static uint64_t
expr___cat6_full_shift(struct decode_scope *scope)
{
    int64_t SRC2_SHIFT = isa_decode_field(scope, "SRC2_SHIFT");
    int64_t TYPE_SHIFT = isa_decode_field(scope, "TYPE_SHIFT");
    return TYPE_SHIFT + SRC2_SHIFT;
}
static uint64_t
expr___cat6_d(struct decode_scope *scope)
{
    int64_t D_MINUS_ONE = isa_decode_field(scope, "D_MINUS_ONE");
    return D_MINUS_ONE + 1;
}
static uint64_t
expr___cat6_type_size(struct decode_scope *scope)
{
    int64_t TYPE_SIZE_MINUS_ONE = isa_decode_field(scope, "TYPE_SIZE_MINUS_ONE");
    return TYPE_SIZE_MINUS_ONE + 1;
}
static uint64_t
expr___cat6_load_size(struct decode_scope *scope)
{
    int64_t LOAD_SIZE_MINUS_ONE = isa_decode_field(scope, "LOAD_SIZE_MINUS_ONE");
    return LOAD_SIZE_MINUS_ONE + 1;
}
static uint64_t
expr___cat6_direct(struct decode_scope *scope)
{
    int64_t MODE = isa_decode_field(scope, "MODE");
    return MODE == 0;
}
static uint64_t
expr_anon_29(struct decode_scope *scope)
{
    int64_t TYPE_SHIFT = isa_decode_field(scope, "TYPE_SHIFT");
    return TYPE_SHIFT == 0;
}
static uint64_t
expr_anon_30(struct decode_scope *scope)
{
    int64_t OFF = isa_decode_field(scope, "OFF");
    return OFF == 0;
}
static uint64_t
expr_anon_31(struct decode_scope *scope)
{
    int64_t SRC2_SHIFT = isa_decode_field(scope, "SRC2_SHIFT");
    return SRC2_SHIFT == 0;
}
static uint64_t
expr_anon_32(struct decode_scope *scope)
{
    int64_t OFF_HI = isa_decode_field(scope, "OFF_HI");
    int64_t OFF_LO = isa_decode_field(scope, "OFF_LO");
    return (OFF_HI << 8) | OFF_LO;
}
static uint64_t
expr_anon_33(struct decode_scope *scope)
{
    int64_t TYPE_SHIFT = isa_decode_field(scope, "TYPE_SHIFT");
    return TYPE_SHIFT == 0;
}
static uint64_t
expr_anon_34(struct decode_scope *scope)
{
    int64_t OFF = isa_decode_field(scope, "OFF");
    return OFF == 0;
}
static uint64_t
expr_anon_35(struct decode_scope *scope)
{
    int64_t SRC2_SHIFT = isa_decode_field(scope, "SRC2_SHIFT");
    return SRC2_SHIFT == 0;
}
static uint64_t
expr_anon_36(struct decode_scope *scope)
{
    int64_t OFF_HI = isa_decode_field(scope, "OFF_HI");
    int64_t OFF_LO = isa_decode_field(scope, "OFF_LO");
    return (OFF_HI << 8) | OFF_LO;
}
static uint64_t
expr_anon_37(struct decode_scope *scope)
{
    int64_t DST_HI = isa_decode_field(scope, "DST_HI");
    int64_t DST_LO = isa_decode_field(scope, "DST_LO");
    return (DST_HI << 8) | DST_LO;
}
static uint64_t
expr_anon_38(struct decode_scope *scope)
{
    int64_t OFFSET_HI = isa_decode_field(scope, "OFFSET_HI");
    int64_t OFFSET_LO = isa_decode_field(scope, "OFFSET_LO");
    return (OFFSET_HI << 5) | OFFSET_LO;
}
static uint64_t
expr_anon_39(struct decode_scope *scope)
{
    int64_t HAS_OFFSET = isa_decode_field(scope, "HAS_OFFSET");
    return !HAS_OFFSET;
}
static uint64_t
expr_anon_40(struct decode_scope *scope)
{
    int64_t TYPED = isa_decode_field(scope, "TYPED");
    return TYPED;
}
static uint64_t
expr_anon_41(struct decode_scope *scope)
{
    int64_t BINDLESS = isa_decode_field(scope, "BINDLESS");
    return BINDLESS;
}
static uint64_t
expr_anon_42(struct decode_scope *scope)
{
    int64_t SRC_IM = isa_decode_field(scope, "SRC_IM");
    return SRC_IM;
}
static uint64_t
expr_anon_43(struct decode_scope *scope)
{
    int64_t SRC_CONST = isa_decode_field(scope, "SRC_CONST");
    return SRC_CONST;
}
static uint64_t
expr_anon_44(struct decode_scope *scope)
{
    int64_t TYPE = isa_decode_field(scope, "TYPE");
    int64_t TYPE_SIZE = isa_decode_field(scope, "TYPE_SIZE");
    return TYPE == 0 && TYPE_SIZE == 0 /* f16 */;
}
static uint64_t
expr_anon_45(struct decode_scope *scope)
{
    int64_t TYPE = isa_decode_field(scope, "TYPE");
    int64_t TYPE_SIZE = isa_decode_field(scope, "TYPE_SIZE");
    return TYPE == 0 && TYPE_SIZE == 1 /* f32 */;
}
static uint64_t
expr_anon_46(struct decode_scope *scope)
{
    int64_t TYPE_SIZE = isa_decode_field(scope, "TYPE_SIZE");
    return TYPE_SIZE == 0 /* b16 */;
}
static uint64_t
expr_anon_47(struct decode_scope *scope)
{
    int64_t TYPE_SIZE = isa_decode_field(scope, "TYPE_SIZE");
    return (TYPE_SIZE == 0) /* b16 */;
}
static uint64_t
expr_anon_48(struct decode_scope *scope)
{
    int64_t TYPE_SIZE = isa_decode_field(scope, "TYPE_SIZE");
    return (TYPE_SIZE == 0) /* b16 */;
}
static uint64_t
expr_anon_49(struct decode_scope *scope)
{
    int64_t SCOPE_HI = isa_decode_field(scope, "SCOPE_HI");
    int64_t SCOPE_LO = isa_decode_field(scope, "SCOPE_LO");
    return (SCOPE_HI << 1) | SCOPE_LO;
}
static uint64_t
expr_anon_50(struct decode_scope *scope)
{
    int64_t TYPE_SIZE = isa_decode_field(scope, "TYPE_SIZE");
    return TYPE_SIZE == 0;
}
static uint64_t
expr_anon_51(struct decode_scope *scope)
{
    int64_t SRC_REG_TYPE = isa_decode_field(scope, "SRC_REG_TYPE");
    return SRC_REG_TYPE == 0;
}
static uint64_t
expr_anon_52(struct decode_scope *scope)
{
    int64_t SRC_REG_TYPE = isa_decode_field(scope, "SRC_REG_TYPE");
    return SRC_REG_TYPE == 1;
}
static uint64_t
expr_anon_53(struct decode_scope *scope)
{
    int64_t SCOPE = isa_decode_field(scope, "SCOPE");
    return SCOPE == 1;
}

/* forward-declarations of bitset decode functions */
static const struct isa_field_decode decode___reg_gpr_gen_0_fields[] = {
};
static void decode___reg_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___reg_const_gen_0_fields[] = {
};
static void decode___reg_const_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___reg_relative_gpr_gen_0_fields[] = {
};
static void decode___reg_relative_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___reg_relative_const_gen_0_fields[] = {
};
static void decode___reg_relative_const_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_gen_0_fields[] = {
};
static void decode___multisrc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___mulitsrc_immed_gen_0_fields[] = {
};
static void decode___mulitsrc_immed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___mulitsrc_immed_flut_gen_0_fields[] = {
};
static void decode___mulitsrc_immed_flut_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_immed_flut_full_gen_0_fields[] = {
};
static void decode___multisrc_immed_flut_full_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_immed_flut_half_gen_0_fields[] = {
};
static void decode___multisrc_immed_flut_half_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_gpr_gen_0_fields[] = {
};
static void decode___multisrc_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_const_gen_0_fields[] = {
};
static void decode___multisrc_const_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_relative_gen_0_fields[] = {
};
static void decode___multisrc_relative_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_relative_gpr_gen_0_fields[] = {
};
static void decode___multisrc_relative_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___multisrc_relative_const_gen_0_fields[] = {
};
static void decode___multisrc_relative_const_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_gen_0_fields[] = {
};
static void decode___instruction_cat0_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_0src_gen_0_fields[] = {
};
static void decode___instruction_cat0_0src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_nop_gen_0_fields[] = {
};
static void decode_nop_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_end_gen_0_fields[] = {
};
static void decode_end_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ret_gen_0_fields[] = {
};
static void decode_ret_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_emit_gen_0_fields[] = {
};
static void decode_emit_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cut_gen_0_fields[] = {
};
static void decode_cut_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_chmask_gen_0_fields[] = {
};
static void decode_chmask_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_chsh_gen_0_fields[] = {
};
static void decode_chsh_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_flow_rev_gen_0_fields[] = {
};
static void decode_flow_rev_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shpe_gen_0_fields[] = {
};
static void decode_shpe_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_predt_gen_0_fields[] = {
};
static void decode_predt_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_predf_gen_0_fields[] = {
};
static void decode_predf_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_prede_gen_0_fields[] = {
};
static void decode_prede_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_1src_gen_0_fields[] = {
};
static void decode___instruction_cat0_1src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_kill_gen_0_fields[] = {
};
static void decode_kill_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_immed_gen_0_fields[] = {
};
static void decode___instruction_cat0_immed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_jump_gen_0_fields[] = {
};
static void decode_jump_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_call_gen_0_fields[] = {
};
static void decode_call_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_bkt_gen_0_fields[] = {
};
static void decode_bkt_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getlast_gen_600_fields[] = {
};
static void decode_getlast_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getone_gen_0_fields[] = {
};
static void decode_getone_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shps_gen_0_fields[] = {
};
static void decode_shps_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_branch_gen_0_fields[] = {
};
static void decode___instruction_cat0_branch_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_brac_gen_0_fields[] = {
};
static void decode_brac_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_brax_gen_0_fields[] = {
};
static void decode_brax_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_branch_1src_gen_0_fields[] = {
};
static void decode___instruction_cat0_branch_1src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_br_gen_0_fields[] = {
};
static void decode_br_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_bany_gen_0_fields[] = {
};
static void decode_bany_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ball_gen_0_fields[] = {
};
static void decode_ball_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat0_branch_2src_gen_0_fields[] = {
};
static void decode___instruction_cat0_branch_2src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_brao_gen_0_fields[] = {
};
static void decode_brao_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_braa_gen_0_fields[] = {
};
static void decode_braa_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_dst_gen_0_fields[] = {
};
static void decode___cat1_dst_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_gen_0_fields[] = {
};
static void decode___instruction_cat1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_typed_gen_0_fields[] = {
};
static void decode___instruction_cat1_typed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_mov_gen_0_fields[] = {
};
static void decode___instruction_cat1_mov_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_immed_src_gen_0_fields[] = {
};
static void decode___cat1_immed_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_const_src_gen_0_fields[] = {
};
static void decode___cat1_const_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_gpr_src_gen_0_fields[] = {
};
static void decode___cat1_gpr_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_relative_gpr_src_gen_0_fields[] = {
};
static void decode___cat1_relative_gpr_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_relative_const_src_gen_0_fields[] = {
};
static void decode___cat1_relative_const_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mov_immed_gen_0_fields[] = {
};
static void decode_mov_immed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mov_const_gen_0_fields[] = {
};
static void decode_mov_const_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_mov_gpr_gen_0_fields[] = {
};
static void decode___instruction_cat1_mov_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mov_gpr_gen_0_fields[] = {
};
static void decode_mov_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_movs_gen_0_fields[] = {
};
static void decode___instruction_cat1_movs_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_movs_immed_gen_0_fields[] = {
};
static void decode_movs_immed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_movs_a0_gen_0_fields[] = {
};
static void decode_movs_a0_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_relative_gen_0_fields[] = {
};
static void decode___instruction_cat1_relative_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mov_relgpr_gen_0_fields[] = {
};
static void decode_mov_relgpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mov_relconst_gen_0_fields[] = {
};
static void decode_mov_relconst_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_multi_src_gen_0_fields[] = {
};
static void decode___cat1_multi_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat1_multi_dst_gen_0_fields[] = {
};
static void decode___cat1_multi_dst_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat1_multi_gen_500_fields[] = {
};
static void decode___instruction_cat1_multi_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_swz_gen_0_fields[] = {
};
static void decode_swz_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_gat_gen_0_fields[] = {
};
static void decode_gat_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sct_gen_0_fields[] = {
};
static void decode_sct_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_movmsk_gen_0_fields[] = {
};
static void decode_movmsk_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat2_gen_0_fields[] = {
};
static void decode___instruction_cat2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat2_1src_gen_0_fields[] = {
};
static void decode___instruction_cat2_1src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat2_2src_gen_0_fields[] = {
};
static void decode___instruction_cat2_2src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat2_2src_cond_gen_0_fields[] = {
};
static void decode___instruction_cat2_2src_cond_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat2_2src_input_gen_0_fields[] = {
};
static void decode___instruction_cat2_2src_input_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_bary_f_gen_0_fields[] = {
};
static void decode_bary_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_flat_b_gen_600_fields[] = {
};
static void decode_flat_b_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_add_f_gen_0_fields[] = {
};
static void decode_add_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_min_f_gen_0_fields[] = {
};
static void decode_min_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_max_f_gen_0_fields[] = {
};
static void decode_max_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mul_f_gen_0_fields[] = {
};
static void decode_mul_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sign_f_gen_0_fields[] = {
};
static void decode_sign_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cmps_f_gen_0_fields[] = {
};
static void decode_cmps_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_absneg_f_gen_0_fields[] = {
};
static void decode_absneg_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cmpv_f_gen_0_fields[] = {
};
static void decode_cmpv_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_floor_f_gen_0_fields[] = {
};
static void decode_floor_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ceil_f_gen_0_fields[] = {
};
static void decode_ceil_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_rndne_f_gen_0_fields[] = {
};
static void decode_rndne_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_rndaz_f_gen_0_fields[] = {
};
static void decode_rndaz_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_trunc_f_gen_0_fields[] = {
};
static void decode_trunc_f_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_add_u_gen_0_fields[] = {
};
static void decode_add_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_add_s_gen_0_fields[] = {
};
static void decode_add_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sub_u_gen_0_fields[] = {
};
static void decode_sub_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sub_s_gen_0_fields[] = {
};
static void decode_sub_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cmps_u_gen_0_fields[] = {
};
static void decode_cmps_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cmps_s_gen_0_fields[] = {
};
static void decode_cmps_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_min_u_gen_0_fields[] = {
};
static void decode_min_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_min_s_gen_0_fields[] = {
};
static void decode_min_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_max_u_gen_0_fields[] = {
};
static void decode_max_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_max_s_gen_0_fields[] = {
};
static void decode_max_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_absneg_s_gen_0_fields[] = {
};
static void decode_absneg_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_and_b_gen_0_fields[] = {
};
static void decode_and_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_or_b_gen_0_fields[] = {
};
static void decode_or_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_not_b_gen_0_fields[] = {
};
static void decode_not_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_xor_b_gen_0_fields[] = {
};
static void decode_xor_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cmpv_u_gen_0_fields[] = {
};
static void decode_cmpv_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cmpv_s_gen_0_fields[] = {
};
static void decode_cmpv_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mul_u24_gen_0_fields[] = {
};
static void decode_mul_u24_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mul_s24_gen_0_fields[] = {
};
static void decode_mul_s24_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mull_u_gen_0_fields[] = {
};
static void decode_mull_u_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_bfrev_b_gen_0_fields[] = {
};
static void decode_bfrev_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_clz_s_gen_0_fields[] = {
};
static void decode_clz_s_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_clz_b_gen_0_fields[] = {
};
static void decode_clz_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shl_b_gen_0_fields[] = {
};
static void decode_shl_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shr_b_gen_0_fields[] = {
};
static void decode_shr_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ashr_b_gen_0_fields[] = {
};
static void decode_ashr_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mgen_b_gen_0_fields[] = {
};
static void decode_mgen_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getbit_b_gen_0_fields[] = {
};
static void decode_getbit_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_setrm_gen_0_fields[] = {
};
static void decode_setrm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cbits_b_gen_0_fields[] = {
};
static void decode_cbits_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shb_gen_0_fields[] = {
};
static void decode_shb_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_msad_gen_0_fields[] = {
};
static void decode_msad_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat3_src_gen_0_fields[] = {
};
static void decode___cat3_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat3_src_gpr_gen_0_fields[] = {
};
static void decode___cat3_src_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat3_src_const_or_immed_gen_0_fields[] = {
};
static void decode___cat3_src_const_or_immed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat3_src_relative_gen_0_fields[] = {
};
static void decode___cat3_src_relative_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat3_src_relative_gpr_gen_0_fields[] = {
};
static void decode___cat3_src_relative_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat3_src_relative_const_gen_0_fields[] = {
};
static void decode___cat3_src_relative_const_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat3_base_gen_0_fields[] = {
};
static void decode___instruction_cat3_base_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat3_gen_0_fields[] = {
};
static void decode___instruction_cat3_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat3_alt_gen_600_fields[] = {
};
static void decode___instruction_cat3_alt_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mad_u16_gen_0_fields[] = {
};
static void decode_mad_u16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_madsh_u16_gen_0_fields[] = {
};
static void decode_madsh_u16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mad_s16_gen_0_fields[] = {
};
static void decode_mad_s16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_madsh_m16_gen_0_fields[] = {
};
static void decode_madsh_m16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mad_u24_gen_0_fields[] = {
};
static void decode_mad_u24_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mad_s24_gen_0_fields[] = {
};
static void decode_mad_s24_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mad_f16_gen_0_fields[] = {
};
static void decode_mad_f16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_mad_f32_gen_0_fields[] = {
};
static void decode_mad_f32_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sel_b16_gen_0_fields[] = {
};
static void decode_sel_b16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sel_b32_gen_0_fields[] = {
};
static void decode_sel_b32_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sel_s16_gen_0_fields[] = {
};
static void decode_sel_s16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sel_s32_gen_0_fields[] = {
};
static void decode_sel_s32_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sel_f16_gen_0_fields[] = {
};
static void decode_sel_f16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sel_f32_gen_0_fields[] = {
};
static void decode_sel_f32_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sad_s16_gen_0_fields[] = {
};
static void decode_sad_s16_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sad_s32_gen_0_fields[] = {
};
static void decode_sad_s32_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shrm_gen_0_fields[] = {
};
static void decode_shrm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shlm_gen_0_fields[] = {
};
static void decode_shlm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shrg_gen_0_fields[] = {
};
static void decode_shrg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shlg_gen_0_fields[] = {
};
static void decode_shlg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_andg_gen_0_fields[] = {
};
static void decode_andg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat3_dp_gen_600_fields[] = {
};
static void decode___instruction_cat3_dp_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dp2acc_gen_0_fields[] = {
};
static void decode_dp2acc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dp4acc_gen_0_fields[] = {
};
static void decode_dp4acc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat3_wmm_gen_600_fields[] = {
};
static void decode___instruction_cat3_wmm_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_wmm_gen_0_fields[] = {
};
static void decode_wmm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_wmm_accu_gen_0_fields[] = {
};
static void decode_wmm_accu_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat4_gen_0_fields[] = {
};
static void decode___instruction_cat4_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_rcp_gen_0_fields[] = {
};
static void decode_rcp_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_rsq_gen_0_fields[] = {
};
static void decode_rsq_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_log2_gen_0_fields[] = {
};
static void decode_log2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_exp2_gen_0_fields[] = {
};
static void decode_exp2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sin_gen_0_fields[] = {
};
static void decode_sin_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_cos_gen_0_fields[] = {
};
static void decode_cos_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sqrt_gen_0_fields[] = {
};
static void decode_sqrt_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_hrsq_gen_0_fields[] = {
};
static void decode_hrsq_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_hlog2_gen_0_fields[] = {
};
static void decode_hlog2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_hexp2_gen_0_fields[] = {
};
static void decode_hexp2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_s2en_bindless_base_gen_0_fields[] = {
};
static void decode___cat5_s2en_bindless_base_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat5_gen_0_fields[] = {
};
static void decode___instruction_cat5_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat5_tex_base_gen_0_fields[] = {
};
static void decode___instruction_cat5_tex_base_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat5_tex_gen_0_fields[] = {
};
static void decode___instruction_cat5_tex_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_isam_gen_0_fields[] = {
};
static void decode_isam_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_isaml_gen_0_fields[] = {
};
static void decode_isaml_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_isamm_gen_0_fields[] = {
};
static void decode_isamm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sam_gen_0_fields[] = {
};
static void decode_sam_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_samb_gen_0_fields[] = {
};
static void decode_samb_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_saml_gen_0_fields[] = {
};
static void decode_saml_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_samgq_gen_0_fields[] = {
};
static void decode_samgq_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getlod_gen_0_fields[] = {
};
static void decode_getlod_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_conv_gen_0_fields[] = {
};
static void decode_conv_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_convm_gen_0_fields[] = {
};
static void decode_convm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getsize_gen_0_fields[] = {
};
static void decode_getsize_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getbuf_gen_0_fields[] = {
};
static void decode_getbuf_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getpos_gen_0_fields[] = {
};
static void decode_getpos_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getinfo_gen_0_fields[] = {
};
static void decode_getinfo_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dsx_gen_0_fields[] = {
};
static void decode_dsx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dsy_gen_0_fields[] = {
};
static void decode_dsy_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_gather4r_gen_0_fields[] = {
};
static void decode_gather4r_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_gather4g_gen_0_fields[] = {
};
static void decode_gather4g_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_gather4b_gen_0_fields[] = {
};
static void decode_gather4b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_gather4a_gen_0_fields[] = {
};
static void decode_gather4a_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_samgp0_gen_0_fields[] = {
};
static void decode_samgp0_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_samgp1_gen_0_fields[] = {
};
static void decode_samgp1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_samgp2_gen_0_fields[] = {
};
static void decode_samgp2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_samgp3_gen_0_fields[] = {
};
static void decode_samgp3_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dsxpp_1_gen_0_fields[] = {
};
static void decode_dsxpp_1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dsypp_1_gen_0_fields[] = {
};
static void decode_dsypp_1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_rgetpos_gen_0_fields[] = {
};
static void decode_rgetpos_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_rgetinfo_gen_0_fields[] = {
};
static void decode_rgetinfo_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_tcinv_gen_0_fields[] = {
};
static void decode_tcinv_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat5_brcst_gen_0_fields[] = {
};
static void decode___instruction_cat5_brcst_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_brcst_active_gen_600_fields[] = {
};
static void decode_brcst_active_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat5_quad_shuffle_gen_600_fields[] = {
};
static void decode___instruction_cat5_quad_shuffle_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_quad_shuffle_brcst_gen_0_fields[] = {
};
static void decode_quad_shuffle_brcst_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_quad_shuffle_horiz_gen_0_fields[] = {
};
static void decode_quad_shuffle_horiz_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_quad_shuffle_vert_gen_0_fields[] = {
};
static void decode_quad_shuffle_vert_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_quad_shuffle_diag_gen_0_fields[] = {
};
static void decode_quad_shuffle_diag_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_src1_gen_0_fields[] = {
};
static void decode___cat5_src1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_src2_gen_0_fields[] = {
};
static void decode___cat5_src2_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_samp_gen_0_fields[] = {
};
static void decode___cat5_samp_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_samp_s2en_bindless_a1_gen_0_fields[] = {
};
static void decode___cat5_samp_s2en_bindless_a1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_tex_s2en_bindless_a1_gen_0_fields[] = {
};
static void decode___cat5_tex_s2en_bindless_a1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_tex_gen_0_fields[] = {
};
static void decode___cat5_tex_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_tex_s2en_bindless_gen_0_fields[] = {
};
static void decode___cat5_tex_s2en_bindless_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_type_gen_0_fields[] = {
};
static void decode___cat5_type_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat5_src3_gen_0_fields[] = {
};
static void decode___cat5_src3_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___const_dst_imm_gen_0_fields[] = {
};
static void decode___const_dst_imm_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___const_dst_a1_gen_0_fields[] = {
};
static void decode___const_dst_a1_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___const_dst_gen_0_fields[] = {
};
static void decode___const_dst_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_gen_0_fields[] = {
};
static void decode___instruction_cat6_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_ldg_gen_0_fields[] = {
};
static void decode___instruction_cat6_ldg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldg_gen_0_fields[] = {
};
static void decode_ldg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldg_k_gen_600_fields[] = {
};
static void decode_ldg_k_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_stg_gen_0_fields[] = {
};
static void decode___instruction_cat6_stg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stg_gen_0_fields[] = {
};
static void decode_stg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_ld_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_ld_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldl_gen_0_fields[] = {
};
static void decode_ldl_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldp_gen_0_fields[] = {
};
static void decode_ldp_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldlw_gen_0_fields[] = {
};
static void decode_ldlw_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldlv_gen_0_fields[] = {
};
static void decode_ldlv_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_st_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_st_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stl_gen_0_fields[] = {
};
static void decode_stl_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stp_gen_0_fields[] = {
};
static void decode_stp_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stlw_gen_0_fields[] = {
};
static void decode_stlw_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stc_gen_600_fields[] = {
};
static void decode_stc_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stsc_gen_700_fields[] = {
};
static void decode_stsc_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_resinfo_gen_0_fields[] = {
};
static void decode_resinfo_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_ibo_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_ibo_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_ibo_load_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_ibo_load_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldib_gen_0_fields[] = {
};
static void decode_ldib_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_ibo_store_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_ibo_store_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_ibo_store_a4xx_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_ibo_store_a4xx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_ibo_store_a5xx_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_ibo_store_a5xx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_local_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_local_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_1src_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_1src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_2src_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_2src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_add_gen_0_fields[] = {
};
static void decode_atomic_add_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_sub_gen_0_fields[] = {
};
static void decode_atomic_sub_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_xchg_gen_0_fields[] = {
};
static void decode_atomic_xchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_inc_gen_0_fields[] = {
};
static void decode_atomic_inc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_dec_gen_0_fields[] = {
};
static void decode_atomic_dec_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_cmpxchg_gen_0_fields[] = {
};
static void decode_atomic_cmpxchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_min_gen_0_fields[] = {
};
static void decode_atomic_min_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_max_gen_0_fields[] = {
};
static void decode_atomic_max_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_and_gen_0_fields[] = {
};
static void decode_atomic_and_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_or_gen_0_fields[] = {
};
static void decode_atomic_or_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_xor_gen_0_fields[] = {
};
static void decode_atomic_xor_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_global_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_global_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_global_a4xx_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_global_a4xx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a3xx_atomic_global_a5xx_gen_0_fields[] = {
};
static void decode___instruction_cat6_a3xx_atomic_global_a5xx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_atomic_global_gen_600_fields[] = {
};
static void decode___instruction_cat6_a6xx_atomic_global_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_add_gen_0_fields[] = {
};
static void decode_atomic_g_add_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_sub_gen_0_fields[] = {
};
static void decode_atomic_g_sub_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_xchg_gen_0_fields[] = {
};
static void decode_atomic_g_xchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_inc_gen_0_fields[] = {
};
static void decode_atomic_g_inc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_dec_gen_0_fields[] = {
};
static void decode_atomic_g_dec_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_cmpxchg_gen_0_fields[] = {
};
static void decode_atomic_g_cmpxchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_min_gen_0_fields[] = {
};
static void decode_atomic_g_min_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_max_gen_0_fields[] = {
};
static void decode_atomic_g_max_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_and_gen_0_fields[] = {
};
static void decode_atomic_g_and_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_or_gen_0_fields[] = {
};
static void decode_atomic_g_or_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_g_xor_gen_0_fields[] = {
};
static void decode_atomic_g_xor_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ray_intersection_gen_700_fields[] = {
};
static void decode_ray_intersection_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_base_gen_600_fields[] = {
};
static void decode___instruction_cat6_a6xx_base_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_gen_0_fields[] = {
};
static void decode___instruction_cat6_a6xx_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat6_ldc_common_gen_0_fields[] = {
};
static void decode___cat6_ldc_common_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldc_k_gen_0_fields[] = {
};
static void decode_ldc_k_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldc_gen_0_fields[] = {
};
static void decode_ldc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getspid_gen_0_fields[] = {
};
static void decode_getspid_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getwid_gen_0_fields[] = {
};
static void decode_getwid_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_getfiberid_gen_600_fields[] = {
};
static void decode_getfiberid_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_ibo_1src_gen_0_fields[] = {
};
static void decode___instruction_cat6_a6xx_ibo_1src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_resinfo_b_gen_0_fields[] = {
};
static void decode_resinfo_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_resbase_gen_0_fields[] = {
};
static void decode_resbase_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_ibo_base_gen_0_fields[] = {
};
static void decode___instruction_cat6_a6xx_ibo_base_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_ibo_gen_0_fields[] = {
};
static void decode___instruction_cat6_a6xx_ibo_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_ibo_load_store_gen_0_fields[] = {
};
static void decode___instruction_cat6_a6xx_ibo_load_store_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat6_a6xx_ibo_atomic_gen_0_fields[] = {
};
static void decode___instruction_cat6_a6xx_ibo_atomic_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stib_b_gen_0_fields[] = {
};
static void decode_stib_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldib_b_gen_0_fields[] = {
};
static void decode_ldib_b_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_add_gen_0_fields[] = {
};
static void decode_atomic_b_add_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_sub_gen_0_fields[] = {
};
static void decode_atomic_b_sub_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_xchg_gen_0_fields[] = {
};
static void decode_atomic_b_xchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_cmpxchg_gen_0_fields[] = {
};
static void decode_atomic_b_cmpxchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_min_gen_0_fields[] = {
};
static void decode_atomic_b_min_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_max_gen_0_fields[] = {
};
static void decode_atomic_b_max_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_and_gen_0_fields[] = {
};
static void decode_atomic_b_and_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_or_gen_0_fields[] = {
};
static void decode_atomic_b_or_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_b_xor_gen_0_fields[] = {
};
static void decode_atomic_b_xor_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_shfl_gen_600_fields[] = {
};
static void decode_shfl_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat6_typed_gen_0_fields[] = {
};
static void decode___cat6_typed_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat6_base_gen_0_fields[] = {
};
static void decode___cat6_base_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat6_src_gen_0_fields[] = {
};
static void decode___cat6_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___cat6_src_const_or_gpr_gen_0_fields[] = {
};
static void decode___cat6_src_const_or_gpr_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat7_gen_0_fields[] = {
};
static void decode___instruction_cat7_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat7_barrier_gen_0_fields[] = {
};
static void decode___instruction_cat7_barrier_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_bar_gen_0_fields[] = {
};
static void decode_bar_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_fence_gen_0_fields[] = {
};
static void decode_fence_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_cat7_data_gen_0_fields[] = {
};
static void decode___instruction_cat7_data_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_sleep_gen_0_fields[] = {
};
static void decode_sleep_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_icinv_gen_0_fields[] = {
};
static void decode_icinv_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dccln_gen_0_fields[] = {
};
static void decode_dccln_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dcinv_gen_0_fields[] = {
};
static void decode_dcinv_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_dcflu_gen_0_fields[] = {
};
static void decode_dcflu_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ccinv_gen_700_fields[] = {
};
static void decode_ccinv_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_lock_gen_700_fields[] = {
};
static void decode_lock_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_unlock_gen_700_fields[] = {
};
static void decode_unlock_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___alias_immed_src_gen_0_fields[] = {
};
static void decode___alias_immed_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___alias_const_src_gen_0_fields[] = {
};
static void decode___alias_const_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___alias_gpr_src_gen_0_fields[] = {
};
static void decode___alias_gpr_src_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___dst_rt_gen_0_fields[] = {
};
static void decode___dst_rt_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_alias_gen_700_fields[] = {
};
static void decode_alias_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode___instruction_gen_300_fields[] = {
};
static void decode___instruction_gen_300(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldg_a_gen_600_fields[] = {
};
static void decode_ldg_a_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldg_a_gen_700_fields[] = {
};
static void decode_ldg_a_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stg_a_gen_600_fields[] = {
};
static void decode_stg_a_gen_600(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stg_a_gen_700_fields[] = {
};
static void decode_stg_a_gen_700(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldgb_gen_0_fields[] = {
};
static void decode_ldgb_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_ldgb_gen_500_fields[] = {
};
static void decode_ldgb_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stgb_gen_500_fields[] = {
};
static void decode_stgb_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stgb_gen_0_fields[] = {
};
static void decode_stgb_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stib_gen_500_fields[] = {
};
static void decode_stib_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_stib_gen_0_fields[] = {
};
static void decode_stib_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_add_gen_0_fields[] = {
};
static void decode_atomic_s_add_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_add_gen_500_fields[] = {
};
static void decode_atomic_s_add_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_sub_gen_0_fields[] = {
};
static void decode_atomic_s_sub_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_sub_gen_500_fields[] = {
};
static void decode_atomic_s_sub_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_xchg_gen_0_fields[] = {
};
static void decode_atomic_s_xchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_xchg_gen_500_fields[] = {
};
static void decode_atomic_s_xchg_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_inc_gen_0_fields[] = {
};
static void decode_atomic_s_inc_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_inc_gen_500_fields[] = {
};
static void decode_atomic_s_inc_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_dec_gen_0_fields[] = {
};
static void decode_atomic_s_dec_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_dec_gen_500_fields[] = {
};
static void decode_atomic_s_dec_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_cmpxchg_gen_0_fields[] = {
};
static void decode_atomic_s_cmpxchg_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_cmpxchg_gen_500_fields[] = {
};
static void decode_atomic_s_cmpxchg_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_min_gen_0_fields[] = {
};
static void decode_atomic_s_min_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_min_gen_500_fields[] = {
};
static void decode_atomic_s_min_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_max_gen_0_fields[] = {
};
static void decode_atomic_s_max_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_max_gen_500_fields[] = {
};
static void decode_atomic_s_max_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_and_gen_0_fields[] = {
};
static void decode_atomic_s_and_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_and_gen_500_fields[] = {
};
static void decode_atomic_s_and_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_or_gen_0_fields[] = {
};
static void decode_atomic_s_or_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_or_gen_500_fields[] = {
};
static void decode_atomic_s_or_gen_500(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_xor_gen_0_fields[] = {
};
static void decode_atomic_s_xor_gen_0(void *out, struct decode_scope *scope);
static const struct isa_field_decode decode_atomic_s_xor_gen_500_fields[] = {
};
static void decode_atomic_s_xor_gen_500(void *out, struct decode_scope *scope);

/*
 * Forward-declarations (so we don't have to figure out which order to
 * emit various tables when they have pointers to each other)
 */

static const struct isa_bitset bitset___reg_gpr_gen_0;
static const struct isa_bitset bitset___reg_const_gen_0;
static const struct isa_bitset bitset___reg_relative_gpr_gen_0;
static const struct isa_bitset bitset___reg_relative_const_gen_0;
static const struct isa_bitset bitset___multisrc_gen_0;
static const struct isa_bitset bitset___mulitsrc_immed_gen_0;
static const struct isa_bitset bitset___mulitsrc_immed_flut_gen_0;
static const struct isa_bitset bitset___multisrc_immed_flut_full_gen_0;
static const struct isa_bitset bitset___multisrc_immed_flut_half_gen_0;
static const struct isa_bitset bitset___multisrc_gpr_gen_0;
static const struct isa_bitset bitset___multisrc_const_gen_0;
static const struct isa_bitset bitset___multisrc_relative_gen_0;
static const struct isa_bitset bitset___multisrc_relative_gpr_gen_0;
static const struct isa_bitset bitset___multisrc_relative_const_gen_0;
static const struct isa_bitset bitset___instruction_cat0_gen_0;
static const struct isa_bitset bitset___instruction_cat0_0src_gen_0;
static const struct isa_bitset bitset_nop_gen_0;
static const struct isa_bitset bitset_end_gen_0;
static const struct isa_bitset bitset_ret_gen_0;
static const struct isa_bitset bitset_emit_gen_0;
static const struct isa_bitset bitset_cut_gen_0;
static const struct isa_bitset bitset_chmask_gen_0;
static const struct isa_bitset bitset_chsh_gen_0;
static const struct isa_bitset bitset_flow_rev_gen_0;
static const struct isa_bitset bitset_shpe_gen_0;
static const struct isa_bitset bitset_predt_gen_0;
static const struct isa_bitset bitset_predf_gen_0;
static const struct isa_bitset bitset_prede_gen_0;
static const struct isa_bitset bitset___instruction_cat0_1src_gen_0;
static const struct isa_bitset bitset_kill_gen_0;
static const struct isa_bitset bitset___instruction_cat0_immed_gen_0;
static const struct isa_bitset bitset_jump_gen_0;
static const struct isa_bitset bitset_call_gen_0;
static const struct isa_bitset bitset_bkt_gen_0;
static const struct isa_bitset bitset_getlast_gen_600;
static const struct isa_bitset bitset_getone_gen_0;
static const struct isa_bitset bitset_shps_gen_0;
static const struct isa_bitset bitset___instruction_cat0_branch_gen_0;
static const struct isa_bitset bitset_brac_gen_0;
static const struct isa_bitset bitset_brax_gen_0;
static const struct isa_bitset bitset___instruction_cat0_branch_1src_gen_0;
static const struct isa_bitset bitset_br_gen_0;
static const struct isa_bitset bitset_bany_gen_0;
static const struct isa_bitset bitset_ball_gen_0;
static const struct isa_bitset bitset___instruction_cat0_branch_2src_gen_0;
static const struct isa_bitset bitset_brao_gen_0;
static const struct isa_bitset bitset_braa_gen_0;
static const struct isa_bitset bitset___cat1_dst_gen_0;
static const struct isa_bitset bitset___instruction_cat1_gen_0;
static const struct isa_bitset bitset___instruction_cat1_typed_gen_0;
static const struct isa_bitset bitset___instruction_cat1_mov_gen_0;
static const struct isa_bitset bitset___cat1_immed_src_gen_0;
static const struct isa_bitset bitset___cat1_const_src_gen_0;
static const struct isa_bitset bitset___cat1_gpr_src_gen_0;
static const struct isa_bitset bitset___cat1_relative_gpr_src_gen_0;
static const struct isa_bitset bitset___cat1_relative_const_src_gen_0;
static const struct isa_bitset bitset_mov_immed_gen_0;
static const struct isa_bitset bitset_mov_const_gen_0;
static const struct isa_bitset bitset___instruction_cat1_mov_gpr_gen_0;
static const struct isa_bitset bitset_mov_gpr_gen_0;
static const struct isa_bitset bitset___instruction_cat1_movs_gen_0;
static const struct isa_bitset bitset_movs_immed_gen_0;
static const struct isa_bitset bitset_movs_a0_gen_0;
static const struct isa_bitset bitset___instruction_cat1_relative_gen_0;
static const struct isa_bitset bitset_mov_relgpr_gen_0;
static const struct isa_bitset bitset_mov_relconst_gen_0;
static const struct isa_bitset bitset___cat1_multi_src_gen_0;
static const struct isa_bitset bitset___cat1_multi_dst_gen_0;
static const struct isa_bitset bitset___instruction_cat1_multi_gen_500;
static const struct isa_bitset bitset_swz_gen_0;
static const struct isa_bitset bitset_gat_gen_0;
static const struct isa_bitset bitset_sct_gen_0;
static const struct isa_bitset bitset_movmsk_gen_0;
static const struct isa_bitset bitset___instruction_cat2_gen_0;
static const struct isa_bitset bitset___instruction_cat2_1src_gen_0;
static const struct isa_bitset bitset___instruction_cat2_2src_gen_0;
static const struct isa_bitset bitset___instruction_cat2_2src_cond_gen_0;
static const struct isa_bitset bitset___instruction_cat2_2src_input_gen_0;
static const struct isa_bitset bitset_bary_f_gen_0;
static const struct isa_bitset bitset_flat_b_gen_600;
static const struct isa_bitset bitset_add_f_gen_0;
static const struct isa_bitset bitset_min_f_gen_0;
static const struct isa_bitset bitset_max_f_gen_0;
static const struct isa_bitset bitset_mul_f_gen_0;
static const struct isa_bitset bitset_sign_f_gen_0;
static const struct isa_bitset bitset_cmps_f_gen_0;
static const struct isa_bitset bitset_absneg_f_gen_0;
static const struct isa_bitset bitset_cmpv_f_gen_0;
static const struct isa_bitset bitset_floor_f_gen_0;
static const struct isa_bitset bitset_ceil_f_gen_0;
static const struct isa_bitset bitset_rndne_f_gen_0;
static const struct isa_bitset bitset_rndaz_f_gen_0;
static const struct isa_bitset bitset_trunc_f_gen_0;
static const struct isa_bitset bitset_add_u_gen_0;
static const struct isa_bitset bitset_add_s_gen_0;
static const struct isa_bitset bitset_sub_u_gen_0;
static const struct isa_bitset bitset_sub_s_gen_0;
static const struct isa_bitset bitset_cmps_u_gen_0;
static const struct isa_bitset bitset_cmps_s_gen_0;
static const struct isa_bitset bitset_min_u_gen_0;
static const struct isa_bitset bitset_min_s_gen_0;
static const struct isa_bitset bitset_max_u_gen_0;
static const struct isa_bitset bitset_max_s_gen_0;
static const struct isa_bitset bitset_absneg_s_gen_0;
static const struct isa_bitset bitset_and_b_gen_0;
static const struct isa_bitset bitset_or_b_gen_0;
static const struct isa_bitset bitset_not_b_gen_0;
static const struct isa_bitset bitset_xor_b_gen_0;
static const struct isa_bitset bitset_cmpv_u_gen_0;
static const struct isa_bitset bitset_cmpv_s_gen_0;
static const struct isa_bitset bitset_mul_u24_gen_0;
static const struct isa_bitset bitset_mul_s24_gen_0;
static const struct isa_bitset bitset_mull_u_gen_0;
static const struct isa_bitset bitset_bfrev_b_gen_0;
static const struct isa_bitset bitset_clz_s_gen_0;
static const struct isa_bitset bitset_clz_b_gen_0;
static const struct isa_bitset bitset_shl_b_gen_0;
static const struct isa_bitset bitset_shr_b_gen_0;
static const struct isa_bitset bitset_ashr_b_gen_0;
static const struct isa_bitset bitset_mgen_b_gen_0;
static const struct isa_bitset bitset_getbit_b_gen_0;
static const struct isa_bitset bitset_setrm_gen_0;
static const struct isa_bitset bitset_cbits_b_gen_0;
static const struct isa_bitset bitset_shb_gen_0;
static const struct isa_bitset bitset_msad_gen_0;
static const struct isa_bitset bitset___cat3_src_gen_0;
static const struct isa_bitset bitset___cat3_src_gpr_gen_0;
static const struct isa_bitset bitset___cat3_src_const_or_immed_gen_0;
static const struct isa_bitset bitset___cat3_src_relative_gen_0;
static const struct isa_bitset bitset___cat3_src_relative_gpr_gen_0;
static const struct isa_bitset bitset___cat3_src_relative_const_gen_0;
static const struct isa_bitset bitset___instruction_cat3_base_gen_0;
static const struct isa_bitset bitset___instruction_cat3_gen_0;
static const struct isa_bitset bitset___instruction_cat3_alt_gen_600;
static const struct isa_bitset bitset_mad_u16_gen_0;
static const struct isa_bitset bitset_madsh_u16_gen_0;
static const struct isa_bitset bitset_mad_s16_gen_0;
static const struct isa_bitset bitset_madsh_m16_gen_0;
static const struct isa_bitset bitset_mad_u24_gen_0;
static const struct isa_bitset bitset_mad_s24_gen_0;
static const struct isa_bitset bitset_mad_f16_gen_0;
static const struct isa_bitset bitset_mad_f32_gen_0;
static const struct isa_bitset bitset_sel_b16_gen_0;
static const struct isa_bitset bitset_sel_b32_gen_0;
static const struct isa_bitset bitset_sel_s16_gen_0;
static const struct isa_bitset bitset_sel_s32_gen_0;
static const struct isa_bitset bitset_sel_f16_gen_0;
static const struct isa_bitset bitset_sel_f32_gen_0;
static const struct isa_bitset bitset_sad_s16_gen_0;
static const struct isa_bitset bitset_sad_s32_gen_0;
static const struct isa_bitset bitset_shrm_gen_0;
static const struct isa_bitset bitset_shlm_gen_0;
static const struct isa_bitset bitset_shrg_gen_0;
static const struct isa_bitset bitset_shlg_gen_0;
static const struct isa_bitset bitset_andg_gen_0;
static const struct isa_bitset bitset___instruction_cat3_dp_gen_600;
static const struct isa_bitset bitset_dp2acc_gen_0;
static const struct isa_bitset bitset_dp4acc_gen_0;
static const struct isa_bitset bitset___instruction_cat3_wmm_gen_600;
static const struct isa_bitset bitset_wmm_gen_0;
static const struct isa_bitset bitset_wmm_accu_gen_0;
static const struct isa_bitset bitset___instruction_cat4_gen_0;
static const struct isa_bitset bitset_rcp_gen_0;
static const struct isa_bitset bitset_rsq_gen_0;
static const struct isa_bitset bitset_log2_gen_0;
static const struct isa_bitset bitset_exp2_gen_0;
static const struct isa_bitset bitset_sin_gen_0;
static const struct isa_bitset bitset_cos_gen_0;
static const struct isa_bitset bitset_sqrt_gen_0;
static const struct isa_bitset bitset_hrsq_gen_0;
static const struct isa_bitset bitset_hlog2_gen_0;
static const struct isa_bitset bitset_hexp2_gen_0;
static const struct isa_bitset bitset___cat5_s2en_bindless_base_gen_0;
static const struct isa_bitset bitset___instruction_cat5_gen_0;
static const struct isa_bitset bitset___instruction_cat5_tex_base_gen_0;
static const struct isa_bitset bitset___instruction_cat5_tex_gen_0;
static const struct isa_bitset bitset_isam_gen_0;
static const struct isa_bitset bitset_isaml_gen_0;
static const struct isa_bitset bitset_isamm_gen_0;
static const struct isa_bitset bitset_sam_gen_0;
static const struct isa_bitset bitset_samb_gen_0;
static const struct isa_bitset bitset_saml_gen_0;
static const struct isa_bitset bitset_samgq_gen_0;
static const struct isa_bitset bitset_getlod_gen_0;
static const struct isa_bitset bitset_conv_gen_0;
static const struct isa_bitset bitset_convm_gen_0;
static const struct isa_bitset bitset_getsize_gen_0;
static const struct isa_bitset bitset_getbuf_gen_0;
static const struct isa_bitset bitset_getpos_gen_0;
static const struct isa_bitset bitset_getinfo_gen_0;
static const struct isa_bitset bitset_dsx_gen_0;
static const struct isa_bitset bitset_dsy_gen_0;
static const struct isa_bitset bitset_gather4r_gen_0;
static const struct isa_bitset bitset_gather4g_gen_0;
static const struct isa_bitset bitset_gather4b_gen_0;
static const struct isa_bitset bitset_gather4a_gen_0;
static const struct isa_bitset bitset_samgp0_gen_0;
static const struct isa_bitset bitset_samgp1_gen_0;
static const struct isa_bitset bitset_samgp2_gen_0;
static const struct isa_bitset bitset_samgp3_gen_0;
static const struct isa_bitset bitset_dsxpp_1_gen_0;
static const struct isa_bitset bitset_dsypp_1_gen_0;
static const struct isa_bitset bitset_rgetpos_gen_0;
static const struct isa_bitset bitset_rgetinfo_gen_0;
static const struct isa_bitset bitset_tcinv_gen_0;
static const struct isa_bitset bitset___instruction_cat5_brcst_gen_0;
static const struct isa_bitset bitset_brcst_active_gen_600;
static const struct isa_bitset bitset___instruction_cat5_quad_shuffle_gen_600;
static const struct isa_bitset bitset_quad_shuffle_brcst_gen_0;
static const struct isa_bitset bitset_quad_shuffle_horiz_gen_0;
static const struct isa_bitset bitset_quad_shuffle_vert_gen_0;
static const struct isa_bitset bitset_quad_shuffle_diag_gen_0;
static const struct isa_bitset bitset___cat5_src1_gen_0;
static const struct isa_bitset bitset___cat5_src2_gen_0;
static const struct isa_bitset bitset___cat5_samp_gen_0;
static const struct isa_bitset bitset___cat5_samp_s2en_bindless_a1_gen_0;
static const struct isa_bitset bitset___cat5_tex_s2en_bindless_a1_gen_0;
static const struct isa_bitset bitset___cat5_tex_gen_0;
static const struct isa_bitset bitset___cat5_tex_s2en_bindless_gen_0;
static const struct isa_bitset bitset___cat5_type_gen_0;
static const struct isa_bitset bitset___cat5_src3_gen_0;
static const struct isa_bitset bitset___const_dst_imm_gen_0;
static const struct isa_bitset bitset___const_dst_a1_gen_0;
static const struct isa_bitset bitset___const_dst_gen_0;
static const struct isa_bitset bitset___instruction_cat6_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_gen_0;
static const struct isa_bitset bitset___instruction_cat6_ldg_gen_0;
static const struct isa_bitset bitset_ldg_gen_0;
static const struct isa_bitset bitset_ldg_k_gen_600;
static const struct isa_bitset bitset___instruction_cat6_stg_gen_0;
static const struct isa_bitset bitset_stg_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_ld_gen_0;
static const struct isa_bitset bitset_ldl_gen_0;
static const struct isa_bitset bitset_ldp_gen_0;
static const struct isa_bitset bitset_ldlw_gen_0;
static const struct isa_bitset bitset_ldlv_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_st_gen_0;
static const struct isa_bitset bitset_stl_gen_0;
static const struct isa_bitset bitset_stp_gen_0;
static const struct isa_bitset bitset_stlw_gen_0;
static const struct isa_bitset bitset_stc_gen_600;
static const struct isa_bitset bitset_stsc_gen_700;
static const struct isa_bitset bitset_resinfo_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_load_gen_0;
static const struct isa_bitset bitset_ldib_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_store_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_store_a4xx_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_store_a5xx_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_local_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_1src_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_2src_gen_0;
static const struct isa_bitset bitset_atomic_add_gen_0;
static const struct isa_bitset bitset_atomic_sub_gen_0;
static const struct isa_bitset bitset_atomic_xchg_gen_0;
static const struct isa_bitset bitset_atomic_inc_gen_0;
static const struct isa_bitset bitset_atomic_dec_gen_0;
static const struct isa_bitset bitset_atomic_cmpxchg_gen_0;
static const struct isa_bitset bitset_atomic_min_gen_0;
static const struct isa_bitset bitset_atomic_max_gen_0;
static const struct isa_bitset bitset_atomic_and_gen_0;
static const struct isa_bitset bitset_atomic_or_gen_0;
static const struct isa_bitset bitset_atomic_xor_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_global_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a6xx_atomic_global_gen_600;
static const struct isa_bitset bitset_atomic_g_add_gen_0;
static const struct isa_bitset bitset_atomic_g_sub_gen_0;
static const struct isa_bitset bitset_atomic_g_xchg_gen_0;
static const struct isa_bitset bitset_atomic_g_inc_gen_0;
static const struct isa_bitset bitset_atomic_g_dec_gen_0;
static const struct isa_bitset bitset_atomic_g_cmpxchg_gen_0;
static const struct isa_bitset bitset_atomic_g_min_gen_0;
static const struct isa_bitset bitset_atomic_g_max_gen_0;
static const struct isa_bitset bitset_atomic_g_and_gen_0;
static const struct isa_bitset bitset_atomic_g_or_gen_0;
static const struct isa_bitset bitset_atomic_g_xor_gen_0;
static const struct isa_bitset bitset_ray_intersection_gen_700;
static const struct isa_bitset bitset___instruction_cat6_a6xx_base_gen_600;
static const struct isa_bitset bitset___instruction_cat6_a6xx_gen_0;
static const struct isa_bitset bitset___cat6_ldc_common_gen_0;
static const struct isa_bitset bitset_ldc_k_gen_0;
static const struct isa_bitset bitset_ldc_gen_0;
static const struct isa_bitset bitset_getspid_gen_0;
static const struct isa_bitset bitset_getwid_gen_0;
static const struct isa_bitset bitset_getfiberid_gen_600;
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_1src_gen_0;
static const struct isa_bitset bitset_resinfo_b_gen_0;
static const struct isa_bitset bitset_resbase_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_base_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_load_store_gen_0;
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_atomic_gen_0;
static const struct isa_bitset bitset_stib_b_gen_0;
static const struct isa_bitset bitset_ldib_b_gen_0;
static const struct isa_bitset bitset_atomic_b_add_gen_0;
static const struct isa_bitset bitset_atomic_b_sub_gen_0;
static const struct isa_bitset bitset_atomic_b_xchg_gen_0;
static const struct isa_bitset bitset_atomic_b_cmpxchg_gen_0;
static const struct isa_bitset bitset_atomic_b_min_gen_0;
static const struct isa_bitset bitset_atomic_b_max_gen_0;
static const struct isa_bitset bitset_atomic_b_and_gen_0;
static const struct isa_bitset bitset_atomic_b_or_gen_0;
static const struct isa_bitset bitset_atomic_b_xor_gen_0;
static const struct isa_bitset bitset_shfl_gen_600;
static const struct isa_bitset bitset___cat6_typed_gen_0;
static const struct isa_bitset bitset___cat6_base_gen_0;
static const struct isa_bitset bitset___cat6_src_gen_0;
static const struct isa_bitset bitset___cat6_src_const_or_gpr_gen_0;
static const struct isa_bitset bitset___instruction_cat7_gen_0;
static const struct isa_bitset bitset___instruction_cat7_barrier_gen_0;
static const struct isa_bitset bitset_bar_gen_0;
static const struct isa_bitset bitset_fence_gen_0;
static const struct isa_bitset bitset___instruction_cat7_data_gen_0;
static const struct isa_bitset bitset_sleep_gen_0;
static const struct isa_bitset bitset_icinv_gen_0;
static const struct isa_bitset bitset_dccln_gen_0;
static const struct isa_bitset bitset_dcinv_gen_0;
static const struct isa_bitset bitset_dcflu_gen_0;
static const struct isa_bitset bitset_ccinv_gen_700;
static const struct isa_bitset bitset_lock_gen_700;
static const struct isa_bitset bitset_unlock_gen_700;
static const struct isa_bitset bitset___alias_immed_src_gen_0;
static const struct isa_bitset bitset___alias_const_src_gen_0;
static const struct isa_bitset bitset___alias_gpr_src_gen_0;
static const struct isa_bitset bitset___dst_rt_gen_0;
static const struct isa_bitset bitset_alias_gen_700;
static const struct isa_bitset bitset___instruction_gen_300;
static const struct isa_bitset bitset_ldg_a_gen_600;
static const struct isa_bitset bitset_ldg_a_gen_700;
static const struct isa_bitset bitset_stg_a_gen_600;
static const struct isa_bitset bitset_stg_a_gen_700;
static const struct isa_bitset bitset_ldgb_gen_0;
static const struct isa_bitset bitset_ldgb_gen_500;
static const struct isa_bitset bitset_stgb_gen_500;
static const struct isa_bitset bitset_stgb_gen_0;
static const struct isa_bitset bitset_stib_gen_500;
static const struct isa_bitset bitset_stib_gen_0;
static const struct isa_bitset bitset_atomic_s_add_gen_0;
static const struct isa_bitset bitset_atomic_s_add_gen_500;
static const struct isa_bitset bitset_atomic_s_sub_gen_0;
static const struct isa_bitset bitset_atomic_s_sub_gen_500;
static const struct isa_bitset bitset_atomic_s_xchg_gen_0;
static const struct isa_bitset bitset_atomic_s_xchg_gen_500;
static const struct isa_bitset bitset_atomic_s_inc_gen_0;
static const struct isa_bitset bitset_atomic_s_inc_gen_500;
static const struct isa_bitset bitset_atomic_s_dec_gen_0;
static const struct isa_bitset bitset_atomic_s_dec_gen_500;
static const struct isa_bitset bitset_atomic_s_cmpxchg_gen_0;
static const struct isa_bitset bitset_atomic_s_cmpxchg_gen_500;
static const struct isa_bitset bitset_atomic_s_min_gen_0;
static const struct isa_bitset bitset_atomic_s_min_gen_500;
static const struct isa_bitset bitset_atomic_s_max_gen_0;
static const struct isa_bitset bitset_atomic_s_max_gen_500;
static const struct isa_bitset bitset_atomic_s_and_gen_0;
static const struct isa_bitset bitset_atomic_s_and_gen_500;
static const struct isa_bitset bitset_atomic_s_or_gen_0;
static const struct isa_bitset bitset_atomic_s_or_gen_500;
static const struct isa_bitset bitset_atomic_s_xor_gen_0;
static const struct isa_bitset bitset_atomic_s_xor_gen_500;

static const struct isa_bitset *__reg_gpr[];
static const struct isa_bitset *__reg_const[];
static const struct isa_bitset *__reg_relative_gpr[];
static const struct isa_bitset *__reg_relative_const[];
static const struct isa_bitset *__multisrc[];
static const struct isa_bitset *__cat1_dst[];
static const struct isa_bitset *__cat1_immed_src[];
static const struct isa_bitset *__cat1_const_src[];
static const struct isa_bitset *__cat1_gpr_src[];
static const struct isa_bitset *__cat1_relative_gpr_src[];
static const struct isa_bitset *__cat1_relative_const_src[];
static const struct isa_bitset *__cat1_multi_src[];
static const struct isa_bitset *__cat1_multi_dst[];
static const struct isa_bitset *__cat3_src[];
static const struct isa_bitset *__cat5_s2en_bindless_base[];
static const struct isa_bitset *__cat5_src1[];
static const struct isa_bitset *__cat5_src2[];
static const struct isa_bitset *__cat5_samp[];
static const struct isa_bitset *__cat5_samp_s2en_bindless_a1[];
static const struct isa_bitset *__cat5_tex_s2en_bindless_a1[];
static const struct isa_bitset *__cat5_tex[];
static const struct isa_bitset *__cat5_tex_s2en_bindless[];
static const struct isa_bitset *__cat5_type[];
static const struct isa_bitset *__cat5_src3[];
static const struct isa_bitset *__const_dst[];
static const struct isa_bitset *__cat6_typed[];
static const struct isa_bitset *__cat6_base[];
static const struct isa_bitset *__cat6_src[];
static const struct isa_bitset *__cat6_src_const_or_gpr[];
static const struct isa_bitset *__alias_immed_src[];
static const struct isa_bitset *__alias_const_src[];
static const struct isa_bitset *__alias_gpr_src[];
static const struct isa_bitset *__dst_rt[];
static const struct isa_bitset *__instruction[];

/*
 * bitset tables:
 */

static const struct isa_case __reg_gpr__case0_gen_0 = {
       .expr     = &expr___reg_gpr_a0,
       .display  = "a0.{SWIZ}",
       .num_fields = 1,
       .fields   = {
          { .name = "#reg-gpr#assert0", .low = 2, .high = 7,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x3d, 0x0 },
          },
       },
};
static const struct isa_case __reg_gpr__case1_gen_0 = {
       .expr     = &expr___reg_gpr_p0,
       .display  = "p0.{SWIZ}",
       .num_fields = 1,
       .fields   = {
          { .name = "#reg-gpr#assert0", .low = 2, .high = 7,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x3e, 0x0 },
          },
       },
};
static const struct isa_case __reg_gpr__default_gen_0 = {
       .display  = "r{GPR}.{SWIZ}",
       .num_fields = 2,
       .fields   = {
          { .name = "SWIZ", .low = 0, .high = 1,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "GPR", .low = 2, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___reg_gpr_gen_0 = {

       .name     = "#reg-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___reg_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___reg_gpr_gen_0_fields),
       .decode_fields = decode___reg_gpr_gen_0_fields,
       .num_cases = 3,
       .cases    = {
            &__reg_gpr__case0_gen_0,
            &__reg_gpr__case1_gen_0,
            &__reg_gpr__default_gen_0,
       },
};
static const struct isa_case __reg_const__default_gen_0 = {
       .display  = "c{CONST}.{SWIZ}",
       .num_fields = 2,
       .fields   = {
          { .name = "SWIZ", .low = 0, .high = 1,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "CONST", .low = 2, .high = 10,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___reg_const_gen_0 = {

       .name     = "#reg-const",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___reg_const_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___reg_const_gen_0_fields),
       .decode_fields = decode___reg_const_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__reg_const__default_gen_0,
       },
};
static const struct isa_case __reg_relative_gpr__case0_gen_0 = {
       .expr     = &expr___offset_zero,
       .display  = "r<a0.x>",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __reg_relative_gpr__default_gen_0 = {
       .display  = "r<a0.x + {OFFSET}>",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 9,
            .type = TYPE_INT,
          },
       },
};
static const struct isa_bitset bitset___reg_relative_gpr_gen_0 = {

       .name     = "#reg-relative-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___reg_relative_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___reg_relative_gpr_gen_0_fields),
       .decode_fields = decode___reg_relative_gpr_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__reg_relative_gpr__case0_gen_0,
            &__reg_relative_gpr__default_gen_0,
       },
};
static const struct isa_case __reg_relative_const__case0_gen_0 = {
       .expr     = &expr___offset_zero,
       .display  = "c<a0.x>",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __reg_relative_const__default_gen_0 = {
       .display  = "c<a0.x + {OFFSET}>",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 9,
            .type = TYPE_INT,
          },
       },
};
static const struct isa_bitset bitset___reg_relative_const_gen_0 = {

       .name     = "#reg-relative-const",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___reg_relative_const_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___reg_relative_const_gen_0_fields),
       .decode_fields = decode___reg_relative_const_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__reg_relative_const__case0_gen_0,
            &__reg_relative_const__default_gen_0,
       },
};
static const struct isa_case __multisrc__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___multisrc_gen_0 = {

       .name     = "#multisrc",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___multisrc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_gen_0_fields),
       .decode_fields = decode___multisrc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc__default_gen_0,
       },
};
static const struct isa_case __mulitsrc_immed__case0_gen_0 = {
       .expr     = &expr___multisrc_half,
       .display  = "{ABSNEG}{SRC_R}h({IMMED})",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __mulitsrc_immed__default_gen_0 = {
       .display  = "{ABSNEG}{SRC_R}{IMMED}",
       .num_fields = 2,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 10,
            .type = TYPE_INT,
          },
          { .name = "ABSNEG", .low = 14, .high = 15,
            .type = TYPE_ENUM,
            .enums = &enum___absneg,
          },
       },
};
static const struct isa_bitset bitset___mulitsrc_immed_gen_0 = {

       .parent   = &bitset___multisrc_gen_0,
       .name     = "#mulitsrc-immed",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3800, 0x0 },
       .decode = decode___mulitsrc_immed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___mulitsrc_immed_gen_0_fields),
       .decode_fields = decode___mulitsrc_immed_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__mulitsrc_immed__case0_gen_0,
            &__mulitsrc_immed__default_gen_0,
       },
};
static const struct isa_case __mulitsrc_immed_flut__default_gen_0 = {
       .num_fields = 2,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 9,
            .type = TYPE_ENUM,
            .enums = &enum___flut,
          },
          { .name = "ABSNEG", .low = 14, .high = 15,
            .type = TYPE_ENUM,
            .enums = &enum___absneg,
          },
       },
};
static const struct isa_bitset bitset___mulitsrc_immed_flut_gen_0 = {

       .parent   = &bitset___multisrc_gen_0,
       .name     = "#mulitsrc-immed-flut",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2800, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3800, 0x0 },
       .decode = decode___mulitsrc_immed_flut_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___mulitsrc_immed_flut_gen_0_fields),
       .decode_fields = decode___mulitsrc_immed_flut_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__mulitsrc_immed_flut__default_gen_0,
       },
};
static const struct isa_case __multisrc_immed_flut_full__default_gen_0 = {
       .display  = "{ABSNEG}{SRC_R}{IMMED}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___multisrc_immed_flut_full_gen_0 = {

       .parent   = &bitset___mulitsrc_immed_flut_gen_0,
       .name     = "#multisrc-immed-flut-full",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2800, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3c00, 0x0 },
       .decode = decode___multisrc_immed_flut_full_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_immed_flut_full_gen_0_fields),
       .decode_fields = decode___multisrc_immed_flut_full_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_immed_flut_full__default_gen_0,
       },
};
static const struct isa_case __multisrc_immed_flut_half__default_gen_0 = {
       .display  = "{ABSNEG}{SRC_R}h{IMMED}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___multisrc_immed_flut_half_gen_0 = {

       .parent   = &bitset___mulitsrc_immed_flut_gen_0,
       .name     = "#multisrc-immed-flut-half",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2c00, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3c00, 0x0 },
       .decode = decode___multisrc_immed_flut_half_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_immed_flut_half_gen_0_fields),
       .decode_fields = decode___multisrc_immed_flut_half_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_immed_flut_half__default_gen_0,
       },
};
static const struct isa_case __multisrc_gpr__default_gen_0 = {
       .display  = "{LAST}{ABSNEG}{SRC_R}{HALF}{SRC}",
       .num_fields = 4,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr___multisrc_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "LAST", .low = 10, .high = 10,
            .display = "(last)",
            .type = TYPE_BOOL,
          },
          { .name = "ABSNEG", .low = 14, .high = 15,
            .type = TYPE_ENUM,
            .enums = &enum___absneg,
          },
       },
};
static const struct isa_bitset bitset___multisrc_gpr_gen_0 = {

       .parent   = &bitset___multisrc_gen_0,
       .name     = "#multisrc-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3b00, 0x0 },
       .decode = decode___multisrc_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_gpr_gen_0_fields),
       .decode_fields = decode___multisrc_gpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_gpr__default_gen_0,
       },
};
static const struct isa_case __multisrc_const__default_gen_0 = {
       .display  = "{ABSNEG}{SRC_R}{HALF}{SRC}",
       .num_fields = 3,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr___multisrc_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 10,
            .type = TYPE_BITSET,
            .bitsets = __reg_const,
          },
          { .name = "ABSNEG", .low = 14, .high = 15,
            .type = TYPE_ENUM,
            .enums = &enum___absneg,
          },
       },
};
static const struct isa_bitset bitset___multisrc_const_gen_0 = {

       .parent   = &bitset___multisrc_gen_0,
       .name     = "#multisrc-const",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1000, 0x0 },
       .dontcare.bitset = { 0x2000, 0x0 },
       .mask.bitset     = { 0x3800, 0x0 },
       .decode = decode___multisrc_const_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_const_gen_0_fields),
       .decode_fields = decode___multisrc_const_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_const__default_gen_0,
       },
};
static const struct isa_case __multisrc_relative__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "ABSNEG", .low = 14, .high = 15,
            .type = TYPE_ENUM,
            .enums = &enum___absneg,
          },
       },
};
static const struct isa_bitset bitset___multisrc_relative_gen_0 = {

       .parent   = &bitset___multisrc_gen_0,
       .name     = "#multisrc-relative",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3800, 0x0 },
       .decode = decode___multisrc_relative_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_relative_gen_0_fields),
       .decode_fields = decode___multisrc_relative_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_relative__default_gen_0,
       },
};
static const struct isa_case __multisrc_relative_gpr__default_gen_0 = {
       .display  = "{ABSNEG}{SRC_R}{HALF}{SRC}",
       .num_fields = 2,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr___multisrc_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 9,
            .type = TYPE_BITSET,
            .bitsets = __reg_relative_gpr,
          },
       },
};
static const struct isa_bitset bitset___multisrc_relative_gpr_gen_0 = {

       .parent   = &bitset___multisrc_relative_gen_0,
       .name     = "#multisrc-relative-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3c00, 0x0 },
       .decode = decode___multisrc_relative_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_relative_gpr_gen_0_fields),
       .decode_fields = decode___multisrc_relative_gpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_relative_gpr__default_gen_0,
       },
};
static const struct isa_case __multisrc_relative_const__default_gen_0 = {
       .display  = "{ABSNEG}{SRC_R}{HALF}{SRC}",
       .num_fields = 2,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr___multisrc_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 9,
            .type = TYPE_BITSET,
            .bitsets = __reg_relative_const,
          },
       },
};
static const struct isa_bitset bitset___multisrc_relative_const_gen_0 = {

       .parent   = &bitset___multisrc_relative_gen_0,
       .name     = "#multisrc-relative-const",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc00, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x3c00, 0x0 },
       .decode = decode___multisrc_relative_const_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___multisrc_relative_const_gen_0_fields),
       .decode_fields = decode___multisrc_relative_const_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__multisrc_relative_const__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0__default_gen_0 = {
       .num_fields = 6,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_BRANCH,
            .call = false,
          },
          { .name = "REPEAT", .low = 40, .high = 42,
            .type = TYPE_ENUM,
            .enums = &enum___rptn,
          },
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "EQ", .low = 48, .high = 48,
            .display = "(eq)",
            .type = TYPE_BOOL,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat0_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat0",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x800 },
       .mask.bitset     = { 0x0, 0xe0000800 },
       .decode = decode___instruction_cat0_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_gen_0_fields),
       .decode_fields = decode___instruction_cat0_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0_0src__default_gen_0 = {
       .display  = "{SY}{SS}{EQ}{JP}{REPEAT}{NAME}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat0_0src_gen_0 = {

       .parent   = &bitset___instruction_cat0_gen_0,
       .name     = "#instruction-cat0-0src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x81f },
       .mask.bitset     = { 0x0, 0xe070e8ff },
       .decode = decode___instruction_cat0_0src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_0src_gen_0_fields),
       .decode_fields = decode___instruction_cat0_0src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0_0src__default_gen_0,
       },
};
static const struct isa_case nop__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_nop_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "nop",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_nop_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_nop_gen_0_fields),
       .decode_fields = decode_nop_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &nop__default_gen_0,
       },
};
static const struct isa_case end__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_end_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "end",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x3000000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_end_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_end_gen_0_fields),
       .decode_fields = decode_end_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &end__default_gen_0,
       },
};
static const struct isa_case ret__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ret_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "ret",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x2000000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_ret_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ret_gen_0_fields),
       .decode_fields = decode_ret_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ret__default_gen_0,
       },
};
static const struct isa_case emit__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_emit_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "emit",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x3800000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_emit_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_emit_gen_0_fields),
       .decode_fields = decode_emit_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &emit__default_gen_0,
       },
};
static const struct isa_case cut__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cut_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "cut",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x4000000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_cut_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cut_gen_0_fields),
       .decode_fields = decode_cut_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cut__default_gen_0,
       },
};
static const struct isa_case chmask__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_chmask_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "chmask",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x4800000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_chmask_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_chmask_gen_0_fields),
       .decode_fields = decode_chmask_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &chmask__default_gen_0,
       },
};
static const struct isa_case chsh__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_chsh_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "chsh",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x5000000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_chsh_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_chsh_gen_0_fields),
       .decode_fields = decode_chsh_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &chsh__default_gen_0,
       },
};
static const struct isa_case flow_rev__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_flow_rev_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "flow_rev",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x5800000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_flow_rev_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_flow_rev_gen_0_fields),
       .decode_fields = decode_flow_rev_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &flow_rev__default_gen_0,
       },
};
static const struct isa_case shpe__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shpe_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "shpe",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x4020000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_shpe_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shpe_gen_0_fields),
       .decode_fields = decode_shpe_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shpe__default_gen_0,
       },
};
static const struct isa_case predt__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_predt_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "predt",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x6820000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_predt_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_predt_gen_0_fields),
       .decode_fields = decode_predt_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &predt__default_gen_0,
       },
};
static const struct isa_case predf__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_predf_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "predf",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x7020000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_predf_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_predf_gen_0_fields),
       .decode_fields = decode_predf_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &predf__default_gen_0,
       },
};
static const struct isa_case prede__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_prede_gen_0 = {

       .parent   = &bitset___instruction_cat0_0src_gen_0,
       .name     = "prede",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x7820000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_prede_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_prede_gen_0_fields),
       .decode_fields = decode_prede_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &prede__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0_1src__default_gen_0 = {
       .display  = "{SY}{SS}{EQ}{JP}{NAME} {INV1}p0.{COMP1}",
       .num_fields = 2,
       .fields   = {
          { .name = "INV1", .low = 52, .high = 52,
            .display = "!",
            .type = TYPE_BOOL,
          },
          { .name = "COMP1", .low = 53, .high = 54,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat0_1src_gen_0 = {

       .parent   = &bitset___instruction_cat0_gen_0,
       .name     = "#instruction-cat0-1src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x800 },
       .mask.bitset     = { 0x0, 0xe000e8ff },
       .decode = decode___instruction_cat0_1src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_1src_gen_0_fields),
       .decode_fields = decode___instruction_cat0_1src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0_1src__default_gen_0,
       },
};
static const struct isa_case kill__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_kill_gen_0 = {

       .parent   = &bitset___instruction_cat0_1src_gen_0,
       .name     = "kill",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x2800000 },
       .dontcare.bitset = { 0x0, 0xc0800 },
       .mask.bitset     = { 0x0, 0xe78ee8ff },
       .decode = decode_kill_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_kill_gen_0_fields),
       .decode_fields = decode_kill_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &kill__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0_immed__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{NAME} #{IMMED}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat0_immed_gen_0 = {

       .parent   = &bitset___instruction_cat0_gen_0,
       .name     = "#instruction-cat0-immed",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x70e8ff },
       .mask.bitset     = { 0x0, 0xe070e8ff },
       .decode = decode___instruction_cat0_immed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_immed_gen_0_fields),
       .decode_fields = decode___instruction_cat0_immed_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0_immed__default_gen_0,
       },
};
static const struct isa_case jump__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_jump_gen_0 = {

       .parent   = &bitset___instruction_cat0_immed_gen_0,
       .name     = "jump",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x1000000 },
       .dontcare.bitset = { 0x0, 0x7ce8ff },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_jump_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_jump_gen_0_fields),
       .decode_fields = decode_jump_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &jump__default_gen_0,
       },
};
static const struct isa_case call__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_call_gen_0 = {

       .parent   = &bitset___instruction_cat0_immed_gen_0,
       .name     = "call",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x1800000 },
       .dontcare.bitset = { 0x0, 0x7ce8ff },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_call_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_call_gen_0_fields),
       .decode_fields = decode_call_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &call__default_gen_0,
       },
};
static const struct isa_case bkt__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_bkt_gen_0 = {

       .parent   = &bitset___instruction_cat0_immed_gen_0,
       .name     = "bkt",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20000 },
       .dontcare.bitset = { 0x0, 0x7ce8ff },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_bkt_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_bkt_gen_0_fields),
       .decode_fields = decode_bkt_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &bkt__default_gen_0,
       },
};
static const struct isa_case getlast__default_gen_600 = {
       .display  = "{SY}{SS}{JP}{NAME}.w{CLUSTER_SIZE} #{IMMED}",
       .num_fields = 2,
       .fields   = {
          { .name = "CLUSTER_SIZE", .low = 0, .high = 0,
            .expr = &expr_anon_0,
            .type = TYPE_UINT,
          },
          { .name = "W", .low = 52, .high = 54,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_getlast_gen_600 = {

       .parent   = &bitset___instruction_cat0_gen_0,
       .name     = "getlast",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x2020000 },
       .dontcare.bitset = { 0x0, 0xce8ff },
       .mask.bitset     = { 0x0, 0xe78ee8ff },
       .decode = decode_getlast_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_getlast_gen_600_fields),
       .decode_fields = decode_getlast_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &getlast__default_gen_600,
       },
};
static const struct isa_case getone__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_getone_gen_0 = {

       .parent   = &bitset___instruction_cat0_immed_gen_0,
       .name     = "getone",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x2820000 },
       .dontcare.bitset = { 0x0, 0x7ce8ff },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_getone_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getone_gen_0_fields),
       .decode_fields = decode_getone_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getone__default_gen_0,
       },
};
static const struct isa_case shps__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shps_gen_0 = {

       .parent   = &bitset___instruction_cat0_immed_gen_0,
       .name     = "shps",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x3820000 },
       .dontcare.bitset = { 0x0, 0x7ce8ff },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_shps_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shps_gen_0_fields),
       .decode_fields = decode_shps_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shps__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0_branch__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat0_branch_gen_0 = {

       .parent   = &bitset___instruction_cat0_gen_0,
       .name     = "#instruction-cat0-branch",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800000 },
       .dontcare.bitset = { 0x0, 0xc0800 },
       .mask.bitset     = { 0x0, 0xe78e0800 },
       .decode = decode___instruction_cat0_branch_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_branch_gen_0_fields),
       .decode_fields = decode___instruction_cat0_branch_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0_branch__default_gen_0,
       },
};
static const struct isa_case brac__default_gen_0 = {
       .display  = "{SY}{SS}{EQ}{JP}{NAME}.{INDEX} #{IMMED}",
       .num_fields = 1,
       .fields   = {
          { .name = "INDEX", .low = 32, .high = 36,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_brac_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_gen_0,
       .name     = "brac",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800060 },
       .dontcare.bitset = { 0x0, 0x7ce800 },
       .mask.bitset     = { 0x0, 0xe7fee8e0 },
       .decode = decode_brac_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_brac_gen_0_fields),
       .decode_fields = decode_brac_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &brac__default_gen_0,
       },
};
static const struct isa_case brax__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_brax_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_gen_0,
       .name     = "brax",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x8000c0 },
       .dontcare.bitset = { 0x0, 0x7ce81f },
       .mask.bitset     = { 0x0, 0xe7fee8ff },
       .decode = decode_brax_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_brax_gen_0_fields),
       .decode_fields = decode_brax_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &brax__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0_branch_1src__default_gen_0 = {
       .display  = "{SY}{SS}{EQ}{JP}{NAME} {INV1}p0.{COMP1}, #{IMMED}",
       .num_fields = 2,
       .fields   = {
          { .name = "INV1", .low = 52, .high = 52,
            .display = "!",
            .type = TYPE_BOOL,
          },
          { .name = "COMP1", .low = 53, .high = 54,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat0_branch_1src_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_gen_0,
       .name     = "#instruction-cat0-branch-1src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800000 },
       .dontcare.bitset = { 0x0, 0xce81f },
       .mask.bitset     = { 0x0, 0xe78ee81f },
       .decode = decode___instruction_cat0_branch_1src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_branch_1src_gen_0_fields),
       .decode_fields = decode___instruction_cat0_branch_1src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0_branch_1src__default_gen_0,
       },
};
static const struct isa_case br__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_br_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_1src_gen_0,
       .name     = "br",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800000 },
       .dontcare.bitset = { 0x0, 0xce81f },
       .mask.bitset     = { 0x0, 0xe78ee8ff },
       .decode = decode_br_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_br_gen_0_fields),
       .decode_fields = decode_br_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &br__default_gen_0,
       },
};
static const struct isa_case bany__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_bany_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_1src_gen_0,
       .name     = "bany",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800080 },
       .dontcare.bitset = { 0x0, 0xce81f },
       .mask.bitset     = { 0x0, 0xe78ee8ff },
       .decode = decode_bany_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_bany_gen_0_fields),
       .decode_fields = decode_bany_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &bany__default_gen_0,
       },
};
static const struct isa_case ball__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ball_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_1src_gen_0,
       .name     = "ball",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x8000a0 },
       .dontcare.bitset = { 0x0, 0xce81f },
       .mask.bitset     = { 0x0, 0xe78ee8ff },
       .decode = decode_ball_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ball_gen_0_fields),
       .decode_fields = decode_ball_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ball__default_gen_0,
       },
};
static const struct isa_case __instruction_cat0_branch_2src__default_gen_0 = {
       .display  = "{SY}{SS}{EQ}{JP}{NAME} {INV1}p0.{COMP1}, {INV2}p0.{COMP2}, #{IMMED}",
       .num_fields = 4,
       .fields   = {
          { .name = "INV2", .low = 45, .high = 45,
            .display = "!",
            .type = TYPE_BOOL,
          },
          { .name = "COMP2", .low = 46, .high = 47,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "INV1", .low = 52, .high = 52,
            .display = "!",
            .type = TYPE_BOOL,
          },
          { .name = "COMP1", .low = 53, .high = 54,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat0_branch_2src_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_gen_0,
       .name     = "#instruction-cat0-branch-2src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800000 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe78e081f },
       .decode = decode___instruction_cat0_branch_2src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat0_branch_2src_gen_0_fields),
       .decode_fields = decode___instruction_cat0_branch_2src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat0_branch_2src__default_gen_0,
       },
};
static const struct isa_case brao__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_brao_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_2src_gen_0,
       .name     = "brao",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800020 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe78e08ff },
       .decode = decode_brao_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_brao_gen_0_fields),
       .decode_fields = decode_brao_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &brao__default_gen_0,
       },
};
static const struct isa_case braa__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_braa_gen_0 = {

       .parent   = &bitset___instruction_cat0_branch_2src_gen_0,
       .name     = "braa",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x800040 },
       .dontcare.bitset = { 0x0, 0xc081f },
       .mask.bitset     = { 0x0, 0xe78e08ff },
       .decode = decode_braa_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_braa_gen_0_fields),
       .decode_fields = decode_braa_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &braa__default_gen_0,
       },
};
static const struct isa_case __cat1_dst__case0_gen_0 = {
       .expr     = &expr_anon_1,
       .display  = "r<a0.x>",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat1_dst__case1_gen_0 = {
       .expr     = &expr_anon_2,
       .display  = "r<a0.x + {OFFSET}>",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat1_dst__default_gen_0 = {
       .display  = "{DST}",
       .num_fields = 1,
       .fields   = {
          { .name = "DST", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___cat1_dst_gen_0 = {

       .name     = "#cat1-dst",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_dst_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_dst_gen_0_fields),
       .decode_fields = decode___cat1_dst_gen_0_fields,
       .num_cases = 3,
       .cases    = {
            &__cat1_dst__case0_gen_0,
            &__cat1_dst__case1_gen_0,
            &__cat1_dst__default_gen_0,
       },
};
static const struct isa_case __instruction_cat1__default_gen_0 = {
       .num_fields = 5,
       .fields   = {
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "UL", .low = 45, .high = 45,
            .display = "(ul)",
            .type = TYPE_BOOL,
          },
          { .name = "ROUND", .low = 55, .high = 56,
            .type = TYPE_ENUM,
            .enums = &enum___round,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat1_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat1",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000400 },
       .decode = decode___instruction_cat1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_gen_0_fields),
       .decode_fields = decode___instruction_cat1_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat1__default_gen_0,
       },
};
static const struct isa_case __instruction_cat1_typed__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr_anon_3,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr_anon_4,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "DST_TYPE", .low = 46, .high = 48,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
          { .name = "SRC_TYPE", .low = 50, .high = 52,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat1_typed_gen_0 = {

       .parent   = &bitset___instruction_cat1_gen_0,
       .name     = "#instruction-cat1-typed",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000400 },
       .decode = decode___instruction_cat1_typed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_typed_gen_0_fields),
       .decode_fields = decode___instruction_cat1_typed_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat1_typed__default_gen_0,
       },
};
static const struct isa_case __instruction_cat1_mov__case0_gen_0 = {
       .expr     = &expr_anon_5,
       .display  = "{SY}{SS}{JP}{REPEAT}{UL}mova {ROUND}a0.x, {SRC_R}{SRC}",
       .num_fields = 3,
       .fields   = {
          { .name = "#instruction-cat1-mov#assert0", .low = 32, .high = 39,
            .type = TYPE_ASSERT,
            .val.bitset = { 0xf4, 0x0 },
          },
          { .name = "#instruction-cat1-mov#assert1", .low = 46, .high = 48,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x4, 0x0 },
          },
          { .name = "#instruction-cat1-mov#assert2", .low = 50, .high = 52,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x4, 0x0 },
          },
       },
};
static const struct isa_case __instruction_cat1_mov__case1_gen_0 = {
       .expr     = &expr_anon_6,
       .display  = "{SY}{SS}{JP}{REPEAT}{UL}mova1 {ROUND}a1.x, {SRC_R}{SRC}",
       .num_fields = 3,
       .fields   = {
          { .name = "#instruction-cat1-mov#assert0", .low = 32, .high = 39,
            .type = TYPE_ASSERT,
            .val.bitset = { 0xf5, 0x0 },
          },
          { .name = "#instruction-cat1-mov#assert1", .low = 46, .high = 48,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x2, 0x0 },
          },
          { .name = "#instruction-cat1-mov#assert2", .low = 50, .high = 52,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x2, 0x0 },
          },
       },
};
static const struct isa_case __instruction_cat1_mov__case2_gen_0 = {
       .expr     = &expr_anon_7,
       .display  = "{SY}{SS}{JP}{REPEAT}{UL}cov.{SRC_TYPE}{DST_TYPE} {ROUND}{DST_HALF}{DST}, {SRC}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_field_params __instruction_cat1_mov__default_gen_0_dst = {
       .num_params = 1,
       .params = {
           { .name= "DST_REL",  .as = "DST_REL" },

       },
};
static const struct isa_case __instruction_cat1_mov__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{REPEAT}{UL}mov.{SRC_TYPE}{DST_TYPE} {ROUND}{DST_HALF}{DST}, {SRC}",
       .num_fields = 3,
       .fields   = {
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __cat1_dst,
            .params = &__instruction_cat1_mov__default_gen_0_dst,
          },
          { .name = "REPEAT", .low = 40, .high = 41,
            .type = TYPE_ENUM,
            .enums = &enum___rptn,
          },
          { .name = "DST_REL", .low = 49, .high = 49,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat1_mov_gen_0 = {

       .parent   = &bitset___instruction_cat1_typed_gen_0,
       .name     = "#instruction-cat1-mov",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe6000400 },
       .decode = decode___instruction_cat1_mov_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_mov_gen_0_fields),
       .decode_fields = decode___instruction_cat1_mov_gen_0_fields,
       .num_cases = 4,
       .cases    = {
            &__instruction_cat1_mov__case0_gen_0,
            &__instruction_cat1_mov__case1_gen_0,
            &__instruction_cat1_mov__case2_gen_0,
            &__instruction_cat1_mov__default_gen_0,
       },
};
static const struct isa_case __cat1_immed_src__case0_gen_0 = {
       .expr     = &expr_anon_8,
       .display  = "h({IMMED})",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 15,
            .type = TYPE_FLOAT,
          },
       },
};
static const struct isa_case __cat1_immed_src__case1_gen_0 = {
       .expr     = &expr_anon_9,
       .display  = "({IMMED})",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_FLOAT,
          },
       },
};
static const struct isa_case __cat1_immed_src__case2_gen_0 = {
       .expr     = &expr_anon_10,
       .display  = "0x{IMMED}",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_HEX,
          },
       },
};
static const struct isa_case __cat1_immed_src__case3_gen_0 = {
       .expr     = &expr_anon_11,
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 15,
            .type = TYPE_INT,
          },
       },
};
static const struct isa_case __cat1_immed_src__case4_gen_0 = {
       .expr     = &expr_anon_12,
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_INT,
          },
       },
};
static const struct isa_case __cat1_immed_src__default_gen_0 = {
       .display  = "{IMMED}",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___cat1_immed_src_gen_0 = {

       .name     = "#cat1-immed-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_immed_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_immed_src_gen_0_fields),
       .decode_fields = decode___cat1_immed_src_gen_0_fields,
       .num_cases = 6,
       .cases    = {
            &__cat1_immed_src__case0_gen_0,
            &__cat1_immed_src__case1_gen_0,
            &__cat1_immed_src__case2_gen_0,
            &__cat1_immed_src__case3_gen_0,
            &__cat1_immed_src__case4_gen_0,
            &__cat1_immed_src__default_gen_0,
       },
};
static const struct isa_case __cat1_const_src__default_gen_0 = {
       .display  = "{SRC_R}{HALF}{CONST}",
       .num_fields = 1,
       .fields   = {
          { .name = "CONST", .low = 0, .high = 10,
            .type = TYPE_BITSET,
            .bitsets = __reg_const,
          },
       },
};
static const struct isa_bitset bitset___cat1_const_src_gen_0 = {

       .name     = "#cat1-const-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_const_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_const_src_gen_0_fields),
       .decode_fields = decode___cat1_const_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat1_const_src__default_gen_0,
       },
};
static const struct isa_case __cat1_gpr_src__default_gen_0 = {
       .display  = "{LAST}{SRC_R}{HALF}{SRC}",
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___cat1_gpr_src_gen_0 = {

       .name     = "#cat1-gpr-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_gpr_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_gpr_src_gen_0_fields),
       .decode_fields = decode___cat1_gpr_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat1_gpr_src__default_gen_0,
       },
};
static const struct isa_case __cat1_relative_gpr_src__default_gen_0 = {
       .display  = "{SRC_R}{HALF}{SRC}",
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 9,
            .type = TYPE_BITSET,
            .bitsets = __reg_relative_gpr,
          },
       },
};
static const struct isa_bitset bitset___cat1_relative_gpr_src_gen_0 = {

       .name     = "#cat1-relative-gpr-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_relative_gpr_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_relative_gpr_src_gen_0_fields),
       .decode_fields = decode___cat1_relative_gpr_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat1_relative_gpr_src__default_gen_0,
       },
};
static const struct isa_case __cat1_relative_const_src__default_gen_0 = {
       .display  = "{SRC_R}{HALF}{SRC}",
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 9,
            .type = TYPE_BITSET,
            .bitsets = __reg_relative_const,
          },
       },
};
static const struct isa_bitset bitset___cat1_relative_const_src_gen_0 = {

       .name     = "#cat1-relative-const-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_relative_const_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_relative_const_src_gen_0_fields),
       .decode_fields = decode___cat1_relative_const_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat1_relative_const_src__default_gen_0,
       },
};
static const struct isa_field_params mov_immed__default_gen_0_src = {
       .num_params = 1,
       .params = {
           { .name= "SRC_TYPE",  .as = "SRC_TYPE" },

       },
};
static const struct isa_case mov_immed__default_gen_0 = {
       .num_fields = 2,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat1_immed_src,
            .params = &mov_immed__default_gen_0_src,
          },
          { .name = "SRC_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mov_immed_gen_0 = {

       .parent   = &bitset___instruction_cat1_mov_gen_0,
       .name     = "mov-immed",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20400000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe6600400 },
       .decode = decode_mov_immed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mov_immed_gen_0_fields),
       .decode_fields = decode_mov_immed_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mov_immed__default_gen_0,
       },
};
static const struct isa_field_params mov_const__default_gen_0_src = {
       .num_params = 2,
       .params = {
           { .name= "SRC_R",  .as = "SRC_R" },
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_case mov_const__default_gen_0 = {
       .num_fields = 2,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 10,
            .type = TYPE_BITSET,
            .bitsets = __cat1_const_src,
            .params = &mov_const__default_gen_0_src,
          },
          { .name = "SRC_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mov_const_gen_0 = {

       .parent   = &bitset___instruction_cat1_mov_gen_0,
       .name     = "mov-const",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20200000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xfffff800, 0xe6600400 },
       .decode = decode_mov_const_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mov_const_gen_0_fields),
       .decode_fields = decode_mov_const_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mov_const__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat1_mov_gpr__default_gen_0_src = {
       .num_params = 3,
       .params = {
           { .name= "LAST",  .as = "LAST" },
           { .name= "SRC_R",  .as = "SRC_R" },
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_case __instruction_cat1_mov_gpr__default_gen_0 = {
       .num_fields = 3,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __cat1_gpr_src,
            .params = &__instruction_cat1_mov_gpr__default_gen_0_src,
          },
          { .name = "LAST", .low = 10, .high = 10,
            .display = "(last)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat1_mov_gpr_gen_0 = {

       .parent   = &bitset___instruction_cat1_mov_gen_0,
       .name     = "#instruction-cat1-mov-gpr",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x300, 0xe6600400 },
       .decode = decode___instruction_cat1_mov_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_mov_gpr_gen_0_fields),
       .decode_fields = decode___instruction_cat1_mov_gpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat1_mov_gpr__default_gen_0,
       },
};
static const struct isa_case mov_gpr__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_mov_gpr_gen_0 = {

       .parent   = &bitset___instruction_cat1_mov_gpr_gen_0,
       .name     = "mov-gpr",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xfffffb00, 0xe6600400 },
       .decode = decode_mov_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mov_gpr_gen_0_fields),
       .decode_fields = decode_mov_gpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mov_gpr__default_gen_0,
       },
};
static const struct isa_case __instruction_cat1_movs__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat1_movs_gen_0 = {

       .parent   = &bitset___instruction_cat1_mov_gpr_gen_0,
       .name     = "#instruction-cat1-movs",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x80000000, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x807ffb00, 0xe6600400 },
       .decode = decode___instruction_cat1_movs_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_movs_gen_0_fields),
       .decode_fields = decode___instruction_cat1_movs_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat1_movs__default_gen_0,
       },
};
static const struct isa_case movs_immed__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{REPEAT}{UL}movs.{SRC_TYPE}{DST_TYPE} {ROUND}{DST_HALF}{DST}, {SRC}, {INVOCATION}",
       .num_fields = 1,
       .fields   = {
          { .name = "INVOCATION", .low = 23, .high = 29,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_movs_immed_gen_0 = {

       .parent   = &bitset___instruction_cat1_movs_gen_0,
       .name     = "movs-immed",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x80000000, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xc07ffb00, 0xe6600400 },
       .decode = decode_movs_immed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_movs_immed_gen_0_fields),
       .decode_fields = decode_movs_immed_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &movs_immed__default_gen_0,
       },
};
static const struct isa_case movs_a0__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{REPEAT}{UL}movs.{SRC_TYPE}{DST_TYPE} {ROUND}{DST_HALF}{DST}, {SRC}, a0.x",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_movs_a0_gen_0 = {

       .parent   = &bitset___instruction_cat1_movs_gen_0,
       .name     = "movs-a0",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc0000000, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xfffffb00, 0xe6600400 },
       .decode = decode_movs_a0_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_movs_a0_gen_0_fields),
       .decode_fields = decode_movs_a0_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &movs_a0__default_gen_0,
       },
};
static const struct isa_case __instruction_cat1_relative__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "SRC_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat1_relative_gen_0 = {

       .parent   = &bitset___instruction_cat1_mov_gen_0,
       .name     = "#instruction-cat1-relative",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xfffff800, 0xe6600400 },
       .decode = decode___instruction_cat1_relative_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_relative_gen_0_fields),
       .decode_fields = decode___instruction_cat1_relative_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat1_relative__default_gen_0,
       },
};
static const struct isa_field_params mov_relgpr__default_gen_0_src = {
       .num_params = 2,
       .params = {
           { .name= "SRC_R",  .as = "SRC_R" },
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_case mov_relgpr__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 9,
            .type = TYPE_BITSET,
            .bitsets = __cat1_relative_gpr_src,
            .params = &mov_relgpr__default_gen_0_src,
          },
       },
};
static const struct isa_bitset bitset_mov_relgpr_gen_0 = {

       .parent   = &bitset___instruction_cat1_relative_gen_0,
       .name     = "mov-relgpr",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xfffffc00, 0xe6600400 },
       .decode = decode_mov_relgpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mov_relgpr_gen_0_fields),
       .decode_fields = decode_mov_relgpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mov_relgpr__default_gen_0,
       },
};
static const struct isa_field_params mov_relconst__default_gen_0_src = {
       .num_params = 2,
       .params = {
           { .name= "SRC_R",  .as = "SRC_R" },
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_case mov_relconst__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 9,
            .type = TYPE_BITSET,
            .bitsets = __cat1_relative_const_src,
            .params = &mov_relconst__default_gen_0_src,
          },
       },
};
static const struct isa_bitset bitset_mov_relconst_gen_0 = {

       .parent   = &bitset___instruction_cat1_relative_gen_0,
       .name     = "mov-relconst",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc00, 0x20000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xfffffc00, 0xe6600400 },
       .decode = decode_mov_relconst_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mov_relconst_gen_0_fields),
       .decode_fields = decode_mov_relconst_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mov_relconst__default_gen_0,
       },
};
static const struct isa_case __cat1_multi_src__default_gen_0 = {
       .display  = "{HALF}{REG}",
       .num_fields = 1,
       .fields   = {
          { .name = "REG", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___cat1_multi_src_gen_0 = {

       .name     = "#cat1-multi-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_multi_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_multi_src_gen_0_fields),
       .decode_fields = decode___cat1_multi_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat1_multi_src__default_gen_0,
       },
};
static const struct isa_case __cat1_multi_dst__default_gen_0 = {
       .display  = "{DST_HALF}{REG}",
       .num_fields = 1,
       .fields   = {
          { .name = "REG", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___cat1_multi_dst_gen_0 = {

       .name     = "#cat1-multi-dst",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat1_multi_dst_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat1_multi_dst_gen_0_fields),
       .decode_fields = decode___cat1_multi_dst_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat1_multi_dst__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat1_multi__default_gen_500_src0 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params __instruction_cat1_multi__default_gen_500_dst0 = {
       .num_params = 1,
       .params = {
           { .name= "DST_HALF",  .as = "DST_HALF" },

       },
};
static const struct isa_case __instruction_cat1_multi__default_gen_500 = {
       .num_fields = 2,
       .fields   = {
          { .name = "SRC0", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_src,
            .params = &__instruction_cat1_multi__default_gen_500_src0,
          },
          { .name = "DST0", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_dst,
            .params = &__instruction_cat1_multi__default_gen_500_dst0,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat1_multi_gen_500 = {

       .parent   = &bitset___instruction_cat1_typed_gen_0,
       .name     = "#instruction-cat1-multi",
       .gen      = {
           .min  = 500,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x24000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe6620c00 },
       .decode = decode___instruction_cat1_multi_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat1_multi_gen_500_fields),
       .decode_fields = decode___instruction_cat1_multi_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat1_multi__default_gen_500,
       },
};
static const struct isa_field_params swz__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params swz__default_gen_0_dst1 = {
       .num_params = 1,
       .params = {
           { .name= "DST_HALF",  .as = "DST_HALF" },

       },
};
static const struct isa_case swz__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{UL}swz.{SRC_TYPE}{DST_TYPE} {ROUND}{DST0}, {DST1}, {SRC0}, {SRC1}",
       .num_fields = 2,
       .fields   = {
          { .name = "SRC1", .low = 8, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_src,
            .params = &swz__default_gen_0_src1,
          },
          { .name = "DST1", .low = 16, .high = 23,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_dst,
            .params = &swz__default_gen_0_dst1,
          },
       },
};
static const struct isa_bitset bitset_swz_gen_0 = {

       .parent   = &bitset___instruction_cat1_multi_gen_500,
       .name     = "swz",
       .gen      = {
           .min  = 500,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x24000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xff000000, 0xe6620f00 },
       .decode = decode_swz_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_swz_gen_0_fields),
       .decode_fields = decode_swz_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &swz__default_gen_0,
       },
};
static const struct isa_field_params gat__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params gat__default_gen_0_src2 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params gat__default_gen_0_src3 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_case gat__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{UL}gat.{SRC_TYPE}{DST_TYPE} {ROUND}{DST0}, {SRC0}, {SRC1}, {SRC2}, {SRC3}",
       .num_fields = 3,
       .fields   = {
          { .name = "SRC1", .low = 8, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_src,
            .params = &gat__default_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 23,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_src,
            .params = &gat__default_gen_0_src2,
          },
          { .name = "SRC3", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_src,
            .params = &gat__default_gen_0_src3,
          },
       },
};
static const struct isa_bitset bitset_gat_gen_0 = {

       .parent   = &bitset___instruction_cat1_multi_gen_500,
       .name     = "gat",
       .gen      = {
           .min  = 500,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x24000100 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe6620f00 },
       .decode = decode_gat_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_gat_gen_0_fields),
       .decode_fields = decode_gat_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &gat__default_gen_0,
       },
};
static const struct isa_field_params sct__default_gen_0_dst1 = {
       .num_params = 1,
       .params = {
           { .name= "DST_HALF",  .as = "DST_HALF" },

       },
};
static const struct isa_field_params sct__default_gen_0_dst2 = {
       .num_params = 1,
       .params = {
           { .name= "DST_HALF",  .as = "DST_HALF" },

       },
};
static const struct isa_field_params sct__default_gen_0_dst3 = {
       .num_params = 1,
       .params = {
           { .name= "DST_HALF",  .as = "DST_HALF" },

       },
};
static const struct isa_case sct__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{UL}sct.{SRC_TYPE}{DST_TYPE} {ROUND}{DST0}, {DST1}, {DST2}, {DST3}, {SRC0}",
       .num_fields = 3,
       .fields   = {
          { .name = "DST1", .low = 8, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_dst,
            .params = &sct__default_gen_0_dst1,
          },
          { .name = "DST2", .low = 16, .high = 23,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_dst,
            .params = &sct__default_gen_0_dst2,
          },
          { .name = "DST3", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat1_multi_dst,
            .params = &sct__default_gen_0_dst3,
          },
       },
};
static const struct isa_bitset bitset_sct_gen_0 = {

       .parent   = &bitset___instruction_cat1_multi_gen_500,
       .name     = "sct",
       .gen      = {
           .min  = 500,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x24000200 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe6620f00 },
       .decode = decode_sct_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sct_gen_0_fields),
       .decode_fields = decode_sct_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sct__default_gen_0,
       },
};
static const struct isa_field_params movmsk__default_gen_0_dst = {
       .num_params = 1,
       .params = {
           { .name= "DST_REL",  .as = "DST_REL" },

       },
};
static const struct isa_case movmsk__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{UL}movmsk.w{W} {DST}",
       .num_fields = 4,
       .fields   = {
          { .name = "W", .low = 0, .high = 0,
            .expr = &expr_anon_13,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __cat1_dst,
            .params = &movmsk__default_gen_0_dst,
          },
          { .name = "REPEAT", .low = 40, .high = 41,
            .type = TYPE_ENUM,
            .enums = &enum___rptn,
          },
          { .name = "DST_REL", .low = 49, .high = 49,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_movmsk_gen_0 = {

       .parent   = &bitset___instruction_cat1_gen_0,
       .name     = "movmsk",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x260cc000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xffffffff, 0xe67dcc00 },
       .decode = decode_movmsk_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_movmsk_gen_0_fields),
       .decode_fields = decode_movmsk_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &movmsk__default_gen_0,
       },
};
static const struct isa_case __instruction_cat2__default_gen_0 = {
       .num_fields = 14,
       .fields   = {
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr___dest_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "ZERO", .low = 0, .high = 0,
            .expr = &expr___zero,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "REPEAT", .low = 40, .high = 41,
            .type = TYPE_ENUM,
            .enums = &enum___rptn,
          },
          { .name = "SAT", .low = 42, .high = 42,
            .display = "(sat)",
            .type = TYPE_BOOL,
          },
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "UL", .low = 45, .high = 45,
            .display = "(ul)",
            .type = TYPE_BOOL,
          },
          { .name = "DST_CONV", .low = 46, .high = 46,
            .type = TYPE_BOOL,
          },
          { .name = "EI", .low = 47, .high = 47,
            .display = "(ei)",
            .type = TYPE_BOOL,
          },
          { .name = "FULL", .low = 52, .high = 52,
            .type = TYPE_BOOL,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC1_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC2_R", .low = 51, .high = 51,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat2_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat2",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000000 },
       .decode = decode___instruction_cat2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat2_gen_0_fields),
       .decode_fields = decode___instruction_cat2_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat2__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat2_1src__case0_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_1src__case0_gen_0 = {
       .expr     = &expr___cat2_cat3_nop_encoding,
       .display  = "{SY}{SS}{JP}{SAT}(nop{NOP}) {UL}{NAME} {EI}{DST_HALF}{DST}, {SRC1}",
       .num_fields = 2,
       .fields   = {
          { .name = "NOP", .low = 0, .high = 0,
            .expr = &expr___cat2_cat3_nop_value,
            .type = TYPE_UINT,
          },
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_1src__case0_gen_0_src1,
          },
       },
};
static const struct isa_field_params __instruction_cat2_1src__default_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "SRC1_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_1src__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{SAT}{REPEAT}{UL}{NAME} {EI}{DST_HALF}{DST}, {SRC1}",
       .num_fields = 1,
       .fields   = {
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_1src__default_gen_0_src1,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat2_1src_gen_0 = {

       .parent   = &bitset___instruction_cat2_gen_0,
       .name     = "#instruction-cat2-1src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40000000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe0070000 },
       .decode = decode___instruction_cat2_1src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat2_1src_gen_0_fields),
       .decode_fields = decode___instruction_cat2_1src_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat2_1src__case0_gen_0,
            &__instruction_cat2_1src__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat2_2src__case0_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_field_params __instruction_cat2_2src__case0_gen_0_src2 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_2src__case0_gen_0 = {
       .expr     = &expr___cat2_cat3_nop_encoding,
       .display  = "{SY}{SS}{JP}{SAT}(nop{NOP}) {UL}{NAME} {EI}{DST_HALF}{DST}, {SRC1}, {SRC2}",
       .num_fields = 3,
       .fields   = {
          { .name = "NOP", .low = 0, .high = 0,
            .expr = &expr___cat2_cat3_nop_value,
            .type = TYPE_UINT,
          },
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src__case0_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src__case0_gen_0_src2,
          },
       },
};
static const struct isa_field_params __instruction_cat2_2src__default_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "SRC1_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_field_params __instruction_cat2_2src__default_gen_0_src2 = {
       .num_params = 2,
       .params = {
           { .name= "SRC2_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_2src__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{SAT}{REPEAT}{UL}{NAME} {EI}{DST_HALF}{DST}, {SRC1}, {SRC2}",
       .num_fields = 2,
       .fields   = {
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src__default_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src__default_gen_0_src2,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat2_2src_gen_0 = {

       .parent   = &bitset___instruction_cat2_gen_0,
       .name     = "#instruction-cat2-2src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40000000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe0070000 },
       .decode = decode___instruction_cat2_2src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat2_2src_gen_0_fields),
       .decode_fields = decode___instruction_cat2_2src_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat2_2src__case0_gen_0,
            &__instruction_cat2_2src__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat2_2src_cond__case0_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_field_params __instruction_cat2_2src_cond__case0_gen_0_src2 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_2src_cond__case0_gen_0 = {
       .expr     = &expr___cat2_cat3_nop_encoding,
       .display  = "{SY}{SS}{JP}{SAT}(nop{NOP}) {UL}{NAME}.{COND} {EI}{DST_HALF}{DST}, {SRC1}, {SRC2}",
       .num_fields = 3,
       .fields   = {
          { .name = "NOP", .low = 0, .high = 0,
            .expr = &expr___cat2_cat3_nop_value,
            .type = TYPE_UINT,
          },
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_cond__case0_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_cond__case0_gen_0_src2,
          },
       },
};
static const struct isa_field_params __instruction_cat2_2src_cond__default_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "SRC1_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_field_params __instruction_cat2_2src_cond__default_gen_0_src2 = {
       .num_params = 2,
       .params = {
           { .name= "SRC2_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_2src_cond__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{SAT}{REPEAT}{UL}{NAME}.{COND} {EI}{DST_HALF}{DST}, {SRC1}, {SRC2}",
       .num_fields = 3,
       .fields   = {
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_cond__default_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_cond__default_gen_0_src2,
          },
          { .name = "COND", .low = 48, .high = 50,
            .type = TYPE_ENUM,
            .enums = &enum___cond,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat2_2src_cond_gen_0 = {

       .parent   = &bitset___instruction_cat2_gen_0,
       .name     = "#instruction-cat2-2src-cond",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000000 },
       .decode = decode___instruction_cat2_2src_cond_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat2_2src_cond_gen_0_fields),
       .decode_fields = decode___instruction_cat2_2src_cond_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat2_2src_cond__case0_gen_0,
            &__instruction_cat2_2src_cond__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat2_2src_input__case0_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_field_params __instruction_cat2_2src_input__case0_gen_0_src2 = {
       .num_params = 2,
       .params = {
           { .name= "ZERO",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_2src_input__case0_gen_0 = {
       .expr     = &expr___cat2_cat3_nop_encoding,
       .display  = "{SY}{SS}{JP}{SAT}(nop{NOP}) {UL}{NAME} {EI}{DST_HALF}{DST}, {SRC1}, {SRC2}",
       .num_fields = 3,
       .fields   = {
          { .name = "NOP", .low = 0, .high = 0,
            .expr = &expr___cat2_cat3_nop_value,
            .type = TYPE_UINT,
          },
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_input__case0_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_input__case0_gen_0_src2,
          },
       },
};
static const struct isa_field_params __instruction_cat2_2src_input__default_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "SRC1_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_field_params __instruction_cat2_2src_input__default_gen_0_src2 = {
       .num_params = 2,
       .params = {
           { .name= "SRC2_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat2_2src_input__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{SAT}{REPEAT}{UL}{NAME} {EI}{DST_HALF}{DST}, {SRC1}, {SRC2}",
       .num_fields = 2,
       .fields   = {
          { .name = "SRC1", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_input__default_gen_0_src1,
          },
          { .name = "SRC2", .low = 16, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat2_2src_input__default_gen_0_src2,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat2_2src_input_gen_0 = {

       .parent   = &bitset___instruction_cat2_gen_0,
       .name     = "#instruction-cat2-2src-input",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47200000 },
       .dontcare.bitset = { 0x0, 0x30000 },
       .mask.bitset     = { 0x0, 0xe7e30000 },
       .decode = decode___instruction_cat2_2src_input_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat2_2src_input_gen_0_fields),
       .decode_fields = decode___instruction_cat2_2src_input_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat2_2src_input__case0_gen_0,
            &__instruction_cat2_2src_input__default_gen_0,
       },
};
static const struct isa_case bary_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_bary_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_input_gen_0,
       .name     = "bary.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47200000 },
       .dontcare.bitset = { 0x0, 0x30000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_bary_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_bary_f_gen_0_fields),
       .decode_fields = decode_bary_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &bary_f__default_gen_0,
       },
};
static const struct isa_case flat_b__default_gen_600 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_flat_b_gen_600 = {

       .parent   = &bitset___instruction_cat2_2src_input_gen_0,
       .name     = "flat.b",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47240000 },
       .dontcare.bitset = { 0x0, 0x30000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_flat_b_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_flat_b_gen_600_fields),
       .decode_fields = decode_flat_b_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &flat_b__default_gen_600,
       },
};
static const struct isa_case add_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_add_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "add.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40000000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_add_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_add_f_gen_0_fields),
       .decode_fields = decode_add_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &add_f__default_gen_0,
       },
};
static const struct isa_case min_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_min_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "min.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40200000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_min_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_min_f_gen_0_fields),
       .decode_fields = decode_min_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &min_f__default_gen_0,
       },
};
static const struct isa_case max_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_max_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "max.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40400000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_max_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_max_f_gen_0_fields),
       .decode_fields = decode_max_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &max_f__default_gen_0,
       },
};
static const struct isa_case mul_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_mul_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "mul.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40600000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_mul_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mul_f_gen_0_fields),
       .decode_fields = decode_mul_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mul_f__default_gen_0,
       },
};
static const struct isa_case sign_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_sign_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "sign.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40800000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_sign_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sign_f_gen_0_fields),
       .decode_fields = decode_sign_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sign_f__default_gen_0,
       },
};
static const struct isa_case cmps_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cmps_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_cond_gen_0,
       .name     = "cmps.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40a00000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe7e00000 },
       .decode = decode_cmps_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cmps_f_gen_0_fields),
       .decode_fields = decode_cmps_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cmps_f__default_gen_0,
       },
};
static const struct isa_case absneg_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_absneg_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "absneg.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40c00000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_absneg_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_absneg_f_gen_0_fields),
       .decode_fields = decode_absneg_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &absneg_f__default_gen_0,
       },
};
static const struct isa_case cmpv_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cmpv_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_cond_gen_0,
       .name     = "cmpv.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x40e00000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe7e00000 },
       .decode = decode_cmpv_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cmpv_f_gen_0_fields),
       .decode_fields = decode_cmpv_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cmpv_f__default_gen_0,
       },
};
static const struct isa_case floor_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_floor_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "floor.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x41200000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_floor_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_floor_f_gen_0_fields),
       .decode_fields = decode_floor_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &floor_f__default_gen_0,
       },
};
static const struct isa_case ceil_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ceil_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "ceil.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x41400000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_ceil_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ceil_f_gen_0_fields),
       .decode_fields = decode_ceil_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ceil_f__default_gen_0,
       },
};
static const struct isa_case rndne_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_rndne_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "rndne.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x41600000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_rndne_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_rndne_f_gen_0_fields),
       .decode_fields = decode_rndne_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &rndne_f__default_gen_0,
       },
};
static const struct isa_case rndaz_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_rndaz_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "rndaz.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x41800000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_rndaz_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_rndaz_f_gen_0_fields),
       .decode_fields = decode_rndaz_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &rndaz_f__default_gen_0,
       },
};
static const struct isa_case trunc_f__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_trunc_f_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "trunc.f",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x41a00000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_trunc_f_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_trunc_f_gen_0_fields),
       .decode_fields = decode_trunc_f_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &trunc_f__default_gen_0,
       },
};
static const struct isa_case add_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_add_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "add.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42000000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_add_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_add_u_gen_0_fields),
       .decode_fields = decode_add_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &add_u__default_gen_0,
       },
};
static const struct isa_case add_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_add_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "add.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42200000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_add_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_add_s_gen_0_fields),
       .decode_fields = decode_add_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &add_s__default_gen_0,
       },
};
static const struct isa_case sub_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_sub_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "sub.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42400000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_sub_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sub_u_gen_0_fields),
       .decode_fields = decode_sub_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sub_u__default_gen_0,
       },
};
static const struct isa_case sub_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_sub_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "sub.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42600000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_sub_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sub_s_gen_0_fields),
       .decode_fields = decode_sub_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sub_s__default_gen_0,
       },
};
static const struct isa_case cmps_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cmps_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_cond_gen_0,
       .name     = "cmps.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe7e00000 },
       .decode = decode_cmps_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cmps_u_gen_0_fields),
       .decode_fields = decode_cmps_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cmps_u__default_gen_0,
       },
};
static const struct isa_case cmps_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cmps_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_cond_gen_0,
       .name     = "cmps.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42a00000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe7e00000 },
       .decode = decode_cmps_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cmps_s_gen_0_fields),
       .decode_fields = decode_cmps_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cmps_s__default_gen_0,
       },
};
static const struct isa_case min_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_min_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "min.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42c00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_min_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_min_u_gen_0_fields),
       .decode_fields = decode_min_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &min_u__default_gen_0,
       },
};
static const struct isa_case min_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_min_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "min.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x42e00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_min_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_min_s_gen_0_fields),
       .decode_fields = decode_min_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &min_s__default_gen_0,
       },
};
static const struct isa_case max_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_max_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "max.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43000000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_max_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_max_u_gen_0_fields),
       .decode_fields = decode_max_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &max_u__default_gen_0,
       },
};
static const struct isa_case max_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_max_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "max.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43200000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_max_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_max_s_gen_0_fields),
       .decode_fields = decode_max_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &max_s__default_gen_0,
       },
};
static const struct isa_case absneg_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_absneg_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "absneg.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43400000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_absneg_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_absneg_s_gen_0_fields),
       .decode_fields = decode_absneg_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &absneg_s__default_gen_0,
       },
};
static const struct isa_case and_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_and_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "and.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43800000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_and_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_and_b_gen_0_fields),
       .decode_fields = decode_and_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &and_b__default_gen_0,
       },
};
static const struct isa_case or_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_or_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "or.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43a00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_or_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_or_b_gen_0_fields),
       .decode_fields = decode_or_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &or_b__default_gen_0,
       },
};
static const struct isa_case not_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_not_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "not.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43c00000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_not_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_not_b_gen_0_fields),
       .decode_fields = decode_not_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &not_b__default_gen_0,
       },
};
static const struct isa_case xor_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_xor_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "xor.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x43e00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_xor_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_xor_b_gen_0_fields),
       .decode_fields = decode_xor_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &xor_b__default_gen_0,
       },
};
static const struct isa_case cmpv_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cmpv_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_cond_gen_0,
       .name     = "cmpv.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x44200000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe7e00000 },
       .decode = decode_cmpv_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cmpv_u_gen_0_fields),
       .decode_fields = decode_cmpv_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cmpv_u__default_gen_0,
       },
};
static const struct isa_case cmpv_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cmpv_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_cond_gen_0,
       .name     = "cmpv.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x44400000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe7e00000 },
       .decode = decode_cmpv_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cmpv_s_gen_0_fields),
       .decode_fields = decode_cmpv_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cmpv_s__default_gen_0,
       },
};
static const struct isa_case mul_u24__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_mul_u24_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "mul.u24",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46000000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_mul_u24_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mul_u24_gen_0_fields),
       .decode_fields = decode_mul_u24_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mul_u24__default_gen_0,
       },
};
static const struct isa_case mul_s24__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_mul_s24_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "mul.s24",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46200000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_mul_s24_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mul_s24_gen_0_fields),
       .decode_fields = decode_mul_s24_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mul_s24__default_gen_0,
       },
};
static const struct isa_case mull_u__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_mull_u_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "mull.u",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46400000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_mull_u_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mull_u_gen_0_fields),
       .decode_fields = decode_mull_u_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mull_u__default_gen_0,
       },
};
static const struct isa_case bfrev_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_bfrev_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "bfrev.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46600000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_bfrev_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_bfrev_b_gen_0_fields),
       .decode_fields = decode_bfrev_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &bfrev_b__default_gen_0,
       },
};
static const struct isa_case clz_s__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_clz_s_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "clz.s",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46800000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_clz_s_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_clz_s_gen_0_fields),
       .decode_fields = decode_clz_s_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &clz_s__default_gen_0,
       },
};
static const struct isa_case clz_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_clz_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "clz.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46a00000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_clz_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_clz_b_gen_0_fields),
       .decode_fields = decode_clz_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &clz_b__default_gen_0,
       },
};
static const struct isa_case shl_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shl_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "shl.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46c00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_shl_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shl_b_gen_0_fields),
       .decode_fields = decode_shl_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shl_b__default_gen_0,
       },
};
static const struct isa_case shr_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shr_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "shr.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x46e00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_shr_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shr_b_gen_0_fields),
       .decode_fields = decode_shr_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shr_b__default_gen_0,
       },
};
static const struct isa_case ashr_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ashr_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "ashr.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47000000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_ashr_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ashr_b_gen_0_fields),
       .decode_fields = decode_ashr_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ashr_b__default_gen_0,
       },
};
static const struct isa_case mgen_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_mgen_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "mgen.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47400000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_mgen_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mgen_b_gen_0_fields),
       .decode_fields = decode_mgen_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mgen_b__default_gen_0,
       },
};
static const struct isa_case getbit_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_getbit_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "getbit.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47600000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_getbit_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getbit_b_gen_0_fields),
       .decode_fields = decode_getbit_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getbit_b__default_gen_0,
       },
};
static const struct isa_case setrm__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_setrm_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "setrm",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47800000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_setrm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_setrm_gen_0_fields),
       .decode_fields = decode_setrm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &setrm__default_gen_0,
       },
};
static const struct isa_case cbits_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cbits_b_gen_0 = {

       .parent   = &bitset___instruction_cat2_1src_gen_0,
       .name     = "cbits.b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47a00000 },
       .dontcare.bitset = { 0xffff0000, 0x70000 },
       .mask.bitset     = { 0xffff0000, 0xe7e70000 },
       .decode = decode_cbits_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cbits_b_gen_0_fields),
       .decode_fields = decode_cbits_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cbits_b__default_gen_0,
       },
};
static const struct isa_case shb__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shb_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "shb",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47c00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_shb_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shb_gen_0_fields),
       .decode_fields = decode_shb_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shb__default_gen_0,
       },
};
static const struct isa_case msad__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_msad_gen_0 = {

       .parent   = &bitset___instruction_cat2_2src_gen_0,
       .name     = "msad",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x47e00000 },
       .dontcare.bitset = { 0x0, 0x70000 },
       .mask.bitset     = { 0x0, 0xe7e70000 },
       .decode = decode_msad_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_msad_gen_0_fields),
       .decode_fields = decode_msad_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &msad__default_gen_0,
       },
};
static const struct isa_case __cat3_src__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___cat3_src_gen_0 = {

       .name     = "#cat3-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat3_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat3_src_gen_0_fields),
       .decode_fields = decode___cat3_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat3_src__default_gen_0,
       },
};
static const struct isa_case __cat3_src_gpr__default_gen_0 = {
       .display  = "{LAST}{HALF}{SRC}",
       .num_fields = 2,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "LAST", .low = 10, .high = 10,
            .display = "(last)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___cat3_src_gpr_gen_0 = {

       .parent   = &bitset___cat3_src_gen_0,
       .name     = "#cat3-src-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x1b00, 0x0 },
       .decode = decode___cat3_src_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat3_src_gpr_gen_0_fields),
       .decode_fields = decode___cat3_src_gpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat3_src_gpr__default_gen_0,
       },
};
static const struct isa_case __cat3_src_const_or_immed__case0_gen_0 = {
       .expr     = &expr_anon_14,
       .display  = "{IMMED}",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 11,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat3_src_const_or_immed__default_gen_0 = {
       .display  = "{HALF}c{CONST}.{SWIZ}",
       .num_fields = 3,
       .fields   = {
          { .name = "#cat3-src-const-or-immed#assert0", .low = 11, .high = 11,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
          { .name = "SWIZ", .low = 0, .high = 1,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "CONST", .low = 2, .high = 10,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___cat3_src_const_or_immed_gen_0 = {

       .parent   = &bitset___cat3_src_gen_0,
       .name     = "#cat3-src-const-or-immed",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1000, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x1000, 0x0 },
       .decode = decode___cat3_src_const_or_immed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat3_src_const_or_immed_gen_0_fields),
       .decode_fields = decode___cat3_src_const_or_immed_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat3_src_const_or_immed__case0_gen_0,
            &__cat3_src_const_or_immed__default_gen_0,
       },
};
static const struct isa_case __cat3_src_relative__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___cat3_src_relative_gen_0 = {

       .parent   = &bitset___cat3_src_gen_0,
       .name     = "#cat3-src-relative",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x1800, 0x0 },
       .decode = decode___cat3_src_relative_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat3_src_relative_gen_0_fields),
       .decode_fields = decode___cat3_src_relative_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat3_src_relative__default_gen_0,
       },
};
static const struct isa_case __cat3_src_relative_gpr__default_gen_0 = {
       .display  = "{HALF}r<a0.x + {OFFSET}>",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 9,
            .type = TYPE_INT,
          },
       },
};
static const struct isa_bitset bitset___cat3_src_relative_gpr_gen_0 = {

       .parent   = &bitset___cat3_src_relative_gen_0,
       .name     = "#cat3-src-relative-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x1c00, 0x0 },
       .decode = decode___cat3_src_relative_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat3_src_relative_gpr_gen_0_fields),
       .decode_fields = decode___cat3_src_relative_gpr_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat3_src_relative_gpr__default_gen_0,
       },
};
static const struct isa_case __cat3_src_relative_const__default_gen_0 = {
       .display  = "{HALF}c<a0.x + {OFFSET}>",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 9,
            .type = TYPE_INT,
          },
       },
};
static const struct isa_bitset bitset___cat3_src_relative_const_gen_0 = {

       .parent   = &bitset___cat3_src_relative_gen_0,
       .name     = "#cat3-src-relative-const",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc00, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x1c00, 0x0 },
       .decode = decode___cat3_src_relative_const_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat3_src_relative_const_gen_0_fields),
       .decode_fields = decode___cat3_src_relative_const_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat3_src_relative_const__default_gen_0,
       },
};
static const struct isa_case __instruction_cat3_base__case0_gen_0 = {
       .expr     = &expr___cat2_cat3_nop_encoding,
       .display  = "{SY}{SS}{JP}{SAT}(nop{NOP}) {UL}{NAME} {DST_HALF}{DST}, {SRC1_NEG}{SRC1}, {SRC2_NEG}{HALF}{SRC2}, {SRC3_NEG}{SRC3}",
       .num_fields = 1,
       .fields   = {
          { .name = "NOP", .low = 0, .high = 0,
            .expr = &expr___cat2_cat3_nop_value,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __instruction_cat3_base__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{SAT}{REPEAT}{UL}{NAME} {DST_HALF}{DST}, {SRC1_NEG}{SRC1_R}{SRC1}, {SRC2_NEG}{SRC2_R}{HALF}{SRC2}, {SRC3_NEG}{SRC3_R}{SRC3}",
       .num_fields = 12,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr___multisrc_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr___dest_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC2_R", .low = 15, .high = 15,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3_R", .low = 29, .high = 29,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "REPEAT", .low = 40, .high = 41,
            .type = TYPE_ENUM,
            .enums = &enum___rptn,
          },
          { .name = "SRC1_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "UL", .low = 45, .high = 45,
            .display = "(ul)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC2", .low = 47, .high = 54,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat3_base_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat3-base",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x60000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000000 },
       .decode = decode___instruction_cat3_base_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat3_base_gen_0_fields),
       .decode_fields = decode___instruction_cat3_base_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat3_base__case0_gen_0,
            &__instruction_cat3_base__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat3__default_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "HALF",  .as = "HALF" },
           { .name= "IMMED_ENCODING",  .as = "IMMED_ENCODING" },

       },
};
static const struct isa_field_params __instruction_cat3__default_gen_0_src3 = {
       .num_params = 2,
       .params = {
           { .name= "HALF",  .as = "HALF" },
           { .name= "IMMED_ENCODING",  .as = "IMMED_ENCODING" },

       },
};
static const struct isa_case __instruction_cat3__default_gen_0 = {
       .num_fields = 8,
       .fields   = {
          { .name = "IMMED_ENCODING", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 0, .high = 12,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3__default_gen_0_src1,
          },
          { .name = "SRC1_NEG", .low = 14, .high = 14,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3", .low = 16, .high = 28,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3__default_gen_0_src3,
          },
          { .name = "SRC2_NEG", .low = 30, .high = 30,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3_NEG", .low = 31, .high = 31,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SAT", .low = 42, .high = 42,
            .display = "(sat)",
            .type = TYPE_BOOL,
          },
          { .name = "DST_CONV", .low = 46, .high = 46,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat3_gen_0 = {

       .parent   = &bitset___instruction_cat3_base_gen_0,
       .name     = "#instruction-cat3",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x60000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe0000000 },
       .decode = decode___instruction_cat3_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat3_gen_0_fields),
       .decode_fields = decode___instruction_cat3_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat3__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat3_alt__default_gen_600_src1 = {
       .num_params = 2,
       .params = {
           { .name= "HALF",  .as = "HALF" },
           { .name= "IMMED_ENCODING",  .as = "IMMED_ENCODING" },

       },
};
static const struct isa_field_params __instruction_cat3_alt__default_gen_600_src3 = {
       .num_params = 2,
       .params = {
           { .name= "HALF",  .as = "HALF" },
           { .name= "IMMED_ENCODING",  .as = "IMMED_ENCODING" },

       },
};
static const struct isa_case __instruction_cat3_alt__default_gen_600 = {
       .num_fields = 9,
       .fields   = {
          { .name = "IMMED_ENCODING", .low = 0, .high = 0,
            .expr = &expr___true,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SAT", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 0, .high = 12,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3_alt__default_gen_600_src1,
          },
          { .name = "SRC1_NEG", .low = 14, .high = 14,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3", .low = 16, .high = 28,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3_alt__default_gen_600_src3,
          },
          { .name = "SRC2_NEG", .low = 30, .high = 30,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3_NEG", .low = 31, .high = 31,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "FULL", .low = 42, .high = 42,
            .type = TYPE_BOOL,
          },
          { .name = "DST_CONV", .low = 46, .high = 46,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat3_alt_gen_600 = {

       .parent   = &bitset___instruction_cat3_base_gen_0,
       .name     = "#instruction-cat3-alt",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x60000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe0000000 },
       .decode = decode___instruction_cat3_alt_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat3_alt_gen_600_fields),
       .decode_fields = decode___instruction_cat3_alt_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat3_alt__default_gen_600,
       },
};
static const struct isa_case mad_u16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mad_u16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "mad.u16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x60000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_mad_u16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mad_u16_gen_0_fields),
       .decode_fields = decode_mad_u16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mad_u16__default_gen_0,
       },
};
static const struct isa_case madsh_u16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_madsh_u16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "madsh.u16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x60800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_madsh_u16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_madsh_u16_gen_0_fields),
       .decode_fields = decode_madsh_u16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &madsh_u16__default_gen_0,
       },
};
static const struct isa_case mad_s16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mad_s16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "mad.s16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x61000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_mad_s16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mad_s16_gen_0_fields),
       .decode_fields = decode_mad_s16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mad_s16__default_gen_0,
       },
};
static const struct isa_case madsh_m16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_madsh_m16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "madsh.m16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x61800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_madsh_m16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_madsh_m16_gen_0_fields),
       .decode_fields = decode_madsh_m16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &madsh_m16__default_gen_0,
       },
};
static const struct isa_case mad_u24__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mad_u24_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "mad.u24",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x62000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_mad_u24_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mad_u24_gen_0_fields),
       .decode_fields = decode_mad_u24_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mad_u24__default_gen_0,
       },
};
static const struct isa_case mad_s24__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mad_s24_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "mad.s24",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x62800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_mad_s24_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mad_s24_gen_0_fields),
       .decode_fields = decode_mad_s24_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mad_s24__default_gen_0,
       },
};
static const struct isa_case mad_f16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mad_f16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "mad.f16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x63000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_mad_f16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mad_f16_gen_0_fields),
       .decode_fields = decode_mad_f16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mad_f16__default_gen_0,
       },
};
static const struct isa_case mad_f32__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_mad_f32_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "mad.f32",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x63800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_mad_f32_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_mad_f32_gen_0_fields),
       .decode_fields = decode_mad_f32_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &mad_f32__default_gen_0,
       },
};
static const struct isa_case sel_b16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sel_b16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sel.b16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x64000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sel_b16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sel_b16_gen_0_fields),
       .decode_fields = decode_sel_b16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sel_b16__default_gen_0,
       },
};
static const struct isa_case sel_b32__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sel_b32_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sel.b32",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x64800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sel_b32_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sel_b32_gen_0_fields),
       .decode_fields = decode_sel_b32_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sel_b32__default_gen_0,
       },
};
static const struct isa_case sel_s16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sel_s16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sel.s16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x65000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sel_s16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sel_s16_gen_0_fields),
       .decode_fields = decode_sel_s16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sel_s16__default_gen_0,
       },
};
static const struct isa_case sel_s32__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sel_s32_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sel.s32",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x65800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sel_s32_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sel_s32_gen_0_fields),
       .decode_fields = decode_sel_s32_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sel_s32__default_gen_0,
       },
};
static const struct isa_case sel_f16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sel_f16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sel.f16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x66000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sel_f16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sel_f16_gen_0_fields),
       .decode_fields = decode_sel_f16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sel_f16__default_gen_0,
       },
};
static const struct isa_case sel_f32__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sel_f32_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sel.f32",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x66800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sel_f32_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sel_f32_gen_0_fields),
       .decode_fields = decode_sel_f32_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sel_f32__default_gen_0,
       },
};
static const struct isa_case sad_s16__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sad_s16_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sad.s16",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x67000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sad_s16_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sad_s16_gen_0_fields),
       .decode_fields = decode_sad_s16_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sad_s16__default_gen_0,
       },
};
static const struct isa_case sad_s32__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sad_s32_gen_0 = {

       .parent   = &bitset___instruction_cat3_gen_0,
       .name     = "sad.s32",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x67800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_sad_s32_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sad_s32_gen_0_fields),
       .decode_fields = decode_sad_s32_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sad_s32__default_gen_0,
       },
};
static const struct isa_case shrm__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shrm_gen_0 = {

       .parent   = &bitset___instruction_cat3_alt_gen_600,
       .name     = "shrm",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x64000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_shrm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shrm_gen_0_fields),
       .decode_fields = decode_shrm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shrm__default_gen_0,
       },
};
static const struct isa_case shlm__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shlm_gen_0 = {

       .parent   = &bitset___instruction_cat3_alt_gen_600,
       .name     = "shlm",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x64800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_shlm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shlm_gen_0_fields),
       .decode_fields = decode_shlm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shlm__default_gen_0,
       },
};
static const struct isa_case shrg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shrg_gen_0 = {

       .parent   = &bitset___instruction_cat3_alt_gen_600,
       .name     = "shrg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x65000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_shrg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shrg_gen_0_fields),
       .decode_fields = decode_shrg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shrg__default_gen_0,
       },
};
static const struct isa_case shlg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_shlg_gen_0 = {

       .parent   = &bitset___instruction_cat3_alt_gen_600,
       .name     = "shlg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x65800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_shlg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_shlg_gen_0_fields),
       .decode_fields = decode_shlg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &shlg__default_gen_0,
       },
};
static const struct isa_case andg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_andg_gen_0 = {

       .parent   = &bitset___instruction_cat3_alt_gen_600,
       .name     = "andg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x66000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800000 },
       .decode = decode_andg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_andg_gen_0_fields),
       .decode_fields = decode_andg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &andg__default_gen_0,
       },
};
static const struct isa_case __instruction_cat3_dp__case0_gen_600 = {
       .expr     = &expr___cat2_cat3_nop_encoding,
       .display  = "{SY}{SS}{JP}{SAT}(nop{NOP}) {UL}{NAME}{SRC_SIGN}{SRC_PACK} {DST}, {SRC1}, {SRC2}, {SRC3_NEG}{SRC3_R}{SRC3}",
       .num_fields = 1,
       .fields   = {
          { .name = "NOP", .low = 0, .high = 0,
            .expr = &expr___cat2_cat3_nop_value,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_field_params __instruction_cat3_dp__default_gen_600_src1 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params __instruction_cat3_dp__default_gen_600_src3 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_case __instruction_cat3_dp__default_gen_600 = {
       .display  = "{SY}{SS}{JP}{SAT}{UL}{NAME}{SRC_SIGN}{SRC_PACK} {DST}, {SRC1_R}{SRC1}, {SRC2_R}{SRC2}, {SRC3_NEG}{SRC3_R}{SRC3}",
       .num_fields = 7,
       .fields   = {
          { .name = "FULL", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 0, .high = 12,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3_dp__default_gen_600_src1,
          },
          { .name = "SRC_SIGN", .low = 14, .high = 14,
            .type = TYPE_ENUM,
            .enums = &enum___signedness,
          },
          { .name = "SRC3", .low = 16, .high = 28,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3_dp__default_gen_600_src3,
          },
          { .name = "SRC_PACK", .low = 30, .high = 30,
            .type = TYPE_ENUM,
            .enums = &enum___8bitvec2pack,
          },
          { .name = "SRC3_NEG", .low = 31, .high = 31,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SAT", .low = 42, .high = 42,
            .display = "(sat)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat3_dp_gen_600 = {

       .parent   = &bitset___instruction_cat3_base_gen_0,
       .name     = "#instruction-cat3-dp",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x60000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe0000000 },
       .decode = decode___instruction_cat3_dp_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat3_dp_gen_600_fields),
       .decode_fields = decode___instruction_cat3_dp_gen_600_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat3_dp__case0_gen_600,
            &__instruction_cat3_dp__default_gen_600,
       },
};
static const struct isa_case dp2acc__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_dp2acc_gen_0 = {

       .parent   = &bitset___instruction_cat3_dp_gen_600,
       .name     = "dp2acc",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x66800000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7804000 },
       .decode = decode_dp2acc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dp2acc_gen_0_fields),
       .decode_fields = decode_dp2acc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dp2acc__default_gen_0,
       },
};
static const struct isa_case dp4acc__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_dp4acc_gen_0 = {

       .parent   = &bitset___instruction_cat3_dp_gen_600,
       .name     = "dp4acc",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x66804000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7804000 },
       .decode = decode_dp4acc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dp4acc_gen_0_fields),
       .decode_fields = decode_dp4acc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dp4acc__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat3_wmm__default_gen_600_src1 = {
       .num_params = 1,
       .params = {
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params __instruction_cat3_wmm__default_gen_600_src3 = {
       .num_params = 2,
       .params = {
           { .name= "HALF",  .as = "HALF" },
           { .name= "IMMED_ENCODING",  .as = "IMMED_ENCODING" },

       },
};
static const struct isa_case __instruction_cat3_wmm__default_gen_600 = {
       .num_fields = 10,
       .fields   = {
          { .name = "IMMED_ENCODING", .low = 0, .high = 0,
            .expr = &expr___true,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SAT", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3_NEG", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr___wmm_dest_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 0, .high = 12,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3_wmm__default_gen_600_src1,
          },
          { .name = "SRC1_NEG", .low = 14, .high = 14,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC3", .low = 16, .high = 28,
            .type = TYPE_BITSET,
            .bitsets = __cat3_src,
            .params = &__instruction_cat3_wmm__default_gen_600_src3,
          },
          { .name = "SRC2_NEG", .low = 30, .high = 30,
            .display = "(neg)",
            .type = TYPE_BOOL,
          },
          { .name = "FULL", .low = 31, .high = 31,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "DST_FULL", .low = 46, .high = 46,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat3_wmm_gen_600 = {

       .parent   = &bitset___instruction_cat3_base_gen_0,
       .name     = "#instruction-cat3-wmm",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x60000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe0000000 },
       .decode = decode___instruction_cat3_wmm_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat3_wmm_gen_600_fields),
       .decode_fields = decode___instruction_cat3_wmm_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat3_wmm__default_gen_600,
       },
};
static const struct isa_case wmm__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_wmm_gen_0 = {

       .parent   = &bitset___instruction_cat3_wmm_gen_600,
       .name     = "wmm",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x67000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800400 },
       .decode = decode_wmm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_wmm_gen_0_fields),
       .decode_fields = decode_wmm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &wmm__default_gen_0,
       },
};
static const struct isa_case wmm_accu__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_wmm_accu_gen_0 = {

       .parent   = &bitset___instruction_cat3_wmm_gen_600,
       .name     = "wmm.accu",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x2000, 0x67000400 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x2000, 0xe7800400 },
       .decode = decode_wmm_accu_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_wmm_accu_gen_0_fields),
       .decode_fields = decode_wmm_accu_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &wmm_accu__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat4__default_gen_0_src = {
       .num_params = 2,
       .params = {
           { .name= "SRC_R",  .as = "SRC_R" },
           { .name= "FULL",  .as = "FULL" },

       },
};
static const struct isa_case __instruction_cat4__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{SAT}{REPEAT}{UL}{NAME} {DST_HALF}{DST}, {SRC}",
       .num_fields = 12,
       .fields   = {
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr___dest_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 15,
            .type = TYPE_BITSET,
            .bitsets = __multisrc,
            .params = &__instruction_cat4__default_gen_0_src,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "REPEAT", .low = 40, .high = 41,
            .type = TYPE_ENUM,
            .enums = &enum___rptn,
          },
          { .name = "SAT", .low = 42, .high = 42,
            .display = "(sat)",
            .type = TYPE_BOOL,
          },
          { .name = "SRC_R", .low = 43, .high = 43,
            .display = "(r)",
            .type = TYPE_BOOL,
          },
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "UL", .low = 45, .high = 45,
            .display = "(ul)",
            .type = TYPE_BOOL,
          },
          { .name = "DST_CONV", .low = 46, .high = 46,
            .type = TYPE_BOOL,
          },
          { .name = "FULL", .low = 52, .high = 52,
            .type = TYPE_BOOL,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat4_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat4",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80000000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe00f8000 },
       .decode = decode___instruction_cat4_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat4_gen_0_fields),
       .decode_fields = decode___instruction_cat4_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat4__default_gen_0,
       },
};
static const struct isa_case rcp__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_rcp_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "rcp",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80000000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_rcp_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_rcp_gen_0_fields),
       .decode_fields = decode_rcp_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &rcp__default_gen_0,
       },
};
static const struct isa_case rsq__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_rsq_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "rsq",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80200000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_rsq_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_rsq_gen_0_fields),
       .decode_fields = decode_rsq_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &rsq__default_gen_0,
       },
};
static const struct isa_case log2__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_log2_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "log2",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80400000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_log2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_log2_gen_0_fields),
       .decode_fields = decode_log2_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &log2__default_gen_0,
       },
};
static const struct isa_case exp2__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_exp2_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "exp2",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80600000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_exp2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_exp2_gen_0_fields),
       .decode_fields = decode_exp2_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &exp2__default_gen_0,
       },
};
static const struct isa_case sin__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_sin_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "sin",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80800000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_sin_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sin_gen_0_fields),
       .decode_fields = decode_sin_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sin__default_gen_0,
       },
};
static const struct isa_case cos__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_cos_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "cos",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80a00000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_cos_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_cos_gen_0_fields),
       .decode_fields = decode_cos_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &cos__default_gen_0,
       },
};
static const struct isa_case sqrt__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_sqrt_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "sqrt",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x80c00000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_sqrt_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sqrt_gen_0_fields),
       .decode_fields = decode_sqrt_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sqrt__default_gen_0,
       },
};
static const struct isa_case hrsq__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_hrsq_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "hrsq",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x81200000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_hrsq_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_hrsq_gen_0_fields),
       .decode_fields = decode_hrsq_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &hrsq__default_gen_0,
       },
};
static const struct isa_case hlog2__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_hlog2_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "hlog2",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x81400000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_hlog2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_hlog2_gen_0_fields),
       .decode_fields = decode_hlog2_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &hlog2__default_gen_0,
       },
};
static const struct isa_case hexp2__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_hexp2_gen_0 = {

       .parent   = &bitset___instruction_cat4_gen_0,
       .name     = "hexp2",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x81600000 },
       .dontcare.bitset = { 0xffff0000, 0xf8000 },
       .mask.bitset     = { 0xffff0000, 0xe7ef8000 },
       .decode = decode_hexp2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_hexp2_gen_0_fields),
       .decode_fields = decode_hexp2_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &hexp2__default_gen_0,
       },
};
static const struct isa_case __cat5_s2en_bindless_base__case0_gen_0 = {
       .expr     = &expr_anon_16,
       .display  = ".base{BASE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __cat5_s2en_bindless_base__default_gen_0 = {
       .display  = "",
       .num_fields = 2,
       .fields   = {
          { .name = "BASE", .low = 0, .high = 0,
            .expr = &expr_anon_15,
            .type = TYPE_UINT,
          },
          { .name = "BASE_LO", .low = 0, .high = 0,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___cat5_s2en_bindless_base_gen_0 = {

       .name     = "#cat5-s2en-bindless-base",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_s2en_bindless_base_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_s2en_bindless_base_gen_0_fields),
       .decode_fields = decode___cat5_s2en_bindless_base_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_s2en_bindless_base__case0_gen_0,
            &__cat5_s2en_bindless_base__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat5__default_gen_0_src1 = {
       .num_params = 2,
       .params = {
           { .name= "NUM_SRC",  .as = "NUM_SRC" },
           { .name= "HALF",  .as = "HALF" },

       },
};
static const struct isa_field_params __instruction_cat5__default_gen_0_src2 = {
       .num_params = 4,
       .params = {
           { .name= "NUM_SRC",  .as = "NUM_SRC" },
           { .name= "HALF",  .as = "HALF" },
           { .name= "O",  .as = "O" },
           { .name= "SRC2_IMM_OFFSET",  .as = "SRC2_IMM_OFFSET" },

       },
};
static const struct isa_field_params __instruction_cat5__default_gen_0_samp = {
       .num_params = 1,
       .params = {
           { .name= "HAS_SAMP",  .as = "HAS_SAMP" },

       },
};
static const struct isa_field_params __instruction_cat5__default_gen_0_tex = {
       .num_params = 1,
       .params = {
           { .name= "HAS_TEX",  .as = "HAS_TEX" },

       },
};
static const struct isa_field_params __instruction_cat5__default_gen_0_type = {
       .num_params = 1,
       .params = {
           { .name= "HAS_TYPE",  .as = "HAS_TYPE" },

       },
};
static const struct isa_case __instruction_cat5__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}{3D}{A}{O}{P}{SV}{1D} {TYPE}({WRMASK}){DST_HALF}{DST}{SRC1}{SRC2}{SAMP}{TEX}",
       .num_fields = 20,
       .fields   = {
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr___type_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr___multisrc_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC2_IMM_OFFSET", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "P", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "1D", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "#instruction-cat5#assert5", .low = 47, .high = 47,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
          { .name = "FULL", .low = 0, .high = 0,
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __cat5_src1,
            .params = &__instruction_cat5__default_gen_0_src1,
          },
          { .name = "SRC2", .low = 9, .high = 16,
            .type = TYPE_BITSET,
            .bitsets = __cat5_src2,
            .params = &__instruction_cat5__default_gen_0_src2,
          },
          { .name = "SAMP", .low = 21, .high = 24,
            .type = TYPE_BITSET,
            .bitsets = __cat5_samp,
            .params = &__instruction_cat5__default_gen_0_samp,
          },
          { .name = "TEX", .low = 25, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat5_tex,
            .params = &__instruction_cat5__default_gen_0_tex,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "WRMASK", .low = 40, .high = 43,
            .type = TYPE_ENUM,
            .enums = &enum___wrmask,
          },
          { .name = "TYPE", .low = 44, .high = 46,
            .type = TYPE_BITSET,
            .bitsets = __cat5_type,
            .params = &__instruction_cat5__default_gen_0_type,
          },
          { .name = "3D", .low = 48, .high = 48,
            .display = ".3d",
            .type = TYPE_BOOL,
          },
          { .name = "A", .low = 49, .high = 49,
            .display = ".a",
            .type = TYPE_BOOL,
          },
          { .name = "S2EN_BINDLESS", .low = 51, .high = 51,
            .type = TYPE_BOOL,
          },
          { .name = "O", .low = 52, .high = 52,
            .display = ".o",
            .type = TYPE_BOOL,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat5_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat5",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x20000, 0xe0000000 },
       .decode = decode___instruction_cat5_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat5_gen_0_fields),
       .decode_fields = decode___instruction_cat5_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat5__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat5_tex_base__case0_gen_0_src3 = {
       .num_params = 4,
       .params = {
           { .name= "BINDLESS",  .as = "BINDLESS" },
           { .name= "DESC_MODE",  .as = "DESC_MODE" },
           { .name= "HAS_SAMP",  .as = "HAS_SAMP" },
           { .name= "HAS_TEX",  .as = "HAS_TEX" },

       },
};
static const struct isa_field_params __instruction_cat5_tex_base__case0_gen_0_base = {
       .num_params = 2,
       .params = {
           { .name= "BINDLESS",  .as = "BINDLESS" },
           { .name= "BASE_HI",  .as = "BASE_HI" },

       },
};
static const struct isa_case __instruction_cat5_tex_base__case0_gen_0 = {
       .expr     = &expr_anon_17,
       .display  = "{SY}{JP}{NAME}{3D}{A}{O}{P}{SV}{S2EN}{UNIFORM}{NONUNIFORM}{BASE}{1D} {TYPE}({WRMASK}){DST_HALF}{DST}{SRC1}{SRC2}{SRC3}{A1}",
       .num_fields = 9,
       .fields   = {
          { .name = "BINDLESS", .low = 0, .high = 0,
            .expr = &expr___cat5_s2enb_is_bindless,
            .type = TYPE_BOOL,
          },
          { .name = "S2EN", .low = 0, .high = 0,
            .expr = &expr___cat5_s2enb_is_indirect,
            .display = ".s2en",
            .type = TYPE_BOOL,
          },
          { .name = "UNIFORM", .low = 0, .high = 0,
            .expr = &expr___cat5_s2enb_is_uniform,
            .display = ".uniform",
            .type = TYPE_BOOL,
          },
          { .name = "NONUNIFORM", .low = 0, .high = 0,
            .expr = &expr___cat5_s2enb_is_nonuniform,
            .display = ".nonuniform",
            .type = TYPE_BOOL,
          },
          { .name = "A1", .low = 0, .high = 0,
            .expr = &expr___cat5_s2enb_uses_a1,
            .display = ", a1.x",
            .type = TYPE_BOOL,
          },
          { .name = "BASE_HI", .low = 19, .high = 20,
            .type = TYPE_UINT,
          },
          { .name = "SRC3", .low = 21, .high = 28,
            .type = TYPE_BITSET,
            .bitsets = __cat5_src3,
            .params = &__instruction_cat5_tex_base__case0_gen_0_src3,
          },
          { .name = "DESC_MODE", .low = 29, .high = 31,
            .type = TYPE_ENUM,
            .enums = &enum___cat5_s2en_bindless_desc_mode,
          },
          { .name = "BASE", .low = 47, .high = 47,
            .type = TYPE_BITSET,
            .bitsets = __cat5_s2en_bindless_base,
            .params = &__instruction_cat5_tex_base__case0_gen_0_base,
          },
       },
};
static const struct isa_case __instruction_cat5_tex_base__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "#instruction-cat5-tex-base#assert0", .low = 19, .high = 20,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___instruction_cat5_tex_base_gen_0 = {

       .parent   = &bitset___instruction_cat5_gen_0,
       .name     = "#instruction-cat5-tex-base",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x20000, 0xe0000000 },
       .decode = decode___instruction_cat5_tex_base_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat5_tex_base_gen_0_fields),
       .decode_fields = decode___instruction_cat5_tex_base_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat5_tex_base__case0_gen_0,
            &__instruction_cat5_tex_base__default_gen_0,
       },
};
static const struct isa_case __instruction_cat5_tex__default_gen_0 = {
       .num_fields = 2,
       .fields   = {
          { .name = "SV", .low = 50, .high = 50,
            .display = ".s",
            .type = TYPE_BOOL,
          },
          { .name = "P", .low = 53, .high = 53,
            .display = ".p",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat5_tex_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_base_gen_0,
       .name     = "#instruction-cat5-tex",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe0000000 },
       .decode = decode___instruction_cat5_tex_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat5_tex_gen_0_fields),
       .decode_fields = decode___instruction_cat5_tex_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat5_tex__default_gen_0,
       },
};
static const struct isa_case isam__default_gen_0 = {
       .num_fields = 7,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "1D", .low = 18, .high = 18,
            .display = ".1d",
            .type = TYPE_BOOL_INV,
          },
          { .name = "SV", .low = 50, .high = 50,
            .display = ".v",
            .type = TYPE_BOOL,
          },
          { .name = "SRC2_IMM_OFFSET", .low = 53, .high = 53,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_isam_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_base_gen_0,
       .name     = "isam",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x20000, 0xe7c00000 },
       .decode = decode_isam_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_isam_gen_0_fields),
       .decode_fields = decode_isam_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &isam__default_gen_0,
       },
};
static const struct isa_case isaml__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___two,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_isaml_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "isaml",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_isaml_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_isaml_gen_0_fields),
       .decode_fields = decode_isaml_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &isaml__default_gen_0,
       },
};
static const struct isa_case isamm__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_isamm_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "isamm",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_isamm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_isamm_gen_0_fields),
       .decode_fields = decode_isamm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &isamm__default_gen_0,
       },
};
static const struct isa_case sam__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_sam_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "sam",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_sam_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sam_gen_0_fields),
       .decode_fields = decode_sam_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sam__default_gen_0,
       },
};
static const struct isa_case samb__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___two,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_samb_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "samb",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa1000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_samb_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_samb_gen_0_fields),
       .decode_fields = decode_samb_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &samb__default_gen_0,
       },
};
static const struct isa_case saml__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___two,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_saml_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "saml",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa1400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_saml_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_saml_gen_0_fields),
       .decode_fields = decode_saml_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &saml__default_gen_0,
       },
};
static const struct isa_case samgq__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_samgq_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "samgq",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa1800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_samgq_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_samgq_gen_0_fields),
       .decode_fields = decode_samgq_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &samgq__default_gen_0,
       },
};
static const struct isa_case getlod__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_getlod_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "getlod",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa1c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_getlod_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getlod_gen_0_fields),
       .decode_fields = decode_getlod_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getlod__default_gen_0,
       },
};
static const struct isa_case conv__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___two,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_conv_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "conv",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa2000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_conv_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_conv_gen_0_fields),
       .decode_fields = decode_conv_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &conv__default_gen_0,
       },
};
static const struct isa_case convm__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___two,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_convm_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "convm",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa2400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_convm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_convm_gen_0_fields),
       .decode_fields = decode_convm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &convm__default_gen_0,
       },
};
static const struct isa_case getsize__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_getsize_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "getsize",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa2800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_getsize_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getsize_gen_0_fields),
       .decode_fields = decode_getsize_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getsize__default_gen_0,
       },
};
static const struct isa_case getbuf__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___zero,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_getbuf_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "getbuf",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa2c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_getbuf_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getbuf_gen_0_fields),
       .decode_fields = decode_getbuf_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getbuf__default_gen_0,
       },
};
static const struct isa_case getpos__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_getpos_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "getpos",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa3000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_getpos_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getpos_gen_0_fields),
       .decode_fields = decode_getpos_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getpos__default_gen_0,
       },
};
static const struct isa_case getinfo__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___zero,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_getinfo_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "getinfo",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa3400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_getinfo_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getinfo_gen_0_fields),
       .decode_fields = decode_getinfo_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getinfo__default_gen_0,
       },
};
static const struct isa_case dsx__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_dsx_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "dsx",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa3800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_dsx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dsx_gen_0_fields),
       .decode_fields = decode_dsx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dsx__default_gen_0,
       },
};
static const struct isa_case dsy__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_dsy_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "dsy",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa3c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_dsy_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dsy_gen_0_fields),
       .decode_fields = decode_dsy_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dsy__default_gen_0,
       },
};
static const struct isa_case gather4r__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_gather4r_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "gather4r",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa4000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_gather4r_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_gather4r_gen_0_fields),
       .decode_fields = decode_gather4r_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &gather4r__default_gen_0,
       },
};
static const struct isa_case gather4g__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_gather4g_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "gather4g",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa4400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_gather4g_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_gather4g_gen_0_fields),
       .decode_fields = decode_gather4g_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &gather4g__default_gen_0,
       },
};
static const struct isa_case gather4b__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_gather4b_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "gather4b",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa4800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_gather4b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_gather4b_gen_0_fields),
       .decode_fields = decode_gather4b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &gather4b__default_gen_0,
       },
};
static const struct isa_case gather4a__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_gather4a_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "gather4a",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa4c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_gather4a_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_gather4a_gen_0_fields),
       .decode_fields = decode_gather4a_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &gather4a__default_gen_0,
       },
};
static const struct isa_case samgp0__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_samgp0_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "samgp0",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa5000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_samgp0_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_samgp0_gen_0_fields),
       .decode_fields = decode_samgp0_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &samgp0__default_gen_0,
       },
};
static const struct isa_case samgp1__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_samgp1_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "samgp1",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa5400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_samgp1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_samgp1_gen_0_fields),
       .decode_fields = decode_samgp1_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &samgp1__default_gen_0,
       },
};
static const struct isa_case samgp2__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_samgp2_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "samgp2",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa5800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_samgp2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_samgp2_gen_0_fields),
       .decode_fields = decode_samgp2_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &samgp2__default_gen_0,
       },
};
static const struct isa_case samgp3__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_samgp3_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "samgp3",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa5c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_samgp3_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_samgp3_gen_0_fields),
       .decode_fields = decode_samgp3_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &samgp3__default_gen_0,
       },
};
static const struct isa_case dsxpp_1__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_dsxpp_1_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "dsxpp.1",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa6000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_dsxpp_1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dsxpp_1_gen_0_fields),
       .decode_fields = decode_dsxpp_1_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dsxpp_1__default_gen_0,
       },
};
static const struct isa_case dsypp_1__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_dsypp_1_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "dsypp.1",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa6400000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_dsypp_1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dsypp_1_gen_0_fields),
       .decode_fields = decode_dsypp_1_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dsypp_1__default_gen_0,
       },
};
static const struct isa_case rgetpos__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_rgetpos_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "rgetpos",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa6800000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_rgetpos_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_rgetpos_gen_0_fields),
       .decode_fields = decode_rgetpos_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &rgetpos__default_gen_0,
       },
};
static const struct isa_case rgetinfo__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___zero,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_rgetinfo_gen_0 = {

       .parent   = &bitset___instruction_cat5_tex_gen_0,
       .name     = "rgetinfo",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa6c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7c00000 },
       .decode = decode_rgetinfo_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_rgetinfo_gen_0_fields),
       .decode_fields = decode_rgetinfo_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &rgetinfo__default_gen_0,
       },
};
static const struct isa_case tcinv__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}",
       .num_fields = 2,
       .fields   = {
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_tcinv_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "tcinv",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa7000000 },
       .dontcare.bitset = { 0xffffffff, 0x3fffff },
       .mask.bitset     = { 0xffffffff, 0xe7ffffff },
       .decode = decode_tcinv_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_tcinv_gen_0_fields),
       .decode_fields = decode_tcinv_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &tcinv__default_gen_0,
       },
};
static const struct isa_case __instruction_cat5_brcst__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat5_brcst_gen_0 = {

       .parent   = &bitset___instruction_cat5_gen_0,
       .name     = "#instruction-cat5-brcst",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa0000000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe0040000 },
       .decode = decode___instruction_cat5_brcst_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat5_brcst_gen_0_fields),
       .decode_fields = decode___instruction_cat5_brcst_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat5_brcst__default_gen_0,
       },
};
static const struct isa_case brcst_active__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.w{CLUSTER_SIZE} {TYPE}({WRMASK}){DST_HALF}{DST}{SRC1}",
       .num_fields = 6,
       .fields   = {
          { .name = "CLUSTER_SIZE", .low = 0, .high = 0,
            .expr = &expr_anon_18,
            .type = TYPE_UINT,
          },
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "W", .low = 19, .high = 20,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_brcst_active_gen_600 = {

       .parent   = &bitset___instruction_cat5_brcst_gen_0,
       .name     = "brcst.active",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa7c00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7e40000 },
       .decode = decode_brcst_active_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_brcst_active_gen_600_fields),
       .decode_fields = decode_brcst_active_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &brcst_active__default_gen_600,
       },
};
static const struct isa_case __instruction_cat5_quad_shuffle__default_gen_600 = {
       .display  = "{SY}{JP}{NAME} {TYPE}({WRMASK}){DST_HALF}{DST}{SRC1}{SRC2}",
       .num_fields = 3,
       .fields   = {
          { .name = "HAS_SAMP", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TEX", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "HAS_TYPE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat5_quad_shuffle_gen_600 = {

       .parent   = &bitset___instruction_cat5_brcst_gen_0,
       .name     = "#instruction-cat5-quad-shuffle",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa7e00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x60000, 0xe7e40000 },
       .decode = decode___instruction_cat5_quad_shuffle_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat5_quad_shuffle_gen_600_fields),
       .decode_fields = decode___instruction_cat5_quad_shuffle_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat5_quad_shuffle__default_gen_600,
       },
};
static const struct isa_case quad_shuffle_brcst__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___two,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_quad_shuffle_brcst_gen_0 = {

       .parent   = &bitset___instruction_cat5_quad_shuffle_gen_600,
       .name     = "quad_shuffle.brcst",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xa7e00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x1e0000, 0xe7e40000 },
       .decode = decode_quad_shuffle_brcst_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_quad_shuffle_brcst_gen_0_fields),
       .decode_fields = decode_quad_shuffle_brcst_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &quad_shuffle_brcst__default_gen_0,
       },
};
static const struct isa_case quad_shuffle_horiz__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_quad_shuffle_horiz_gen_0 = {

       .parent   = &bitset___instruction_cat5_quad_shuffle_gen_600,
       .name     = "quad_shuffle.horiz",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x80000, 0xa7e00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x1e0000, 0xe7e40000 },
       .decode = decode_quad_shuffle_horiz_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_quad_shuffle_horiz_gen_0_fields),
       .decode_fields = decode_quad_shuffle_horiz_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &quad_shuffle_horiz__default_gen_0,
       },
};
static const struct isa_case quad_shuffle_vert__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_quad_shuffle_vert_gen_0 = {

       .parent   = &bitset___instruction_cat5_quad_shuffle_gen_600,
       .name     = "quad_shuffle.vert",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x100000, 0xa7e00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x1e0000, 0xe7e40000 },
       .decode = decode_quad_shuffle_vert_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_quad_shuffle_vert_gen_0_fields),
       .decode_fields = decode_quad_shuffle_vert_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &quad_shuffle_vert__default_gen_0,
       },
};
static const struct isa_case quad_shuffle_diag__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "NUM_SRC", .low = 0, .high = 0,
            .expr = &expr___one,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_quad_shuffle_diag_gen_0 = {

       .parent   = &bitset___instruction_cat5_quad_shuffle_gen_600,
       .name     = "quad_shuffle.diag",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x180000, 0xa7e00000 },
       .dontcare.bitset = { 0x20000, 0x0 },
       .mask.bitset     = { 0x1e0000, 0xe7e40000 },
       .decode = decode_quad_shuffle_diag_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_quad_shuffle_diag_gen_0_fields),
       .decode_fields = decode_quad_shuffle_diag_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &quad_shuffle_diag__default_gen_0,
       },
};
static const struct isa_case __cat5_src1__case0_gen_0 = {
       .expr     = &expr_anon_19,
       .display  = ", {HALF}{SRC}",
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_case __cat5_src1__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-src1#assert0", .low = 0, .high = 7,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_src1_gen_0 = {

       .name     = "#cat5-src1",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_src1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_src1_gen_0_fields),
       .decode_fields = decode___cat5_src1_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_src1__case0_gen_0,
            &__cat5_src1__default_gen_0,
       },
};
static const struct isa_case __cat5_src2__case0_gen_0 = {
       .expr     = &expr_anon_20,
       .display  = ", {HALF}{SRC}",
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_case __cat5_src2__case1_gen_0 = {
       .expr     = &expr_anon_21,
       .display  = "{OFF}",
       .num_fields = 1,
       .fields   = {
          { .name = "OFF", .low = 0, .high = 7,
            .type = TYPE_UOFFSET,
          },
       },
};
static const struct isa_case __cat5_src2__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-src2#assert0", .low = 0, .high = 7,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_src2_gen_0 = {

       .name     = "#cat5-src2",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_src2_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_src2_gen_0_fields),
       .decode_fields = decode___cat5_src2_gen_0_fields,
       .num_cases = 3,
       .cases    = {
            &__cat5_src2__case0_gen_0,
            &__cat5_src2__case1_gen_0,
            &__cat5_src2__default_gen_0,
       },
};
static const struct isa_case __cat5_samp__case0_gen_0 = {
       .expr     = &expr_anon_22,
       .display  = ", s#{SAMP}",
       .num_fields = 1,
       .fields   = {
          { .name = "SAMP", .low = 0, .high = 3,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat5_samp__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-samp#assert0", .low = 0, .high = 3,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_samp_gen_0 = {

       .name     = "#cat5-samp",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_samp_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_samp_gen_0_fields),
       .decode_fields = decode___cat5_samp_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_samp__case0_gen_0,
            &__cat5_samp__default_gen_0,
       },
};
static const struct isa_case __cat5_samp_s2en_bindless_a1__case0_gen_0 = {
       .expr     = &expr_anon_23,
       .display  = ", s#{SAMP}",
       .num_fields = 1,
       .fields   = {
          { .name = "SAMP", .low = 0, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat5_samp_s2en_bindless_a1__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-samp-s2en-bindless-a1#assert0", .low = 0, .high = 7,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_samp_s2en_bindless_a1_gen_0 = {

       .name     = "#cat5-samp-s2en-bindless-a1",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_samp_s2en_bindless_a1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_samp_s2en_bindless_a1_gen_0_fields),
       .decode_fields = decode___cat5_samp_s2en_bindless_a1_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_samp_s2en_bindless_a1__case0_gen_0,
            &__cat5_samp_s2en_bindless_a1__default_gen_0,
       },
};
static const struct isa_case __cat5_tex_s2en_bindless_a1__case0_gen_0 = {
       .expr     = &expr_anon_24,
       .display  = ", t#{TEX}",
       .num_fields = 1,
       .fields   = {
          { .name = "TEX", .low = 0, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat5_tex_s2en_bindless_a1__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-tex-s2en-bindless-a1#assert0", .low = 0, .high = 7,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_tex_s2en_bindless_a1_gen_0 = {

       .name     = "#cat5-tex-s2en-bindless-a1",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_tex_s2en_bindless_a1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_tex_s2en_bindless_a1_gen_0_fields),
       .decode_fields = decode___cat5_tex_s2en_bindless_a1_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_tex_s2en_bindless_a1__case0_gen_0,
            &__cat5_tex_s2en_bindless_a1__default_gen_0,
       },
};
static const struct isa_case __cat5_tex__case0_gen_0 = {
       .expr     = &expr_anon_25,
       .display  = ", t#{TEX}",
       .num_fields = 1,
       .fields   = {
          { .name = "TEX", .low = 0, .high = 6,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat5_tex__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-tex#assert0", .low = 0, .high = 6,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_tex_gen_0 = {

       .name     = "#cat5-tex",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_tex_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_tex_gen_0_fields),
       .decode_fields = decode___cat5_tex_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_tex__case0_gen_0,
            &__cat5_tex__default_gen_0,
       },
};
static const struct isa_case __cat5_tex_s2en_bindless__case0_gen_0 = {
       .expr     = &expr_anon_26,
       .display  = ", t#{TEX}",
       .num_fields = 1,
       .fields   = {
          { .name = "TEX", .low = 0, .high = 3,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat5_tex_s2en_bindless__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "#cat5-tex-s2en-bindless#assert0", .low = 0, .high = 3,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_bitset bitset___cat5_tex_s2en_bindless_gen_0 = {

       .name     = "#cat5-tex-s2en-bindless",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_tex_s2en_bindless_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_tex_s2en_bindless_gen_0_fields),
       .decode_fields = decode___cat5_tex_s2en_bindless_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_tex_s2en_bindless__case0_gen_0,
            &__cat5_tex_s2en_bindless__default_gen_0,
       },
};
static const struct isa_case __cat5_type__case0_gen_0 = {
       .expr     = &expr_anon_27,
       .display  = "({TYPE})",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __cat5_type__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "TYPE", .low = 0, .high = 2,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset___cat5_type_gen_0 = {

       .name     = "#cat5-type",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_type_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_type_gen_0_fields),
       .decode_fields = decode___cat5_type_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat5_type__case0_gen_0,
            &__cat5_type__default_gen_0,
       },
};
static const struct isa_case __cat5_src3__case0_gen_0 = {
       .expr     = &expr___cat5_s2enb_is_indirect,
       .display  = ", {SRC_HALF}{SRC}",
       .num_fields = 2,
       .fields   = {
          { .name = "SRC_HALF", .low = 0, .high = 0,
            .expr = &expr_anon_28,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_field_params __cat5_src3__case1_gen_0_samp = {
       .num_params = 1,
       .params = {
           { .name= "HAS_SAMP",  .as = "HAS_SAMP" },

       },
};
static const struct isa_case __cat5_src3__case1_gen_0 = {
       .expr     = &expr___cat5_s2enb_uses_a1_gen6,
       .display  = "{SAMP}",
       .num_fields = 1,
       .fields   = {
          { .name = "SAMP", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __cat5_samp_s2en_bindless_a1,
            .params = &__cat5_src3__case1_gen_0_samp,
          },
       },
};
static const struct isa_field_params __cat5_src3__case2_gen_0_tex = {
       .num_params = 1,
       .params = {
           { .name= "HAS_TEX",  .as = "HAS_TEX" },

       },
};
static const struct isa_case __cat5_src3__case2_gen_0 = {
       .expr     = &expr___cat5_s2enb_uses_a1_gen7,
       .display  = "{TEX}",
       .num_fields = 1,
       .fields   = {
          { .name = "TEX", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __cat5_tex_s2en_bindless_a1,
            .params = &__cat5_src3__case2_gen_0_tex,
          },
       },
};
static const struct isa_field_params __cat5_src3__default_gen_0_samp = {
       .num_params = 1,
       .params = {
           { .name= "HAS_SAMP",  .as = "HAS_SAMP" },

       },
};
static const struct isa_field_params __cat5_src3__default_gen_0_tex = {
       .num_params = 1,
       .params = {
           { .name= "HAS_TEX",  .as = "HAS_TEX" },

       },
};
static const struct isa_case __cat5_src3__default_gen_0 = {
       .display  = "{SAMP}{TEX}",
       .num_fields = 2,
       .fields   = {
          { .name = "SAMP", .low = 0, .high = 3,
            .type = TYPE_BITSET,
            .bitsets = __cat5_samp,
            .params = &__cat5_src3__default_gen_0_samp,
          },
          { .name = "TEX", .low = 4, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __cat5_tex_s2en_bindless,
            .params = &__cat5_src3__default_gen_0_tex,
          },
       },
};
static const struct isa_bitset bitset___cat5_src3_gen_0 = {

       .name     = "#cat5-src3",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat5_src3_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat5_src3_gen_0_fields),
       .decode_fields = decode___cat5_src3_gen_0_fields,
       .num_cases = 4,
       .cases    = {
            &__cat5_src3__case0_gen_0,
            &__cat5_src3__case1_gen_0,
            &__cat5_src3__case2_gen_0,
            &__cat5_src3__default_gen_0,
       },
};
static const struct isa_case __const_dst_imm__default_gen_0 = {
       .display  = "{OFFSET}",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___const_dst_imm_gen_0 = {

       .parent   = &bitset___const_dst_gen_0,
       .name     = "#const-dst-imm",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x100, 0x0 },
       .decode = decode___const_dst_imm_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___const_dst_imm_gen_0_fields),
       .decode_fields = decode___const_dst_imm_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__const_dst_imm__default_gen_0,
       },
};
static const struct isa_case __const_dst_a1__default_gen_0 = {
       .display  = "a1.x{OFFSET}",
       .num_fields = 1,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 7,
            .type = TYPE_UOFFSET,
          },
       },
};
static const struct isa_bitset bitset___const_dst_a1_gen_0 = {

       .parent   = &bitset___const_dst_gen_0,
       .name     = "#const-dst-a1",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x100, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x100, 0x0 },
       .decode = decode___const_dst_a1_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___const_dst_a1_gen_0_fields),
       .decode_fields = decode___const_dst_a1_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__const_dst_a1__default_gen_0,
       },
};
static const struct isa_case __const_dst__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___const_dst_gen_0 = {

       .name     = "#const-dst",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___const_dst_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___const_dst_gen_0_fields),
       .decode_fields = decode___const_dst_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__const_dst__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6__default_gen_0 = {
       .num_fields = 3,
       .fields   = {
          { .name = "TYPE_HALF", .low = 0, .high = 0,
            .expr = &expr___type_half,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat6",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000000 },
       .decode = decode___instruction_cat6_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_gen_0_fields),
       .decode_fields = decode___instruction_cat6_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_gen_0 = {

       .parent   = &bitset___instruction_cat6_gen_0,
       .name     = "#instruction-cat6-a3xx",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000000 },
       .decode = decode___instruction_cat6_a3xx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_ldg__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_CONST",  .as = "SRC_CONST" },

       },
};
static const struct isa_case __instruction_cat6_ldg__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "SRC1_CONST", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 14, .high = 21,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src_const_or_gpr,
            .params = &__instruction_cat6_ldg__default_gen_0_src1,
          },
          { .name = "SIZE", .low = 24, .high = 26,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_ldg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "#instruction-cat6-ldg",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800001, 0xc0000000 },
       .dontcare.bitset = { 0x38000000, 0x100 },
       .mask.bitset     = { 0xf8800001, 0xe7f00100 },
       .decode = decode___instruction_cat6_ldg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_ldg_gen_0_fields),
       .decode_fields = decode___instruction_cat6_ldg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_ldg__default_gen_0,
       },
};
static const struct isa_case ldg__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, g[{SRC1}{OFF}], {SIZE}",
       .num_fields = 1,
       .fields   = {
          { .name = "OFF", .low = 1, .high = 13,
            .type = TYPE_OFFSET,
          },
       },
};
static const struct isa_bitset bitset_ldg_gen_0 = {

       .parent   = &bitset___instruction_cat6_ldg_gen_0,
       .name     = "ldg",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800001, 0xc0000000 },
       .dontcare.bitset = { 0x38000000, 0x1ff00 },
       .mask.bitset     = { 0xf8c00001, 0xe7f1ff00 },
       .decode = decode_ldg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldg_gen_0_fields),
       .decode_fields = decode_ldg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldg__default_gen_0,
       },
};
static const struct isa_field_params ldg_k__default_gen_600_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_CONST",  .as = "SRC_CONST" },

       },
};
static const struct isa_case ldg_k__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} c[{DST}], g[{SRC1}{OFF}], {SIZE}",
       .num_fields = 5,
       .fields   = {
          { .name = "SRC1_CONST", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "OFF", .low = 1, .high = 13,
            .type = TYPE_OFFSET,
          },
          { .name = "SRC1", .low = 14, .high = 21,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src_const_or_gpr,
            .params = &ldg_k__default_gen_600_src1,
          },
          { .name = "SIZE", .low = 23, .high = 26,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 40,
            .type = TYPE_BITSET,
            .bitsets = __const_dst,
          },
       },
};
static const struct isa_bitset bitset_ldg_k_gen_600 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "ldg.k",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc0100000 },
       .dontcare.bitset = { 0x38000000, 0x1fe00 },
       .mask.bitset     = { 0xf8400001, 0xe7f1fe00 },
       .decode = decode_ldg_k_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_ldg_k_gen_600_fields),
       .decode_fields = decode_ldg_k_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &ldg_k__default_gen_600,
       },
};
static const struct isa_field_params __instruction_cat6_stg__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_CONST",  .as = "SRC_CONST" },

       },
};
static const struct isa_case __instruction_cat6_stg__default_gen_0 = {
       .num_fields = 5,
       .fields   = {
          { .name = "SRC1_CONST", .low = 0, .high = 0,
            .expr = &expr___false,
            .type = TYPE_BOOL,
          },
          { .name = "SRC3", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SIZE", .low = 24, .high = 26,
            .type = TYPE_UINT,
          },
          { .name = "DST_OFF", .low = 40, .high = 40,
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src_const_or_gpr,
            .params = &__instruction_cat6_stg__default_gen_0_src1,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_stg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "#instruction-cat6-stg",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0c00000 },
       .dontcare.bitset = { 0x38000001, 0x0 },
       .mask.bitset     = { 0xf8000001, 0xe7e00000 },
       .decode = decode___instruction_cat6_stg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_stg_gen_0_fields),
       .decode_fields = decode___instruction_cat6_stg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_stg__default_gen_0,
       },
};
static const struct isa_case stg__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} g[{SRC1}{OFF}], {TYPE_HALF}{SRC3}, {SIZE}",
       .num_fields = 3,
       .fields   = {
          { .name = "OFF", .low = 0, .high = 12,
            .expr = &expr_anon_32,
            .type = TYPE_OFFSET,
          },
          { .name = "OFF_HI", .low = 9, .high = 13,
            .type = TYPE_INT,
          },
          { .name = "OFF_LO", .low = 32, .high = 39,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_stg_gen_0 = {

       .parent   = &bitset___instruction_cat6_stg_gen_0,
       .name     = "stg",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc0c00000 },
       .dontcare.bitset = { 0x387fc001, 0x0 },
       .mask.bitset     = { 0xf8ffc001, 0xe7f00000 },
       .decode = decode_stg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stg_gen_0_fields),
       .decode_fields = decode_stg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stg__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_ld__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "OFF", .low = 1, .high = 13,
            .type = TYPE_OFFSET,
          },
          { .name = "SRC", .low = 14, .high = 21,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SIZE", .low = 24, .high = 31,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_ld_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "#instruction-cat6-a3xx-ld",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800001, 0xc0000000 },
       .dontcare.bitset = { 0x400000, 0x31ff00 },
       .mask.bitset     = { 0xc00001, 0xe031ff00 },
       .decode = decode___instruction_cat6_a3xx_ld_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_ld_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_ld_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_ld__default_gen_0,
       },
};
static const struct isa_case ldl__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {DST}, l[{SRC}{OFF}], {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldl_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ld_gen_0,
       .name     = "ldl",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800001, 0xc0400000 },
       .dontcare.bitset = { 0x400000, 0x31ff00 },
       .mask.bitset     = { 0xc00001, 0xe7f1ff00 },
       .decode = decode_ldl_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldl_gen_0_fields),
       .decode_fields = decode_ldl_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldl__default_gen_0,
       },
};
static const struct isa_case ldp__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, p[{SRC}{OFF}], {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldp_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ld_gen_0,
       .name     = "ldp",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800001, 0xc0800000 },
       .dontcare.bitset = { 0x400000, 0x31ff00 },
       .mask.bitset     = { 0xc00001, 0xe7f1ff00 },
       .decode = decode_ldp_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldp_gen_0_fields),
       .decode_fields = decode_ldp_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldp__default_gen_0,
       },
};
static const struct isa_case ldlw__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {DST}, l[{SRC}{OFF}], {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldlw_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ld_gen_0,
       .name     = "ldlw",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800001, 0xc2800000 },
       .dontcare.bitset = { 0x400000, 0x31ff00 },
       .mask.bitset     = { 0xc00001, 0xe7f1ff00 },
       .decode = decode_ldlw_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldlw_gen_0_fields),
       .decode_fields = decode_ldlw_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldlw__default_gen_0,
       },
};
static const struct isa_case ldlv__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {DST}, l[{OFF}], {SIZE}",
       .num_fields = 3,
       .fields   = {
          { .name = "OFF", .low = 1, .high = 13,
            .type = TYPE_UINT,
          },
          { .name = "SIZE", .low = 24, .high = 31,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset_ldlv_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "ldlv",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc00000, 0xc7c00000 },
       .dontcare.bitset = { 0x0, 0x31ff00 },
       .mask.bitset     = { 0xffc001, 0xe7f1ff00 },
       .decode = decode_ldlv_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldlv_gen_0_fields),
       .decode_fields = decode_ldlv_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldlv__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_st__default_gen_0 = {
       .num_fields = 6,
       .fields   = {
          { .name = "OFF", .low = 0, .high = 12,
            .expr = &expr_anon_36,
            .type = TYPE_OFFSET,
          },
          { .name = "SRC", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "OFF_HI", .low = 9, .high = 13,
            .type = TYPE_INT,
          },
          { .name = "SIZE", .low = 24, .high = 31,
            .type = TYPE_UINT,
          },
          { .name = "OFF_LO", .low = 32, .high = 39,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_st_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "#instruction-cat6-a3xx-st",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc0000100 },
       .dontcare.bitset = { 0x7fc000, 0x300000 },
       .mask.bitset     = { 0xffc000, 0xe0300100 },
       .decode = decode___instruction_cat6_a3xx_st_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_st_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_st_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_st__default_gen_0,
       },
};
static const struct isa_case stl__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} l[{DST}{OFF}], {SRC}, {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stl_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_st_gen_0,
       .name     = "stl",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc1000100 },
       .dontcare.bitset = { 0x7fc001, 0x300000 },
       .mask.bitset     = { 0xffc001, 0xe7f00100 },
       .decode = decode_stl_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stl_gen_0_fields),
       .decode_fields = decode_stl_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stl__default_gen_0,
       },
};
static const struct isa_case stp__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} p[{DST}{OFF}], {SRC}, {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stp_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_st_gen_0,
       .name     = "stp",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc1400100 },
       .dontcare.bitset = { 0x7fc000, 0x300000 },
       .mask.bitset     = { 0xffc001, 0xe7f00100 },
       .decode = decode_stp_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stp_gen_0_fields),
       .decode_fields = decode_stp_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stp__default_gen_0,
       },
};
static const struct isa_case stlw__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} l[{DST}{OFF}], {SRC}, {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stlw_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_st_gen_0,
       .name     = "stlw",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc2c00100 },
       .dontcare.bitset = { 0x7fc001, 0x300000 },
       .mask.bitset     = { 0xffc001, 0xe7f00100 },
       .decode = decode_stlw_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stlw_gen_0_fields),
       .decode_fields = decode_stlw_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stlw__default_gen_0,
       },
};
static const struct isa_case stc__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} c[{DST}], {SRC}, {SIZE}",
       .num_fields = 3,
       .fields   = {
          { .name = "SRC", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SIZE", .low = 24, .high = 26,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 40,
            .type = TYPE_BITSET,
            .bitsets = __const_dst,
          },
       },
};
static const struct isa_bitset bitset_stc_gen_600 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "stc",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc7000000 },
       .dontcare.bitset = { 0xf8003e01, 0x31fe00 },
       .mask.bitset     = { 0xf8fffe01, 0xe7f1fe00 },
       .decode = decode_stc_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_stc_gen_600_fields),
       .decode_fields = decode_stc_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &stc__default_gen_600,
       },
};
static const struct isa_field_params stsc__default_gen_700_src = {
       .num_params = 1,
       .params = {
           { .name= "SRC_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case stsc__default_gen_700 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} c[{DST}], {SRC}, {SIZE}",
       .num_fields = 7,
       .fields   = {
          { .name = "SRC_IM", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "DST", .low = 0, .high = 12,
            .expr = &expr_anon_37,
            .type = TYPE_UINT,
          },
          { .name = "stsc#assert2", .low = 40, .high = 40,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
          { .name = "SRC", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &stsc__default_gen_700_src,
          },
          { .name = "DST_HI", .low = 9, .high = 13,
            .type = TYPE_UINT,
          },
          { .name = "SIZE", .low = 24, .high = 31,
            .type = TYPE_UINT,
          },
          { .name = "DST_LO", .low = 32, .high = 39,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_stsc_gen_700 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "stsc",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc00000, 0xc7400000 },
       .dontcare.bitset = { 0x3fc001, 0x1fe00 },
       .mask.bitset     = { 0xffc001, 0xe7f1fe00 },
       .decode = decode_stsc_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_stsc_gen_700_fields),
       .decode_fields = decode_stsc_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &stsc__default_gen_700,
       },
};
static const struct isa_field_params resinfo__default_gen_0_ssbo = {
       .num_params = 1,
       .params = {
           { .name= "SSBO_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case resinfo__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE}.{D}d {DST}, g[{SSBO}]",
       .num_fields = 5,
       .fields   = {
          { .name = "D", .low = 0, .high = 0,
            .expr = &expr___cat6_d,
            .type = TYPE_UINT,
          },
          { .name = "D_MINUS_ONE", .low = 9, .high = 10,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SSBO", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &resinfo__default_gen_0_ssbo,
          },
          { .name = "SSBO_IM", .low = 53, .high = 53,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_resinfo_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "resinfo",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc3c00000 },
       .dontcare.bitset = { 0xff0039ff, 0x100000 },
       .mask.bitset     = { 0xfffff9ff, 0xe7d00100 },
       .decode = decode_resinfo_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_resinfo_gen_0_fields),
       .decode_fields = decode_resinfo_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &resinfo__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_a3xx_ibo__default_gen_0_ssbo = {
       .num_params = 1,
       .params = {
           { .name= "SSBO_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a3xx_ibo__default_gen_0 = {
       .num_fields = 7,
       .fields   = {
          { .name = "D", .low = 0, .high = 0,
            .expr = &expr___cat6_d,
            .type = TYPE_UINT,
          },
          { .name = "TYPE_SIZE", .low = 0, .high = 0,
            .expr = &expr___cat6_type_size,
            .type = TYPE_UINT,
          },
          { .name = "D_MINUS_ONE", .low = 9, .high = 10,
            .type = TYPE_UINT,
          },
          { .name = "TYPED", .low = 11, .high = 11,
            .type = TYPE_BITSET,
            .bitsets = __cat6_typed,
          },
          { .name = "TYPE_SIZE_MINUS_ONE", .low = 12, .high = 13,
            .type = TYPE_UINT,
          },
          { .name = "SSBO", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_ibo__default_gen_0_ssbo,
          },
          { .name = "SSBO_IM", .low = 53, .high = 53,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "#instruction-cat6-a3xx-ibo",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x100000 },
       .mask.bitset     = { 0x0, 0xe0100000 },
       .decode = decode___instruction_cat6_a3xx_ibo_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_ibo_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_ibo_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_ibo__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_a3xx_ibo_load__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_field_params __instruction_cat6_a3xx_ibo_load__default_gen_0_src2 = {
       .num_params = 1,
       .params = {
           { .name= "SRC2_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a3xx_ibo_load__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE} {DST}, g[{SSBO}], {SRC1}, {SRC2}",
       .num_fields = 5,
       .fields   = {
          { .name = "SRC1", .low = 14, .high = 21,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_ibo_load__default_gen_0_src1,
          },
          { .name = "SRC1_IM", .low = 22, .high = 22,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2_IM", .low = 23, .high = 23,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_ibo_load__default_gen_0_src2,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_load_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_gen_0,
       .name     = "#instruction-cat6-a3xx-ibo-load",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x1fe, 0x100100 },
       .mask.bitset     = { 0x1fe, 0xe0100100 },
       .decode = decode___instruction_cat6_a3xx_ibo_load_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_ibo_load_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_ibo_load_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_ibo_load__default_gen_0,
       },
};
static const struct isa_case ldib__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldib_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_load_gen_0,
       .name     = "ldib",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc1800000 },
       .dontcare.bitset = { 0x1fe, 0x100100 },
       .mask.bitset     = { 0x1ff, 0xe7d00100 },
       .decode = decode_ldib_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldib_gen_0_fields),
       .decode_fields = decode_ldib_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldib__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_a3xx_ibo_store__default_gen_0_src2 = {
       .num_params = 1,
       .params = {
           { .name= "SRC2_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_field_params __instruction_cat6_a3xx_ibo_store__default_gen_0_src3 = {
       .num_params = 1,
       .params = {
           { .name= "SRC3_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a3xx_ibo_store__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE} g[{SSBO}], {SRC1}, {SRC2}, {SRC3}",
       .num_fields = 5,
       .fields   = {
          { .name = "SRC1", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SRC2_IM", .low = 23, .high = 23,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_ibo_store__default_gen_0_src2,
          },
          { .name = "SRC3", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_ibo_store__default_gen_0_src3,
          },
          { .name = "SRC3_IM", .low = 40, .high = 40,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_store_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_gen_0,
       .name     = "#instruction-cat6-a3xx-ibo-store",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc000, 0xe0100000 },
       .decode = decode___instruction_cat6_a3xx_ibo_store_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_ibo_store_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_ibo_store_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_ibo_store__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_ibo_store_a4xx__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_store_a4xx_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_store_gen_0,
       .name     = "#instruction-cat6-a3xx-ibo-store-a4xx",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc001, 0xe0100000 },
       .decode = decode___instruction_cat6_a3xx_ibo_store_a4xx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_ibo_store_a4xx_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_ibo_store_a4xx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_ibo_store_a4xx__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_ibo_store_a5xx__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_ibo_store_a5xx_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_store_gen_0,
       .name     = "#instruction-cat6-a3xx-ibo-store-a5xx",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc0000000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc001, 0xe0100000 },
       .decode = decode___instruction_cat6_a3xx_ibo_store_a5xx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_ibo_store_a5xx_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_ibo_store_a5xx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_ibo_store_a5xx__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_a3xx_atomic__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_field_params __instruction_cat6_a3xx_atomic__default_gen_0_src2 = {
       .num_params = 1,
       .params = {
           { .name= "SRC2_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE}.l {DST}, l[{SRC1}], {SRC2}",
       .num_fields = 10,
       .fields   = {
          { .name = "D", .low = 0, .high = 0,
            .expr = &expr___cat6_d,
            .type = TYPE_UINT,
          },
          { .name = "TYPE_SIZE", .low = 0, .high = 0,
            .expr = &expr___cat6_type_size,
            .type = TYPE_UINT,
          },
          { .name = "D_MINUS_ONE", .low = 9, .high = 10,
            .type = TYPE_UINT,
          },
          { .name = "TYPED", .low = 11, .high = 11,
            .type = TYPE_BITSET,
            .bitsets = __cat6_typed,
          },
          { .name = "TYPE_SIZE_MINUS_ONE", .low = 12, .high = 13,
            .type = TYPE_UINT,
          },
          { .name = "SRC1", .low = 14, .high = 21,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_atomic__default_gen_0_src1,
          },
          { .name = "SRC1_IM", .low = 22, .high = 22,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2_IM", .low = 23, .high = 23,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_atomic__default_gen_0_src2,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_gen_0 = {

       .parent   = &bitset___instruction_cat6_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x0, 0xe0000100 },
       .decode = decode___instruction_cat6_a3xx_atomic_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic_local__default_gen_0 = {
       .num_fields = 1,
       .fields   = {
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_local_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic-local",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe031ff00 },
       .decode = decode___instruction_cat6_a3xx_atomic_local_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_local_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_local_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic_local__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic_1src__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_1src_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_local_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic-1src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe031ff00 },
       .decode = decode___instruction_cat6_a3xx_atomic_1src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_1src_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_1src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic_1src__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic_2src__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_2src_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_local_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic-2src",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe031ff00 },
       .decode = decode___instruction_cat6_a3xx_atomic_2src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_2src_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_2src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic_2src__default_gen_0,
       },
};
static const struct isa_case atomic_add__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_add_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.add",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_add_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_add_gen_0_fields),
       .decode_fields = decode_atomic_add_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_add__default_gen_0,
       },
};
static const struct isa_case atomic_sub__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_sub_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.sub",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4400000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_sub_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_sub_gen_0_fields),
       .decode_fields = decode_atomic_sub_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_sub__default_gen_0,
       },
};
static const struct isa_case atomic_xchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_xchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.xchg",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4800000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_xchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_xchg_gen_0_fields),
       .decode_fields = decode_atomic_xchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_xchg__default_gen_0,
       },
};
static const struct isa_case atomic_inc__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_inc_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_1src_gen_0,
       .name     = "atomic.inc",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4c00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_inc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_inc_gen_0_fields),
       .decode_fields = decode_atomic_inc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_inc__default_gen_0,
       },
};
static const struct isa_case atomic_dec__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_dec_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_1src_gen_0,
       .name     = "atomic.dec",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_dec_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_dec_gen_0_fields),
       .decode_fields = decode_atomic_dec_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_dec__default_gen_0,
       },
};
static const struct isa_case atomic_cmpxchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_cmpxchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.cmpxchg",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5400000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_cmpxchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_cmpxchg_gen_0_fields),
       .decode_fields = decode_atomic_cmpxchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_cmpxchg__default_gen_0,
       },
};
static const struct isa_case atomic_min__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_min_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.min",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5800000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_min_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_min_gen_0_fields),
       .decode_fields = decode_atomic_min_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_min__default_gen_0,
       },
};
static const struct isa_case atomic_max__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_max_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.max",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5c00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_max_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_max_gen_0_fields),
       .decode_fields = decode_atomic_max_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_max__default_gen_0,
       },
};
static const struct isa_case atomic_and__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_and_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.and",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc6000000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_and_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_and_gen_0_fields),
       .decode_fields = decode_atomic_and_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_and__default_gen_0,
       },
};
static const struct isa_case atomic_or__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_or_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.or",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc6400000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_or_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_or_gen_0_fields),
       .decode_fields = decode_atomic_or_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_or__default_gen_0,
       },
};
static const struct isa_case atomic_xor__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_xor_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_2src_gen_0,
       .name     = "atomic.xor",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc6800000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_xor_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_xor_gen_0_fields),
       .decode_fields = decode_atomic_xor_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_xor__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_a3xx_atomic_global__default_gen_0_ssbo = {
       .num_params = 1,
       .params = {
           { .name= "SSBO_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic_global__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE}.g {DST}, g[{SSBO}], {SRC1}, {SRC2}, {SRC3}",
       .num_fields = 4,
       .fields   = {
          { .name = "SRC3", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SSBO", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a3xx_atomic_global__default_gen_0_ssbo,
          },
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type_atomic,
          },
          { .name = "SSBO_IM", .low = 53, .high = 53,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_global_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic-global",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x0, 0xc0100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x0, 0xe0100100 },
       .decode = decode___instruction_cat6_a3xx_atomic_global_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_global_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_global_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic_global__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic_global_a4xx__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic-global-a4xx",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x0, 0xc0100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe0100100 },
       .decode = decode___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_global_a4xx_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_global_a4xx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic_global_a4xx__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a3xx_atomic_global_a5xx__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_gen_0,
       .name     = "#instruction-cat6-a3xx-atomic-global-a5xx",
       .gen      = {
           .min  = 300,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc0100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe0100100 },
       .decode = decode___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a3xx_atomic_global_a5xx_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a3xx_atomic_global_a5xx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a3xx_atomic_global_a5xx__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a6xx_atomic_global__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE}.g {DST}, {SRC1}, {SRC2}",
       .num_fields = 1,
       .fields   = {
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type_atomic,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_atomic_global_gen_600 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_gen_0,
       .name     = "#instruction-cat6-a6xx-atomic-global",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc0100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe031ff00 },
       .decode = decode___instruction_cat6_a6xx_atomic_global_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_atomic_global_gen_600_fields),
       .decode_fields = decode___instruction_cat6_a6xx_atomic_global_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx_atomic_global__default_gen_600,
       },
};
static const struct isa_case atomic_g_add__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_add_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.add",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_add_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_add_gen_0_fields),
       .decode_fields = decode_atomic_g_add_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_add__default_gen_0,
       },
};
static const struct isa_case atomic_g_sub__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_sub_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.sub",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_sub_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_sub_gen_0_fields),
       .decode_fields = decode_atomic_g_sub_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_sub__default_gen_0,
       },
};
static const struct isa_case atomic_g_xchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_xchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.xchg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_xchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_xchg_gen_0_fields),
       .decode_fields = decode_atomic_g_xchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_xchg__default_gen_0,
       },
};
static const struct isa_case atomic_g_inc__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_inc_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.inc",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc4d00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_inc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_inc_gen_0_fields),
       .decode_fields = decode_atomic_g_inc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_inc__default_gen_0,
       },
};
static const struct isa_case atomic_g_dec__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_dec_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.dec",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_dec_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_dec_gen_0_fields),
       .decode_fields = decode_atomic_g_dec_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_dec__default_gen_0,
       },
};
static const struct isa_case atomic_g_cmpxchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_cmpxchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.cmpxchg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_cmpxchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_cmpxchg_gen_0_fields),
       .decode_fields = decode_atomic_g_cmpxchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_cmpxchg__default_gen_0,
       },
};
static const struct isa_case atomic_g_min__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_min_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.min",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_min_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_min_gen_0_fields),
       .decode_fields = decode_atomic_g_min_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_min__default_gen_0,
       },
};
static const struct isa_case atomic_g_max__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_max_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.max",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc5d00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_max_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_max_gen_0_fields),
       .decode_fields = decode_atomic_g_max_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_max__default_gen_0,
       },
};
static const struct isa_case atomic_g_and__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_and_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.and",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc6100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_and_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_and_gen_0_fields),
       .decode_fields = decode_atomic_g_and_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_and__default_gen_0,
       },
};
static const struct isa_case atomic_g_or__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_or_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.or",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc6500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_or_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_or_gen_0_fields),
       .decode_fields = decode_atomic_g_or_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_or__default_gen_0,
       },
};
static const struct isa_case atomic_g_xor__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_g_xor_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_atomic_global_gen_600,
       .name     = "atomic.g.xor",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc6900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1ff, 0xe7f1ff00 },
       .decode = decode_atomic_g_xor_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_g_xor_gen_0_fields),
       .decode_fields = decode_atomic_g_xor_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_g_xor__default_gen_0,
       },
};
static const struct isa_field_params ray_intersection__default_gen_700_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_CONST",  .as = "SRC_CONST" },

       },
};
static const struct isa_case ray_intersection__default_gen_700 = {
       .display  = "{SY}{JP}{NAME} {DST}, [{SRC1}], {SRC2}, {SRC3}, {SRC4}",
       .num_fields = 6,
       .fields   = {
          { .name = "SRC2", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SRC1_CONST", .low = 13, .high = 13,
            .type = TYPE_BOOL,
          },
          { .name = "SRC1", .low = 14, .high = 21,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src_const_or_gpr,
            .params = &ray_intersection__default_gen_700_src1,
          },
          { .name = "SRC3", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SRC4", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset_ray_intersection_gen_700 = {

       .parent   = &bitset___instruction_cat6_gen_0,
       .name     = "ray_intersection",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x1, 0xc3800000 },
       .dontcare.bitset = { 0xc01e00, 0x3e0100 },
       .mask.bitset     = { 0xc01e01, 0xe7fe0100 },
       .decode = decode_ray_intersection_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_ray_intersection_gen_700_fields),
       .decode_fields = decode_ray_intersection_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &ray_intersection__default_gen_700,
       },
};
static const struct isa_field_params __instruction_cat6_a6xx_base__default_gen_600_base = {
       .num_params = 1,
       .params = {
           { .name= "BINDLESS",  .as = "BINDLESS" },

       },
};
static const struct isa_case __instruction_cat6_a6xx_base__default_gen_600 = {
       .num_fields = 5,
       .fields   = {
          { .name = "TYPE_SIZE", .low = 0, .high = 0,
            .expr = &expr___cat6_type_size,
            .type = TYPE_UINT,
          },
          { .name = "BASE", .low = 1, .high = 3,
            .type = TYPE_BITSET,
            .bitsets = __cat6_base,
            .params = &__instruction_cat6_a6xx_base__default_gen_600_base,
          },
          { .name = "MODE", .low = 6, .high = 7,
            .type = TYPE_ENUM,
            .enums = &enum___cat6_src_mode,
          },
          { .name = "BINDLESS", .low = 8, .high = 8,
            .type = TYPE_BOOL,
          },
          { .name = "TYPE_SIZE_MINUS_ONE", .low = 12, .high = 13,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_base_gen_600 = {

       .parent   = &bitset___instruction_cat6_gen_0,
       .name     = "#instruction-cat6-a6xx-base",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0xe0000100 },
       .decode = decode___instruction_cat6_a6xx_base_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_base_gen_600_fields),
       .decode_fields = decode___instruction_cat6_a6xx_base_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx_base__default_gen_600,
       },
};
static const struct isa_case __instruction_cat6_a6xx__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_base_gen_600,
       .name     = "#instruction-cat6-a6xx",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x30, 0xe7c00100 },
       .decode = decode___instruction_cat6_a6xx_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a6xx_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx__default_gen_0,
       },
};
static const struct isa_field_params __cat6_ldc_common__default_gen_0_src2 = {
       .num_params = 1,
       .params = {
           { .name= "SRC2_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_field_params __cat6_ldc_common__default_gen_0_src1 = {
       .num_params = 1,
       .params = {
           { .name= "SRC1_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __cat6_ldc_common__default_gen_0 = {
       .num_fields = 4,
       .fields   = {
          { .name = "SRC2_IM", .low = 0, .high = 0,
            .expr = &expr___cat6_direct,
            .type = TYPE_BOOL,
          },
          { .name = "SRC1_IM", .low = 23, .high = 23,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__cat6_ldc_common__default_gen_0_src2,
          },
          { .name = "SRC1", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__cat6_ldc_common__default_gen_0_src1,
          },
       },
};
static const struct isa_bitset bitset___cat6_ldc_common_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_gen_0,
       .name     = "#cat6-ldc-common",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x478000, 0xc0060000 },
       .dontcare.bitset = { 0x300001, 0x80000 },
       .mask.bitset     = { 0x7fc031, 0xe7ce0100 },
       .decode = decode___cat6_ldc_common_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat6_ldc_common_gen_0_fields),
       .decode_fields = decode___cat6_ldc_common_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__cat6_ldc_common__default_gen_0,
       },
};
static const struct isa_case ldc_k__default_gen_0 = {
       .display  = "{SY}{JP}ldc.{LOAD_SIZE}.k.{MODE}{BASE} c[a1.x], {SRC1}, {SRC2}",
       .num_fields = 2,
       .fields   = {
          { .name = "LOAD_SIZE", .low = 0, .high = 0,
            .expr = &expr___cat6_load_size,
            .type = TYPE_UINT,
          },
          { .name = "LOAD_SIZE_MINUS_ONE", .low = 32, .high = 39,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_ldc_k_gen_0 = {

       .parent   = &bitset___cat6_ldc_common_gen_0,
       .name     = "ldc.k",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x478000, 0xc0360000 },
       .dontcare.bitset = { 0x300e01, 0x80000 },
       .mask.bitset     = { 0x7fce31, 0xe7fe0100 },
       .decode = decode_ldc_k_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldc_k_gen_0_fields),
       .decode_fields = decode_ldc_k_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldc_k__default_gen_0,
       },
};
static const struct isa_case ldc__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}{U}.offset{OFFSET}.{TYPE_SIZE}.{MODE}{BASE} {DST}, {SRC1}, {SRC2}",
       .num_fields = 3,
       .fields   = {
          { .name = "U", .low = 11, .high = 11,
            .display = ".u",
            .type = TYPE_BOOL,
          },
          { .name = "OFFSET", .low = 9, .high = 10,
            .type = TYPE_UINT,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset_ldc_gen_0 = {

       .parent   = &bitset___cat6_ldc_common_gen_0,
       .name     = "ldc",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x478000, 0xc0260000 },
       .dontcare.bitset = { 0x300001, 0x80000 },
       .mask.bitset     = { 0x7fc031, 0xe7fe0100 },
       .decode = decode_ldc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldc_gen_0_fields),
       .decode_fields = decode_ldc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldc__default_gen_0,
       },
};
static const struct isa_case getspid__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {DST}",
       .num_fields = 2,
       .fields   = {
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset_getspid_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_gen_0,
       .name     = "getspid",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x490000, 0xc0200000 },
       .dontcare.bitset = { 0xffb00e00, 0x11fe00 },
       .mask.bitset     = { 0xffffce31, 0xe7f1ff00 },
       .decode = decode_getspid_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getspid_gen_0_fields),
       .decode_fields = decode_getspid_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getspid__default_gen_0,
       },
};
static const struct isa_case getwid__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {DST}",
       .num_fields = 2,
       .fields   = {
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset_getwid_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_gen_0,
       .name     = "getwid",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x494000, 0xc0200000 },
       .dontcare.bitset = { 0xffb00e00, 0x11fe00 },
       .mask.bitset     = { 0xffffce31, 0xe7f1ff00 },
       .decode = decode_getwid_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_getwid_gen_0_fields),
       .decode_fields = decode_getwid_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &getwid__default_gen_0,
       },
};
static const struct isa_case getfiberid__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {DST}",
       .num_fields = 2,
       .fields   = {
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset_getfiberid_gen_600 = {

       .parent   = &bitset___instruction_cat6_a6xx_gen_0,
       .name     = "getfiberid",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc98000, 0xc0200000 },
       .dontcare.bitset = { 0xff300e00, 0x11fe00 },
       .mask.bitset     = { 0xffffce31, 0xe7f1ff00 },
       .decode = decode_getfiberid_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_getfiberid_gen_600_fields),
       .decode_fields = decode_getfiberid_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &getfiberid__default_gen_600,
       },
};
static const struct isa_field_params __instruction_cat6_a6xx_ibo_1src__default_gen_0_ssbo = {
       .num_params = 1,
       .params = {
           { .name= "SSBO_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a6xx_ibo_1src__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE}.{MODE}{BASE} {DST}, {SSBO}",
       .num_fields = 8,
       .fields   = {
          { .name = "D", .low = 0, .high = 0,
            .expr = &expr___cat6_d,
            .type = TYPE_UINT,
          },
          { .name = "TRUE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "SSBO_IM", .low = 0, .high = 0,
            .expr = &expr___cat6_direct,
            .type = TYPE_BOOL,
          },
          { .name = "D_MINUS_ONE", .low = 9, .high = 10,
            .type = TYPE_UINT,
          },
          { .name = "TYPED", .low = 11, .high = 11,
            .type = TYPE_BITSET,
            .bitsets = __cat6_typed,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SSBO", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a6xx_ibo_1src__default_gen_0_ssbo,
          },
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_1src_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_gen_0,
       .name     = "#instruction-cat6-a6xx-ibo-1src",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x600000, 0xc0200000 },
       .dontcare.bitset = { 0xff000000, 0x100000 },
       .mask.bitset     = { 0xfff00031, 0xe7f00100 },
       .decode = decode___instruction_cat6_a6xx_ibo_1src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_ibo_1src_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a6xx_ibo_1src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx_ibo_1src__default_gen_0,
       },
};
static const struct isa_case resinfo_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_resinfo_b_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_1src_gen_0,
       .name     = "resinfo.b",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x63c000, 0xc0200000 },
       .dontcare.bitset = { 0xff000000, 0x100000 },
       .mask.bitset     = { 0xffffc031, 0xe7f00100 },
       .decode = decode_resinfo_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_resinfo_b_gen_0_fields),
       .decode_fields = decode_resinfo_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &resinfo_b__default_gen_0,
       },
};
static const struct isa_case resbase__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_resbase_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_1src_gen_0,
       .name     = "resbase",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x630000, 0xc0200000 },
       .dontcare.bitset = { 0xff000000, 0x100000 },
       .mask.bitset     = { 0xffffc031, 0xe7f00100 },
       .decode = decode_resbase_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_resbase_gen_0_fields),
       .decode_fields = decode_resbase_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &resbase__default_gen_0,
       },
};
static const struct isa_field_params __instruction_cat6_a6xx_ibo_base__default_gen_0_ssbo = {
       .num_params = 1,
       .params = {
           { .name= "SSBO_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case __instruction_cat6_a6xx_ibo_base__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE}.{MODE}{BASE} {TYPE_HALF}{SRC1}, {SRC2}{OFFSET}, {SSBO}",
       .num_fields = 9,
       .fields   = {
          { .name = "D", .low = 0, .high = 0,
            .expr = &expr___cat6_d,
            .type = TYPE_UINT,
          },
          { .name = "TRUE", .low = 0, .high = 0,
            .expr = &expr___true,
            .type = TYPE_BOOL,
          },
          { .name = "OFFSET", .low = 0, .high = 0,
            .expr = &expr___zero,
            .display = "",
            .type = TYPE_UOFFSET,
          },
          { .name = "SSBO_IM", .low = 0, .high = 0,
            .expr = &expr___cat6_direct,
            .type = TYPE_BOOL,
          },
          { .name = "D_MINUS_ONE", .low = 9, .high = 10,
            .type = TYPE_UINT,
          },
          { .name = "TYPED", .low = 11, .high = 11,
            .type = TYPE_BITSET,
            .bitsets = __cat6_typed,
          },
          { .name = "SRC2", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SRC1", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SSBO", .low = 41, .high = 48,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &__instruction_cat6_a6xx_ibo_base__default_gen_0_ssbo,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_base_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_base_gen_600,
       .name     = "#instruction-cat6-a6xx-ibo-base",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x600000, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x700000, 0xe0000100 },
       .decode = decode___instruction_cat6_a6xx_ibo_base_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_ibo_base_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a6xx_ibo_base_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx_ibo_base__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a6xx_ibo__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_base_gen_0,
       .name     = "#instruction-cat6-a6xx-ibo",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x600000, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xf00030, 0xe7c00100 },
       .decode = decode___instruction_cat6_a6xx_ibo_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_ibo_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a6xx_ibo_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx_ibo__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a6xx_ibo_load_store__case0_gen_0 = {
       .expr     = &expr_anon_39,
       .num_fields = 2,
       .fields   = {
          { .name = "#instruction-cat6-a6xx-ibo-load-store#assert0", .low = 4, .high = 5,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
          { .name = "#instruction-cat6-a6xx-ibo-load-store#assert1", .low = 54, .high = 58,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
       },
};
static const struct isa_case __instruction_cat6_a6xx_ibo_load_store__default_gen_0 = {
       .num_fields = 5,
       .fields   = {
          { .name = "OFFSET", .low = 0, .high = 0,
            .expr = &expr_anon_38,
            .type = TYPE_UOFFSET,
          },
          { .name = "HAS_OFFSET", .low = 23, .high = 23,
            .type = TYPE_BOOL,
          },
          { .name = "OFFSET_HI", .low = 4, .high = 5,
            .type = TYPE_UINT,
          },
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type,
          },
          { .name = "OFFSET_LO", .low = 54, .high = 58,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_load_store_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_base_gen_0,
       .name     = "#instruction-cat6-a6xx-ibo-load-store",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x600000, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x700000, 0xe0000100 },
       .decode = decode___instruction_cat6_a6xx_ibo_load_store_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_ibo_load_store_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a6xx_ibo_load_store_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__instruction_cat6_a6xx_ibo_load_store__case0_gen_0,
            &__instruction_cat6_a6xx_ibo_load_store__default_gen_0,
       },
};
static const struct isa_case __instruction_cat6_a6xx_ibo_atomic__default_gen_0 = {
       .display  = "{SY}{JP}{NAME}.{TYPED}.{D}d.{TYPE}.{TYPE_SIZE}.{MODE}{BASE} {SRC1}, {SRC2}, {SSBO}",
       .num_fields = 1,
       .fields   = {
          { .name = "TYPE", .low = 49, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___type_atomic,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat6_a6xx_ibo_atomic_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_gen_0,
       .name     = "#instruction-cat6-a6xx-ibo-atomic",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x600000, 0xc0000000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0xf00030, 0xe7c00100 },
       .decode = decode___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat6_a6xx_ibo_atomic_gen_0_fields),
       .decode_fields = decode___instruction_cat6_a6xx_ibo_atomic_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat6_a6xx_ibo_atomic__default_gen_0,
       },
};
static const struct isa_case stib_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stib_b_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_load_store_gen_0,
       .name     = "stib.b",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x674000, 0xc0200000 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x7fc001, 0xe0300100 },
       .decode = decode_stib_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stib_b_gen_0_fields),
       .decode_fields = decode_stib_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stib_b__default_gen_0,
       },
};
static const struct isa_case ldib_b__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldib_b_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_load_store_gen_0,
       .name     = "ldib.b",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x618000, 0xc0200000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0x7fc001, 0xe0300100 },
       .decode = decode_ldib_b_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldib_b_gen_0_fields),
       .decode_fields = decode_ldib_b_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldib_b__default_gen_0,
       },
};
static const struct isa_case atomic_b_add__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_add_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.add",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x640000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_add_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_add_gen_0_fields),
       .decode_fields = decode_atomic_b_add_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_add__default_gen_0,
       },
};
static const struct isa_case atomic_b_sub__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_sub_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.sub",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x644000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_sub_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_sub_gen_0_fields),
       .decode_fields = decode_atomic_b_sub_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_sub__default_gen_0,
       },
};
static const struct isa_case atomic_b_xchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_xchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.xchg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x648000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_xchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_xchg_gen_0_fields),
       .decode_fields = decode_atomic_b_xchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_xchg__default_gen_0,
       },
};
static const struct isa_case atomic_b_cmpxchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_cmpxchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.cmpxchg",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x654000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_cmpxchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_cmpxchg_gen_0_fields),
       .decode_fields = decode_atomic_b_cmpxchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_cmpxchg__default_gen_0,
       },
};
static const struct isa_case atomic_b_min__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_min_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.min",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x658000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_min_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_min_gen_0_fields),
       .decode_fields = decode_atomic_b_min_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_min__default_gen_0,
       },
};
static const struct isa_case atomic_b_max__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_max_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.max",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x65c000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_max_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_max_gen_0_fields),
       .decode_fields = decode_atomic_b_max_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_max__default_gen_0,
       },
};
static const struct isa_case atomic_b_and__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_and_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.and",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x660000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_and_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_and_gen_0_fields),
       .decode_fields = decode_atomic_b_and_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_and__default_gen_0,
       },
};
static const struct isa_case atomic_b_or__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_or_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.or",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x664000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_or_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_or_gen_0_fields),
       .decode_fields = decode_atomic_b_or_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_or__default_gen_0,
       },
};
static const struct isa_case atomic_b_xor__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_b_xor_gen_0 = {

       .parent   = &bitset___instruction_cat6_a6xx_ibo_atomic_gen_0,
       .name     = "atomic.b.xor",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x668000, 0xc0300000 },
       .dontcare.bitset = { 0x1, 0x0 },
       .mask.bitset     = { 0xffc031, 0xe7f00100 },
       .decode = decode_atomic_b_xor_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_b_xor_gen_0_fields),
       .decode_fields = decode_atomic_b_xor_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_b_xor__default_gen_0,
       },
};
static const struct isa_field_params shfl__default_gen_600_src2 = {
       .num_params = 1,
       .params = {
           { .name= "SRC2_IM",  .as = "SRC_IM" },

       },
};
static const struct isa_case shfl__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{MODE}.{TYPE} {TYPE_HALF}{DST}, {TYPE_HALF}{SRC1}, {SRC2}",
       .num_fields = 5,
       .fields   = {
          { .name = "SRC1", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SRC2_IM", .low = 23, .high = 23,
            .type = TYPE_BOOL,
          },
          { .name = "SRC2", .low = 24, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __cat6_src,
            .params = &shfl__default_gen_600_src2,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "MODE", .low = 45, .high = 47,
            .type = TYPE_ENUM,
            .enums = &enum___cat6_shfl_mode,
          },
       },
};
static const struct isa_bitset bitset_shfl_gen_600 = {

       .parent   = &bitset___instruction_cat6_a3xx_gen_0,
       .name     = "shfl",
       .gen      = {
           .min  = 600,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xc6e00000 },
       .dontcare.bitset = { 0x0, 0x11f00 },
       .mask.bitset     = { 0x7ffe01, 0xe7f11f00 },
       .decode = decode_shfl_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_shfl_gen_600_fields),
       .decode_fields = decode_shfl_gen_600_fields,
       .num_cases = 1,
       .cases    = {
            &shfl__default_gen_600,
       },
};
static const struct isa_case __cat6_typed__case0_gen_0 = {
       .expr     = &expr_anon_40,
       .display  = "typed",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __cat6_typed__default_gen_0 = {
       .display  = "untyped",
       .num_fields = 1,
       .fields   = {
          { .name = "TYPED", .low = 0, .high = 0,
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___cat6_typed_gen_0 = {

       .name     = "#cat6-typed",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat6_typed_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat6_typed_gen_0_fields),
       .decode_fields = decode___cat6_typed_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat6_typed__case0_gen_0,
            &__cat6_typed__default_gen_0,
       },
};
static const struct isa_case __cat6_base__case0_gen_0 = {
       .expr     = &expr_anon_41,
       .display  = ".base{BASE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __cat6_base__default_gen_0 = {
       .display  = "",
       .num_fields = 1,
       .fields   = {
          { .name = "BASE", .low = 0, .high = 2,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___cat6_base_gen_0 = {

       .name     = "#cat6-base",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat6_base_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat6_base_gen_0_fields),
       .decode_fields = decode___cat6_base_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat6_base__case0_gen_0,
            &__cat6_base__default_gen_0,
       },
};
static const struct isa_case __cat6_src__case0_gen_0 = {
       .expr     = &expr_anon_42,
       .display  = "{IMMED}",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_case __cat6_src__default_gen_0 = {
       .display  = "r{GPR}.{SWIZ}",
       .num_fields = 2,
       .fields   = {
          { .name = "SWIZ", .low = 0, .high = 1,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "GPR", .low = 2, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___cat6_src_gen_0 = {

       .name     = "#cat6-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat6_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat6_src_gen_0_fields),
       .decode_fields = decode___cat6_src_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat6_src__case0_gen_0,
            &__cat6_src__default_gen_0,
       },
};
static const struct isa_case __cat6_src_const_or_gpr__case0_gen_0 = {
       .expr     = &expr_anon_43,
       .display  = "c{GPR}.{SWIZ}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case __cat6_src_const_or_gpr__default_gen_0 = {
       .display  = "r{GPR}.{SWIZ}",
       .num_fields = 2,
       .fields   = {
          { .name = "SWIZ", .low = 0, .high = 1,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "GPR", .low = 2, .high = 7,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___cat6_src_const_or_gpr_gen_0 = {

       .name     = "#cat6-src-const-or-gpr",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___cat6_src_const_or_gpr_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___cat6_src_const_or_gpr_gen_0_fields),
       .decode_fields = decode___cat6_src_const_or_gpr_gen_0_fields,
       .num_cases = 2,
       .cases    = {
            &__cat6_src_const_or_gpr__case0_gen_0,
            &__cat6_src_const_or_gpr__default_gen_0,
       },
};
static const struct isa_case __instruction_cat7__default_gen_0 = {
       .num_fields = 3,
       .fields   = {
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat7_gen_0 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "#instruction-cat7",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe0000000 },
       .dontcare.bitset = { 0xffffffff, 0xfff },
       .mask.bitset     = { 0xffffffff, 0xe0000fff },
       .decode = decode___instruction_cat7_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat7_gen_0_fields),
       .decode_fields = decode___instruction_cat7_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat7__default_gen_0,
       },
};
static const struct isa_case __instruction_cat7_barrier__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{NAME}{G}{L}{R}{W}",
       .num_fields = 4,
       .fields   = {
          { .name = "W", .low = 51, .high = 51,
            .display = ".w",
            .type = TYPE_BOOL,
          },
          { .name = "R", .low = 52, .high = 52,
            .display = ".r",
            .type = TYPE_BOOL,
          },
          { .name = "L", .low = 53, .high = 53,
            .display = ".l",
            .type = TYPE_BOOL,
          },
          { .name = "G", .low = 54, .high = 54,
            .display = ".g",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat7_barrier_gen_0 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "#instruction-cat7-barrier",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe0020000 },
       .dontcare.bitset = { 0xffffffff, 0x5efff },
       .mask.bitset     = { 0xffffffff, 0xe007efff },
       .decode = decode___instruction_cat7_barrier_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat7_barrier_gen_0_fields),
       .decode_fields = decode___instruction_cat7_barrier_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat7_barrier__default_gen_0,
       },
};
static const struct isa_case bar__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_bar_gen_0 = {

       .parent   = &bitset___instruction_cat7_barrier_gen_0,
       .name     = "bar",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe0020000 },
       .dontcare.bitset = { 0xffffffff, 0x5efff },
       .mask.bitset     = { 0xffffffff, 0xe787efff },
       .decode = decode_bar_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_bar_gen_0_fields),
       .decode_fields = decode_bar_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &bar__default_gen_0,
       },
};
static const struct isa_case fence__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_fence_gen_0 = {

       .parent   = &bitset___instruction_cat7_barrier_gen_0,
       .name     = "fence",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe0820000 },
       .dontcare.bitset = { 0xffffffff, 0x5efff },
       .mask.bitset     = { 0xffffffff, 0xe787efff },
       .decode = decode_fence_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_fence_gen_0_fields),
       .decode_fields = decode_fence_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &fence__default_gen_0,
       },
};
static const struct isa_case __instruction_cat7_data__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{NAME}{TYPE}",
       .num_fields = 1,
       .fields   = {
          { .name = "TYPE", .low = 51, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___dccln_type,
          },
       },
};
static const struct isa_bitset bitset___instruction_cat7_data_gen_0 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "#instruction-cat7-data",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe0000000 },
       .dontcare.bitset = { 0xffffffff, 0x67efff },
       .mask.bitset     = { 0xffffffff, 0xe077efff },
       .decode = decode___instruction_cat7_data_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_cat7_data_gen_0_fields),
       .decode_fields = decode___instruction_cat7_data_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction_cat7_data__default_gen_0,
       },
};
static const struct isa_case sleep__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{NAME}{DURATION}",
       .num_fields = 1,
       .fields   = {
          { .name = "DURATION", .low = 51, .high = 51,
            .type = TYPE_ENUM,
            .enums = &enum___sleep_duration,
          },
       },
};
static const struct isa_bitset bitset_sleep_gen_0 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "sleep",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe1000000 },
       .dontcare.bitset = { 0xffffffff, 0x77efff },
       .mask.bitset     = { 0xffffffff, 0xe7f7efff },
       .decode = decode_sleep_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_sleep_gen_0_fields),
       .decode_fields = decode_sleep_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &sleep__default_gen_0,
       },
};
static const struct isa_case icinv__default_gen_0 = {
       .display  = "{SY}{SS}{JP}{NAME}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_icinv_gen_0 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "icinv",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe1800000 },
       .dontcare.bitset = { 0xffffffff, 0x7fefff },
       .mask.bitset     = { 0xffffffff, 0xe7ffefff },
       .decode = decode_icinv_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_icinv_gen_0_fields),
       .decode_fields = decode_icinv_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &icinv__default_gen_0,
       },
};
static const struct isa_case dccln__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_dccln_gen_0 = {

       .parent   = &bitset___instruction_cat7_data_gen_0,
       .name     = "dccln",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe2000000 },
       .dontcare.bitset = { 0xffffffff, 0x67efff },
       .mask.bitset     = { 0xffffffff, 0xe7f7efff },
       .decode = decode_dccln_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dccln_gen_0_fields),
       .decode_fields = decode_dccln_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dccln__default_gen_0,
       },
};
static const struct isa_case dcinv__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_dcinv_gen_0 = {

       .parent   = &bitset___instruction_cat7_data_gen_0,
       .name     = "dcinv",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe2800000 },
       .dontcare.bitset = { 0xffffffff, 0x67efff },
       .mask.bitset     = { 0xffffffff, 0xe7f7efff },
       .decode = decode_dcinv_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dcinv_gen_0_fields),
       .decode_fields = decode_dcinv_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dcinv__default_gen_0,
       },
};
static const struct isa_case dcflu__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_dcflu_gen_0 = {

       .parent   = &bitset___instruction_cat7_data_gen_0,
       .name     = "dcflu",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe3000000 },
       .dontcare.bitset = { 0xffffffff, 0x67efff },
       .mask.bitset     = { 0xffffffff, 0xe7f7efff },
       .decode = decode_dcflu_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_dcflu_gen_0_fields),
       .decode_fields = decode_dcflu_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &dcflu__default_gen_0,
       },
};
static const struct isa_case ccinv__default_gen_700 = {
       .display  = "{SY}{SS}{JP}{NAME}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ccinv_gen_700 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "ccinv",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe2d20000 },
       .dontcare.bitset = { 0xffffffff, 0x5efff },
       .mask.bitset     = { 0xffffffff, 0xe7ffefff },
       .decode = decode_ccinv_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_ccinv_gen_700_fields),
       .decode_fields = decode_ccinv_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &ccinv__default_gen_700,
       },
};
static const struct isa_case lock__default_gen_700 = {
       .display  = "{SY}{SS}{JP}{NAME}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_lock_gen_700 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "lock",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe3c20000 },
       .dontcare.bitset = { 0xffffffff, 0xfff },
       .mask.bitset     = { 0xffffffff, 0xe7ffefff },
       .decode = decode_lock_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_lock_gen_700_fields),
       .decode_fields = decode_lock_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &lock__default_gen_700,
       },
};
static const struct isa_case unlock__default_gen_700 = {
       .display  = "{SY}{SS}{JP}{NAME}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_unlock_gen_700 = {

       .parent   = &bitset___instruction_cat7_gen_0,
       .name     = "unlock",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe3ca0000 },
       .dontcare.bitset = { 0xffffffff, 0xfff },
       .mask.bitset     = { 0xffffffff, 0xe7ffefff },
       .decode = decode_unlock_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_unlock_gen_700_fields),
       .decode_fields = decode_unlock_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &unlock__default_gen_700,
       },
};
static const struct isa_case __alias_immed_src__case0_gen_0 = {
       .expr     = &expr_anon_44,
       .display  = "h({IMMED})",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 15,
            .type = TYPE_FLOAT,
          },
       },
};
static const struct isa_case __alias_immed_src__case1_gen_0 = {
       .expr     = &expr_anon_45,
       .display  = "({IMMED})",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_FLOAT,
          },
       },
};
static const struct isa_case __alias_immed_src__case2_gen_0 = {
       .expr     = &expr_anon_46,
       .display  = "h(0x{IMMED})",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 15,
            .type = TYPE_HEX,
          },
       },
};
static const struct isa_case __alias_immed_src__default_gen_0 = {
       .display  = "(0x{IMMED})",
       .num_fields = 1,
       .fields   = {
          { .name = "IMMED", .low = 0, .high = 31,
            .type = TYPE_HEX,
          },
       },
};
static const struct isa_bitset bitset___alias_immed_src_gen_0 = {

       .name     = "#alias-immed-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___alias_immed_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___alias_immed_src_gen_0_fields),
       .decode_fields = decode___alias_immed_src_gen_0_fields,
       .num_cases = 4,
       .cases    = {
            &__alias_immed_src__case0_gen_0,
            &__alias_immed_src__case1_gen_0,
            &__alias_immed_src__case2_gen_0,
            &__alias_immed_src__default_gen_0,
       },
};
static const struct isa_case __alias_const_src__default_gen_0 = {
       .display  = "{HALF}{CONST}",
       .num_fields = 2,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr_anon_47,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "CONST", .low = 0, .high = 10,
            .type = TYPE_BITSET,
            .bitsets = __reg_const,
          },
       },
};
static const struct isa_bitset bitset___alias_const_src_gen_0 = {

       .name     = "#alias-const-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___alias_const_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___alias_const_src_gen_0_fields),
       .decode_fields = decode___alias_const_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__alias_const_src__default_gen_0,
       },
};
static const struct isa_case __alias_gpr_src__default_gen_0 = {
       .display  = "{HALF}{SRC}",
       .num_fields = 2,
       .fields   = {
          { .name = "HALF", .low = 0, .high = 0,
            .expr = &expr_anon_48,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset___alias_gpr_src_gen_0 = {

       .name     = "#alias-gpr-src",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___alias_gpr_src_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___alias_gpr_src_gen_0_fields),
       .decode_fields = decode___alias_gpr_src_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__alias_gpr_src__default_gen_0,
       },
};
static const struct isa_case __dst_rt__default_gen_0 = {
       .display  = "rt{RT}.{SWIZ}",
       .num_fields = 2,
       .fields   = {
          { .name = "SWIZ", .low = 0, .high = 1,
            .type = TYPE_ENUM,
            .enums = &enum___swiz,
          },
          { .name = "RT", .low = 2, .high = 4,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset___dst_rt_gen_0 = {

       .name     = "#dst-rt",
       .gen      = {
           .min  = 0,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___dst_rt_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode___dst_rt_gen_0_fields),
       .decode_fields = decode___dst_rt_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &__dst_rt__default_gen_0,
       },
};
static const struct isa_field_params alias__case0_gen_700_src = {
       .num_params = 1,
       .params = {
           { .name= "TYPE_SIZE",  .as = "TYPE_SIZE" },

       },
};
static const struct isa_case alias__case0_gen_700 = {
       .expr     = &expr_anon_51,
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 7,
            .type = TYPE_BITSET,
            .bitsets = __alias_gpr_src,
            .params = &alias__case0_gen_700_src,
          },
       },
};
static const struct isa_field_params alias__case1_gen_700_src = {
       .num_params = 1,
       .params = {
           { .name= "TYPE_SIZE",  .as = "TYPE_SIZE" },

       },
};
static const struct isa_case alias__case1_gen_700 = {
       .expr     = &expr_anon_52,
       .num_fields = 1,
       .fields   = {
          { .name = "SRC", .low = 0, .high = 10,
            .type = TYPE_BITSET,
            .bitsets = __alias_const_src,
            .params = &alias__case1_gen_700_src,
          },
       },
};
static const struct isa_case alias__case2_gen_700 = {
       .expr     = &expr_anon_53,
       .num_fields = 2,
       .fields   = {
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr___false,
            .display = "",
            .type = TYPE_BOOL,
          },
          { .name = "DST", .low = 32, .high = 36,
            .type = TYPE_BITSET,
            .bitsets = __dst_rt,
          },
       },
};
static const struct isa_field_params alias__default_gen_700_src = {
       .num_params = 2,
       .params = {
           { .name= "TYPE_SIZE",  .as = "TYPE_SIZE" },
           { .name= "TYPE",  .as = "TYPE" },

       },
};
static const struct isa_case alias__default_gen_700 = {
       .display  = "{SY}{SS}{JP}{NAME}.{SCOPE}.{TYPE}{TYPE_SIZE}.{TABLE_SIZE_MINUS_ONE} {DST_HALF}{DST}, {SRC}",
       .num_fields = 13,
       .fields   = {
          { .name = "SCOPE", .low = 0, .high = 0,
            .expr = &expr_anon_49,
            .type = TYPE_ENUM,
            .enums = &enum___alias_scope,
          },
          { .name = "DST_HALF", .low = 0, .high = 0,
            .expr = &expr_anon_50,
            .display = "h",
            .type = TYPE_BOOL,
          },
          { .name = "SRC", .low = 0, .high = 31,
            .type = TYPE_BITSET,
            .bitsets = __alias_immed_src,
            .params = &alias__default_gen_700_src,
          },
          { .name = "DST", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "TABLE_SIZE_MINUS_ONE", .low = 40, .high = 43,
            .type = TYPE_UINT,
          },
          { .name = "SS", .low = 44, .high = 44,
            .display = "(ss)",
            .type = TYPE_BOOL,
          },
          { .name = "SCOPE_LO", .low = 47, .high = 47,
            .type = TYPE_UINT,
          },
          { .name = "TYPE", .low = 48, .high = 48,
            .type = TYPE_ENUM,
            .enums = &enum___alias_type,
          },
          { .name = "SCOPE_HI", .low = 49, .high = 49,
            .type = TYPE_UINT,
          },
          { .name = "TYPE_SIZE", .low = 50, .high = 50,
            .type = TYPE_ENUM,
            .enums = &enum___alias_type_size,
          },
          { .name = "SRC_REG_TYPE", .low = 51, .high = 52,
            .type = TYPE_ENUM,
            .enums = &enum___alias_src_reg_type,
          },
          { .name = "JP", .low = 59, .high = 59,
            .display = "(jp)",
            .type = TYPE_BOOL,
          },
          { .name = "SY", .low = 60, .high = 60,
            .display = "(sy)",
            .type = TYPE_BOOL,
          },
       },
};
static const struct isa_bitset bitset_alias_gen_700 = {

       .parent   = &bitset___instruction_gen_300,
       .name     = "alias",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0xe4000000 },
       .dontcare.bitset = { 0x0, 0x606000 },
       .mask.bitset     = { 0x0, 0xe7e06000 },
       .decode = decode_alias_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_alias_gen_700_fields),
       .decode_fields = decode_alias_gen_700_fields,
       .num_cases = 4,
       .cases    = {
            &alias__case0_gen_700,
            &alias__case1_gen_700,
            &alias__case2_gen_700,
            &alias__default_gen_700,
       },
};
static const struct isa_case __instruction__default_gen_300 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset___instruction_gen_300 = {

       .name     = "#instruction",
       .gen      = {
           .min  = 300,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x0, 0x0 },
       .dontcare.bitset = { 0x0, 0x0 },
       .mask.bitset     = { 0x0, 0x0 },
       .decode = decode___instruction_gen_300,
       .num_decode_fields = ARRAY_SIZE(decode___instruction_gen_300_fields),
       .decode_fields = decode___instruction_gen_300_fields,
       .num_cases = 1,
       .cases    = {
            &__instruction__default_gen_300,
       },
};
static const struct isa_case ldg_a__case0_gen_600 = {
       .expr     = &expr_anon_29,
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, g[{SRC1}+{SRC2}{OFF}], {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case ldg_a__case1_gen_600 = {
       .expr     = &expr_anon_30,
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, g[{SRC1}+({SRC2}<<{FULL_SHIFT})], {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case ldg_a__case2_gen_600 = {
       .expr     = &expr_anon_31,
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, g[{SRC1}+(({SRC2}{OFF})<<{TYPE_SHIFT})], {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case ldg_a__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, g[{SRC1}+((({SRC2}<<{SRC2_SHIFT}){OFF})<<{TYPE_SHIFT})], {SIZE}",
       .num_fields = 6,
       .fields   = {
          { .name = "TYPE_SHIFT", .low = 0, .high = 0,
            .expr = &expr___cat6_type_shift,
            .type = TYPE_UINT,
          },
          { .name = "FULL_SHIFT", .low = 0, .high = 0,
            .expr = &expr___cat6_full_shift,
            .type = TYPE_UINT,
          },
          { .name = "ldg.a#assert2", .low = 11, .high = 11,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
          { .name = "SRC2", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "OFF", .low = 9, .high = 10,
            .type = TYPE_UOFFSET,
          },
          { .name = "SRC2_SHIFT", .low = 12, .high = 13,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_ldg_a_gen_600 = {

       .parent   = &bitset___instruction_cat6_ldg_gen_0,
       .name     = "ldg.a",
       .gen      = {
           .min  = 600,
           .max  = 699,
       },
       .match.bitset    = { 0xc00001, 0xc0000000 },
       .dontcare.bitset = { 0x38000000, 0x1ff00 },
       .mask.bitset     = { 0xf8c00001, 0xe7f1ff00 },
       .decode = decode_ldg_a_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_ldg_a_gen_600_fields),
       .decode_fields = decode_ldg_a_gen_600_fields,
       .num_cases = 4,
       .cases    = {
            &ldg_a__case0_gen_600,
            &ldg_a__case1_gen_600,
            &ldg_a__case2_gen_600,
            &ldg_a__default_gen_600,
       },
};
static const struct isa_case ldg_a__default_gen_700 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} {TYPE_HALF}{DST}, g[{SRC1}+{SRC2}+{OFF}], {SIZE}",
       .num_fields = 3,
       .fields   = {
          { .name = "SRC2", .low = 1, .high = 8,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
          { .name = "SRC1_CONST", .low = 13, .high = 13,
            .type = TYPE_BOOL,
          },
          { .name = "OFF", .low = 41, .high = 48,
            .type = TYPE_UINT,
          },
       },
};
static const struct isa_bitset bitset_ldg_a_gen_700 = {

       .parent   = &bitset___instruction_cat6_ldg_gen_0,
       .name     = "ldg.a",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0xc00001, 0xc0000000 },
       .dontcare.bitset = { 0x38001e00, 0x100 },
       .mask.bitset     = { 0xf8c01e01, 0xe7f00100 },
       .decode = decode_ldg_a_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_ldg_a_gen_700_fields),
       .decode_fields = decode_ldg_a_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &ldg_a__default_gen_700,
       },
};
static const struct isa_case stg_a__case0_gen_600 = {
       .expr     = &expr_anon_33,
       .display  = "{SY}{JP}{NAME}.{TYPE} g[{SRC1}+{SRC2}{OFF}], {TYPE_HALF}{SRC3}, {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case stg_a__case1_gen_600 = {
       .expr     = &expr_anon_34,
       .display  = "{SY}{JP}{NAME}.{TYPE} g[{SRC1}+({SRC2}<<{FULL_SHIFT})], {TYPE_HALF}{SRC3}, {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case stg_a__case2_gen_600 = {
       .expr     = &expr_anon_35,
       .display  = "{SY}{JP}{NAME}.{TYPE} g[{SRC1}+(({SRC2}{OFF})<<{TYPE_SHIFT})], {TYPE_HALF}{SRC3}, {SIZE}",
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_case stg_a__default_gen_600 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} g[{SRC1}+((({SRC2}<<{SRC2_SHIFT}){OFF})<<{TYPE_SHIFT})], {TYPE_HALF}{SRC3}, {SIZE}",
       .num_fields = 6,
       .fields   = {
          { .name = "TYPE_SHIFT", .low = 0, .high = 0,
            .expr = &expr___cat6_type_shift,
            .type = TYPE_UINT,
          },
          { .name = "FULL_SHIFT", .low = 0, .high = 0,
            .expr = &expr___cat6_full_shift,
            .type = TYPE_UINT,
          },
          { .name = "stg.a#assert2", .low = 11, .high = 11,
            .type = TYPE_ASSERT,
            .val.bitset = { 0x0, 0x0 },
          },
          { .name = "OFF", .low = 9, .high = 10,
            .type = TYPE_UOFFSET,
          },
          { .name = "SRC2_SHIFT", .low = 12, .high = 13,
            .type = TYPE_UINT,
          },
          { .name = "SRC2", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset_stg_a_gen_600 = {

       .parent   = &bitset___instruction_cat6_stg_gen_0,
       .name     = "stg.a",
       .gen      = {
           .min  = 600,
           .max  = 699,
       },
       .match.bitset    = { 0x800000, 0xc0d00000 },
       .dontcare.bitset = { 0x387fc001, 0x0 },
       .mask.bitset     = { 0xf8ffc001, 0xe7f00000 },
       .decode = decode_stg_a_gen_600,
       .num_decode_fields = ARRAY_SIZE(decode_stg_a_gen_600_fields),
       .decode_fields = decode_stg_a_gen_600_fields,
       .num_cases = 4,
       .cases    = {
            &stg_a__case0_gen_600,
            &stg_a__case1_gen_600,
            &stg_a__case2_gen_600,
            &stg_a__default_gen_600,
       },
};
static const struct isa_case stg_a__default_gen_700 = {
       .display  = "{SY}{JP}{NAME}.{TYPE} g[{SRC1}+{SRC2}+{OFF}], {TYPE_HALF}{SRC3}, {SIZE}",
       .num_fields = 3,
       .fields   = {
          { .name = "SRC1_CONST", .low = 13, .high = 13,
            .type = TYPE_BOOL,
          },
          { .name = "OFF", .low = 14, .high = 21,
            .type = TYPE_UINT,
          },
          { .name = "SRC2", .low = 32, .high = 39,
            .type = TYPE_BITSET,
            .bitsets = __reg_gpr,
          },
       },
};
static const struct isa_bitset bitset_stg_a_gen_700 = {

       .parent   = &bitset___instruction_cat6_stg_gen_0,
       .name     = "stg.a",
       .gen      = {
           .min  = 700,
           .max  = 4294967295,
       },
       .match.bitset    = { 0x800000, 0xc0d00000 },
       .dontcare.bitset = { 0x38001e01, 0x0 },
       .mask.bitset     = { 0xf8c01e01, 0xe7f00000 },
       .decode = decode_stg_a_gen_700,
       .num_decode_fields = ARRAY_SIZE(decode_stg_a_gen_700_fields),
       .decode_fields = decode_stg_a_gen_700_fields,
       .num_cases = 1,
       .cases    = {
            &stg_a__default_gen_700,
       },
};
static const struct isa_case ldgb__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldgb_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_load_gen_0,
       .name     = "ldgb",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x1, 0xc6c00000 },
       .dontcare.bitset = { 0x1fe, 0x100100 },
       .mask.bitset     = { 0x1ff, 0xe7d00100 },
       .decode = decode_ldgb_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_ldgb_gen_0_fields),
       .decode_fields = decode_ldgb_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &ldgb__default_gen_0,
       },
};
static const struct isa_case ldgb__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_ldgb_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_load_gen_0,
       .name     = "ldgb",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x0, 0xc6c00000 },
       .dontcare.bitset = { 0x1ff, 0x100100 },
       .mask.bitset     = { 0x1ff, 0xe7d00100 },
       .decode = decode_ldgb_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_ldgb_gen_500_fields),
       .decode_fields = decode_ldgb_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &ldgb__default_gen_500,
       },
};
static const struct isa_case stgb__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stgb_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_store_a5xx_gen_0,
       .name     = "stgb",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc7000000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc001, 0xe7d00000 },
       .decode = decode_stgb_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_stgb_gen_500_fields),
       .decode_fields = decode_stgb_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &stgb__default_gen_500,
       },
};
static const struct isa_case stgb__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stgb_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_store_a4xx_gen_0,
       .name     = "stgb",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc7000000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc001, 0xe7d00000 },
       .decode = decode_stgb_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stgb_gen_0_fields),
       .decode_fields = decode_stgb_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stgb__default_gen_0,
       },
};
static const struct isa_case stib__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stib_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_store_a5xx_gen_0,
       .name     = "stib",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc7400000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc001, 0xe7d00000 },
       .decode = decode_stib_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_stib_gen_500_fields),
       .decode_fields = decode_stib_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &stib__default_gen_500,
       },
};
static const struct isa_case stib__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_stib_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_ibo_store_a4xx_gen_0,
       .name     = "stib",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc7400000 },
       .dontcare.bitset = { 0x7fc000, 0x100000 },
       .mask.bitset     = { 0x7fc001, 0xe7d00000 },
       .decode = decode_stib_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_stib_gen_0_fields),
       .decode_fields = decode_stib_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &stib__default_gen_0,
       },
};
static const struct isa_case atomic_s_add__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_add_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.add",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc4100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_add_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_add_gen_0_fields),
       .decode_fields = decode_atomic_s_add_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_add__default_gen_0,
       },
};
static const struct isa_case atomic_s_add__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_add_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.add",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc4100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_add_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_add_gen_500_fields),
       .decode_fields = decode_atomic_s_add_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_add__default_gen_500,
       },
};
static const struct isa_case atomic_s_sub__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_sub_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.sub",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc4500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_sub_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_sub_gen_0_fields),
       .decode_fields = decode_atomic_s_sub_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_sub__default_gen_0,
       },
};
static const struct isa_case atomic_s_sub__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_sub_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.sub",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc4500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_sub_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_sub_gen_500_fields),
       .decode_fields = decode_atomic_s_sub_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_sub__default_gen_500,
       },
};
static const struct isa_case atomic_s_xchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_xchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.xchg",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc4900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_xchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_xchg_gen_0_fields),
       .decode_fields = decode_atomic_s_xchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_xchg__default_gen_0,
       },
};
static const struct isa_case atomic_s_xchg__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_xchg_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.xchg",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc4900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_xchg_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_xchg_gen_500_fields),
       .decode_fields = decode_atomic_s_xchg_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_xchg__default_gen_500,
       },
};
static const struct isa_case atomic_s_inc__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_inc_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.inc",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc4d00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_inc_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_inc_gen_0_fields),
       .decode_fields = decode_atomic_s_inc_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_inc__default_gen_0,
       },
};
static const struct isa_case atomic_s_inc__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_inc_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.inc",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc4d00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_inc_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_inc_gen_500_fields),
       .decode_fields = decode_atomic_s_inc_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_inc__default_gen_500,
       },
};
static const struct isa_case atomic_s_dec__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_dec_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.dec",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc5100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_dec_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_dec_gen_0_fields),
       .decode_fields = decode_atomic_s_dec_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_dec__default_gen_0,
       },
};
static const struct isa_case atomic_s_dec__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_dec_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.dec",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc5100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_dec_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_dec_gen_500_fields),
       .decode_fields = decode_atomic_s_dec_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_dec__default_gen_500,
       },
};
static const struct isa_case atomic_s_cmpxchg__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_cmpxchg_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.cmpxchg",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc5500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_cmpxchg_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_cmpxchg_gen_0_fields),
       .decode_fields = decode_atomic_s_cmpxchg_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_cmpxchg__default_gen_0,
       },
};
static const struct isa_case atomic_s_cmpxchg__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_cmpxchg_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.cmpxchg",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc5500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_cmpxchg_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_cmpxchg_gen_500_fields),
       .decode_fields = decode_atomic_s_cmpxchg_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_cmpxchg__default_gen_500,
       },
};
static const struct isa_case atomic_s_min__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_min_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.min",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc5900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_min_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_min_gen_0_fields),
       .decode_fields = decode_atomic_s_min_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_min__default_gen_0,
       },
};
static const struct isa_case atomic_s_min__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_min_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.min",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc5900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_min_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_min_gen_500_fields),
       .decode_fields = decode_atomic_s_min_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_min__default_gen_500,
       },
};
static const struct isa_case atomic_s_max__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_max_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.max",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc5d00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_max_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_max_gen_0_fields),
       .decode_fields = decode_atomic_s_max_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_max__default_gen_0,
       },
};
static const struct isa_case atomic_s_max__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_max_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.max",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc5d00000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_max_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_max_gen_500_fields),
       .decode_fields = decode_atomic_s_max_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_max__default_gen_500,
       },
};
static const struct isa_case atomic_s_and__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_and_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.and",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc6100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_and_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_and_gen_0_fields),
       .decode_fields = decode_atomic_s_and_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_and__default_gen_0,
       },
};
static const struct isa_case atomic_s_and__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_and_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.and",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc6100000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_and_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_and_gen_500_fields),
       .decode_fields = decode_atomic_s_and_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_and__default_gen_500,
       },
};
static const struct isa_case atomic_s_or__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_or_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.or",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc6500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_or_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_or_gen_0_fields),
       .decode_fields = decode_atomic_s_or_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_or__default_gen_0,
       },
};
static const struct isa_case atomic_s_or__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_or_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.or",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc6500000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_or_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_or_gen_500_fields),
       .decode_fields = decode_atomic_s_or_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_or__default_gen_500,
       },
};
static const struct isa_case atomic_s_xor__default_gen_0 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_xor_gen_0 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a4xx_gen_0,
       .name     = "atomic.s.xor",
       .gen      = {
           .min  = 300,
           .max  = 499,
       },
       .match.bitset    = { 0x0, 0xc6900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_xor_gen_0,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_xor_gen_0_fields),
       .decode_fields = decode_atomic_s_xor_gen_0_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_xor__default_gen_0,
       },
};
static const struct isa_case atomic_s_xor__default_gen_500 = {
       .num_fields = 0,
       .fields   = {
       },
};
static const struct isa_bitset bitset_atomic_s_xor_gen_500 = {

       .parent   = &bitset___instruction_cat6_a3xx_atomic_global_a5xx_gen_0,
       .name     = "atomic.s.xor",
       .gen      = {
           .min  = 500,
           .max  = 599,
       },
       .match.bitset    = { 0x1, 0xc6900000 },
       .dontcare.bitset = { 0x0, 0x100 },
       .mask.bitset     = { 0x1, 0xe7d00100 },
       .decode = decode_atomic_s_xor_gen_500,
       .num_decode_fields = ARRAY_SIZE(decode_atomic_s_xor_gen_500_fields),
       .decode_fields = decode_atomic_s_xor_gen_500_fields,
       .num_cases = 1,
       .cases    = {
            &atomic_s_xor__default_gen_500,
       },
};

/*
 * bitset hierarchy root tables (where decoding starts from):
 */

static const struct isa_bitset *__reg_gpr[] = {
             &bitset___reg_gpr_gen_0,
    (void *)0
};
static const struct isa_bitset *__reg_const[] = {
             &bitset___reg_const_gen_0,
    (void *)0
};
static const struct isa_bitset *__reg_relative_gpr[] = {
             &bitset___reg_relative_gpr_gen_0,
    (void *)0
};
static const struct isa_bitset *__reg_relative_const[] = {
             &bitset___reg_relative_const_gen_0,
    (void *)0
};
static const struct isa_bitset *__multisrc[] = {
             &bitset___mulitsrc_immed_gen_0,
             &bitset___multisrc_immed_flut_full_gen_0,
             &bitset___multisrc_immed_flut_half_gen_0,
             &bitset___multisrc_gpr_gen_0,
             &bitset___multisrc_const_gen_0,
             &bitset___multisrc_relative_gpr_gen_0,
             &bitset___multisrc_relative_const_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_dst[] = {
             &bitset___cat1_dst_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_immed_src[] = {
             &bitset___cat1_immed_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_const_src[] = {
             &bitset___cat1_const_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_gpr_src[] = {
             &bitset___cat1_gpr_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_relative_gpr_src[] = {
             &bitset___cat1_relative_gpr_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_relative_const_src[] = {
             &bitset___cat1_relative_const_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_multi_src[] = {
             &bitset___cat1_multi_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat1_multi_dst[] = {
             &bitset___cat1_multi_dst_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat3_src[] = {
             &bitset___cat3_src_gpr_gen_0,
             &bitset___cat3_src_const_or_immed_gen_0,
             &bitset___cat3_src_relative_gpr_gen_0,
             &bitset___cat3_src_relative_const_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_s2en_bindless_base[] = {
             &bitset___cat5_s2en_bindless_base_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_src1[] = {
             &bitset___cat5_src1_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_src2[] = {
             &bitset___cat5_src2_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_samp[] = {
             &bitset___cat5_samp_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_samp_s2en_bindless_a1[] = {
             &bitset___cat5_samp_s2en_bindless_a1_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_tex_s2en_bindless_a1[] = {
             &bitset___cat5_tex_s2en_bindless_a1_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_tex[] = {
             &bitset___cat5_tex_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_tex_s2en_bindless[] = {
             &bitset___cat5_tex_s2en_bindless_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_type[] = {
             &bitset___cat5_type_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat5_src3[] = {
             &bitset___cat5_src3_gen_0,
    (void *)0
};
static const struct isa_bitset *__const_dst[] = {
             &bitset___const_dst_imm_gen_0,
             &bitset___const_dst_a1_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat6_typed[] = {
             &bitset___cat6_typed_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat6_base[] = {
             &bitset___cat6_base_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat6_src[] = {
             &bitset___cat6_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__cat6_src_const_or_gpr[] = {
             &bitset___cat6_src_const_or_gpr_gen_0,
    (void *)0
};
static const struct isa_bitset *__alias_immed_src[] = {
             &bitset___alias_immed_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__alias_const_src[] = {
             &bitset___alias_const_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__alias_gpr_src[] = {
             &bitset___alias_gpr_src_gen_0,
    (void *)0
};
static const struct isa_bitset *__dst_rt[] = {
             &bitset___dst_rt_gen_0,
    (void *)0
};
static const struct isa_bitset *__instruction[] = {
             &bitset_nop_gen_0,
             &bitset_end_gen_0,
             &bitset_ret_gen_0,
             &bitset_emit_gen_0,
             &bitset_cut_gen_0,
             &bitset_chmask_gen_0,
             &bitset_chsh_gen_0,
             &bitset_flow_rev_gen_0,
             &bitset_shpe_gen_0,
             &bitset_predt_gen_0,
             &bitset_predf_gen_0,
             &bitset_prede_gen_0,
             &bitset_kill_gen_0,
             &bitset_jump_gen_0,
             &bitset_call_gen_0,
             &bitset_bkt_gen_0,
             &bitset_getlast_gen_600,
             &bitset_getone_gen_0,
             &bitset_shps_gen_0,
             &bitset_brac_gen_0,
             &bitset_brax_gen_0,
             &bitset_br_gen_0,
             &bitset_bany_gen_0,
             &bitset_ball_gen_0,
             &bitset_brao_gen_0,
             &bitset_braa_gen_0,
             &bitset_mov_immed_gen_0,
             &bitset_mov_const_gen_0,
             &bitset_mov_gpr_gen_0,
             &bitset_movs_immed_gen_0,
             &bitset_movs_a0_gen_0,
             &bitset_mov_relgpr_gen_0,
             &bitset_mov_relconst_gen_0,
             &bitset_swz_gen_0,
             &bitset_gat_gen_0,
             &bitset_sct_gen_0,
             &bitset_movmsk_gen_0,
             &bitset_bary_f_gen_0,
             &bitset_flat_b_gen_600,
             &bitset_add_f_gen_0,
             &bitset_min_f_gen_0,
             &bitset_max_f_gen_0,
             &bitset_mul_f_gen_0,
             &bitset_sign_f_gen_0,
             &bitset_cmps_f_gen_0,
             &bitset_absneg_f_gen_0,
             &bitset_cmpv_f_gen_0,
             &bitset_floor_f_gen_0,
             &bitset_ceil_f_gen_0,
             &bitset_rndne_f_gen_0,
             &bitset_rndaz_f_gen_0,
             &bitset_trunc_f_gen_0,
             &bitset_add_u_gen_0,
             &bitset_add_s_gen_0,
             &bitset_sub_u_gen_0,
             &bitset_sub_s_gen_0,
             &bitset_cmps_u_gen_0,
             &bitset_cmps_s_gen_0,
             &bitset_min_u_gen_0,
             &bitset_min_s_gen_0,
             &bitset_max_u_gen_0,
             &bitset_max_s_gen_0,
             &bitset_absneg_s_gen_0,
             &bitset_and_b_gen_0,
             &bitset_or_b_gen_0,
             &bitset_not_b_gen_0,
             &bitset_xor_b_gen_0,
             &bitset_cmpv_u_gen_0,
             &bitset_cmpv_s_gen_0,
             &bitset_mul_u24_gen_0,
             &bitset_mul_s24_gen_0,
             &bitset_mull_u_gen_0,
             &bitset_bfrev_b_gen_0,
             &bitset_clz_s_gen_0,
             &bitset_clz_b_gen_0,
             &bitset_shl_b_gen_0,
             &bitset_shr_b_gen_0,
             &bitset_ashr_b_gen_0,
             &bitset_mgen_b_gen_0,
             &bitset_getbit_b_gen_0,
             &bitset_setrm_gen_0,
             &bitset_cbits_b_gen_0,
             &bitset_shb_gen_0,
             &bitset_msad_gen_0,
             &bitset_mad_u16_gen_0,
             &bitset_madsh_u16_gen_0,
             &bitset_mad_s16_gen_0,
             &bitset_madsh_m16_gen_0,
             &bitset_mad_u24_gen_0,
             &bitset_mad_s24_gen_0,
             &bitset_mad_f16_gen_0,
             &bitset_mad_f32_gen_0,
             &bitset_sel_b16_gen_0,
             &bitset_sel_b32_gen_0,
             &bitset_sel_s16_gen_0,
             &bitset_sel_s32_gen_0,
             &bitset_sel_f16_gen_0,
             &bitset_sel_f32_gen_0,
             &bitset_sad_s16_gen_0,
             &bitset_sad_s32_gen_0,
             &bitset_shrm_gen_0,
             &bitset_shlm_gen_0,
             &bitset_shrg_gen_0,
             &bitset_shlg_gen_0,
             &bitset_andg_gen_0,
             &bitset_dp2acc_gen_0,
             &bitset_dp4acc_gen_0,
             &bitset_wmm_gen_0,
             &bitset_wmm_accu_gen_0,
             &bitset_rcp_gen_0,
             &bitset_rsq_gen_0,
             &bitset_log2_gen_0,
             &bitset_exp2_gen_0,
             &bitset_sin_gen_0,
             &bitset_cos_gen_0,
             &bitset_sqrt_gen_0,
             &bitset_hrsq_gen_0,
             &bitset_hlog2_gen_0,
             &bitset_hexp2_gen_0,
             &bitset_isam_gen_0,
             &bitset_isaml_gen_0,
             &bitset_isamm_gen_0,
             &bitset_sam_gen_0,
             &bitset_samb_gen_0,
             &bitset_saml_gen_0,
             &bitset_samgq_gen_0,
             &bitset_getlod_gen_0,
             &bitset_conv_gen_0,
             &bitset_convm_gen_0,
             &bitset_getsize_gen_0,
             &bitset_getbuf_gen_0,
             &bitset_getpos_gen_0,
             &bitset_getinfo_gen_0,
             &bitset_dsx_gen_0,
             &bitset_dsy_gen_0,
             &bitset_gather4r_gen_0,
             &bitset_gather4g_gen_0,
             &bitset_gather4b_gen_0,
             &bitset_gather4a_gen_0,
             &bitset_samgp0_gen_0,
             &bitset_samgp1_gen_0,
             &bitset_samgp2_gen_0,
             &bitset_samgp3_gen_0,
             &bitset_dsxpp_1_gen_0,
             &bitset_dsypp_1_gen_0,
             &bitset_rgetpos_gen_0,
             &bitset_rgetinfo_gen_0,
             &bitset_tcinv_gen_0,
             &bitset_brcst_active_gen_600,
             &bitset_quad_shuffle_brcst_gen_0,
             &bitset_quad_shuffle_horiz_gen_0,
             &bitset_quad_shuffle_vert_gen_0,
             &bitset_quad_shuffle_diag_gen_0,
             &bitset_ldg_gen_0,
             &bitset_ldg_a_gen_600,
             &bitset_ldg_a_gen_700,
             &bitset_ldg_k_gen_600,
             &bitset_stg_gen_0,
             &bitset_stg_a_gen_600,
             &bitset_stg_a_gen_700,
             &bitset_ldl_gen_0,
             &bitset_ldp_gen_0,
             &bitset_ldlw_gen_0,
             &bitset_ldlv_gen_0,
             &bitset_stl_gen_0,
             &bitset_stp_gen_0,
             &bitset_stlw_gen_0,
             &bitset_stc_gen_600,
             &bitset_stsc_gen_700,
             &bitset_resinfo_gen_0,
             &bitset_ldgb_gen_0,
             &bitset_ldgb_gen_500,
             &bitset_ldib_gen_0,
             &bitset_stgb_gen_500,
             &bitset_stgb_gen_0,
             &bitset_stib_gen_500,
             &bitset_stib_gen_0,
             &bitset_atomic_add_gen_0,
             &bitset_atomic_sub_gen_0,
             &bitset_atomic_xchg_gen_0,
             &bitset_atomic_inc_gen_0,
             &bitset_atomic_dec_gen_0,
             &bitset_atomic_cmpxchg_gen_0,
             &bitset_atomic_min_gen_0,
             &bitset_atomic_max_gen_0,
             &bitset_atomic_and_gen_0,
             &bitset_atomic_or_gen_0,
             &bitset_atomic_xor_gen_0,
             &bitset_atomic_s_add_gen_0,
             &bitset_atomic_s_add_gen_500,
             &bitset_atomic_s_sub_gen_0,
             &bitset_atomic_s_sub_gen_500,
             &bitset_atomic_s_xchg_gen_0,
             &bitset_atomic_s_xchg_gen_500,
             &bitset_atomic_s_inc_gen_0,
             &bitset_atomic_s_inc_gen_500,
             &bitset_atomic_s_dec_gen_0,
             &bitset_atomic_s_dec_gen_500,
             &bitset_atomic_s_cmpxchg_gen_0,
             &bitset_atomic_s_cmpxchg_gen_500,
             &bitset_atomic_s_min_gen_0,
             &bitset_atomic_s_min_gen_500,
             &bitset_atomic_s_max_gen_0,
             &bitset_atomic_s_max_gen_500,
             &bitset_atomic_s_and_gen_0,
             &bitset_atomic_s_and_gen_500,
             &bitset_atomic_s_or_gen_0,
             &bitset_atomic_s_or_gen_500,
             &bitset_atomic_s_xor_gen_0,
             &bitset_atomic_s_xor_gen_500,
             &bitset_atomic_g_add_gen_0,
             &bitset_atomic_g_sub_gen_0,
             &bitset_atomic_g_xchg_gen_0,
             &bitset_atomic_g_inc_gen_0,
             &bitset_atomic_g_dec_gen_0,
             &bitset_atomic_g_cmpxchg_gen_0,
             &bitset_atomic_g_min_gen_0,
             &bitset_atomic_g_max_gen_0,
             &bitset_atomic_g_and_gen_0,
             &bitset_atomic_g_or_gen_0,
             &bitset_atomic_g_xor_gen_0,
             &bitset_ray_intersection_gen_700,
             &bitset_ldc_k_gen_0,
             &bitset_ldc_gen_0,
             &bitset_getspid_gen_0,
             &bitset_getwid_gen_0,
             &bitset_getfiberid_gen_600,
             &bitset_resinfo_b_gen_0,
             &bitset_resbase_gen_0,
             &bitset_stib_b_gen_0,
             &bitset_ldib_b_gen_0,
             &bitset_atomic_b_add_gen_0,
             &bitset_atomic_b_sub_gen_0,
             &bitset_atomic_b_xchg_gen_0,
             &bitset_atomic_b_cmpxchg_gen_0,
             &bitset_atomic_b_min_gen_0,
             &bitset_atomic_b_max_gen_0,
             &bitset_atomic_b_and_gen_0,
             &bitset_atomic_b_or_gen_0,
             &bitset_atomic_b_xor_gen_0,
             &bitset_shfl_gen_600,
             &bitset_bar_gen_0,
             &bitset_fence_gen_0,
             &bitset_sleep_gen_0,
             &bitset_icinv_gen_0,
             &bitset_dccln_gen_0,
             &bitset_dcinv_gen_0,
             &bitset_dcflu_gen_0,
             &bitset_ccinv_gen_700,
             &bitset_lock_gen_700,
             &bitset_unlock_gen_700,
             &bitset_alias_gen_700,
    (void *)0
};

#include "isaspec_decode_impl.c"

static void decode___reg_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___reg_const_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___reg_relative_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___reg_relative_const_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___mulitsrc_immed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___mulitsrc_immed_flut_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_immed_flut_full_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_immed_flut_half_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_const_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_relative_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_relative_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___multisrc_relative_const_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_0src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_nop_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_end_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ret_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_emit_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cut_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_chmask_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_chsh_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_flow_rev_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shpe_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_predt_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_predf_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_prede_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_1src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_kill_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_immed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_jump_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_call_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_bkt_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getlast_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_getone_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shps_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_branch_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_brac_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_brax_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_branch_1src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_br_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_bany_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ball_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat0_branch_2src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_brao_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_braa_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_dst_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_typed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_mov_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_immed_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_const_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_gpr_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_relative_gpr_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_relative_const_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mov_immed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mov_const_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_mov_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mov_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_movs_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_movs_immed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_movs_a0_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_relative_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mov_relgpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mov_relconst_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_multi_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat1_multi_dst_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat1_multi_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_swz_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_gat_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sct_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_movmsk_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat2_1src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat2_2src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat2_2src_cond_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat2_2src_input_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_bary_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_flat_b_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_add_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_min_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_max_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mul_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sign_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cmps_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_absneg_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cmpv_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_floor_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ceil_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_rndne_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_rndaz_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_trunc_f_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_add_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_add_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sub_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sub_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cmps_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cmps_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_min_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_min_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_max_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_max_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_absneg_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_and_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_or_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_not_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_xor_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cmpv_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cmpv_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mul_u24_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mul_s24_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mull_u_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_bfrev_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_clz_s_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_clz_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shl_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shr_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ashr_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mgen_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getbit_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_setrm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cbits_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shb_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_msad_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat3_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat3_src_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat3_src_const_or_immed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat3_src_relative_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat3_src_relative_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat3_src_relative_const_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat3_base_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat3_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat3_alt_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_mad_u16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_madsh_u16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mad_s16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_madsh_m16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mad_u24_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mad_s24_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mad_f16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_mad_f32_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sel_b16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sel_b32_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sel_s16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sel_s32_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sel_f16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sel_f32_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sad_s16_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sad_s32_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shrm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shlm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shrg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shlg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_andg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat3_dp_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_dp2acc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dp4acc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat3_wmm_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_wmm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_wmm_accu_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat4_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_rcp_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_rsq_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_log2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_exp2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sin_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_cos_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sqrt_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_hrsq_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_hlog2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_hexp2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_s2en_bindless_base_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat5_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat5_tex_base_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat5_tex_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_isam_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_isaml_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_isamm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sam_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_samb_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_saml_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_samgq_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getlod_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_conv_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_convm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getsize_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getbuf_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getpos_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getinfo_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dsx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dsy_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_gather4r_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_gather4g_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_gather4b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_gather4a_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_samgp0_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_samgp1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_samgp2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_samgp3_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dsxpp_1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dsypp_1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_rgetpos_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_rgetinfo_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_tcinv_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat5_brcst_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_brcst_active_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat5_quad_shuffle_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_quad_shuffle_brcst_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_quad_shuffle_horiz_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_quad_shuffle_vert_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_quad_shuffle_diag_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_src1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_src2_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_samp_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_samp_s2en_bindless_a1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_tex_s2en_bindless_a1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_tex_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_tex_s2en_bindless_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_type_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat5_src3_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___const_dst_imm_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___const_dst_a1_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___const_dst_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_ldg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldg_k_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_stg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_ld_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldl_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldp_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldlw_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldlv_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_st_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stl_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stp_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stlw_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stc_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_stsc_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode_resinfo_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_ibo_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_ibo_load_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldib_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_ibo_store_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_ibo_store_a4xx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_ibo_store_a5xx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_local_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_1src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_2src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_add_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_sub_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_xchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_inc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_dec_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_cmpxchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_min_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_max_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_and_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_or_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_xor_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_global_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_global_a4xx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a3xx_atomic_global_a5xx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_atomic_global_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_add_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_sub_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_xchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_inc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_dec_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_cmpxchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_min_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_max_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_and_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_or_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_g_xor_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ray_intersection_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_base_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat6_ldc_common_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldc_k_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getspid_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getwid_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_getfiberid_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_ibo_1src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_resinfo_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_resbase_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_ibo_base_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_ibo_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_ibo_load_store_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat6_a6xx_ibo_atomic_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stib_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldib_b_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_add_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_sub_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_xchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_cmpxchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_min_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_max_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_and_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_or_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_b_xor_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_shfl_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode___cat6_typed_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat6_base_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat6_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___cat6_src_const_or_gpr_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat7_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat7_barrier_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_bar_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_fence_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_cat7_data_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_sleep_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_icinv_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dccln_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dcinv_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_dcflu_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ccinv_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode_lock_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode_unlock_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode___alias_immed_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___alias_const_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___alias_gpr_src_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode___dst_rt_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_alias_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode___instruction_gen_300(void *out, struct decode_scope *scope)
{
}
static void decode_ldg_a_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_ldg_a_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode_stg_a_gen_600(void *out, struct decode_scope *scope)
{
}
static void decode_stg_a_gen_700(void *out, struct decode_scope *scope)
{
}
static void decode_ldgb_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_ldgb_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_stgb_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_stgb_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_stib_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_stib_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_add_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_add_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_sub_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_sub_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_xchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_xchg_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_inc_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_inc_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_dec_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_dec_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_cmpxchg_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_cmpxchg_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_min_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_min_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_max_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_max_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_and_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_and_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_or_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_or_gen_500(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_xor_gen_0(void *out, struct decode_scope *scope)
{
}
static void decode_atomic_s_xor_gen_500(void *out, struct decode_scope *scope)
{
}

void ir3_isa_disasm(void *bin, int sz, FILE *out, const struct isa_decode_options *options)
{
    isa_disasm(bin, sz, out, options);
}

bool ir3_isa_decode(void *out, void *bin, const struct isa_decode_options *options)
{
    return isa_decode(out, bin, options);
}

uint32_t ir3_isa_get_gpu_id(struct decode_scope *scope)
{
    return isa_get_gpu_id(scope);
}
