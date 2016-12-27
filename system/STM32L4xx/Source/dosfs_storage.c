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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "armv7m.h"
#include "dosfs_core.h"
#include "usbd_msc.h"

static int8_t dosfs_storage_init(uint8_t lun)
{
    return 0;
}

static int8_t dosfs_storage_deinit(uint8_t lun)
{
    dosfs_volume.lock &= ~(DOSFS_VOLUME_LOCK_WRITE_LOCK | DOSFS_VOLUME_LOCK_SCSI_LOCK);

    return 0;
}

static int8_t dosfs_storage_get_capacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    int status;
    uint8_t media, write_protected;
    uint32_t block_count, au_size, serial;

    if (dosfs_volume.state == DOSFS_VOLUME_STATE_NONE)
    {
	return -1;
    }

    if ((dosfs_volume.lock & (DOSFS_VOLUME_LOCK_FILE_LOCK | DOSFS_VOLUME_LOCK_FIND_LOCK | DOSFS_VOLUME_LOCK_API_LOCK)) ||
	((dosfs_volume.lock & DOSFS_VOLUME_LOCK_DEVICE_LEASE) && ((armv7m_systick_millis() - dosfs_volume.lease) < 250)))
    {
	return -1;
    }
    
    dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_DEVICE_LEASE;

    status = (*dosfs_volume.interface->info)(dosfs_volume.context, &media, &write_protected, &block_count, &au_size, &serial);

    if (status != F_NO_ERROR)
    {
	return -1;
    }

    *block_num  = block_count;
    *block_size = 512;

    return 0;
}

static int8_t dosfs_storage_is_ready(uint8_t lun)
{
    //fprintf(stderr, "ISREADY()\r\n");

    if (dosfs_volume.state == DOSFS_VOLUME_STATE_NONE)
    {
	return -1;
    }

    if ((dosfs_volume.lock & (DOSFS_VOLUME_LOCK_FILE_LOCK | DOSFS_VOLUME_LOCK_FIND_LOCK | DOSFS_VOLUME_LOCK_API_LOCK)) ||
	((dosfs_volume.lock & DOSFS_VOLUME_LOCK_DEVICE_LEASE) && ((armv7m_systick_millis() - dosfs_volume.lease) < 250)))
    {
	return -1;
    }
    
    dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_DEVICE_LEASE;

    return 0;
}

static int8_t dosfs_storage_is_write_protected(uint8_t lun)
{
    return 0;
}

static int8_t dosfs_storage_is_changed(uint8_t lun)
{
    if (dosfs_volume.lock & DOSFS_VOLUME_LOCK_DEVICE_MODIFIED)
    {
	dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_DEVICE_MODIFIED;

	if (dosfs_volume.lock & DOSFS_VOLUME_LOCK_HOST_ACCESSED)
	{
	    return -1;
	}
    }
    
    return 0;
}

static int8_t dosfs_storage_start_stop_unit(uint8_t lun, uint8_t start, uint8_t loej)
{
    if (loej)
    {
	dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_WRITE_LOCK;
    }

    return 0;
}

static int8_t dosfs_storage_prevent_allow_medium_removal(uint8_t lun, uint8_t param)
{
    return 0;
}

static int8_t dosfs_storage_acquire(uint8_t lun)
{
    if ((dosfs_volume.lock & (DOSFS_VOLUME_LOCK_FILE_LOCK | DOSFS_VOLUME_LOCK_FIND_LOCK | DOSFS_VOLUME_LOCK_API_LOCK)) ||
	((dosfs_volume.lock & DOSFS_VOLUME_LOCK_DEVICE_LEASE) && ((armv7m_systick_millis() - dosfs_volume.lease) < 250)))
    {
	return -1;
    }
    
    dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_DEVICE_LEASE;
    dosfs_volume.lock |= DOSFS_VOLUME_LOCK_SCSI_LOCK;
    
    return 0;
}

static int8_t dosfs_storage_read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len, uint8_t last)
{
    int status;

    status = (*dosfs_volume.interface->read)(dosfs_volume.context, blk_addr, blk_len, buf);

    dosfs_volume.lease = armv7m_systick_millis();
    dosfs_volume.lock |= DOSFS_VOLUME_LOCK_HOST_LEASE;
    
    if (status != F_NO_ERROR)
    {
	dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_SCSI_LOCK;

	return -1;
    }

    if (last)
    {
	dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_SCSI_LOCK;
    }

    dosfs_volume.lock |= DOSFS_VOLUME_LOCK_HOST_ACCESSED;

    return 0;
}

static int8_t dosfs_storage_write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len, uint8_t last)
{
    int status;

    status = (*dosfs_volume.interface->write)(dosfs_volume.context, blk_addr, blk_len, buf, NULL);

    dosfs_volume.lease = armv7m_systick_millis();
    dosfs_volume.lock |= DOSFS_VOLUME_LOCK_HOST_LEASE;

    if (status != F_NO_ERROR)
    {
	dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_SCSI_LOCK;

	return -1;
    }

    if (last)
    {
	dosfs_volume.lock &= ~DOSFS_VOLUME_LOCK_SCSI_LOCK;
    }

    dosfs_volume.lock |= DOSFS_VOLUME_LOCK_HOST_ACCESSED;

    dosfs_volume.lock |= DOSFS_VOLUME_LOCK_WRITE_LOCK;

    return 0;
}


static int8_t dosfs_storage_get_maxlun(void)
{
    return 0;
}

static const int8_t dosfs_storage_inquiry_data[] = {
  
  /* LUN 0 */
  0x00,		
  0x80,		
  0x02,		
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,	
  0x00,
  'D', 'O', 'S', 'F', 'S', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'D', 'r', 'a', 'g', 'o', 'n', 'f', 'l', /* Product      : 16 Bytes */
  'y', ' ', 'S', 'F', 'L', 'A', 'S', 'H',
  '0', '.', '0' ,'1',                     /* Version      : 4 Bytes */
}; 

const USBD_StorageTypeDef dosfs_storage_interface =
{
    dosfs_storage_init,
    dosfs_storage_deinit,
    dosfs_storage_get_capacity,
    dosfs_storage_is_ready,
    dosfs_storage_is_write_protected,
    dosfs_storage_is_changed,
    dosfs_storage_start_stop_unit,
    dosfs_storage_prevent_allow_medium_removal,
    dosfs_storage_acquire,
    dosfs_storage_read,
    dosfs_storage_write,
    dosfs_storage_get_maxlun,
    dosfs_storage_inquiry_data,
};
