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
#include <time.h>
#include <os/platform.h>
#include <argparse/argparse.h>
#include <device/device.h>
#include <updi/nvm.h>
#include <ihex/ihex.h>
#include <string/split.h>
#include <file/fop.h>
#include <crc/crc.h>
#include <infoblock/ib.h>
#include "cupdi.h"

/* CUPDI Software version */
#define SOFTWARE_VERSION "1.10"

/* The firmware Version control file relatve directory to Hex file */
#define VAR_FILE_RELATIVE_POS "qtouch\\touch.h"
#define VCS_HEX_FILE_EXTENSION_NAME "ihex"
#define MAP_FILE_EXTENSION_NAME "map"
#define SAVE_FILE_EXTENSION_NAME "save"
#define DUMP_FILE_EXTENSION_NAME "dump"

static const char *const usage[] = {
    "Simple command line interface for UPDI programming:",
    "cupdi [options] [[--] args]",
    "Erase chip: cupdi -c COM2 -d tiny817 -e ",
    "Flash hex file: cupdi -c COM2 -d tiny817 --program -f c:/817.hex",
    NULL,
};

/* Flash operation flag*/
enum { FLAG_UNLOCK, FLAG_ERASE, FLAG_PROG, FLAG_UPDATE, FLAG_CHECK, FLAG_COMPARE, FLAG_VERIFY, FLAG_SAVE, FLAG_DUMP, FLAG_INFO};
enum { PACK_BUILD, PACK_SHOW};

int main(int argc, const char *argv[])
{
    char *dev_name = NULL;
    char *comport = NULL;
    int baudrate = 115200;
    char *file = NULL;
    char *fuses = NULL;
    char *read = NULL;
    char *write = NULL;
    char *dbgview = NULL;
    int flag = 0;
    int unlock = false;
    int verbose = 1;
    int reset = false;
    int disable = false;
    int test = false;
    int version = false;
    int pack = 0;
    //char *pack_version = NULL;

    const device_info_t * dev;
    void *nvm_ptr;
    int result;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Basic options"),
        OPT_STRING('d', "device", &dev_name, "Target device"),
        OPT_STRING('c', "comport", &comport, "Com port to use (Windows: COMx | *nix: /dev/ttyX)"),
        OPT_INTEGER('b', "baudrate", &baudrate, "Baud rate, default=115200"),
        OPT_STRING('f', "file", &file, "Intel HEX file to flash"),
        OPT_BIT('u', "unlock", &flag, "Perform a chip unlock (implied with --unlock)", NULL, (1 << FLAG_UNLOCK), 0),
        OPT_BIT('e', "erase", &flag, "Perform a chip erase (implied with --flash)", NULL, (1 << FLAG_ERASE), 0),
        OPT_BIT('-', "program", &flag, "Program Intel HEX file to flash", NULL, (1 << FLAG_PROG), 0),
        OPT_BIT('-', "update", &flag, "Program infoblock update to eeprom(need map file)", NULL, (1 << FLAG_UPDATE), 0),
        OPT_BIT('-', "check", &flag, "Check flash content with infoblock CRC", NULL, (1 << FLAG_CHECK), 0),
        OPT_BIT('-', "compare", &flag, "Compare vcs HEX file with infoblock and fuses content", NULL, (1 << FLAG_COMPARE), 0),
        OPT_BIT('-', "verify", &flag, "check and compare", NULL, (1 << FLAG_VERIFY), 0),
        OPT_BIT('i', "info", &flag, "Get Infoblock infomation of firmware", NULL, (1 << FLAG_INFO), 0),
        OPT_BIT('-', "save", &flag, "Save flash to a VCS HEX file", NULL, (1 << FLAG_SAVE), 0),
        OPT_BIT('-', "dump", &flag, "Dump flash to a Intel HEX file", NULL, (1 << FLAG_DUMP), 0),
        OPT_STRING('-', "fuses", &fuses, "Fuse to set [addr0]:[dat0];[dat1];|[addr1]..."),
        OPT_STRING('r', "read", &read, "Direct read from memory [addr1]:[n1]|[addr2]:[n2]..."),
        OPT_STRING('w', "write", &write, "Direct write to memory [addr0]:[dat0];[dat1];|[addr1]..."),
        OPT_STRING('-', "dbgview", &dbgview, "get ref/delta/cc value operation ds=[ptc_qtlib_node_stat1]|dr=[qtlib_key_data_set1]|loop=[n]|keys=[n] (loop(Hex) set to 0 loop forvever, default 1, keys default 1)"),
        OPT_INTEGER('v', "verbose", &verbose, "Set verbose mode (SILENCE|UPDI|NVM|APP|LINK|PHY|SER): [0~6], default 0, suggest 2 for status information"),
        OPT_BOOLEAN('-', "reset", &reset, "UPDI reset device"),
        OPT_BOOLEAN('-', "disable", &disable, "UPDI disable"),
        OPT_BOOLEAN('t', "test", &test, "Test UPDI device"),
        OPT_BOOLEAN('-', "version", &version, "Show version"),
        OPT_BIT('-', "pack-build", &pack, "Pack info block to Intel HEX file, (macro FIRMWARE_VERSION at 'touch.h')save with extension'.ihex'", NULL, (1 << PACK_BUILD), 0),
        OPT_BIT('-', "pack-info", &pack, "Shwo packed file(ihex) info", NULL, (1 << PACK_SHOW), 0),
        //OPT_STRING('-', "pack2", &build_version, "Pack info block to Intel HEX file by given 4 bytes CVS code(Big Endian), save with extension'.ihex'"),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nA brief description of what the program does and how it works.", "\nAdditional description of the program after the description of the arguments.");
    
    if (argc <= 1) {
        argparse_usage(&argparse);
        return 0;
    }

    argc = argparse_parse(&argparse, argc, argv);
    if (argc != 0) {
        DBG_INFO(DEFAULT_DEBUG, "argc: %d\n", argc);
        for (int i = 0; i < argc; i++) {
            DBG_INFO(DEFAULT_DEBUG, "argv[%d]: %s\n", i, argv[i]);
        }
    }
    //<Part 0> The command below requires nothing
    //set parameter
    set_verbose_level(verbose);

    if (version) {
        DBG_INFO(UPDI_DEBUG, "CUPDI version: %s", SOFTWARE_VERSION);
        return 0;
    }

    DBG_INFO(UPDI_DEBUG, "Device name %d %p, %s", sizeof(bool), dev_name, dev_name);

    //<Part 1> The command below requires device name
    if (!dev_name) {
        DBG_INFO(UPDI_DEBUG, "No DEV Name appointed");
        return ERROR_PTR;
    }

    dev = get_chip_info(dev_name);
    if (!dev) {
        DBG_INFO(UPDI_DEBUG, "Device %s not support", dev_name);
        return -2;
    }

    //covert hex file to ihex file
    if (pack) {
        if (file) {
            if (TEST_BIT(pack, PACK_BUILD)) {
                result = dev_pack_to_vcs_hex_file(dev, file);
                if (result) {
                    DBG_INFO(UPDI_DEBUG, "Device pack hex file '%s' failed %d", file, result);
                    return -3;
                }
            }

            if (TEST_BIT(pack, PACK_SHOW)) {
                result = dev_vcs_hex_file_show_info(dev, file);
                if (result) {
                    DBG_INFO(UPDI_DEBUG, "Device show ihex file '%s' failed %d", file, result);
                    return -4;
                }
            }
        }
        else {
            DBG_INFO(UPDI_DEBUG, "Device pack file not appointed");
            return -5;
        }

        return 0;
    }

    //<Part 2> The command below requires common port
    if (!comport) {
        DBG_INFO(UPDI_DEBUG, "No COM PORT appointed");
        return ERROR_PTR;
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

    //unlock
    if (write || fuses || flag) {
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

        result = nvm_get_device_info(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "nvm_get_device_info in program failed");
            result = -6;
            goto out;
        }
    }

    //erase
    if (TEST_BIT(flag, FLAG_ERASE)) {
        result = updi_erase(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM chip erase failed %d", result);
            result = -7;
            goto out;
        }
    }

    //program and dump
    if (file) {
        if (TEST_BIT(flag, FLAG_UPDATE)) {
            result = updi_update(nvm_ptr, file);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "updi_update failed %d", result);
                result = -10;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_PROG)) {
            result = updi_program(nvm_ptr, file);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "updi_program failed %d", result);
                result = -9;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_COMPARE) || TEST_BIT(flag, FLAG_VERIFY)) {
            result = updi_compare(nvm_ptr, file);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "updi_verifiy_infoblock failed %d", result);
                result = -11;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_SAVE)) {
            result = updi_save(nvm_ptr, file);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "NVM save failed %d", result);
                result = -11;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_DUMP)) {
            result = updi_dump(nvm_ptr, file);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "NVM dump failed %d", result);
                result = -11;
                goto out;
            }
        }
    }

    //show info block
    if (TEST_BIT(flag, FLAG_INFO)) {
        result = updi_show_infoblock(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "updi_show_infoblock failed %d", result);
            result = -11;
            goto out;
        }
    }

    //check firwware content
    if (TEST_BIT(flag, FLAG_CHECK) || TEST_BIT(flag, FLAG_VERIFY)) {
        result = updi_verifiy_infoblock(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "updi_verifiy_infoblock failed %d", result);
            result = -11;
            goto out;
        }
    }

    //read
    if (read) {
        result = updi_read(nvm_ptr, read);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "Read failed %d", result);
            result = -12;
            goto out;
        }
    }

    //write
    if (write) {
        result = updi_write(nvm_ptr, write);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "Write failed %d", result);
            result = -13;
            goto out;
        }
    }

    //fuse
    if (fuses) {
        result = updi_write_fuse(nvm_ptr, fuses);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM set fuse failed %d", result);
            result = -14;
            goto out;
        }
    }

    //reset
    if (reset) {
        result = nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM reset failed %d", result);
            result = -15;
            goto out;
        }
    }

    //debug show
    if (dbgview) {
        result = updi_debugview(nvm_ptr, dbgview);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "Debugview failed %d", result);
            result = -16;
            goto out;
        }
    }

    if (disable) {
        result = nvm_disable(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "nvm_disable failed %d", result);
            result = -17;
            goto out;
        }
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
    Print segment info in hex data
    @dev: device info structure, get by get_chip_info()
    @type: NVM type
    @dhex: hex data structure
    @return 0 if found the segment data
*/
int dev_hex_show(const device_info_t * dev, int type, hex_data_t *dhex)
{
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    int i, result = -3;

    result = dev_get_nvm_info(dev, type, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info type %d failed %d", type, result);
        return -2;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    for (i = 0; i < ARRAY_SIZE(dhex->segment); i++) {
        seg = &dhex->segment[i];
        if (seg->sid == sid) {
            DBG(UPDI_DEBUG, "nvm-%d: ", seg->data, seg->len, "%02x ", type);
            result = 0;
        }
    }

    return result;
}

/*
    Get Info block from eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @info: output buffer for infoblock
    @return 0 successful, other value failed
*/
int get_infoblock_from_eeprom(void *nvm_ptr, information_container_t *info)
{
    nvm_info_t iblock;
    information_header_t header;
    char * buf = NULL;
    int len;
    int result;

    result = nvm_get_block_info(nvm_ptr, NVM_EEPROM, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }

    result = nvm_read_eeprom(nvm_ptr, INFO_BLOCK_ADDRESS_IN_EEPROM, (u8 *)&header, sizeof(header));
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_eeprom failed %d", result);
        return -3;
    }

    len = header.data.size;
    if (len <= 0 || len > iblock.nvm_size || len > ib_max_block_size()) {
        DBG_INFO(UPDI_DEBUG, "Header '%04x', size = %d is incorrect", header.data.version.value, len);
        return -4;
    }

    buf = malloc(len);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "malloc infoblock size %d failed", len);
        return -5;
    }

    result = nvm_read_eeprom(nvm_ptr, INFO_BLOCK_ADDRESS_IN_EEPROM, (u8 *)buf, len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_eeprom %d bytes failed %d", len, result);
        return -6;
    }

    result = infoblock_set_data_ptr(info, buf, len, BIT_MASK(MEM_SHARE_RELEASE));
    if (result) {
        DBG_INFO(UPDI_DEBUG, "Info Block set data ptr failed %d", result);
        DBG(UPDI_DEBUG, "Info Block:", (unsigned char *)buf, len, "%02X ");
        result = -5;
    }

    return result;
}

/*
    Get infoblock from hex data structure
    @block: nvm info
    @dhex: hex data structure
    @output: infoblock container output buffer
    return 0 if sucessful, else failed
*/
int _get_infoblock_from_hex_info(nvm_info_t *block, hex_data_t *dhex, information_container_t *output)
{
    information_header_t *head;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    int start, size, off;
    int result;

    sid = ADDR_TO_SEGMENTID(block->nvm_start);
    seg = get_segment_by_id(dhex, sid);
    if (seg) {
        start = seg->addr_from;
        if (start < INFO_BLOCK_ADDRESS_IN_EEPROM)
            start += INFO_BLOCK_ADDRESS_IN_EEPROM;

        off = start - INFO_BLOCK_ADDRESS_IN_EEPROM;
        size = seg->addr_to - start;
        if (size >= sizeof(*head)) {
            head = (information_header_t *)(seg->data + off);
            result = infoblock_set_data_ptr(output, (char *)head, size, BIT_MASK(MEM_SHARE));
            if (result) {
                DBG_INFO(UPDI_DEBUG, "Info Block matching failed, failed %d", result);
                DBG(UPDI_DEBUG, "Info Block:", (unsigned char *)head, size, "%02X ");
                result = -3;
            }
        }
        else {
            DBG_INFO(UPDI_DEBUG, "Info Block size %d is too small in seg", size);
            result = -4;
        }
    }
    else {
        DBG_INFO(UPDI_DEBUG, "Info Block segment not found");
        result = -5;
    }

    return result;
}

/*
    Get infoblock from hex data structure
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @dhex: hex data structure
    @output: infoblock output buffer
    return 0 if sucessful, else failed
*/
int get_infoblock_from_hex_info_nvm(void *nvm_ptr, hex_data_t *dhex, information_container_t *output)
{
    nvm_info_t iblock;
    int result;

    result = nvm_get_block_info(nvm_ptr, NVM_EEPROM, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }

    return _get_infoblock_from_hex_info(&iblock, dhex, output);
}

/*
    Get infoblock from hex data structure
    @dev: device info structure, get by get_chip_info()
    @dhex: hex data structure
    @output: infoblock container output buffer
    return 0 if sucessful, else failed
*/
int get_infoblock_from_hex_info_dev(const device_info_t * dev, hex_data_t *dhex, information_container_t *output)
{
    nvm_info_t iblock;
    int result;

    result = dev_get_nvm_info(dev, NVM_EEPROM, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        return -2;
    }
    
    return _get_infoblock_from_hex_info(&iblock, dhex, output);
}

/*
    Save nvm content to hex data structure
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: NVM type
    @dhex: hex data structure
    @off: address off
    @buf: data buffer pointer
    @size: data buffer size
    return 0 if success, else failed
*/
int save_content_to_segment(void *nvm_ptr, int type, hex_data_t *dhex, ihex_address_t off, char *buf, int size)
{
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    int result;

    result = nvm_get_block_info(nvm_ptr, type, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }

    if (size > iblock.nvm_size) {
        DBG_INFO(UPDI_DEBUG, "size %d failed %d", size);
        return -3;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    seg = set_segment_data_by_id_addr(dhex, sid, off, size, buf, SEG_ALLOC_MEMORY);
    if (!seg) {
        DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed %d", result);
        return -4;
    }

    return 0;
}

/*
    Get Firmware version from file
    @file: file name to search
    @ver: version output buffer
    return 0 if sucessful, else failed
*/
int load_version_value_from_file(const char *file, int *ver)
{
    unsigned int version;
    int result;

    result = search_defined_value_int_from_file(file, "FIRMWARE_VERSION", &version);
    if (result != 1) {
        DBG_INFO(UPDI_DEBUG, "search_defined_value_int_from_file fw version failed %d", result);
        return -2;
    }

    *ver = _swap_int32(version);

    return 0;
}

/*
Get Firmware signal/reference structure address from file
@file: file name to search
@ver: version output buffer
return 0 if sucessful, else failed
*/
int load_dsdr_value_from_file(const char *file, int *dsdr)
{
    unsigned int ds, dr;
    int result;

    result = search_map_value_int_from_file(file, "ptc_qtlib_node_stat1", &ds);
    if (result != 1) {
        DBG_INFO(UPDI_DEBUG, "search_map_value_int_from_file ds failed %d", result);
        return -2;
    }

    result = search_map_value_int_from_file(file, "qtlib_key_data_set1", &dr);
    if (result != 1) {
        DBG_INFO(UPDI_DEBUG, "search_map_value_int_from_file dr failed %d", result);
        return -3;
    }

    if (!ds || !dr) {
        DBG_INFO(UPDI_DEBUG, "get ds/dr reg address invalid", result);
        return -4;
    }

    *dsdr = L16_TO_LT32(ds, dr);
    
    return 0;
}

/*
    UPDI Show Info block at eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int updi_show_infoblock(void *nvm_ptr)
{
    information_container_t info_container;
    int result;

    memset(&info_container, 0, sizeof(info_container));
    result = get_infoblock_from_eeprom(nvm_ptr, &info_container);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_infoblock_from_eeprom failed", result);
        return -2;
    }

    ib_show(&info_container);
    ib_destory(&info_container);
    return 0;
}

/*
    Get flash content by indicated size, if negative mean whole flash
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @type: nvm type
    @req_size: size pointer for input, output buf size, NULL mean whole NVM block size
    @return buffer pointer, NULL indicated failed
*/
char *get_nvm_content(void *nvm_ptr, int type, int *req_size)
{
    nvm_info_t iblock;
    char *buf = NULL;
    int size;
    int result;

    result = nvm_get_block_info(nvm_ptr, type, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_flash_info failed %d", result);
        return NULL;
    }

    if (!req_size || *req_size <= 0)
        size = iblock.nvm_size;
    else
        size = *req_size;

    if (iblock.nvm_size < size) {
        DBG_INFO(UPDI_DEBUG, "size %d invalid", size);
        return NULL;
    }

    buf = malloc(size);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "alloc memory size = %d  failed", size);
        return NULL;
    }

    result = nvm_read_mem(nvm_ptr, iblock.nvm_start, buf, size);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_mem failed %d", result);
        free(buf);
        return NULL;
    }

    if (req_size)
        *req_size = size;

    return buf;
}

/*
    UPDI verify Infoblock information in eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 mean pass, other value failed
*/
int updi_verifiy_infoblock(void *nvm_ptr)
{
    information_container_t info_container;
    char *buf = NULL;
    int len, crc, info_crc;
    int result;

    memset(&info_container, 0, sizeof(info_container));
    result = get_infoblock_from_eeprom(nvm_ptr, &info_container);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_infoblock_from_eeprom failed", result);
        return -2;
    }

    len = ib_get(&info_container, IB_FW_SIZE);
    if (len <= 0) {
        DBG_INFO(UPDI_DEBUG, "get firmware size = %d failed", len);
        result = -3;
        goto out;
    }
    
    buf = get_nvm_content(nvm_ptr, NVM_FLASH, &len);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "get_flash_content failed");
        result = -4;
        goto out;
    }

    // DBG(UPDI_DEBUG, "Flash data ", buf, len, "%02x ");

    crc = calc_crc24(buf, len);
    info_crc = ib_get(&info_container, IB_CRC_FW);
    if (info_crc < 0 || info_crc != crc) {
        DBG_INFO(UPDI_DEBUG, "Info Block read fw crc24 mismatch %06x(%06x)", info_crc, crc);
        result = -5;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "Pass");
out:
    if (buf)
        free(buf);

    ib_destory(&info_container);
    return result;
}

/*
    UPDI Program flash
    This flowchart is: load firmware file->erase chip->program firmware
    @nvm_ptr: updi_nvm_init() device handle
    @file: hex/ihex file path
    @returns 0 - success, other value failed code
*/
int updi_program(void *nvm_ptr, const char *file)
{
    hex_data_t *dhex = NULL;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    nvm_info_t iflash;
    char * vcs_file = NULL;
    int i, result = 0;

    result = nvm_get_block_info(nvm_ptr, NVM_FLASH, &iflash);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }
        
    dhex = get_hex_info_from_file(file);
    if (!dhex) {
        DBG_INFO(UPDI_DEBUG, "get_hex_info_from_file failed %d");
        return -3;
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
            result = nvm_write_auto(nvm_ptr, SEGMENTID_TO_ADDR(seg->sid) + seg->addr_from, seg->data, seg->len);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "nvm_write_auto %d failed %d", i, result);
                result = -5;
                goto out;
            }
        }
    }

    DBG_INFO(UPDI_DEBUG, "Program finished");

out:
    if (dhex)
        release_dhex(dhex);
    return result;
}

/*
    Compare chip infoblock crc whether it's match with hex data
    @nvm_ptr: updi_nvm_init() device handle
    @dhex: hex data structure
    return 0 if CRC matched, else failed
*/
int compare_nvm_crc(void *nvm_ptr, hex_data_t *dhex)
{
    information_container_t file_info_container, eeprom_info_block;
    int ecrc, fcrc;
    int result;

    memset(&file_info_container, 0, sizeof(file_info_container));
    memset(&eeprom_info_block, 0, sizeof(eeprom_info_block));

    result = get_infoblock_from_hex_info_nvm(nvm_ptr, dhex, &file_info_container);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_infoblock_from_hex_info failed %d", result);
        result = -3;
        goto out;
    }

    result = get_infoblock_from_eeprom(nvm_ptr, &eeprom_info_block);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_infoblock_from_eeprom failed %d", result);
        result = -3;
        goto out;
    }

    ecrc = ib_get(&eeprom_info_block, IB_CRC_FW);
    fcrc = ib_get(&file_info_container, IB_CRC_FW);
    if (ecrc < 0 || fcrc < 0 || ecrc != fcrc) {
        DBG_INFO(UPDI_DEBUG, "Fw CRC 0x%x mismatch file CRC 0x%x", ecrc, fcrc);
        result = -5;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "CRC 0x%x matched ", ecrc);

out:
    ib_destory(&eeprom_info_block);
    ib_destory(&file_info_container);
    return result;
}

/*
    Compare chip fuses whether it's match with hex data
    @nvm_ptr: updi_nvm_init() device handle
    @dhex: hex data structure
    return 0 if fuses matched, else failed
*/
int compare_nvm_fuses(void *nvm_ptr, hex_data_t *dhex)
{
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    char * buf;
    int len;
    int i, result;

    result = nvm_get_block_info(nvm_ptr, NVM_FUSES, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_fuses_info failed %d", result);
        return -2;
    }

    len = 0;
    buf = get_nvm_content(nvm_ptr, NVM_FUSES, &len);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "get_fuses_content failed");
        return -3;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    for (i = 0; i < ARRAY_SIZE(dhex->segment); i++) {
        seg = &dhex->segment[i];
        if (seg->sid == sid) {
            if ((int)(seg->addr_from + seg->len) <= len) {
                if (memcmp(seg->data, buf + seg->addr_from, seg->len)) {
                    DBG_INFO(UPDI_DEBUG, "Fuses content mismatch:");
                    DBG(UPDI_DEBUG, "Fuses: ", buf, len, "%02x ");
                    DBG(UPDI_DEBUG, "Seg: ", seg->data, seg->len, "%02x ");
                    result = -6;
                    goto out;
                }
            }
            else {
                DBG_INFO(UPDI_DEBUG, "fuses in hex file overflow, seg size %d, buffer size %d", seg->addr_from + seg->len, len);
            }
        }
    }

out:
    if (buf)
        free(buf);

    return result;
}

/*
    UPDI compare nvm crc and fuses byte with file
    @nvm_ptr: updi_nvm_init() device handle
    @file: ihex firmware file
    return 0 if match, else not match
*/
int updi_compare(void *nvm_ptr, const char *file)
{
    hex_data_t *dhex = NULL;
    int result;

    dhex = get_hex_info_from_file(file);
    if (!dhex) {
        DBG_INFO(UPDI_DEBUG, "get_hex_info_from_file failed %d");
        return -2;
    }

    result = compare_nvm_crc(nvm_ptr, dhex);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "crc mismatch");
        result = -3;
        goto out;
    }

    result = compare_nvm_fuses(nvm_ptr, dhex);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "fuses mismatch");
        result = -4;
        goto out;
    }

out:
    if (dhex)
        release_dhex(dhex);

    return result;
}

/*
    UPDI compare and program firmware
    @nvm_ptr: updi_nvm_init() device handle
    @file: ihex firmware file
    return 0 if success, else failed
*/
int updi_update(void *nvm_ptr, const char *file)
{
    int result;

    result = updi_compare(nvm_ptr, file);
    if (result) {
        result = updi_program(nvm_ptr, file);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "updi_program failed %d", result);
            result = -2;
        }
    }

    return result;
}

/*
    UPDI Save flash content to a ihex file
    @nvm_ptr: updi_nvm_init() device handle
    @file: Hex file path for output
    @returns 0 - success, other value failed code
*/
int updi_save(void *nvm_ptr, const char *file)
{
    hex_data_t dhex_info;
    information_container_t info_container;
    unsigned char *buf = NULL;
    int len;
    int crc, ecrc;
    char * save_file = NULL;
    int result;

    memset(&dhex_info, 0, sizeof(dhex_info));
    memset(&info_container, 0, sizeof(info_container));
    //Get infoblock first
    result = get_infoblock_from_eeprom(nvm_ptr, &info_container);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_infoblock_from_eeprom failed", result);
        return -2;
    }

    //flash content
    len = ib_get(&info_container, IB_FW_SIZE);
    if (len <= 0) {
        DBG_INFO(UPDI_DEBUG, "get_flash_content failed");
        result = -3;
        goto out;
    }

    buf = get_nvm_content(nvm_ptr, NVM_FLASH, &len);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "get_flash_content failed");
        result = -3;
        goto out;
    }
    
    crc = calc_crc24(buf, len);
    ecrc = ib_get(&info_container, IB_CRC_FW);
    if (ecrc < 0 || ecrc != crc) {
        DBG_INFO(UPDI_DEBUG, "Info Block read fw crc24 mismatch %06x(%06x), force save flash data with size %d", ecrc, crc, len);
        /*
        result = -4;
        goto out;
        */
    }

    result = save_content_to_segment(nvm_ptr, NVM_FLASH, &dhex_info, 0, buf, len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_flash_content_to_segment failed %d", result);
        result = -5;
        goto out;
    }
    free(buf);
    buf = NULL;

    //eeprom content
    len = ib_get(&info_container, IB_HEAD_SIZE);
    if (len <= 0) {
        DBG_INFO(UPDI_DEBUG, "get eeprom size = %d failed", len);
        result = -6;
        goto out;
    }
    result = save_content_to_segment(nvm_ptr, NVM_EEPROM, &dhex_info, INFO_BLOCK_ADDRESS_IN_EEPROM, (char *)info_container.head, len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_eeprom_content_to_segment failed %d", result);
        result = -7;
        goto out;
    }

    //fuse content
    len = 0;
    buf = get_nvm_content(nvm_ptr, NVM_FUSES, &len);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "get_fuses_content failed");
        result = -8;
        goto out;
    }
    
    result = save_content_to_segment(nvm_ptr, NVM_FUSES, &dhex_info, 0, buf, len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_eeprom_content_to_segment failed %d", result);
        result = -9;
        goto out;
    }

    //save hex content to file
    save_file = trim_name_with_extesion(file, '.', 1, SAVE_FILE_EXTENSION_NAME);
    if (!save_file) {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", SAVE_FILE_EXTENSION_NAME, result);
        result = -9;
        goto out;
    }

    result = save_hex_info_to_file(save_file, &dhex_info);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_hex_info failed %d", result);
        result = -10;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "Save Hex to \"%s\"", save_file);

out:
    if (buf)
        free(buf);

    if (save_file)
        free(save_file);

    unload_segments(&dhex_info);
    ib_destory(&info_container);
    return result;
}

/*
    UPDI dump whole nvm content to a Hex file
    @nvm_ptr: updi_nvm_init() device handle
    @file: Hex file path for output
    @returns 0 - success, other value failed code
*/
int updi_dump(void *nvm_ptr, const char *file)
{
    hex_data_t dhex_info;
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    char * buf;
    char * save_file = NULL;

    int i, result = 0;

    memset(&dhex_info, 0, sizeof(dhex_info));
    for (i = 0; i < NUM_NVM_TYPES; i++) {
        result = nvm_get_block_info(nvm_ptr, i, &iblock);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "nvm_get_flash_info failed %d", result);
            result = -3;
            break;
        }

        buf = malloc(iblock.nvm_size);
        if (!buf) {
            DBG_INFO(UPDI_DEBUG, "malloc nvm buf size = %d failed", iblock.nvm_size);
            result = -4;
            break;
        }

        result = nvm_read_mem(nvm_ptr, iblock.nvm_start, buf, iblock.nvm_size);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "nvm_read_mem type %d failed %d", i, result);
            result = -5;
            free(buf);
            break;
        }

        sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
        seg = set_segment_data_by_id_addr(&dhex_info, sid, 0, iblock.nvm_size, buf, SEG_ALLOC_MEMORY);
        if (!seg) {
            DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr type %d failed %d", i, result);
            result = -5;
            free(buf);
            break;
        }

        free(buf);
    }

    save_file = trim_name_with_extesion(file, '.', 1, DUMP_FILE_EXTENSION_NAME);
    if (!save_file) {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", DUMP_FILE_EXTENSION_NAME, result);
        return -2;
    }

    result = save_hex_info_to_file(save_file, &dhex_info);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_hex_info failed %d", result);
        result = -5;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "Dump Hex to \"%s\"", save_file);

out:
    if (save_file)
        free(save_file);

    unload_segments(&dhex_info);
    
    return result;
}

/*
    Load flash segment from hex file
    @dev: device info structure, get by get_chip_info()
    @file: hex file name
    @dhex: hex data structure
    return flash segment if sucessful, else NULL
*/
segment_buffer_t * load_flash_segment_from_file(const device_info_t * dev, const char *file, hex_data_t *dhex)
{
    nvm_info_t iblock;

    segment_buffer_t *seg = NULL;
    ihex_segment_t sid;
    int result;

    if (!dev || !file || !dhex)
        return NULL;

    //Flash data content
    result = load_segments_from_file(file, dhex);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "load_hex_file '%s' failed %d", file, result);
        return NULL;
    }

    //check and set default segment to flash
    result = dev_get_nvm_info(dev, NVM_FLASH, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -3;
        goto out;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    seg = get_segment_by_id_addr(dhex, sid, 0);
    if (!seg) {
        seg = get_segment_by_id_addr(dhex, 0, 0);
        if (seg) {
            set_default_segment_id(dhex, sid);
        }
    }

out:    
    return seg;
}

/*
    Load version segment from vcs file
    @dev: device info structure, get by get_chip_info()
    @file: vcs file name
    @data: flash data buffer
    @len: flash data len
    @dhex: hex data structure
    return flash segment if sucessful, else NULL
*/
segment_buffer_t * load_version_segment_from_file(const device_info_t * dev, const char *file, const char *data, int len, hex_data_t *dhex)
{
    nvm_info_t iblock;
    segment_buffer_t *seg = NULL;
    ihex_segment_t sid;
    unsigned int version, dsdr_addr;
    int crc24, size;
    information_container_t info_container;
    char * vcs_file = NULL;
    char * map_file = NULL;
    int result;

    memset(&info_container, 0, sizeof(info_container));

    vcs_file = trim_name_with_extesion(file, '\\', 2, VAR_FILE_RELATIVE_POS);
    if (!vcs_file) {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed", VAR_FILE_RELATIVE_POS);
        goto out;
    }

    result = load_version_value_from_file(vcs_file, &version);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "load_version_value_from_file failed %d", result);
        goto out;
    }

    map_file = trim_name_with_extesion(file, '.', 1, MAP_FILE_EXTENSION_NAME);
    if (!map_file) {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", MAP_FILE_EXTENSION_NAME, result);
        goto out;
    }

    result = load_dsdr_value_from_file(map_file, &dsdr_addr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "load_dsdr_value_from_file failed %d", result);
        goto out;
    }

    result = dev_get_nvm_info(dev, NVM_EEPROM, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        goto out;
    }

    crc24 = calc_crc24(data, len);
    result = create_information_block_s2(&info_container, crc24, len, version, dsdr_addr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "create_information_block_s2 failed %d", result);
        goto out;
    }

    size = ib_get(&info_container, IB_HEAD_SIZE);
    if (size <= 0) {
        DBG_INFO(UPDI_DEBUG, "get head size = %d failed %d", size, result);
        goto out;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    unload_segment_by_sid(dhex, sid);
    seg = set_segment_data_by_id_addr(dhex, sid, INFO_BLOCK_ADDRESS_IN_EEPROM, size, (char *)info_container.head, SEG_ALLOC_MEMORY);
    if (!seg) {
        DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed");
        goto out;
    }

out:
    ib_destory(&info_container);
    if (vcs_file)
        free(vcs_file);

    return seg;
}

/*
    Load fuse segment from vcs file
    @dev: device info structure, get by get_chip_info()
    @file: vcs file name
    @dhex: hex data structure
    return fuse segment count if success, negative mean failed
*/
int load_fuse_content_from_file(const device_info_t * dev, const char * file, hex_data_t *dhex)
{
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    unsigned char val;
    unsigned int *fuses_config = NULL;
    const int invalid_value = 0x800;
    char *vcs_file = NULL;
    int i, result = 0;

    result = dev_get_nvm_info(dev, NVM_FUSES, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        return -2;
    }

    fuses_config = malloc(iblock.nvm_size * sizeof(*fuses_config));
    if (!fuses_config) {
        DBG_INFO(UPDI_DEBUG, "malloc fuses_config failed %d");
        result = -3;
        goto out;
    }

    vcs_file = trim_name_with_extesion(file, '\\', 2, VAR_FILE_RELATIVE_POS);
    if (!vcs_file) {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", VAR_FILE_RELATIVE_POS, result);
        goto out;
    }

    result = search_defined_array_int_from_file(vcs_file, "FUSES_CONTENT", fuses_config, iblock.nvm_size, invalid_value);
    if (result == 0) {
        DBG_INFO(UPDI_DEBUG, "No fuse content defined at '%s'", vcs_file);
        goto out;
    }
    else if (result < 0 || result > iblock.nvm_size) {
        DBG_INFO(UPDI_DEBUG, "search_defined_array_int_from_file failed %d", result);
        result = -4;
        goto out;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    unload_segment_by_sid(dhex, sid);

    for (i = 0; i < result; i++) {
        if (fuses_config[i] != invalid_value)
            set_segment_data_by_id_addr(dhex, sid, i, 1, NULL, 0);
    }

    for (i = 0; i < result; i++) {
        if (fuses_config[i] != invalid_value) {
            val = fuses_config[i] & 0xff;
            seg = set_segment_data_by_id_addr(dhex, sid, i, 1, &val, SEG_ALLOC_MEMORY);
            if (!seg) {
                DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed %d val 0x%02x", i, val);
                result = -5;
                goto out;
            }
            else {
                DBG_INFO(UPDI_DEBUG, "Fuse[%d]: %02x", i, val);
            }
        }
    }
out:
    if (vcs_file)
        free(vcs_file);

    if (fuses_config)
        free(fuses_config);

    return result;
}

/*
    Device pack raw Hex content to a vcs Hex file
    @dev: device info structure, get by get_chip_info()
    @file: raw Hex file path for input
    @returns 0 - success, other value failed code
*/
int dev_pack_to_vcs_hex_file(const device_info_t * dev, const char *file)
{
    hex_data_t dhex_info;
    segment_buffer_t *seg;
    char * ihex_file = NULL;

    int result = 0;

    memset(&dhex_info, 0, sizeof(dhex_info));
    
    //get first flash segment from file
    seg = load_flash_segment_from_file(dev, file, &dhex_info);
    if (!seg) {
        DBG_INFO(UPDI_DEBUG, "load_flash_segment_from_file failed");
        result = -3;
        goto out;
    }

    seg = load_version_segment_from_file(dev, file, seg->data, seg->len, &dhex_info);
    if (!seg) {
        DBG_INFO(UPDI_DEBUG, "Skip load_version_segment_from_file (failed)");
        //result = -5;
        //goto out;
    }

    result = load_fuse_content_from_file(dev, file, &dhex_info);
    if (result < 0) {
        DBG_INFO(UPDI_DEBUG, "Skip load_fuse_content_from_file(error=%d)", result);
        //result = -6;
        //goto out;
    }

    ihex_file = trim_name_with_extesion(file, '.', 1, VCS_HEX_FILE_EXTENSION_NAME);
    if (!ihex_file) {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", VCS_HEX_FILE_EXTENSION_NAME, result);
        return -7;
    }

    result = save_hex_info_to_file(ihex_file, &dhex_info);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_hex_info failed %d", result);
        result = -8;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "Saved Hex to \"%s\"", ihex_file);

out:

    if (ihex_file)
        free(ihex_file);

    unload_segments(&dhex_info);

    return result;
}

/*
    Device show vcs Hex file info
    @dev: device info structure, get by get_chip_info()
    @file: raw Hex file path for input
    @returns 0 - success, other value failed code
*/
int dev_vcs_hex_file_show_info(const device_info_t * dev, const char *file)
{
    hex_data_t dhex_info;
    information_container_t file_info_container;
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    int fw_size, fw_crc, crc;
    int result = 0;

    memset(&dhex_info, 0, sizeof(dhex_info));
    memset(&file_info_container, 0, sizeof(file_info_container));

    result = load_segments_from_file(file, &dhex_info);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "load_hex_file '%s' failed %d", file, result);
        return -2;
    }

    result = get_infoblock_from_hex_info_dev(dev, &dhex_info, &file_info_container);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_infoblock_from_hex_info failed %d", result);
        result = -3;
        goto out;
    }

    result = dev_get_nvm_info(dev, NVM_FLASH, &iblock);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -4;
        goto out;
    }

    sid = ADDR_TO_SEGMENTID(iblock.nvm_start);
    seg = get_segment_by_id(&dhex_info, sid);
    if (!seg) {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -4;
        goto out;
    }

    fw_size = ib_get(&file_info_container, IB_FW_SIZE);
    if (fw_size <= 0 || seg->len < fw_size) {
        DBG_INFO(UPDI_DEBUG, "seg size not enough, seg = %d, infoblock size %d", seg->len, fw_size);
        result = -5;
        goto out;
    }

    fw_crc = ib_get(&file_info_container, IB_CRC_FW);
    crc = calc_crc24(seg->data, fw_size);
    if (fw_crc < 0 || crc < 0 || fw_crc != crc) {
        DBG_INFO(UPDI_DEBUG, "Info Block read file crc24 mismatch %06x(%06x)", fw_crc, crc);
        result = -6;
        goto out;
    }

    //Show ib area
    ib_show(&file_info_container);

    //Fuse area
    dev_hex_show(dev, NVM_FUSES, &dhex_info);
out:
    ib_destory(&file_info_container);
    unload_segments(&dhex_info);

    return result;
}

/*
    Memory Read
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and count. Format: [addr];[count]
    @returns 0 - success, other value failed code
*/
int _updi_read_mem(void *nvm_ptr, char *cmd, u8 *outbuf, int outlen)
{
    char** tk_s, **tk_w;    //token section, token words
#define UPDI_READ_STROKEN_WORDS_LEN 255
    int address, len, copylen, outlen_left = outlen;
    char *buf;
    int i, k, result = 0;

    tk_s = str_split(cmd, '|');
    for (k = 0; tk_s && tk_s[k]; k++) {
        tk_w = str_split(tk_s[k], ':');
        for (i = 0, address = ERROR_PTR; tk_w && tk_w[i]; i++) {
            if (result == 0) {   //only work when no error occur
                if (i == 0)
                    address = (int)strtol(tk_w[i], NULL, 16);
                else if (i == 1) {
                    if (VALID_PTR(address)) {
                        len = (int)(strtol(tk_w[i], NULL, 10));    //Max size 255 once
                        if (len > UPDI_READ_STROKEN_WORDS_LEN) {
                            DBG_INFO(UPDI_DEBUG, "Read memory len %d over max, set to", len, UPDI_READ_STROKEN_WORDS_LEN);
                            len = UPDI_READ_STROKEN_WORDS_LEN;
                        }

                        buf = malloc(len);
                        if (!buf) {
                            DBG_INFO(UPDI_DEBUG, "mallloc memory %d failed", len);
                            result = -3;
                            continue;
                        }

                        result = nvm_read_mem(nvm_ptr, address, buf, len);
                        if (result) {
                            DBG_INFO(UPDI_DEBUG, "nvm_read_mem failed %d", result);
                            result = -4;
                        }
                        else {
                            if (outbuf && outlen_left > 0) {
                                copylen = min(outlen, len);
                                memcpy(outbuf, buf, copylen);
                                outbuf += copylen;
                                outlen_left -= copylen;
                            }
                            else {
                                //Debug output:
                                DBG(DEFAULT_DEBUG, "Read tk[%d]:", buf, len, "%02x ", i);
                            }
                        }

                        free(buf);
                    }
                }
            }
            free(tk_w[i]);
        }

        if (!tk_w) {
            DBG_INFO(UPDI_DEBUG, "Parse read str tk_w: %s failed", tk_s[k]);
        }
        else
            free(tk_w);
        free(tk_s[k]);
    }

    if (!tk_s) {
        DBG_INFO(UPDI_DEBUG, "Parse read str tk_s: %s failed", cmd);
    }else
        free(tk_s);

    if (result)
        return result;

    return outlen - outlen_left;
}

/*
    UPDI Memory Read
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and count. Format: [addr];[count]
    @returns 0 - success, other value failed code
*/
int updi_read(void *nvm_ptr, char *cmd)
{
    return _updi_read_mem(nvm_ptr, cmd, NULL, 0);
}

/*
    Memory Write
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and data. Format: [addr];[dat0];[dat1];[dat2]|[addr1]...
    @op: operating function for write
    @returns 0 - success, other value failed code
*/
int _updi_write(void *nvm_ptr, char *cmd, nvm_op opw)
{
    char** tk_s, **tk_w, **tokens;
    int address;
#define UPDI_WRITE_STROKEN_LEN 16
    char buf[UPDI_WRITE_STROKEN_LEN];
    int i, j, k, m, result = 0;
    bool dirty = false;

    tk_s = str_split(cmd, '|');
    for (k = 0; tk_s && tk_s[k]; k++) {
        tk_w = str_split(tk_s[k], ':');
        for (m = 0, address = ERROR_PTR; tk_w && tk_w[m]; m++) {
            if (result == 0) { //only work when no error occur
                if (m == 0)
                    address = (int)strtol(tk_w[m], NULL, 16);
                else if (m == 1) {
                    tokens = str_split(tk_w[m], ';');
                    for (i = 0; tokens && tokens[i]; i++) {
                        DBG_INFO(UPDI_DEBUG, "Write[%d]: %s", i, tokens[i]);

                        j = i % UPDI_WRITE_STROKEN_LEN;
                        buf[j] = (char)(strtol(tokens[i], NULL, 16) & 0xff);
                        dirty = true;
                        if (j + 1 == UPDI_WRITE_STROKEN_LEN) {
                            result = opw(nvm_ptr, address + i - j, buf, j + 1);
                            if (result) {
                                DBG_INFO(UPDI_DEBUG, "opw failed %d", result);
                                result = -4;
                            }
                            dirty = false;
                        }
                        free(tokens[i]);
                    }
                    //write the left data
                    if (dirty && result == 0) {
                        result = opw(nvm_ptr, address + i - j - 1, buf, j + 1);
                        if (result) {
                            DBG_INFO(UPDI_DEBUG, "opw failed %d", result);
                            result = -5;
                        }
                    }
                    DBG_INFO(DEFAULT_DEBUG, "Write address %x(%d), result %d", address, i, result);

                    if (!tokens) {
                        DBG_INFO(UPDI_DEBUG, "Parse write str: %s failed", tk_w[m]);
                    }
                    else
                        free(tokens);
                }
            }
            free(tk_w[m]);
        }
        if (!tk_w) {
            DBG_INFO(UPDI_DEBUG, "Parse write str: %s failed", tk_s[k]);
        }
        else
            free(tk_w);

        free(tk_s[k]);
    }

    if (!tk_s) {
        DBG_INFO(UPDI_DEBUG, "Parse write str: %s failed", cmd);
    }
    else
        free(tk_s);

    return result;
}

/*
UPDI Memory Write
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and data. Format: [addr0]:[dat0];[dat1];[dat2]|[addr1]:...
    @returns 0 - success, other value failed code
*/
int updi_write(void *nvm_ptr, char *cmd)
{
    return _updi_write(nvm_ptr, cmd, nvm_write_auto);
}

/*
UPDI Fuse Write
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and data. Format: [addr0]:[dat0];[dat1];[dat2]|[addr1]:...
    @returns 0 - success, other value failed code
*/
int updi_write_fuse(void *nvm_ptr, char *cmd)
{
    return _updi_write(nvm_ptr, cmd, nvm_write_fuse);
}

/*
    UPDI Reset chip
    @nvm_ptr: updi_nvm_init() device handle
    @returns 0 - success, other value failed code
*/
int updi_reset(void *nvm_ptr)
{
    return nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET);
}

/*
    Debug view
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and data. Format: ds=[ptr]|dr[ptr]|loop=[dat1]|keys[dat2]
            ds pointer: ptc_qtlib_node_stat1 address, which store structure qtm_acq_node_data_t, we get signal and cc value in it
            dr pointer: qtlib_key_data_set1 address, which store qtm_touch_key_data_t, we get ref value in it
    @returns 0 - success, other value failed code. this function will print the log in console
*/

/* ---------------------------------------------------------------------------------------- */
/* Acquisition Node run-time data */
/* ---------------------------------------------------------------------------------------- */
PACK(
typedef struct {
    uint8_t  node_acq_status;
    uint16_t node_acq_signals;
    uint16_t node_comp_caps;
}) qtm_acq_node_data_t;

/* ---------------------------------------------------------------------------------------- */
/* Key sensor run-time data */
/* ---------------------------------------------------------------------------------------- */
PACK(
typedef struct {
    uint8_t              sensor_state;         /* Disabled, Off, On, Filter, Cal... */
    uint8_t              sensor_state_counter; /* State counter */
    uint8_t              node_data_struct_ptr[2]; /* Pointer to node data structure */
    uint16_t             channel_reference;    /* Reference signal */
}) qtm_touch_key_data_t;

/* param tag */
enum {
    SIGNAL_ADDR,
    REFERENCE_ADDR,
    LOOP_CNT,
    KEY_CNT,
    MAX_PARAM_NUM
};

const char *token_tag[MAX_PARAM_NUM] = {
    "ds"/*ptc_qtlib_node_stat1->signal and cc value*/,
    "dr" /*qtlib_key_data_set1->ref value*/,
    "loop",
    "keys" };

int updi_debugview(void *nvm_ptr, char *cmd)
{
    char** tk_s, **tk_w;    //token section, token words
    int16_t val, ref_value, signal_value, delta_value;
    double cc_value;
    int i,j, result = 0;
    
    //time varible
    time_t timer;
    char timebuf[26];
    struct tm* tm_info;    

    //debug varible
    qtm_acq_node_data_t ptc_signal;
    qtm_touch_key_data_t ptc_ref;
    int params[MAX_PARAM_NUM] = {0/*SIGNAL_ADDR*/, 0/*REFERENCE_ADDR*/, 0/*LOOP_CNT*/, 0/*KEY_CNT*/}; //loop value default set to 1, keys default set to 1

    //memset(&params, 0, sizeof(params));

    tk_s = str_split(cmd, '|');
    if (!tk_s) {
        DBG_INFO(UPDI_DEBUG, "Parse debugview str tk_s: %s failed", cmd);
        return -2;
    }

    for (i = 0; tk_s[i]; i++) {
        tk_w = str_split(tk_s[i], '=');
        if (!tk_w) {
            DBG_INFO(UPDI_DEBUG, "Parse debugview str tk_w: %s failed", tk_s[i]);
            continue;
        }

        if (tk_w[0] && tk_w[1]) {
            for (j = 0; j < MAX_PARAM_NUM; j++) {
                if (!strcmp(token_tag[j], tk_w[0])) {
                    params[j] = (int)strtol(tk_w[1], NULL, 16);
                    break;
                }
            }
        }

        for (j = 0; tk_w[j]; j++)
            free(tk_w[j]);
        free(tk_s[i]);
    }
    free(tk_s);

    //if LOOP_CNT less than or qual 0: loop forever
    for (i = 0; params[LOOP_CNT] <= 0 || i < params[LOOP_CNT]; i++) {
        for (j = 0; j < params[KEY_CNT]; j++) {

            memset(&ptc_signal, 0, sizeof(ptc_signal));
            memset(&ptc_ref, 0, sizeof(ptc_ref));

            if (params[SIGNAL_ADDR]) {
                result = nvm_read_mem(nvm_ptr, (u16)(params[SIGNAL_ADDR] + j * sizeof(ptc_signal)), (void *)&ptc_signal, sizeof(ptc_signal));
                if (result) {
                    DBG_INFO(UPDI_DEBUG, "nvm_read_mem signal failed %d", result);
                    result = -4;
                    break;
                }
            }

            if (params[REFERENCE_ADDR]) {
                result = nvm_read_mem(nvm_ptr, (u16)(params[REFERENCE_ADDR] +j * sizeof(ptc_ref)), (void *)&ptc_ref, sizeof(ptc_ref));
                if (result) {
                    DBG_INFO(UPDI_DEBUG, "nvm_read_mem reference failed %d", result);
                    result = -5;
                    break;
                }
            }

            time(&timer);
            tm_info = localtime(&timer);
            strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm_info);

            val = (int16_t)lt_int16_to_cpu(ptc_signal.node_comp_caps);
            cc_value = (val & 0x0F)*0.00675 + ((val >> 4) & 0x0F)*0.0675 + ((val >> 8) & 0x0F)*0.675 + ((val >> 12) & 0x3) * 6.75;
            ref_value = (int16_t)lt_int16_to_cpu(ptc_ref.channel_reference);
            signal_value = (int16_t)lt_int16_to_cpu(ptc_signal.node_acq_signals);
            delta_value = signal_value - ref_value;

            //Debug output:
            /*
            DBG(DEFAULT_DEBUG, "signal raw:", (unsigned char *)&ptc_signal, sizeof(ptc_signal), "0x%02x ");
            DBG(DEFAULT_DEBUG, "ref raw:", (unsigned char *)&ptc_ref, sizeof(ptc_signal), "0x%02x ");
            */
            DBG_INFO(DEFAULT_DEBUG, "T[%s][%d-%d]: delta,%hd, ref,%hd, signal,%hd, cc,%.2f, sensor_state,%02xH, node_state,%02xH", timebuf, i, j,
                delta_value,
                ref_value,
                signal_value,
                cc_value,
                ptc_ref.sensor_state,
                ptc_signal.node_acq_status);
        }
    }

    return result;
}
