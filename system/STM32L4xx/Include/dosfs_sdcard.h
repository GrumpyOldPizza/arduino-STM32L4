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

#if !defined(_DOSFS_SDCARD_H)
#define _DOSFS_SDCARD_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dosfs_driver.h"

#include "stm32l4_gpio.h"
#include "stm32l4_spi.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct _dosfs_sdcard_t dosfs_sdcard_t;

#define DOSFS_SDCARD_STATE_NONE              0
#define DOSFS_SDCARD_STATE_INIT              1
#define DOSFS_SDCARD_STATE_RESET             2
#define DOSFS_SDCARD_STATE_READY             3
#define DOSFS_SDCARD_STATE_WRITE_MULTIPLE    4

#define DOSFS_SDCARD_TYPE_NONE               0
#define DOSFS_SDCARD_TYPE_SDSC               1
#define DOSFS_SDCARD_TYPE_SDHC               2

#define DOSFS_SDCARD_FLAG_COMMAND_SUBSEQUENT 0x01

#define DOSFS_SDCARD_MODE_NONE               0
#define DOSFS_SDCARD_MODE_IDENTIFY           1
#define DOSFS_SDCARD_MODE_DATA_TRANSFER      2
#define DOSFS_SDCARD_MODE_DATA_TRANSFER_HS   3

struct _dosfs_sdcard_t {
    uint8_t                 state;
    uint8_t                 media;
    uint8_t                 flags;
    uint8_t                 shift;
    uint32_t                speed;
    uint8_t                 response[8];
    uint32_t                address;
    uint32_t                count;
    volatile uint8_t        *p_status;

#if (DOSFS_CONFIG_SDCARD_SIMULATE == 1)
    uint8_t                 *image;
#endif /* (DOSFS_CONFIG_SDCARD_SIMULATE == 1) */

#if (DOSFS_CONFIG_STATISTICS == 1)
    struct {
        uint32_t                sdcard_reset;
        uint32_t                sdcard_send_command;
        uint32_t                sdcard_send_command_retry;
        uint32_t                sdcard_send_command_fail;
        uint32_t                sdcard_send_data;
        uint32_t                sdcard_send_data_retry;
        uint32_t                sdcard_send_data_fail;
        uint32_t                sdcard_receive_data;
        uint32_t                sdcard_receive_data_retry;
        uint32_t                sdcard_receive_data_fail;
	uint32_t                sdcard_erase;
	uint32_t                sdcard_read_single;
	uint32_t                sdcard_read_multiple;
	uint32_t                sdcard_write_single;
	uint32_t                sdcard_write_multiple;
	uint32_t                sdcard_write_coalesce;
    }                       statistics;
#endif /* (DOSFS_CONFIG_STATISTICS == 1) */

    uint8_t                 pin_cs;
    uint32_t                option;
    stm32l4_spi_t           spi;
};

#if 1 || (DOSFS_CONFIG_SDCARD_CRC == 1)

extern const uint8_t dosfs_crc7_table[256];
extern const uint16_t dosfs_crc16_table[256];

extern uint8_t dosfs_compute_crc7(const uint8_t *data, uint32_t count);
extern uint16_t dosfs_compute_crc16(const uint8_t *data, uint32_t count);

#define DOSFS_UPDATE_CRC7(_crc7, _data)    { (_crc7)  = dosfs_crc7_table[((uint8_t)(_crc7) << 1) ^ (uint8_t)(_data)]; }
#define DOSFS_UPDATE_CRC16(_crc16, _data)  { (_crc16) = (uint16_t)((_crc16) << 8) ^ dosfs_crc16_table[(uint8_t)((_crc16) >> 8) ^ (uint8_t)(_data)]; }

#endif /* (DOSFS_CONFIG_SDCARD_CRC == 1) */

#if (DOSFS_CONFIG_STATISTICS == 1)

#define DOSFS_SDCARD_STATISTICS_COUNT(_name)         { sdcard->statistics._name += 1; }
#define DOSFS_SDCARD_STATISTICS_COUNT_N(_name,_n)    { sdcard->statistics._name += (_n); }

#else /* (DOSFS_CONFIG_STATISTICS == 1) */

#define DOSFS_SDCARD_STATISTICS_COUNT(_name)         /**/
#define DOSFS_SDCARD_STATISTICS_COUNT_N(_name,_n)    /**/

#endif /* (DOSFS_CONFIG_STATISTICS == 1) */

#ifdef __cplusplus
}
#endif

#endif /*_DOSFS_SDCARD_H */
