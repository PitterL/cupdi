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
#include <regex/re.h>
#include "cupdi.h"

#define SOFTWARE_VERSION "1.04" 

static const char *const usage[] = {
    "Simple command line interface for UPDI programming:",
    "cupdi [options] [[--] args]",
    "Erase chip: cupdi -c COM2 -d tiny817 -e ",
    "Flash hex file: cupdi -c COM2 -d tiny817 -f c:/817.hex",
    NULL,
};

enum { FLAG_UNLOCK, FLAG_ERASE, FLAG_PROG, FLAG_UPDATE, FLAG_CHECK, FLAG_SAVE, FLAG_INFO};
enum { OP_READ, OP_WRITE };

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
    bool unlock = false;
    int verbose = 1;
    bool reset = false;
    bool test = false;
    bool version = false;

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
        OPT_BIT('p', "program", &flag, "Program Intel HEX file to flash", NULL, (1 << FLAG_PROG), 0),
        OPT_BIT('-', "update", &flag, "Program infoblock update to eeprom(need map file)", NULL, (1 << FLAG_UPDATE), 0),
        OPT_BIT('k', "check", &flag, "Compare Intel HEX file with flash content", NULL, (1 << FLAG_CHECK), 0),
        OPT_BIT('i', "info", &flag, "Get Infoblock infomation of firmware", NULL, (1 << FLAG_INFO), 0),
        OPT_BIT('s', "save", &flag, "Save flash to a Intel HEX file", NULL, (1 << FLAG_SAVE), 0),
        OPT_STRING('u', "fuses", &fuses, "Fuse to set [addr0]:[dat0];[dat1];|[addr1]..."),
        OPT_STRING('r', "read", &read, "Direct read from memory [addr1]:[n1]|[addr2]:[n2]..."),
        OPT_STRING('w', "write", &write, "Direct write to memory [addr0]:[dat0];[dat1];|[addr1]..."),
        OPT_STRING('g', "dbgview", &dbgview, "get ref/delta/cc value operation ds=[ptc_qtlib_node_stat1]|dr=[qtlib_key_data_set1]|loop=[n]|keys=[n] (loop(Hex) set to 0 loop forvever, default 1, keys default 1)"),
        OPT_INTEGER('v', "verbose", &verbose, "Set verbose mode (SILENCE|UPDI|NVM|APP|LINK|PHY|SER): [0~6], default 0, suggest 2 for status information"),
        OPT_BOOLEAN('-', "reset", &reset, "UPDI reset device"),
        OPT_BOOLEAN('t', "test", &test, "Test UPDI device"),
        OPT_BOOLEAN('-', "version", &version, "Show version"),
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

    //set parameter
    set_verbose_level(verbose);

    if (version) {
      DBG_INFO(UPDI_DEBUG, "CUPDI version: %s", SOFTWARE_VERSION);
      return 0;
    }

    if (!dev_name) {
        DBG_INFO(UPDI_DEBUG, "No DEV Name appointed");
        return ERROR_PTR;
    }

    if (!comport) {
        DBG_INFO(UPDI_DEBUG, "No COM PORT appointed");
        return ERROR_PTR;
    }

    if (file) {
        if (!flag)
            SET_BIT(flag, FLAG_PROG);
    }

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

    result = nvm_get_device_info(nvm_ptr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_device_info failed");
        result = -4;
        goto out;
    }

    //programming unlock
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

    if (TEST_BIT(flag, FLAG_ERASE)) {
        result = updi_erase(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM chip erase failed %d", result);
            result = -7;
            goto out;
        }
    }

    if (file) {
        if (TEST_BIT(flag, FLAG_PROG) || TEST_BIT(flag, FLAG_UPDATE)) {
            result = updi_flash(nvm_ptr, file, flag);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "updi_flash failed %d", result);
                result = -9;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_SAVE)) {
            result = updi_save(nvm_ptr, file);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "NVM save failed %d", result);
                result = -10;
                goto out;
            }
        }
    }

    if (TEST_BIT(flag, FLAG_INFO)) {
        result = updi_read_infoblock(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "updi_read_infoblock failed %d", result);
            result = -11;
            goto out;
        }
    }

    if (TEST_BIT(flag, FLAG_CHECK)) {
        result = updi_verify_infoblock(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "updi_verify_infoblock failed %d", result);
            result = -11;
            goto out;
        }
    }

    if (read) {
        result = updi_read(nvm_ptr, read);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "Read failed %d", result);
            result = -12;
            goto out;
        }
    }

    if (write) {
        result = updi_write(nvm_ptr, write);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "Write failed %d", result);
            result = -13;
            goto out;
        }
    }

    if (fuses) {
        result = updi_write_fuse(nvm_ptr, fuses);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM set fuse failed %d", result);
            result = -14;
            goto out;
        }
    }

    if (reset) {
        result = nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "NVM reset failed %d", result);
            result = -15;
            goto out;
        }
    }

    if (dbgview) {
        result = updi_debugview(nvm_ptr, dbgview);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "Debugview failed %d", result);
            result = -16;
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
load hex file to memory, this will align the address and size to flash page size, patch with 0xff
    @file: Hex file path
    @iflash: chip flash information, get by nvm_get_flash_info()
    @returns pointer to bin content memory, Null failed
*/
void unload_hex(void *dhex_ptr);
hex_data_t *load_hex(const char *file, const nvm_info_t *iflash)
{
    hex_data_t hinfo, *dhex = NULL;
    u32 from, to, size, len, off;
    u32 mask = iflash->nvm_pagesize - 1;
    int result;

    memset(&hinfo, 0, sizeof(hinfo));
    result = get_hex_info(file, &hinfo);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_hex_info failed %d", result);
        return NULL;
    }
    
    //align the data to page size
    from = hinfo.addr_from & ~mask;
    to = ((hinfo.addr_to + mask) & ~mask) - 1;
    size = to - from + 1;
    off = hinfo.addr_from & mask;
    len = size + sizeof(*dhex);
    
    if (to >= iflash->nvm_size) {
        DBG_INFO(UPDI_DEBUG, "hex addr(%04x ~ %04x) over flash size ", from, to);
        return NULL;
    }

    dhex = (hex_data_t *)malloc(len);
    if (!dhex) {
        DBG_INFO(UPDI_DEBUG, "malloc hexi memory(%d) failed", len);
        return NULL;
    }
    memcpy(dhex, &hinfo, sizeof(*dhex));
    dhex->data = (unsigned char *)(dhex + 1);
    dhex->len = size;
    dhex->offset = off;
    memset(dhex->data, 0xff, size);

    result = get_hex_info(file, dhex);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "get_hex_info failed %d", result);
        result = -4;
        goto failed;
    }

    dhex->addr_from = from;
    dhex->addr_to = to;

    return dhex;

failed:
    unload_hex(dhex);
    return NULL;
}

/*
    destory hex memory pointer
    @dhex_ptr: point to memory block by load_hex()
    @no return
*/
void unload_hex(void *dhex_ptr)
{
    if (dhex_ptr) {
        free(dhex_ptr);
    }
}

/*
#deprecated, we have infoblock at eeprom, so this could verified by crc24, see updi_verify_infoblock()
Verify the hex memory with flash content
    @nvm_ptr: updi_nvm_init() device handle
    @dhex: point to memory block by load_hex()
    @return 0 mean consistent, other value mismatch
*/
int verify_hex(void *nvm_ptr, hex_data_t *dhex)
{
    u8 * rdata;
    int i, result;

    //compare data
    rdata = malloc(dhex->len);
    if (!rdata) {
        DBG_INFO(UPDI_DEBUG, "malloc rdata failed");
        return -2;
    }

    result = nvm_read_flash(nvm_ptr, dhex->addr_from, rdata, dhex->len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_flash failed %d", result);
        result = -3;
        goto out;
    }

    for (i = 0; i < dhex->len; i++) {
        if (dhex->data[i] != rdata[i]) {
            DBG_INFO(UPDI_DEBUG, "check flash data failed at %d, %02x-%02x", i, dhex->data[i], rdata[i]);
            break;
        }
    }

    if (i < dhex->len) {
        DBG_INFO(UPDI_DEBUG, "data verifcation failed");
        result = -4;
        goto out;
    }

    DBG(UPDI_DEBUG, "Flash data verified", rdata, dhex->len, "%02x ");

out:
    if (rdata)
        free(rdata);

    return result;
}

/*
Get varible address in map file, the format is: <address> <name>
    @file: hex file name, the function will replace the extension name to .map 
    @varlist: varible name array
    @varnum: varrible name array length
    @out: varible value output array buffer, the length should equal varnum
    @return how many varibles get, negative mean error occur
*/
int search_address_from_map_file(const char *file, const char *varlist[], int varnum, unsigned int *out)
{
    char *mapfile = NULL;
    char *content = NULL;
  
    tre_comp tregex;
    char pat_str1[TRE_MAX_BUFLEN], *pat_str2;
    const char *st;

    unsigned int val;
    const char *pat_trunc = "\\s+0x[0-9a-fA-F]{8}\\s+";
    int i, find = 0;

    mapfile = make_name_with_extesion(file, "map");
    if (!mapfile) {
        DBG_INFO(UPDI_DEBUG, "make map file name failed");
        find = -2;
        goto out;
    }

    content = read_whole_file(mapfile);
    if (!content) {
        DBG_INFO(UPDI_DEBUG, "load map file '%s' failed", mapfile);
        find = -3;
        goto out;
    }

    for (i = 0; i < varnum; i++) {
        out[i] = 0;

        if (!varlist[i])
            continue;

        //create pattern to search
        pat_str1[0] = '\0';
        strncat(pat_str1, pat_trunc, sizeof(pat_str1) - 1);
        strncat(pat_str1, varlist[i], sizeof(pat_str1) - strlen(pat_trunc) - 1);

        tre_compile(pat_str1, &tregex);
        st = tre_match(&tregex, content, NULL);

        if (st) {
            pat_str2 = "0x[0-9a-fA-F]{8}";
            tre_compile(pat_str2, &tregex);
            st = tre_match(&tregex, st, NULL);
            if (st) {
                val = (unsigned int)strtol(st, NULL, 16); // 0 is error
                if (val) {
                    DBG_INFO(UPDI_DEBUG, "map '%s': 0x%x", varlist[i], val);
                    out[i] = val;
                    find++;
                }
            }
        }
    }

out:
    if (content)
        free(content);

    if (mapfile)
        free(mapfile);

    return find;
}

/*
Get 'fw_version' address in map file
    @file: hex file name (the function will replace the extension name to .map)
    @return ERROR_PTR mean address invalid, other value get address from file
*/

unsigned int get_version_address_from_map(const char *file)
{
    const char *varlist[] = { "fw_version" };
    unsigned int address = 0;
    int result;

    result = search_address_from_map_file(file, varlist, 1, &address);
    if (result <= 0) {
        DBG_INFO(UPDI_DEBUG, "search_address_from_map_file failed %d", result);
        return ERROR_PTR;
    }

    return address;
}

/*
Get firmware version from memory
    @nvm_ptr: updi_nvm_init() device handle
    @address: varible address in memory
    @return INVALID_FIRMWARE_CODE if not found, else the big endian firmware version
*/
unsigned int get_firmware_varible(void *nvm_ptr, unsigned int address)
{
    char ver[4];
    int result;
#define INVALID_FIRMWARE_CODE 0xFF2E7858 //'Xx. 16.16'

    if (!VALID_PTR(address)) {
        DBG_INFO(UPDI_DEBUG, "invalid fw address %x", address);
        return 0;
    }
    
    result = nvm_read_mem(nvm_ptr, (u16)address, ver, sizeof(ver));
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_mem failed %d", result);
        return -2;
    }
    else {
        // Big endian store in firmware
        DBG(UPDI_DEBUG, "Firmware varible #1:", ver, 4, "0x%02x ");
        DBG_INFO(UPDI_DEBUG, "Found firmware varible: '%c%c%c v%d.%d'", ver[3], ver[2], ver[1], ver[0] >> 4, ver[0] & 0xf);

        //Check whether it's visible character
        if (ver[3] > 31 && ver[3] < 126 &&
            ver[2] > 31 && ver[2] < 126 &&
            ver[1] > 31 && ver[1] < 126) {
            return (ver[3] << 24) | (ver[2] << 16) | (ver[1] << 8) | ver[0];
        }
        else {
            DBG_INFO(UPDI_DEBUG, "Invalid firmware version, set to 0x%08x", INVALID_FIRMWARE_CODE);
            return INVALID_FIRMWARE_CODE;
        }
    }
}

/*
Read Info block from eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int read_infoblock(void *nvm_ptr, unsigned char *data, int len)
{
    int result;

    result = nvm_read_eeprom(nvm_ptr, INFO_BLOCK_ADDRESS_IN_EEPROM, data, len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_eeprom failed %d", result);
        return -2;
    }

    //DBG(UPDI_DEBUG, "Info Block:", data, len, "%02X ");

    result = calc_crc8(data, len);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "Info Block calc crc8 mismatch %02x:", result);
        DBG(UPDI_DEBUG, "Info Block:", data, len, "%02x ");
        return -3;
    }

    return result;
}

/*
Write Info block to eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @mapfile: map file name, to get firmware version address
    @data: data buffer
    @len: data len
    @return 0 successful, other value failed
*/
int write_infoblock(void *nvm_ptr, const char *mapfile, const unsigned char *data, int len)
{
    unsigned int address;
    unsigned int version;
    unsigned int fw_crc;
    unsigned char infoblock[INFO_BLOCK_SIZE];
    int result = -1, retry = 2;

    //Build Infor block should get value from sram, so there should do the reset
   
    result = nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_reset failed %d", result);
        return -2;
    }
    
    address = get_version_address_from_map(mapfile);
    if (!VALID_PTR(address)) {
        DBG_INFO(UPDI_DEBUG, "get_version_address_from_map failed");
        return -3;
    }

    version = get_firmware_varible(nvm_ptr, address);
    if (version <= 0) {
        DBG_INFO(UPDI_DEBUG, "get_firmware_varible failed");
        return -4;
    }

    memset(infoblock, 0, sizeof(infoblock));

    // 4 bytes fw version (big endian)
    infoblock[0] = (version >> 24) & 0xff;
    infoblock[1] = (version >> 16) & 0xff;
    infoblock[2] = (version >> 8) & 0xff;
    infoblock[3] = version & 0xff;
    DBG(UPDI_DEBUG, "Firmware version #2:", infoblock, 4, "%02x ");
    DBG_INFO(UPDI_DEBUG, "version: '%c%c%c v%d.%d'", infoblock[0], infoblock[1], infoblock[2], infoblock[3] >> 4, infoblock[3] & 0xf);

    // 4 bytes fw size (little endian)
    infoblock[4] = len & 0xff;
    infoblock[5] = (len >> 8) & 0xff;
    infoblock[6] = (len >> 16) & 0xff;
    infoblock[7] = (len >> 24) & 0xff;
    DBG_INFO(UPDI_DEBUG, "Firmware size: %d", len);

    // skip unused ...

    // 3 bytes fw crc (little endian)
    fw_crc = calc_crc24(data, len);
    infoblock[INFO_BLOCK_SIZE - 4] = fw_crc & 0xff;
    infoblock[INFO_BLOCK_SIZE - 3] = (fw_crc >> 8) & 0xff;
    infoblock[INFO_BLOCK_SIZE - 2] = (fw_crc >> 16) & 0xff;
    DBG_INFO(UPDI_DEBUG, "Firmware crc: 0x%06x", fw_crc);

    // 1 byte infoblock crc at end
    infoblock[INFO_BLOCK_SIZE - 1] = calc_crc8(infoblock, INFO_BLOCK_SIZE - 1);
    DBG_INFO(UPDI_DEBUG, "Infoblock crc: 0x%02x", infoblock[INFO_BLOCK_SIZE - 1]);

    result = nvm_write_eeprom(nvm_ptr, INFO_BLOCK_ADDRESS_IN_EEPROM, infoblock, sizeof(infoblock));
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_write_eeprom failed %d", result);
        return -5;
    }

    return 0;
}

/*
UPDI Read Info block from eeprom
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int updi_read_infoblock(void *nvm_ptr)
{
    unsigned char infoblock[INFO_BLOCK_SIZE];
    unsigned char *buf = NULL;
    int size, read_crc;
    int result;

    result = read_infoblock(nvm_ptr, infoblock, sizeof(infoblock));
    if (result) {
        DBG_INFO(UPDI_DEBUG, "updi_read_infoblock failed", result);
        return -2;
    }

    size = (infoblock[7] << 24) | (infoblock[6] << 16) | (infoblock[5] << 8) | infoblock[4];
    read_crc = (infoblock[INFO_BLOCK_SIZE - 2] << 16) | (infoblock[INFO_BLOCK_SIZE - 3] << 8) | infoblock[INFO_BLOCK_SIZE - 4];

    DBG(UPDI_DEBUG, "Firmware version #3:", infoblock, 4, "%02x ");
    DBG_INFO(UPDI_DEBUG, "FW version: '%c%c%c v%d.%d', size: %d bytes, crc: 0x%x",
        infoblock[0], infoblock[1], infoblock[2], infoblock[3] >> 4, infoblock[3] & 0xf,
        size,
        read_crc);

    return 0;
}

/*
UPDI verify Info block information
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 mean pass, other value failed
*/
int updi_verify_infoblock(void *nvm_ptr)
{
    unsigned char infoblock[INFO_BLOCK_SIZE];
    unsigned char *buf = NULL;
    int size, crc, read_crc;
    int result;

    result = read_infoblock(nvm_ptr, infoblock, sizeof(infoblock));
    if (result) {
        DBG_INFO(UPDI_DEBUG, "updi_read_infoblock failed", result);
        return -2;
    }

    size = (infoblock[7] << 24) | (infoblock[6] << 16) | (infoblock[5] << 8) | infoblock[4];
    DBG_INFO(UPDI_DEBUG, "Fw version: '%c%c%c v%d.%d', size %d(%4x)", infoblock[0], infoblock[1], infoblock[2], infoblock[3] >> 4, infoblock[3] & 0xf, size, size);
    DBG(UPDI_DEBUG, "Firmware version:", infoblock, 4, "%02x ");

    if (infoblock[7] || infoblock[6]) {
        DBG(UPDI_DEBUG, "Infoblock size invalid:", infoblock + 4, 4, "%02x ");
        return -3;
    }

    buf = malloc(size);
    if (!buf) {
        DBG_INFO(UPDI_DEBUG, "Verify info alloc memory failed");
        return -4;
    }

    result = nvm_read_flash(nvm_ptr, 0, buf, size);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_flash failed %d", result);
        result = -5;
        goto out;
    }

    // DBG(UPDI_DEBUG, "Flash data ", buf, size, "%02x ");

    crc = calc_crc24(buf, size);
    read_crc = (infoblock[INFO_BLOCK_SIZE - 2] << 16) | (infoblock[INFO_BLOCK_SIZE - 3] << 8) | infoblock[INFO_BLOCK_SIZE - 4];
    if (read_crc != crc) {
        DBG_INFO(UPDI_DEBUG, "Info Block read fw crc24 mismatch %06x(%06x)", read_crc, crc);
        return result;
    }

    DBG_INFO(UPDI_DEBUG, "Pass");
out:
    if (buf)
        free(buf);

    return result;
}

/*
UPDI Program flash
    This flowchart is: load firmware file->erase chip->program firmware->create infoblock->verify firmware in flash
    @nvm_ptr: updi_nvm_init() device handle
    @file: Hex file path
    @prog: Whether do program, if not, only verify the content
    @returns 0 - success, other value failed code
*/
int updi_flash(void *nvm_ptr, const char *file, int flag)
{
    hex_data_t *dhex = NULL;
    nvm_info_t flash;
    int result = 0;

    if (TEST_BIT(flag, FLAG_PROG) || TEST_BIT(flag, FLAG_UPDATE)) {
        result = nvm_get_flash_info(nvm_ptr, &flash);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "nvm_get_flash_info failed %d", result);
            result = -4;
            goto out;
        }

        dhex = load_hex(file, &flash);
        if (!dhex) {
            DBG_INFO(UPDI_DEBUG, "updi_load_hex failed");
            return -2;
        }

        if (TEST_BIT(flag, FLAG_PROG)) {
            result = nvm_chip_erase(nvm_ptr);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "nvm_chip_erase failed %d", result);
                result = -3;
                goto out;
            }

            result = nvm_write_flash(nvm_ptr, dhex->addr_from, dhex->data, dhex->len);
            if (result) {
                DBG_INFO(UPDI_DEBUG, "nvm_write_flash failed %d", result);
                result = -4;
                goto out;
            }
        }

        //verify_hex() not need here, use updi_verify_infoblock instead later

        result = write_infoblock(nvm_ptr, file, dhex->data, dhex->len);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "write_infoblock failed %d", result);
            result = -6;
            goto out;
        }
    }

    //Verify Infoblock
    result = updi_verify_infoblock(nvm_ptr);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "updi_verify_infoblock failed %d", result);
        result = -7;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "Flash check finished");

out:
    if (dhex)
        unload_hex(dhex);
    return result;
}

/*
 UPDI Save flash content to a Hex file
    @nvm_ptr: updi_nvm_init() device handle
    @file: Hex file path for output
    @returns 0 - success, other value failed code
*/
int updi_save(void *nvm_ptr, const char *file)
{
    nvm_info_t flash;
    hex_data_t * dhex;
    char * new_file;
    const char *new_file_posfix = ".save";
    size_t size;
    int result = 0;

    result = nvm_get_flash_info(nvm_ptr, &flash);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_get_flash_info failed %d", result);
        return -2;
    }

    size = flash.nvm_size + sizeof(*dhex) + strlen(file) + strlen(new_file_posfix) + 1;
    dhex = (hex_data_t *)malloc(size);
    if (!dhex) {
        DBG_INFO(UPDI_DEBUG, "malloc hexi memory(%d) failed", size);
        return -3;
    }
    memset(dhex, 0, sizeof(*dhex));
    dhex->data = (unsigned char *)(dhex + 1);
    dhex->len = flash.nvm_size;
    dhex->offset = 0;
    dhex->total_size = dhex->actual_size = dhex->len;
    dhex->addr_from = 0;
    dhex->addr_to = flash.nvm_size - 1;
   
    result = nvm_read_flash(nvm_ptr, flash.nvm_start, dhex->data, flash.nvm_size);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "nvm_read_flash failed %d", result);
        result = -4;
        goto out;
    }

    new_file = dhex->data + dhex->len;
    new_file[0] = '\0';
    strcat(new_file, file);
    strcat(new_file, new_file_posfix);
    result = save_hex_info(new_file, dhex);
    if (result) {
        DBG_INFO(UPDI_DEBUG, "save_hex_info failed %d", result);
        result = -5;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "Saved Hex to \"%s\"", new_file);

out:
    free(dhex);
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
    }

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

            val = (int16_t)swap_int16(ptc_signal.node_comp_caps);
            cc_value = (val & 0x0F)*0.00675 + ((val >> 4) & 0x0F)*0.0675 + ((val >> 8) & 0x0F)*0.675 + ((val >> 12) & 0x3) * 6.75;
            ref_value = (int16_t)swap_int16(ptc_ref.channel_reference);
            signal_value = (int16_t)swap_int16(ptc_signal.node_acq_signals);
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
