/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
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

#if !defined(_DOSFS_SFLASH_H)
#define _DOSFS_SFLASH_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dosfs_device.h"

#include "stm32l4_qspi.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define DOSFS_SFLASH_ERASE_COUNT_THRESHOLD      16

#define DOSFS_SFLASH_SECTOR_IDENT_0             0x52444154     /* "RFAT" */
#define DOSFS_SFLASH_SECTOR_IDENT_1             0x4e4f3031     /* "NO01" */

#define DOSFS_SFLASH_BLOCK_NOT_ALLOCATED         0xffff
#define DOSFS_SFLASH_BLOCK_RESERVED              0xfffe
#define DOSFS_SFLASH_BLOCK_DELETED               0x0000

#define DOSFS_SFLASH_PAGE_SIZE                   0x00000100
#define DOSFS_SFLASH_BLOCK_SIZE                  0x00000200
#define DOSFS_SFLASH_ERASE_SIZE                  0x00010000
#define DOSFS_SFLASH_DATA_SIZE                   0x02000000

#define DOSFS_SFLASH_XLATE_ENTRIES               256
#define DOSFS_SFLASH_XLATE_SEGMENT_SHIFT         8
#define DOSFS_SFLASH_XLATE_INDEX_MASK            0x000000ff
#define DOSFS_SFLASH_XLATE_OFFSET                128
#define DOSFS_SFLASH_XLATE_COUNT                 ((((DOSFS_SFLASH_DATA_SIZE / DOSFS_SFLASH_ERASE_SIZE) * ((DOSFS_SFLASH_ERASE_SIZE / DOSFS_SFLASH_BLOCK_SIZE) -1) -2) + (DOSFS_SFLASH_XLATE_ENTRIES -1)) / DOSFS_SFLASH_XLATE_ENTRIES)

#define DOSFS_SFLASH_XLATE_ENTRY_NOT_ALLOCATED   0xffff

#define DOSFS_SFLASH_BLOCK_INFO_ENTRIES          128

#define DOSFS_SFLASH_LOGICAL_BLOCK_MASK          0x0000007f
#define DOSFS_SFLASH_LOGICAL_SECTOR_MASK         0x00007f80
#define DOSFS_SFLASH_LOGICAL_SECTOR_SHIFT        7

#define DOSFS_SFLASH_LOGICAL_BLOCK_NOT_ALLOCATED 0x0000

#define DOSFS_SFLASH_PHYSICAL_ILLEGAL            0xffffffff

#define DOSFS_SFLASH_VICTIM_DELETED_MASK         0xfe
#define DOSFS_SFLASH_VICTIM_DELETED_SHIFT        1
#define DOSFS_SFLASH_VICTIM_DELETED_INCREMENT    0x02
#define DOSFS_SFLASH_VICTIM_ALLOCATED_MASK       0x01

#define DOSFS_SFLASH_STATE_NONE                  0
#define DOSFS_SFLASH_STATE_READY                 1
#define DOSFS_SFLASH_STATE_NOT_FORMATTED         2


#define DOSFS_SFLASH_COMMAND_WREN_VOLATILE (QSPI_COMMAND_INSTRUCTION_SINGLE | 0x50)
#define DOSFS_SFLASH_COMMAND_WREN          (QSPI_COMMAND_INSTRUCTION_SINGLE | 0x06)
#define DOSFS_SFLASH_COMMAND_WRDI          (QSPI_COMMAND_INSTRUCTION_SINGLE | 0x04)
#define DOSFS_SFLASH_COMMAND_RDID          (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x9f)
#define DOSFS_SFLASH_COMMAND_RDSR          (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x05)
#define DOSFS_SFLASH_COMMAND_WRSR          (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x01)
#define DOSFS_SFLASH_COMMAND_RDCR          (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x35)
#define DOSFS_SFLASH_COMMAND_RDFS          (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x70)
#define DOSFS_SFLASH_COMMAND_CLFS          (QSPI_COMMAND_INSTRUCTION_SINGLE | 0x50)
#define DOSFS_SFLASH_COMMAND_RDSCUR        (QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x2b)
#define DOSFS_SFLASH_COMMAND_RDSFDP        ((8 << QSPI_COMMAND_WAIT_STATES_SHIFT) | QSPI_COMMAND_DATA_SINGLE | QSPI_COMMAND_ADDRESS_SINGLE | QSPI_COMMAND_ADDRESS_24BIT | QSPI_COMMAND_INSTRUCTION_SINGLE | 0x5a)
#define DOSFS_SFLASH_COMMAND_RDP           (QSPI_COMMAND_INSTRUCTION_SINGLE | 0xab)


#define DOSFS_SFLASH_FEATURE_SR_VOLATILE_SELECT     0x00000001
#define DOSFS_SFLASH_FEATURE_HOLD_RESET_DISABLE     0x00000002
#define DOSFS_SFLASH_FEATURE_QE_AUTO                0x00000004
#define DOSFS_SFLASH_FEATURE_QE_SR_6                0x00000008
#define DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE_RESET 0x00000010
#define DOSFS_SFLASH_FEATURE_QE_CR_1_VOLATILE       0x00000020
#define DOSFS_SFLASH_FEATURE_QE_CR_1                0x00000040
#define DOSFS_SFLASH_FEATURE_RDSR                   0x00000080 /* SPANSION */
#define DOSFS_SFLASH_FEATURE_RDFS                   0x00000100 /* MICRON   */
#define DOSFS_SFLASH_FEATURE_RDSCUR                 0x00000200 /* MACRONIX */

typedef struct _dosfs_sflash_t dosfs_sflash_t;

struct _dosfs_sflash_t {
    uint8_t                 state;
    uint8_t                 address;
    uint16_t                xlate_count;
    uint32_t                data_size;

    uint8_t                 sector_table[DOSFS_SFLASH_DATA_SIZE / DOSFS_SFLASH_ERASE_SIZE];
#if (DOSFS_SFLASH_DATA_SIZE == 0x02000000)
    uint32_t                sector_mask[(DOSFS_SFLASH_DATA_SIZE / DOSFS_SFLASH_ERASE_SIZE) / 32];
#endif /* DOSFS_SFLASH_DATA_SIZE */
    uint16_t                block_table[DOSFS_SFLASH_XLATE_OFFSET];
    uint16_t                xlate_logical; 
    uint16_t                xlate_table[DOSFS_SFLASH_XLATE_COUNT];
    uint16_t                xlate2_logical; 
    uint16_t                xlate2_table[DOSFS_SFLASH_XLATE_COUNT];

    uint16_t                alloc_sector;
    uint8_t                 alloc_index;
    uint8_t                 alloc_count;
    uint32_t                alloc_free;
    uint32_t                alloc_mask[DOSFS_SFLASH_BLOCK_INFO_ENTRIES / 32];

    uint16_t                victim_sector;
    uint8_t                 victim_delta[DOSFS_SFLASH_DATA_SIZE / DOSFS_SFLASH_ERASE_SIZE];
    uint8_t                 victim_score[DOSFS_SFLASH_DATA_SIZE / DOSFS_SFLASH_ERASE_SIZE];

    uint32_t                reclaim_offset;
    uint32_t                reclaim_erase_count;

    uint32_t                erase_count_max;

    uint32_t                *cache[2];

#if (DOSFS_CONFIG_SFLASH_SIMULATE == 1)
    uint8_t                 *image;
#endif /* (DOSFS_CONFIG_SFLASH_SIMULATE == 1) */

#if (DOSFS_CONFIG_STATISTICS == 1)
    struct {
        uint32_t                sflash_command_address;
        uint32_t                sflash_command_read;
        uint32_t                sflash_command_write;
        uint32_t                sflash_command_erase;
        uint32_t                sflash_nor_read;
        uint32_t                sflash_nor_write;
        uint32_t                sflash_nor_erase;
        uint32_t                sflash_nor_pfail;
        uint32_t                sflash_nor_efail;
        uint32_t                sflash_ftl_read;
        uint32_t                sflash_ftl_write;
        uint32_t                sflash_ftl_delete;
	uint32_t                sflash_ftl_xlate_miss;
	uint32_t                sflash_ftl_xlate_hit;
	uint32_t                sflash_ftl_xlate2_miss;
	uint32_t                sflash_ftl_xlate2_hit;
    }                       statistics;
#endif /* (DOSFS_CONFIG_STATISTICS == 1) */

    uint32_t                features;
    uint8_t                 mid;
    uint16_t                did;
    uint32_t                command_erase;
    uint32_t                command_program;
    uint32_t                command_read;
    stm32l4_qspi_t          qspi;
};

/* RESERVED -> XLATE_SECONDARY -> XLATE -> DELETED
 * RESERVED -> DATA_WRITTEN -> DATA_COMMITTED -> DATA_DELETED -> DELETED
 * RESERVED -> RECLAIM -> ERASE -> VICTIM -> DELETED
 */
#define DOSFS_SFLASH_INFO_TYPE_MASK             0x00f00000
#define DOSFS_SFLASH_INFO_TYPE_DELETED          0x00000000     /* 0000 sector is deleted                      */
#define DOSFS_SFLASH_INFO_TYPE_UNUSED_0001      0x00100000     /* 0001                                        */
#define DOSFS_SFLASH_INFO_TYPE_UNUSED_0010      0x00200000     /* 0010                                        */
#define DOSFS_SFLASH_INFO_TYPE_UNUSED_0011      0x00300000     /* 0011                                        */
#define DOSFS_SFLASH_INFO_TYPE_UNUSED_0101      0x00500000     /* 0101                                        */
#define DOSFS_SFLASH_INFO_TYPE_UNUSED_0111      0x00700000     /* 0111                                        */
#define DOSFS_SFLASH_INFO_TYPE_UNUSED_1001      0x00900000     /* 1001                                        */
#define DOSFS_SFLASH_INFO_TYPE_DATA_DELETED     0x00400000     /* 0100 sector is user data (to be deleted)    */
#define DOSFS_SFLASH_INFO_TYPE_DATA_COMMITTED   0x00600000     /* 0110 sector is user data (valid)            */
#define DOSFS_SFLASH_INFO_TYPE_VICTIM           0x00800000     /* 1000 sector is victim header                */
#define DOSFS_SFLASH_INFO_TYPE_ERASE            0x00a00000     /* 1010 sector is erase header (one bit off)   */
#define DOSFS_SFLASH_INFO_TYPE_RECLAIM          0x00b00000     /* 1011 sector is reclaim header               */
#define DOSFS_SFLASH_INFO_TYPE_XLATE            0x00c00000     /* 1100 sector is logical to physical xlate    */
#define DOSFS_SFLASH_INFO_TYPE_XLATE_SECONDARY  0x00d00000     /* 1101 sector is patch to xlate               */
#define DOSFS_SFLASH_INFO_TYPE_DATA_WRITTEN     0x00e00000     /* 1110 sector is user data (not comitted)     */
#define DOSFS_SFLASH_INFO_TYPE_RESERVED         0x00f00000     /* 1111 sector is reserved                     */
#define DOSFS_SFLASH_INFO_NOT_WRITTEN_TO        0x00080000     /* sector is erased (i.e. not written to)     */
#define DOSFS_SFLASH_INFO_DATA_MASK             0x0007ffff 
#define DOSFS_SFLASH_INFO_EXTENDED_MASK         0xff000000
#define DOSFS_SFLASH_INFO_EXTENDED_SHIFT        24
#define DOSFS_SFLASH_INFO_ERASED                0xffffffff

#define DOSFS_SFLASH_INFO_EXTENDED_SIZE         (4 * 4)        /* bytes needed for one sideband data     */
#define DOSFS_SFLASH_INFO_EXTENDED_TOTAL        (16 * 4)       /* bytes needed for all sideband data     */

#if (DOSFS_CONFIG_STATISTICS == 1)

#define DOSFS_SFLASH_STATISTICS_COUNT(_name)         { sflash->statistics._name += 1; }
#define DOSFS_SFLASH_STATISTICS_COUNT_N(_name,_n)    { sflash->statistics._name += (_n); }

#else /* (DOSFS_CONFIG_STATISTICS == 1) */

#define DOSFS_SFLASH_STATISTICS_COUNT(_name)         /**/
#define DOSFS_SFLASH_STATISTICS_COUNT_N(_name,_n)    /**/

#endif /* (DOSFS_CONFIG_STATISTICS == 1) */

#ifdef __cplusplus
}
#endif

#endif /*_DOSFS_SFLASH_H */
