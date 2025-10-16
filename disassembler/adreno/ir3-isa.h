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

#ifndef _IR3_ISA_H_
#define _IR3_ISA_H_

#include "isaspec.h"

#ifdef __cplusplus
extern "C" {
#endif

void ir3_isa_disasm(void *bin, int sz, FILE *out, const struct isa_decode_options *options);
bool ir3_isa_decode(void *out, void *bin, const struct isa_decode_options *options);

struct decode_scope;

uint32_t ir3_isa_get_gpu_id(struct decode_scope *scope);

/**
 * Allows to use gpu_id in expr functions
 */
#define ISA_GPU_ID() ir3_isa_get_gpu_id(scope)

#ifdef __cplusplus
}
#endif

#endif /* _IR3_ISA_H_ */

