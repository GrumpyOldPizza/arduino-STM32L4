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

#if !defined(_STM32L4_SDMMC_H)
#define _STM32L4_SDMMC_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dosfs_device.h"

#include "stm32l4_gpio.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct _stm32l4_sdmmc_t stm32l4_sdmmc_t;

#define STM32L4_SDMMC_STATE_NONE                   0
#define STM32L4_SDMMC_STATE_INIT                   1
#define STM32L4_SDMMC_STATE_RESET                  2
#define STM32L4_SDMMC_STATE_READY                  3
#define STM32L4_SDMMC_STATE_READ_MULTIPLE          4
#define STM32L4_SDMMC_STATE_WRITE_MULTIPLE         5
#define STM32L4_SDMMC_STATE_WRITE_STOP             6

#define STM32L4_SDMMC_OPTION_HIGH_SPEED            0x01

#define STM32L4_SDMMC_MODE_NONE                    0
#define STM32L4_SDMMC_MODE_IDENTIFY                1
#define STM32L4_SDMMC_MODE_DATA_TRANSFER           2
#define STM32L4_SDMMC_MODE_DATA_TRANSFER_WIDE      3
#define STM32L4_SDMMC_MODE_DATA_TRANSFER_WIDE_HS   4

struct _stm32l4_sdmmc_t {
    uint8_t                 state;
    uint8_t                 media;
    uint8_t                 option;
    uint8_t                 shift;
    uint32_t                speed;
    uint32_t                au_size;
    uint32_t                erase_size;
    uint32_t                erase_timeout;
    uint32_t                erase_offset;
    uint32_t                address;
    uint32_t                count;
    volatile uint8_t        *p_status;
    uint32_t                read_timeout;
    uint32_t                write_timeout;
    uint32_t                RCA;
    uint32_t                OCR;
    uint8_t                 CID[16];
    uint8_t                 CSD[16];
    uint8_t                 SCR[8];
    uint8_t                 SSR[64];

#if (DOSFS_CONFIG_STATISTICS == 1)
    struct {
        uint32_t                sdcard_idle;
        uint32_t                sdcard_reset;
        uint32_t                sdcard_command;
        uint32_t                sdcard_command_crcfail;
        uint32_t                sdcard_command_timeout;
        uint32_t                sdcard_command_retry;
        uint32_t                sdcard_command_fail;
        uint32_t                sdcard_transmit;
        uint32_t                sdcard_transmit_crcfail;
        uint32_t                sdcard_transmit_timeout;
        uint32_t                sdcard_transmit_timeout2;
        uint32_t                sdcard_transmit_retry;
        uint32_t                sdcard_transmit_fail;
        uint32_t                sdcard_receive;
        uint32_t                sdcard_receive_crcfail;
        uint32_t                sdcard_receive_timeout;
        uint32_t                sdcard_receive_retry;
        uint32_t                sdcard_receive_fail;
	uint32_t                sdcard_erase;
	uint32_t                sdcard_erase_timeout;
	uint32_t                sdcard_read_single;
	uint32_t                sdcard_read_multiple;
	uint32_t                sdcard_write_single;
	uint32_t                sdcard_write_multiple;
        uint32_t                sdcard_read_stop;
        uint32_t                sdcard_write_stop;
        uint32_t                sdcard_write_sync;
        uint32_t                sdcard_command_crcfail_2[64];
        uint32_t                sdcard_command_timeout_2[64];
    }                       statistics;
#endif /* (DOSFS_CONFIG_STATISTICS == 1) */
};

extern int stm32l4_sdmmc_initialize(uint32_t option);

#if (DOSFS_CONFIG_STATISTICS == 1)

#define STM32L4_SDMMC_STATISTICS_COUNT(_name)         { sdmmc->statistics._name += 1; }
#define STM32L4_SDMMC_STATISTICS_COUNT_N(_name,_n)    { sdmmc->statistics._name += (_n); }

#else /* (DOSFS_CONFIG_STATISTICS == 1) */

#define STM32L4_SDMMC_STATISTICS_COUNT(_name)         /**/
#define STM32L4_SDMMC_STATISTICS_COUNT_N(_name,_n)    /**/

#endif /* (DOSFS_CONFIG_STATISTICS == 1) */

#ifdef __cplusplus
}
#endif

#endif /*_STM32L4_SDMMC_H */
