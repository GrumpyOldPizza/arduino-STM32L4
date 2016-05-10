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

#if !defined(_DOSFS_PORT_h)
#define _DOSFS_PORT_h

#ifdef __cplusplus
 extern "C" {
#endif

extern bool     stm32l4_sdcard_spi_init(dosfs_sdcard_t *sdcard);
extern bool     stm32l4_sdcard_spi_present(dosfs_sdcard_t *sdcard);
extern uint32_t stm32l4_sdcard_spi_mode(dosfs_sdcard_t *sdcard, int mode);
extern void     stm32l4_sdcard_spi_select(dosfs_sdcard_t *sdcard);
extern void     stm32l4_sdcard_spi_deselect(dosfs_sdcard_t *sdcard);
extern void     stm32l4_sdcard_spi_send(dosfs_sdcard_t *sdcard, uint8_t data);
extern uint8_t  stm32l4_sdcard_spi_receive(dosfs_sdcard_t *sdcard);
extern void     stm32l4_sdcard_spi_send_block(dosfs_sdcard_t *sdcard, const uint8_t *data);
extern uint32_t stm32l4_sdcard_spi_receive_block(dosfs_sdcard_t *sdcard, uint8_t *data);

#define DOSFS_PORT_SDCARD_SPI_INIT(_sdcard)                  stm32l4_sdcard_spi_init((_sdcard))
#define DOSFS_PORT_SDCARD_SPI_PRESENT(_sdcard)               stm32l4_sdcard_spi_present((_sdcard))
#define DOSFS_PORT_SDCARD_SPI_MODE(_sdcard, _mode)           stm32l4_sdcard_spi_mode((_sdcard), (_mode))
#define DOSFS_PORT_SDCARD_SPI_SELECT(_sdcard)                stm32l4_sdcard_spi_select((_sdcard))
#define DOSFS_PORT_SDCARD_SPI_DESELECT(_sdcard)              stm32l4_sdcard_spi_deselect((_sdcard))
#define DOSFS_PORT_SDCARD_SPI_SEND(_sdcard, _data)           stm32l4_sdcard_spi_send((_sdcard), (_data))
#define DOSFS_PORT_SDCARD_SPI_RECEIVE(_sdcard)               stm32l4_sdcard_spi_receive((_sdcard))

#define DOSFS_PORT_SDCARD_SPI_SEND_BLOCK(_sdcard, _data)     stm32l4_sdcard_spi_send_block((_sdcard), (_data))
#define DOSFS_PORT_SDCARD_SPI_RECEIVE_BLOCK(_sdcard, _data)  stm32l4_sdcard_spi_receive_block((_sdcard), (_data))

#ifdef __cplusplus
}
#endif

#endif /* _DOSFS_PORT_h */
