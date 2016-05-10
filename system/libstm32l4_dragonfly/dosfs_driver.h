/*
 * Copyright (c) 2014 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#if !defined(_DOSFS_DRIVER_H)
#define _DOSFS_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dosfs_api.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct _dosfs_driver_t dosfs_driver_t;

#define DOSFS_BLK_SIZE       512
#define DOSFS_BLK_MASK       511
#define DOSFS_BLK_SHIFT      9

#define DOSFS_MEDIA_NONE     0
#define DOSFS_MEDIA_SFLASH   1
#define DOSFS_MEDIA_SDSC     2
#define DOSFS_MEDIA_SDHC     3

typedef struct _dosfs_interface_t {
    int                     (*release)(void *context);
    int                     (*info)(void *context, uint8_t *p_type, uint8_t *p_write_protected, uint32_t *p_block_count, uint32_t *p_au_size, uint32_t *p_serial);
    int                     (*format)(void *context);
    int                     (*reclaim)(void *context, uint32_t size);
    int                     (*discard)(void *context, uint32_t address, uint32_t length);
    int                     (*read)(void *context, uint32_t address, uint8_t *data);
    int                     (*read_sequential)(void *context, uint32_t address, uint32_t length, uint8_t *data);
    int                     (*write)(void *context, uint32_t address, const uint8_t *data);
    int                     (*write_sequential)(void *context, uint32_t address, uint32_t length, const uint8_t *data, volatile uint8_t *p_status);
    int                     (*sync)(void *context, volatile uint8_t *p_status);
} dosfs_interface_t;

#ifdef __cplusplus
}
#endif

#endif /*_DOSFS_DRIVER_H */
