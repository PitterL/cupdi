/*
Copyright(c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

"""
pyupdi is a Python utility for programming AVR devices with UPDI interface
using a standard TTL serial port.

Connect RX and TX together with a suitable resistor and connect this node
to the UPDI pin of the AVR device.

Be sure to connect a common ground, and use a TTL serial adapter running at
the same voltage as the AVR device.

Vcc                     Vcc
+ -++-+
|                       |
+-------------------- - +|                       | +-------------------- +
| Serial port + -++-+AVR device        |
|                     |      +---------- + |                    |
|                  TX + ------ + 4k7 + -------- - +UPDI               |
|                     |      +---------- + |    |                    |
|                     |                      |    |                    |
|                  RX + ---------------------- + |                    |
|                     |                           |                    |
|                     +-- + +-- + |
+-------------------- - +|                     |  +-------------------- +
+-++-+
GND                   GND


This is C version of UPDI interface achievement, referred to the Python version here:
    https://github.com/mraardvark/pyupdi.git

    My purpose is you could use UPDI at any enviroment like Computer / MCU Embedded system, just transplant the code the there.
    I used other vendor library, thanks:
        argparse: https://github.com/cofyc/argparse.git
        ihex:       https://github.com/arkku/ihex.git
        sercom:     https://github.com/dpogue/Terminal-Emulator.git
    If you have any question, please query Pater to mailbox atpboy444@hotmail.com
    If you like it please help to mark STAR at https://github.com/PitterL/cupdi.git
 */

#include <stdio.h>
#include <platform/platform.h>
#include <device/device.h>
#include <updi/nvm.h>
#include <hex_file/ihex.h>
#include <crc/crc.h>
#include "cupdi.h"
#include "hex_file/ihex.h"

#ifdef CUPDI

/* Fuse content */
// BOD level 2(2.6v Sampled 1Khz at Sleep, Enabled at Active), OSC 16Mhz, NVM protect after POR, 
// EEPROM erased
/*BYTE order, ignore set as NULL or the value*/
u8 fuse_data[] = {0x00, 0x46, 0x7D, 0xFF, 0x00, 0xF6, 0xFF, 0x00, 0x00, 0xFF, 0xC5 };	

/* CUPDI Software version */
#define SOFTWARE_VERSION "1.10"

int cupdi_operate()
{
    char *dev_name = NULL;
    char *comport = NULL;          // no significant meaning
    int baudrate = 115200;
    const device_info_t * dev;
    void *nvm_ptr;
    int result;
	
	dev_name = "tiny1617";

    dev = get_chip_info(dev_name);
    if (!dev) {
        DBG_INFO(UPDI_DEBUG, "Device %s not support", dev_name);
        return -2;
    }
      
    nvm_ptr = updi_nvm_init(comport, baudrate, (void *)dev);
    if (!nvm_ptr) {
        DBG_INFO(UPDI_DEBUG, "Nvm initialize failed");
        result = -3;
        goto out;
    }

    //check device id
    result = nvm_get_device_info(nvm_ptr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_device_info failed");
        result = -4;
        goto out;
    }

    result = nvm_enter_progmode(nvm_ptr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "Device is locked(%d). Performing unlock with chip erase.", result);
        result = nvm_unlock_device(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM unlock device failed %d", result);
            result = -5;
            goto out;
        }
    }

    result = updi_write_fuse(nvm_ptr);
	if (result) {
		DBG_INFO(UPDI_DEBUG, "updi_program failed %d", result);
		result = -6;
		goto out;
	}

    result = updi_program(nvm_ptr);//file);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "updi_program failed %d", result);
        result = -9;
        goto out;
    }
  
 out:
    nvm_leave_progmode(nvm_ptr);
    updi_nvm_deinit(nvm_ptr);

    return result;
}

/*
    Erase the chip
    @nvm_ptr: updi_nvm_init() device handle
    @returns 0 - success, other value failed code
*/
int updi_erase(void *nvm_ptr)
{
    int result;

    result = nvm_chip_erase(nvm_ptr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "NVM chip erase failed %d", result);
        return -2;
    }

    return 0;
}

/*
UPDI Fuse Write
    @nvm_ptr: updi_nvm_init() device handle
    @returns 0 - success, other value failed code
*/
int updi_write_fuse(void *nvm_ptr)
{
	int result;
	int index;
	nvm_info_t info;
	u8 data[16];
	
	result = nvm_get_block_info(nvm_ptr, NVM_FUSES, &info);
	if (result) {
		DBG_INFO(NVM_DEBUG, "nvm_get_block_info failed");
		return -1;
	}
	
	result = nvm_read_fuse(nvm_ptr, 0, data, info.nvm_size);
	if (result) {
		DBG_INFO(NVM_DEBUG, "nvm_read_fuse failed");
		return -2;
	}
	
	for (index = 0; index < info.nvm_size; index++) {
		if (data[index] != fuse_data[index]) {
			result = nvm_write_fuse(nvm_ptr, index, (const u8 *)&fuse_data[index], 1);
			if (result) {
				DBG_INFO(NVM_DEBUG, "nvm_write_fuse failed");
				return -3;
			}
		}
	}
	
	return 0;
}

int set_default_segment_id(hex_data_t *dhex, ihex_segment_t segmentid)
{
	segment_buffer_t *seg;
	int result = 0;

	for (int i = 0; i < MAX_SEGMENT_COUNT_IN_RECORDS; i++) {
		seg = &dhex->segment[i];
		if (!seg->sid && seg->addr_to && seg->data) {
			seg->sid = segmentid;
			result++;
		}
	}

	return result;
}

/*
    UPDI Program flash
    This flowchart is: load firmware file->erase chip->program firmware
    @nvm_ptr: updi_nvm_init() device handle
    @file: hex/ihex file path
    @returns 0 - success, other value failed code
*/
int updi_program(void *nvm_ptr)
{
    hex_data_t *dhex = &hexdata;//NULL;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    nvm_info_t iflash;
    int i, result = 0;

    result = nvm_get_block_info(nvm_ptr, NVM_FLASH, &iflash);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }
        
    sid = ADDR_TO_SEGMENTID(iflash.nvm_start);
    set_default_segment_id(dhex, sid);

    result = nvm_chip_erase(nvm_ptr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_chip_erase failed %d", result);
        result = -4;
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(dhex->segment); i++) {
        seg = &dhex->segment[i];
        if (seg->data) {
            result = nvm_write_flash/*nvm_write_auto*/(nvm_ptr, SEGMENTID_TO_ADDR(seg->sid) + seg->addr_from, (u8 *)seg->data, seg->len);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "nvm_write_auto %d failed %d", i, result);
                result = -5;
                goto out;
            }
        }
    }

    DBG_INFO(UPDI_DEBUG, "Program finished");

out:
    return result;
}

/*
    UPDI Reset chip
    @nvm_ptr: updi_nvm_init() device handle
    @returns 0 - success, other value failed code
*/
/*int updi_reset(void *nvm_ptr)
{
    return nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET);
}
*/
#endif
