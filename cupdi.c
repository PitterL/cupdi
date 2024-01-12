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
#include <ext/ext.h>
#include "cupdi.h"

/*
    Release Note:
    v1.19
        <a> 20230912: support ipx format hex file load, covert and program, but haven't tested the avrda programming yet
            1. the file name will be with .ipe added when using `--ipe --pack-rebuild` to convert normal hex to ipe hex(magic offset placement)
            2. the attiny flash default change from 0x8000 to 0x0 to align with ipe format.
            3. the programming function is not fully tested
        <b> 20230912: Patch for 3rd programmer compatible
            1. Using mapped address to generate sid when flag is Segment Address, while un-mapped address for Linear Address
            2. don't merge the data in same segment if the data offset address is not successive
        <d> 1. Using Mapped address as primary address in program and packing
            2. add lockbits in extend memory
            3. in auto read/write, truck the size if larger than blocksize
            4. in linear address, real address will be added with flash start address
        <f> 1. Added num of breaks issued option at initialization, defaut = 2
            2. Fixed an issue loop around when issue reset/disable failed
            3. Fixed a code issue of support 'uoff, eoff' in command storage(not tested)
            4. Fixed a issue of support `C0` type configure storage, which don't recogize the header and tail
            5. Added of function of link_dump()
        <g> 1. split -i and --info command, for -i, the chip won't enter into programming mode
            2. in updi disable, we will re-assert break signal and STATUB read to avoid some error to block UPDI communication
    CUPDI Software version
*/
#define SOFTWARE_VERSION "1.19g"

/* The firmware Version control file relatve directory to Hex file */
#define VAR_FILE_RELATIVE_POS_0 "qtouch\\pack.h"
#define VAR_FILE_RELATIVE_POS_1 "mpt\\board.h"
#define VAR_FILE_RELATIVE_POS_2 "mpt2\\board.h"
#define VAR_FILE_RELATIVE_POS_3 "mpt3\\board.h"
#define VAR_FILE_RELATIVE_POS_MPLAB "..\\..\\mpt\\board.h"

#define BOARD_FILES                     \
    {                                   \
        VAR_FILE_RELATIVE_POS_0,        \
            VAR_FILE_RELATIVE_POS_1,    \
            VAR_FILE_RELATIVE_POS_2,    \
            VAR_FILE_RELATIVE_POS_3,    \
            VAR_FILE_RELATIVE_POS_MPLAB \
    }

#define VCS_IPE_HEX_FILE_EXTENSION_NAME "ipe.ihex"
#define VCS_STD_HEX_FILE_EXTENSION_NAME "std.ihex"
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
enum
{
    FLAG_PROG_MODE,
    FLAG_UNLOCK,
    FLAG_ERASE,
    FLAG_PAGE_ERASE,
    FLAG_PROG,
    FLAG_UPDATE,
    FLAG_CHECK,
    FLAG_COMPARE,
    FLAG_VERIFY,
    FLAG_SAVE,
    FLAG_DUMP,
    FLAG_INFO
};
enum
{
    PACK_BUILD,
    PACK_BUILD_SELFTEST,
    PACK_BUILD_CFG_VER,
    PACK_SHOW,
    PACK_REBUILD
};

/* Global varibles */

/* declaration */
int get_storage_type(B_BLOCK_TYPE btype);
int get_storage_offset(B_BLOCK_TYPE btype);

int main(int argc, const char *argv[])
{
    char *dev_name = NULL;
    char *comport = NULL;
    int baudrate = 115200;
    int guard = 0;
	int breaks = 2;
    char *file = NULL;
    char *read = NULL;
    char *write = NULL;
    char *erase = NULL;
    char *dbgview = NULL;
    char *selftest = NULL;
    char *storage = NULL;
    int flag = 0;
    bool show_info = false;
    bool unlock = false;
    int verbose = 1;
    bool reset = false;
    bool halt = false;
    bool disable = false;
    bool test = false;
    bool version = false;
    bool ipe_format = false;
    int pack = 0;

    const device_info_t *dev = NULL;
    void *nvm_ptr = NULL;
    int result;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Basic options"),
        OPT_STRING('d', "device", &dev_name, "Target device"),
        OPT_STRING('c', "comport", &comport, "Com port to use (Windows: COMx | *nix: /dev/ttyX)"),
        OPT_INTEGER('b', "baudrate", &baudrate, "Baud rate, default=115200"),
        OPT_INTEGER('g', "guard", &guard, "Guard time, default 16 cycles"),
        OPT_INTEGER('-', "break", &breaks, "Break sent at initialize, default 2 break"),
        OPT_STRING('f', "file", &file, "Intel HEX file to flash"),
        OPT_BIT('p', "", &flag, "Perform a chip enter program mode", NULL, (1 << FLAG_PROG_MODE), 0),
        OPT_BIT('u', "unlock", &flag, "Perform a chip unlock (implied with --unlock)", NULL, (1 << FLAG_UNLOCK), 0),
        OPT_BIT('e', "erase", &flag, "Perform a chip erase (implied with --flash)", NULL, (1 << FLAG_ERASE), 0),
        OPT_STRING('-', "pageerase", &erase, "Perform a page erase [Addr]:[n0]", NULL, (1 << FLAG_PAGE_ERASE), 0),
        OPT_BIT('-', "program", &flag, "Program Intel HEX file to flash", NULL, (1 << FLAG_PROG), 0),
        OPT_BIT('-', "update", &flag, "Program infoblock update to eeprom(need map file)", NULL, (1 << FLAG_UPDATE), 0),
        OPT_BIT('-', "check", &flag, "Check flash content with infoblock CRC", NULL, (1 << FLAG_CHECK), 0),
        OPT_BIT('-', "compare", &flag, "Compare vcs HEX file with infoblock and fuses content", NULL, (1 << FLAG_COMPARE), 0),
        OPT_BIT('-', "verify", &flag, "Check infoblock infomation (and compare hex if offered)", NULL, (1 << FLAG_VERIFY), 0),
        OPT_BIT('-', "info", &flag, "Get Infoblock infomation of firmware", NULL, (1 << FLAG_INFO), 0),
        OPT_BOOLEAN('i', "", &show_info, "Get Infoblock infomation of firmware"),
        OPT_BIT('-', "save", &flag, "Save flash to a VCS HEX file", NULL, (1 << FLAG_SAVE), 0),
        OPT_BIT('-', "dump", &flag, "Dump flash to a Intel HEX file", NULL, (1 << FLAG_DUMP), 0),
        OPT_STRING('r', "read", &read, "Direct read from any memory [addr0:size]|[addre1:size]...  Note Address is Hex type dig, and Number is auto type dig"),
        OPT_STRING('w', "write", &write, "Direct write to any memory [addr0]:[data0];[data1];[data1]|[addr1]... Note address and data format with Hex type dig"),
        OPT_STRING('-', "dbgview", &dbgview, "get ref/delta/cc value operation ds=[ptc_qtlib_node_stat1]|dr=[qtlib_key_data_set1]|loop=[n]|st=[n]|keys=[n]", NULL, (intptr_t) ""),
        OPT_STRING('-', "selftest", &selftest, "check ref/cc value operation in test range sighi=[n]|siglo=[n]|range=[n]|slot=[n]|st=[n]|keys=[n]", NULL, (intptr_t) ""),
        OPT_INTEGER('v', "verbose", &verbose, "Set verbose mode (SILENCE|UPDI|NVM|APP|LINK|PHY|SER): [0~6], default 0, suggest 2 for status information"),
        OPT_STRING('-', "storage", &storage, "Use the storage to store infoblock infoblock=[0: userrow, 1: eeprom]|uoff=0x[n]|eoff=0x[n]|ipe=1 default storage=0|uoff=0|eoff=0", NULL, (intptr_t) ""),
        OPT_BOOLEAN('-', "reset", &reset, "UPDI reset device"),
        OPT_BOOLEAN('-', "halt", &halt, "UPDI halt device"),
        OPT_BOOLEAN('-', "disable", &disable, "UPDI disable"),
        OPT_BOOLEAN('t', "test", &test, "Test UPDI device"),
        OPT_BOOLEAN('-', "version", &version, "Show version"),
        OPT_BOOLEAN('-', "ipe", &ipe_format, "Using MPLAB IPE format HEX (of Magic offset)"),
        OPT_BIT('-', "pack-build", &pack, "Pack info block to Intel HEX file, (macro FIRMWARE_VERSION at 'touch.h')save with extension'.ihex'", NULL, (1 << PACK_BUILD), 0),
        OPT_BIT('-', "pack-build-selftest", &pack, "Pack info block to ihex file with cfg selftest data", NULL, (1 << PACK_BUILD_SELFTEST), 0),
        OPT_BIT('-', "pack-build-cfg", &pack, "Pack info block to ihex file with pending cfg head only", NULL, (1 << PACK_BUILD_CFG_VER), 0),
        OPT_BIT('-', "pack-info", &pack, "Show packed file(ihex) info", NULL, (1 << PACK_SHOW), 0),
        OPT_BIT('-', "pack-rebuild", &pack, "Save packed file", NULL, (1 << PACK_REBUILD), 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nA brief description of what the program does and how it works.", "\nAdditional description of the program after the description of the arguments.");

    if (argc <= 1)
    {
        DBG_INFO(UPDI_DEBUG, "CUPDI version: %s", SOFTWARE_VERSION);
        argparse_usage(&argparse);
        return 0;
    }

    argc = argparse_parse(&argparse, argc, argv);
    if (argc != 0)
    {
        DBG_INFO(DEFAULT_DEBUG, "argc: %d\n", argc);
        for (int i = 0; i < argc; i++)
        {
            DBG_INFO(DEFAULT_DEBUG, "argv[%d]: %s\n", i, argv[i]);
        }
    }
    //<Part 0> The command below requires nothing
    // set parameter
    set_verbose_level(verbose);

    if (version)
    {
        DBG_INFO(UPDI_DEBUG, "CUPDI version: %s", SOFTWARE_VERSION);
        return 0;
    }

    if (storage)
    {
        updi_storage(nvm_ptr, storage);
    }

    //<Part 1> The command below requires device name
    if (!dev_name)
    {
        DBG_INFO(UPDI_DEBUG, "No DEV Name appointed");
        return ERROR_PTR;
    }

    dev = get_chip_info(dev_name);
    if (!dev)
    {
        DBG_INFO(UPDI_DEBUG, "Device %s not support", dev_name);
        return -2;
    }

    // covert hex file to ihex file
    if (pack)
    {
        if (file)
        {
            if (TEST_BIT(pack, PACK_BUILD) ||
                TEST_BIT(pack, PACK_BUILD_SELFTEST) ||
                TEST_BIT(pack, PACK_BUILD_CFG_VER) ||
                TEST_BIT(pack, PACK_REBUILD))
            {
                result = dev_pack_to_vcs_hex_file(dev, file, pack, ipe_format);
                if (result)
                {
                    DBG_INFO(UPDI_DEBUG, "Device pack hex file '%s' failed %d", file, result);
                    return -3;
                }
            }

            if (TEST_BIT(pack, PACK_SHOW))
            {
                result = dev_vcs_hex_file_show_info(dev, file);
                if (result)
                {
                    DBG_INFO(OTHER_ERROR, "Device show ihex file '%s' failed %d", file, result);
                    return -4;
                }
            }
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "Device pack file not appointed");
            return -5;
        }

        return 0;
    }

    //<Part 2> The command below requires common port
    if (!comport)
    {
        DBG_INFO(UPDI_DEBUG, "No COM PORT appointed");
        return ERROR_PTR;
    }

    nvm_ptr = updi_nvm_init(comport, baudrate, guard, breaks, (void *)dev);
    if (!nvm_ptr)
    {
        DBG_INFO(UPDI_DEBUG, "Nvm initialize failed");
        result = -3;
        goto out;
    }

    // check device id
    result = nvm_get_device_info(nvm_ptr);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_device_info failed");
        result = -4;
        goto out;
    }

    // unlock
    if (flag /* || read || write || erase || dbgview || selftest*/)
    {
        result = nvm_enter_progmode(nvm_ptr);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "Device enter progmode failed(%d)", result);
            if (TEST_BIT(flag, FLAG_UNLOCK))
            {
                DBG_INFO(UPDI_DEBUG, "Device is locked(%d). Performing unlock with chip erase.", result);
                result = nvm_unlock_device(nvm_ptr);
                if (result)
                {
                    DBG_INFO(UPDI_DEBUG, "NVM unlock device failed %d", result);
                    result = -5;
                    goto out;
                }
            }

            result = nvm_get_device_info(nvm_ptr);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "nvm_get_device_info in program failed %d", result);
                result = -6;
                goto out;
            }
        }
    }

    // erase
    if (TEST_BIT(flag, FLAG_ERASE))
    {
        result = updi_erase(nvm_ptr);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "NVM chip erase failed %d", result);
            result = -7;
            goto out;
        }
    }
    else if (TEST_BIT(flag, FLAG_PAGE_ERASE))
    {
        result = updi_page_erase(nvm_ptr, erase);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "NVM chip erase failed %d", result);
            result = -8;
            goto out;
        }
    }

    // program and dump
    if (file)
    {
        if (TEST_BIT(flag, FLAG_COMPARE) || TEST_BIT(flag, FLAG_VERIFY) || TEST_BIT(flag, FLAG_UPDATE))
        {
            result = updi_compare(nvm_ptr, file, dev);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "updi_compare failed %d", result);
                result = -9;
                // goto out;
            }
            else
            {
                if (TEST_BIT(flag, FLAG_UPDATE) || TEST_BIT(flag, FLAG_VERIFY))
                {
                    result = updi_check(nvm_ptr);
                    if (result)
                    {
                        DBG_INFO(UPDI_DEBUG, "updi_check failed %d", result);
                        result = -10;
                        // goto out;
                    }
                }
            }
        }

        if (TEST_BIT(flag, FLAG_PROG) || (result && TEST_BIT(flag, FLAG_UPDATE)))
        {
            result = updi_program(nvm_ptr, file, dev, TEST_BIT(flag, FLAG_VERIFY));
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "updi_program failed %d", result);
                result = -9;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_SAVE))
        {
            result = updi_save(nvm_ptr, file, dev, ipe_format);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "NVM save failed %d", result);
                result = -11;
                goto out;
            }
        }

        if (TEST_BIT(flag, FLAG_DUMP))
        {
            result = updi_dump(nvm_ptr, file, dev, ipe_format);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "NVM dump failed %d", result);
                result = -11;
                goto out;
            }
        }
    }

    // show info block
    if (TEST_BIT(flag, FLAG_INFO) || show_info)
    {
        result = updi_show_ext_info(nvm_ptr);
        if (result)
        {
            DBG_INFO(OTHER_ERROR, "updi_show_ext_info failed %d", result);
            // result = -11;
            // goto out;
        }

        result = updi_show_fuse(nvm_ptr);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "updi_show_fuse failed %d", result);
            // result = -11;
            // goto out;
        }
    }

    // Check
    if (TEST_BIT(flag, FLAG_CHECK))
    {
        result = updi_check(nvm_ptr);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "updi_check failed %d", result);
            result = -11;
            goto out;
        }
    }

    // write
    if (write)
    {
        result = updi_write(nvm_ptr, write, TEST_BIT(flag, FLAG_CHECK));
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "Write failed %d", result);
            result = -13;
            goto out;
        }
    }

    // read
    if (read)
    {
        result = updi_read(nvm_ptr, read);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "Read failed %d", result);
            result = -12;
            goto out;
        }
    }

    // wait programming completion
    result = nvm_wait(nvm_ptr);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "NVM wait failed %d", result);
        result = -14;
        goto out;
    }

    // Selftest
    if (selftest)
    {
        result = updi_selftest(nvm_ptr, selftest);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "selftest failed %d", result);
            result = -16;
            goto out;
        }
    }

    // debug show
    if (dbgview)
    {
        result = updi_debugview(nvm_ptr, dbgview);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "Debugview failed %d", result);
            result = -17;
            goto out;
        }
    }

out:
    if (nvm_in_progmode(nvm_ptr))
    {
        nvm_leave_progmode(nvm_ptr, !halt);
    }
    else
    {
        // reset
        if (reset)
        {
            result = nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET, !halt);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "NVM reset failed %d", result);
                result = -15;
            }
        }

        if (disable)
        {
            result = nvm_disable(nvm_ptr);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "nvm_disable failed %d", result);
                result = -18;
            }
        }
    }

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
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "NVM chip erase failed %d", result);
        return -2;
    }

    return 0;
}

ihex_segment_t _block_segment_id(nvm_info_t *block, int flag)
{
    ihex_segment_t sid = 0;
    unsigned int address;

    /* Using mapped address to generate sid when flag is Segment Address,
        this is for the compability of the 3rd part flash tools of using extend segment for location.
        But for IPE tool, the Linear address using unmapped address with magicoff placement.
        So here:
            <1> Segment flag: mapped address
            <2> Linear flag: unmapped address
    */

    if (flag & SEG_EX_SEGMENT_ADDRESS)
    {
        address = block->nvm_mapped_start ? block->nvm_mapped_start : block->nvm_start;
        sid = ADDR_TO_EX_SEGMENT_ID(address);
    }
    else if (flag & SEG_EX_LINEAR_ADDRESS)
    {
        if (block->nvm_magicoff)
        {
            sid = block->nvm_magicoff;
        }
        else
        {
            address = block->nvm_start;
            sid = ADDR_TO_EX_LINEAR_ID(address);
        }
    }

    return sid;
}

unsigned int _block_segment_offset(nvm_info_t *block, int flag)
{
    unsigned int offset;

    if (flag & SEG_EX_SEGMENT_ADDRESS)
    {
        offset = ADDR_OFFSET_EX_SEGMENT(block->nvm_start);
    }
    else if (flag & SEG_EX_LINEAR_ADDRESS)
    {
        // Linear address is difficult to express the offset, set ZERO temperarily
        if (block->nvm_magicoff)
        {
            offset = 0;
        }
        else
        {
            offset = ADDR_OFFSET_EX_LINEAR(block->nvm_start);
        }
    }

    return offset;
}

unsigned int _segment_id_to_address(ihex_segment_t sid, int flag)
{
    unsigned int address = 0;

    if (flag & SEG_EX_SEGMENT_ADDRESS)
    {
        address = EX_SEGMENT_ID_TO_ADDR(sid);
    }
    else if (flag & SEG_EX_LINEAR_ADDRESS)
    {
        //FIXME, not considerred the Magicoff type Linear address

        address = EX_LINEAR_ID_TO_ADDR(sid);
    }

    return address;
}

unsigned int _block_start_address(nvm_info_t *block, int flag)
{
    unsigned int address;

    if (flag & FLAG_ADDR_REAL)
    {
        address = block->nvm_start;
    }
    else
    {
        if (block->nvm_mapped_start)
        {
            address = block->nvm_mapped_start;
        }
        else
        {
            address = block->nvm_start;
        }
    }

    return address;
}

int align_segment(segment_buffer_t *seg, const void *param, ihex_seg_type_t flag)
{
    const device_info_t *dev = (const device_info_t *)param;
    nvm_info_t iblock;
    ihex_segment_t sid;
    ihex_segment_t base;
    unsigned int offset;
    int i, result = 0;

    if (!seg->flag)
    {
        // <1> No segment flag indicated, it's Flash area
        result = dev_get_nvm_info_ext(dev, NVM_FLASH, &iblock, NULL);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info type `NVM_FLASH` failed %d, set sid Zero", result);
			sid = 0;
        }
        else
        {
			sid = _block_segment_id(&iblock, flag);
        }
        seg->sid = sid;
        seg->flag = flag;
    }
    else if (seg->flag == SEG_EX_SEGMENT_ADDRESS)
    {
        // <2> Segment -> Segment or linear(magicoff) address
        base = EX_SEGMENT_ID_TO_ADDR(seg->sid);

        for (i = 0; i < NUM_NVM_EX_TYPES; i++)
        {
            result = dev_get_nvm_info_ext(dev, i, &iblock, NULL);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info type %d failed %d", i, result);
            }
            else
            {
                if (base == iblock.nvm_start ||
                    (iblock.nvm_mapped_start && base == iblock.nvm_mapped_start))
                {
                    seg->sid = _block_segment_id(&iblock, flag);
                    seg->flag = flag;

                    break;
                }
            }
        }

        if (i == NUM_NVM_EX_TYPES)
        {
            DBG_INFO(UPDI_ERROR, "Segment address 0x%x to Linear not found", base);
            result = -2;
        }
    }
    else if (seg->flag == SEG_EX_LINEAR_ADDRESS)
    {
        if (flag == SEG_EX_SEGMENT_ADDRESS)
        {
            // <3> Linear to Segment address
            if (LINEAR_ID_MAGIC(seg->sid))
            {
                // Magicoff placement
                for (i = 0; i < NUM_NVM_EX_TYPES; i++)
                {
                    result = dev_get_nvm_info_ext(dev, i, &iblock, NULL);
                    if (result)
                    {
                        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info type %d failed %d", i, result);
                    }
                    else
                    {
                        if (iblock.nvm_magicoff && iblock.nvm_magicoff == seg->sid)
                        {
                            seg->sid = _block_segment_id(&iblock, flag);
                            seg->flag = flag;

                            // Adjust the offset when cover to segment address, some packing is not segment aligned(Lockbits)
                            offset = _block_segment_offset(&iblock, flag);
                            seg->addr_from += offset;
                            seg->addr_to += offset;
                            break;
                        }
                    }
                }

                if (i == NUM_NVM_EX_TYPES)
                {
                    DBG_INFO(UPDI_ERROR, "Linear address(magic) 0x%x to Segment not found", seg->sid);
                    result = -3;
                }
            }
            else
            {
                // Actual Linear address, directly convert to Segment address
                // Flash content only since linear address only record upper 16 bit
                result = dev_get_nvm_info_ext(dev, NVM_FLASH, &iblock, NULL);
                if (result)
                {
                    DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info type `NVM_FLASH` failed %d, set sid Zero", result);
                    sid = 0;
                }
                else
                {
                    sid = _block_segment_id(&iblock, flag);
                }

                seg->sid = (seg->sid << (EX_LINEAR_ADDRESS_SHIFT - EX_SEGMENT_ADDRESS_SHIFT)) + sid;
                seg->flag = flag;
            }
        }
        else if (flag == SEG_EX_LINEAR_ADDRESS)
        {
            // Linear to Linear address
            // Do nothing...
        }
    }

    return result;
}

int dev_hex_load(const device_info_t *dev, const char *file, hex_data_t *dhex)
{
    int result;

    result = load_segments_from_file(file, dhex);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "load_hex_file '%s' failed %d", file, result);
        return -1;
    }

    result = walk_segments_by_id(dhex, SEG_EX_SEGMENT_ADDRESS, align_segment, dev);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "walk_segments_by_id '%s' failed %d", file, result);
        return -2;
    }

    return 0;
}

int dev_hex_save(const device_info_t *dev, const char *file, ihex_seg_type_t flag, hex_data_t *dhex)
{
    int result;

    result = walk_segments_by_id(dhex, flag, align_segment, dev);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "walk_segments_by_id '%s' failed %d", file, result);
        return -2;
    }

    result = save_hex_info_to_file(file, dhex);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "save_hex_file '%s' failed %d", file, result);
        return -3;
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
int dev_hex_show(const device_info_t *dev, int type, hex_data_t *dhex)
{
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    char *name = "";
    int i, result = -3;

    result = dev_get_nvm_info_ext(dev, type, &iblock, &name);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info type %d failed %d", type, result);
        return -2;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    for (i = 0; i < ARRAY_SIZE(dhex->segments); i++)
    {
        seg = &dhex->segments[i];
        if (seg->sid == sid)
        {
            DBG(UPDI_DEBUG, "NVM%d-%s: ", seg->data, seg->len, "%02X ", type, name);
            result = 0;
        }
    }

    return result;
}

/*
    Get infoblock from hex data structure
    @block: nvm info
    @offset: data offset
    @dhex: hex data structure
    @output: ext data container output buffer
    @btype: Ext info block type: BLOCK_INFO|BLOCK_CFG
    return 0 if sucessful, else failed
*/
int _get_ext_info_from_hex_info(nvm_info_t *block, int offset, hex_data_t *dhex, void *output, B_BLOCK_TYPE btype)
{
    ext_header_t *head;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    int start, size, off;
    int result;

    sid = _block_segment_id(block, SEG_EX_SEGMENT_ADDRESS);
    seg = get_segment_by_id(dhex, sid);
    if (seg)
    {
        start = seg->addr_from;
        if (start < offset)
            start += offset;

        off = start - offset;
        size = seg->addr_to - start;
        if (size >= sizeof(*head))
        {
            head = (ext_header_t *)(seg->data + off);
            result = ext_info_set_data_ptr(output, (char *)head, size, BIT_MASK(MEM_SHARE) | BIT_MASK(btype));
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "Info Block matching failed, failed %d", result);
                DBG(UPDI_DEBUG, "Info Block:", (unsigned char *)head, size, "%02X ");
                result = -3;
            }
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "Info Block size %d is too small in seg", size);
            result = -5;
        }
    }
    else
    {
        DBG_INFO(OTHER_ERROR, "Block segmentid 0x%x not found", sid);
        result = -6;
    }

    return result;
}

/*
    Get infoblock from hex data structure
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @dhex: hex data structure
    @contnr: ext data output buffer
    @flag: Ext info block type: BLOCK_INFO|BLOCK_CFG
    return 0 if sucessful, else failed
*/
int get_ext_data_from_hex_nvm(void *nvm_ptr, hex_data_t *dhex, void *contnr, B_BLOCK_TYPE btype)
{
    nvm_info_t iblock;
    NVM_TYPE_T nvm_type;
    int offset, result;

    nvm_type = get_storage_type(btype);
    offset = get_storage_offset(btype);

    result = nvm_get_block_info(nvm_ptr, nvm_type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }

    return _get_ext_info_from_hex_info(&iblock, offset, dhex, contnr, btype);
}

/*
    Get infoblock from hex data structure
    @dev: device info structure, get by get_chip_info()
    @dhex: hex data structure
    @output: ext data output buffer
    @flag: Ext info block type: BLOCK_INFO|BLOCK_CFG
    return 0 if sucessful, else failed
*/
int get_ext_data_from_hex_dev(const device_info_t *dev, hex_data_t *dhex, void *contnr, B_BLOCK_TYPE btype)
{
    nvm_info_t iblock;
    NVM_TYPE_T nvm_type;
    int offset, result;

    nvm_type = get_storage_type(btype);
    offset = get_storage_offset(btype);

    result = dev_get_nvm_info(dev, nvm_type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        return -3;
    }

    result = _get_ext_info_from_hex_info(&iblock, offset, dhex, contnr, btype);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "_get_ext_info_from_hex_info failed %d", result);
        return -4;
    }

    return 0;
}

/*
    Get ext information from storage
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @contnr: output buffer for infoblock
    @block_type: Ext info block type: BLOCK_INFO|BLOCK_CFG
    @return 0 successful, other value failed
*/
int get_ext_info_from_storage(void *nvm_ptr, void *contnr, B_BLOCK_TYPE btype)
{
    nvm_info_t iblock;
    int nvm_type;
    unsigned start, offset;

    config_header_t header;
    char *buf = NULL;
    int len;
    int result;

    nvm_type = get_storage_type(btype);
    offset = get_storage_offset(btype);

    result = nvm_get_block_info(nvm_ptr, nvm_type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_ERROR, "nvm_get_block_info(%d) failed %d", nvm_type, result);
        return -3;
    }

    start = _block_start_address(&iblock, FLAG_ADDR_MAPPED);
    result = nvm_read_auto(nvm_ptr, start + offset, (u8 *)&header, sizeof(header));
    if (result)
    {
        DBG_INFO(UPDI_ERROR, "nvm_read_auto (0x%x off %d)failed %d", start, offset, result);
        return -4;
    }

    if (!VALID_HEADER(header.data))
    {
        DBG_INFO(UPDI_ERROR, "Ext header(%hhd %hhd) is invalid in NVM-%d",
                 _VER(header.data, 0), _VER(header.data, 1),
                 nvm_type);
        return -5;
    }

    len = header.data.size;
    if (len <= 0 || (u32)len > iblock.nvm_size || len > ext_info_max_size(btype))
    {
        DBG_INFO(UPDI_ERROR, "Header '%04x', size = %d is incorrect", header.data.version.value, len);
        return -6;
    }

    buf = malloc(len);
    if (!buf)
    {
        DBG_INFO(UPDI_ERROR, "malloc infoblock size %d failed", len);
        return -7;
    }

    result = nvm_read_auto(nvm_ptr, start + offset, (u8 *)buf, len);
    if (result)
    {
        DBG_INFO(UPDI_ERROR, "nvm_read_auto(0x%x off %d) %d bytes failed %d", start, offset, len, result);
        return -8;
    }

    result = ext_info_set_data_ptr(contnr, buf, len, BIT_MASK(MEM_SHARE_RELEASE) | BIT_MASK(btype));
    if (result)
    {
        DBG_INFO(UPDI_ERROR, "Ext Block(nvm-%d) set data ptr failed %d", nvm_type, result);
        DBG(UPDI_ERROR, "Ext Block:", (unsigned char *)buf, len, "%02X ");
        result = -9;
    }

    return result;
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
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
        return -2;
    }

    if ((u32)size > iblock.nvm_size)
    {
        DBG_INFO(UPDI_DEBUG, "size %d failed %d", size);
        return -3;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    seg = set_segment_data_by_id_addr(dhex, sid, off, size, buf, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
    if (!seg)
    {
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

    result = search_defined_value_int_from_file(file, "PROJECT_CODE", &version, HEX_FORMAT);
    if (result != 1)
    {
        DBG_INFO(OTHER_ERROR, "search_defined_value_int_from_file fw version failed %d", result);
        return -2;
    }

    *ver = _swap_int32(version);

    return 0;
}

typedef struct
{
    unsigned int *buf;
    const char *names[2];
    int count;
} varible_search_t;

/*
Get Firmware signal/reference structure address from file
@file: file name to search
@ver: version output buffer
return 0 if sucessful, else failed
*/
int load_varible_address_from_file(const char *file, varible_address_t *output)
{
    unsigned int ds, dr, node, acq;

    varible_search_t search_list[] = {
        {&ds, {"ptc_qtlib_node_stat1", "qtm_node_stat1"}, 2},
        {&dr, {"qtlib_key_data_set1", "qtm_key_data_set1"}, 2},
        {&node, {"ptc_seq_node_cfg1", "qtm_seq_node_cfg1"}, 2},
        {&acq, {"ptc_qtlib_acq_gen1", "qtm_acq_gen1"}, 2},
    };

    int i, j, result;

    for (i = 0; i < ARRAY_SIZE(search_list); i++)
    {
        for (j = 0; j < search_list[i].count; j++)
        {
            result = search_map_value_int_from_file(file, search_list[i].names[j], search_list[i].buf);
            if (result != 1)
            {
                DBG_INFO(OTHER_ERROR, "search_map_value_int_from_file %d-%d skipped %s", i, j, search_list[i].names[j]);
            }
            else
            {
                DBG_INFO(OTHER_ERROR, "search_map_value_int_from_file ds got %s", search_list[i].names[j]);
                break;
            }
        }

        if (result != 1)
        {
            DBG_INFO(UPDI_ERROR, "search_map_value_int_from_file item[%d] failed", i);
            return -2;
        }
    }

    if (output)
    {
        output->dsdr.data.ds = (uint16_t)ds;
        output->dsdr.data.dr = (uint16_t)dr;
        output->acqnd.data.acq = (uint16_t)acq;
        output->acqnd.data.node = (uint16_t)node;
    }

    return 0;
}

/*
    UPDI Show Ext Info
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int updi_show_ext_info(void *nvm_ptr)
{
    information_container_t info_container;
    config_container_t cfg_container;
    information_header_t header;
    int result;

    memset(&info_container, 0, sizeof(info_container));
    memset(&cfg_container, 0, sizeof(cfg_container));

    // Info block
    result = get_ext_info_from_storage(nvm_ptr, &info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_ERROR, "get_ext_info_from_storage(info) failed", result);
        return -2;
    }
    ext_show(&info_container);

    header.value = ext_get(&info_container, B_HEAD);
    if (HEADER_MINOR(header.data, INFO_BLOCK_S3_VER_MINOR))
    {
        header.value = ext_get(&info_container, IB_CFG);
        if (VALID_HEADER(header.data))
        {
            result = get_ext_info_from_storage(nvm_ptr, &cfg_container, BLOCK_CFG);
            if (result)
            {
                DBG_INFO(UPDI_ERROR, "get_ext_info_from_storage(cfg) failed", result);
                result = -3;
                goto out;
            }
            ext_show(&cfg_container);
        }
    }
    else
    {
        // low version not support config crc
    }

out:
    ext_destory(&info_container);
    ext_destory(&cfg_container);

    return result;
}

/*
    UPDI Show Fuse
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 successful, other value failed
*/
int updi_show_fuse(void *nvm_ptr)
{
    nvm_info_t iblock;
    char *buf, *name;
    int result, len;

    result = nvm_get_block_info_ext(nvm_ptr, NVM_FUSES, &iblock, &name);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_fuses_info failed %d", result);
        return -2;
    }

    len = 0;
    buf = nvm_get_content(nvm_ptr, NVM_FUSES, &len);
    if (!buf)
    {
        DBG_INFO(UPDI_DEBUG, "get_fuses_content failed");
        return -3;
    }

    DBG_INFO(UPDI_DEBUG, "==========================");
    DBG(UPDI_DEBUG, "NVM%d-%s: ", buf, len, "%02X ", NVM_FUSES, name);
    DBG_INFO(UPDI_DEBUG, "");

    free(buf);

    return 0;
}

/*
    UPDI verify Infoblock information in eeprom with flash content
    @nvm_ptr: NVM object pointer, acquired from updi_nvm_init()
    @return 0 mean pass, other value failed
*/
int updi_check(void *nvm_ptr)
{
    information_container_t info_container;
    config_container_t cfg_container;
    information_header_t header;
    char *buf = NULL;
    int len, crc, info_crc;
    int result;

    memset(&info_container, 0, sizeof(info_container));
    memset(&cfg_container, 0, sizeof(cfg_container));

    // Info block
    result = get_ext_info_from_storage(nvm_ptr, &info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "get_ext_info_from_storage(info) failed", result);
        result = -2;
        goto out;
    }

    len = ext_get(&info_container, IB_FW_SIZE);
    if (len <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "ext_get `IB_FW_SIZE` failed %d", len);
        result = -3;
        goto out;
    }

    // Flash content
    buf = nvm_get_content(nvm_ptr, NVM_FLASH, &len);
    if (!buf)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_content `NVM_FLASH` failed");
        result = -4;
        goto out;
    }

    // DBG(UPDI_DEBUG, "Flash data ", buf, len, "%02x ");

    crc = calc_crc24(buf, len);
    info_crc = ext_get(&info_container, IB_CRC_FW);
    if (info_crc <= 0 || info_crc != crc)
    {
        DBG_INFO(UPDI_DEBUG, "Info Block read fw crc24 mismatch %06x(%06x)", info_crc, crc);
        result = -5;
        goto out;
    }
    free(buf);
    buf = NULL;

    header.value = ext_get(&info_container, B_HEAD);
    if (HEADER_MINOR(header.data, INFO_BLOCK_S3_VER_MINOR))
    {
        // Fuse
        len = 0;
        info_crc = ext_get(&info_container, B_HEAD);
        buf = nvm_get_content(nvm_ptr, NVM_FUSES, &len);
        if (!buf)
        {
            DBG_INFO(UPDI_DEBUG, "nvm_get_content `NVM_FUSES` failed");
            result = -6;
            goto out;
        }

        crc = calc_crc8(buf, len);
        info_crc = (unsigned char)ext_get(&info_container, IB_FUSE_CRC);
        if (info_crc <= 0 || info_crc != crc)
        {
            DBG_INFO(UPDI_DEBUG, "Info Block read fuse crc8 mismatch %02x(%02x)", info_crc, crc);
            result = -7;
            goto out;
        }
        free(buf);
        buf = NULL;

        // Config
        header.value = ext_get(&info_container, IB_CFG);
        if (VALID_HEADER(header.data))
        {
            result = get_ext_info_from_storage(nvm_ptr, &cfg_container, BLOCK_CFG);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "get_ext_info_from_storage(cfg) failed", result);
                result = -8;
                goto out;
            }
        }
    }

    DBG_INFO(UPDI_DEBUG, "Pass");
out:
    if (buf)
        free(buf);

    ext_destory(&info_container);
    ext_destory(&cfg_container);

    return result;
}

/*
    UPDI Program flash
    This flowchart is: load firmware file->erase chip->program firmware
    @nvm_ptr: updi_nvm_init() device handle
    @file: hex/ihex file path
    @dev: device info structure, get by get_chip_info()
    @check: whether readback data for double checking
    @returns 0 - success, other value failed code
*/
int updi_program(void *nvm_ptr, const char *file, const device_info_t *dev, bool check)
{
    hex_data_t dhex_info;
    segment_buffer_t *seg;
    char *vcs_file = NULL;
    int i, result = 0;

    memset(&dhex_info, 0, sizeof(dhex_info));

    // Load hex file
    result = dev_hex_load(dev, file, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "load_hex_file '%s' failed %d", file, result);
        return -2;
    }

    /*
        Default we don't erase the whole chip for partical programming
        result = nvm_chip_erase(nvm_ptr);
        if (result) {
            DBG_INFO(UPDI_DEBUG, "nvm_chip_erase failed %d", result);
            result = -3;
            goto out;
        }
    */

    for (i = 0; i < ARRAY_SIZE(dhex_info.segments); i++)
    {
        seg = &dhex_info.segments[i];
        if (seg->data)
        {
            result = nvm_write_auto(nvm_ptr, _segment_id_to_address(seg->sid, seg->flag) + seg->addr_from, seg->data, seg->len, check);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "nvm_write_auto %d failed %d", i, result);
                result = -4;
                goto out;
            }
        }
    }

    DBG_INFO(UPDI_DEBUG, "Program finished");

out:
    unload_segments(&dhex_info);

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
    char *buf;
    int len;
    int i, result;

    result = nvm_get_block_info(nvm_ptr, NVM_FUSES, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_fuses_info failed %d", result);
        return -2;
    }

    len = 0;
    buf = nvm_get_content(nvm_ptr, NVM_FUSES, &len);
    if (!buf)
    {
        DBG_INFO(UPDI_DEBUG, "get_fuses_content failed");
        return -3;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    for (i = 0; i < ARRAY_SIZE(dhex->segments); i++)
    {
        seg = &dhex->segments[i];
        if (seg->sid == sid)
        {
            if ((int)(seg->addr_from + seg->len) <= len)
            {
                if (memcmp(seg->data, buf + seg->addr_from, seg->len))
                {
                    DBG_INFO(UPDI_DEBUG, "Fuses content mismatch:");
                    DBG(UPDI_DEBUG, "Fuses: ", buf, len, "%02x ");
                    DBG(UPDI_DEBUG, "Seg: ", seg->data, seg->len, "%02x ");
                    result = -6;
                    goto out;
                }
            }
            else
            {
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
    Compare chip infoblock crc whether it's match with hex data
    @nvm_ptr: updi_nvm_init() device handle
    @dhex: hex data structure
    return 0 if CRC matched, else failed
*/
int compare_nvm_crc(void *nvm_ptr, hex_data_t *dhex)
{
    information_container_t file_info_container, nvm_info_container;
    config_container_t file_cfg_contrainer, nvm_cfg_contrainer;
    information_header_t header;
    int nvm_crc, file_crc;
    int result;

    memset(&file_info_container, 0, sizeof(file_info_container));
    memset(&nvm_info_container, 0, sizeof(nvm_info_container));
    memset(&file_cfg_contrainer, 0, sizeof(file_cfg_contrainer));
    memset(&nvm_cfg_contrainer, 0, sizeof(nvm_cfg_contrainer));

    // Get info container from hex file
    result = get_ext_data_from_hex_nvm(nvm_ptr, dhex, &file_info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "get_ext_data_from_hex_nvm(info file) failed %d", result);
        result = -2;
        goto out;
    }

    // Get info container from nvm
    result = get_ext_info_from_storage(nvm_ptr, &nvm_info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "get_ext_info_from_storage(info nvm) failed %d", result);
        result = -3;
        goto out;
    }

    // Infoblock and Firmware CRC (32bit)
    nvm_crc = ext_get(&nvm_info_container, IB_CRC);
    file_crc = ext_get(&file_info_container, IB_CRC);
    if (file_crc == 0 || nvm_crc == -1 || nvm_crc != file_crc)
    {
        DBG_INFO(UPDI_DEBUG, "INFO + FW CRC 0x%x mismatch file CRC 0x%x", nvm_crc, file_crc);
        result = -4;
        goto out;
    }

    header.value = ext_get(&file_info_container, B_HEAD);
    if (HEADER_MINOR(header.data, INFO_BLOCK_S3_VER_MINOR))
    {
        header.value = ext_get(&file_info_container, IB_CFG);
        if (VALID_HEADER(header.data))
        {
            // Get config container from file
            result = get_ext_data_from_hex_nvm(nvm_ptr, dhex, &file_cfg_contrainer, BLOCK_CFG);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "get_ext_data_from_hex_nvm(cfg file) failed", result);
                result = -5;
            }
            else
            {
                // Get config container from nvm
                result = get_ext_info_from_storage(nvm_ptr, &nvm_cfg_contrainer, BLOCK_CFG);
                if (result)
                {
                    DBG_INFO(UPDI_DEBUG, "get_ext_info_from_storage(cfg nvm) failed %d", result);
                    result = -7;
                    goto out;
                }
                else
                {
                    nvm_crc = ext_get(&nvm_cfg_contrainer, CB_CFG_CRC);
                    file_crc = ext_get(&file_cfg_contrainer, CB_CFG_CRC);
                    if (file_crc == 0 || nvm_crc == -1 || nvm_crc != file_crc)
                    {
                        DBG_INFO(UPDI_DEBUG, "Config CRC 0x%x mismatch file CRC 0x%x", nvm_crc, file_crc);
                        result = -8;
                        goto out;
                    }
                }
            }
        }
    }
    else
    {
        result = compare_nvm_fuses(nvm_ptr, dhex);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "fuses mismatch");
            result = -6;
            goto out;
        }
    }

    DBG_INFO(UPDI_DEBUG, "CRC 0x%x matched ", nvm_crc);

out:
    ext_destory(&nvm_info_container);
    ext_destory(&file_info_container);
    ext_destory(&nvm_cfg_contrainer);
    ext_destory(&file_cfg_contrainer);

    return result;
}

/*
    UPDI compare nvm crc and fuses byte with hex file
    @nvm_ptr: updi_nvm_init() device handle
    @file: ihex firmware file
    @dev: device info structure, get by get_chip_info()
    return 0 if match, else not match
*/
int updi_compare(void *nvm_ptr, const char *file, const device_info_t *dev)
{
    hex_data_t dhex_info;
    int result;

    memset(&dhex_info, 0, sizeof(dhex_info));

    // Load hex file
    result = dev_hex_load(dev, file, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "load_hex_file '%s' failed %d", file, result);
        return -2;
    }

    result = compare_nvm_crc(nvm_ptr, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "crc mismatch");
        result = -3;
        goto out;
    }
out:
    unload_segments(&dhex_info);

    return result;
}

/*
    UPDI Save flash content to a ihex file
    @nvm_ptr: updi_nvm_init() device handle
    @file: Hex file path for output
    @dev: device info structure, get by get_chip_info()
    @ipe_format: using mplab ipe format hex file
    @returns 0 - success, other value failed code
*/
int updi_save(void *nvm_ptr, const char *file, const device_info_t *dev, bool ipe_format)
{
    hex_data_t dhex_info;
    information_container_t info_container;
    NVM_TYPE_T nvm_type;
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    unsigned char *buf = NULL;
    int len;
    int crc, ecrc;
    char *save_file = NULL;
    int i, result;
    unsigned start, offset;

    memset(&dhex_info, 0, sizeof(dhex_info));
    memset(&info_container, 0, sizeof(info_container));

    // Get infoblock first
    result = get_ext_info_from_storage(nvm_ptr, &info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "get_ext_info_from_storage failed", result);
        return -2;
    }

    // Flash content
    len = ib_get(&info_container, IB_FW_SIZE);
    if (len <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "ib_get `IB_FW_SIZE` failed");
        result = -3;
        goto out;
    }

    buf = nvm_get_content(nvm_ptr, NVM_FLASH, &len);
    if (!buf)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_get_content `NVM_FLASH` failed");
        result = -3;
        goto out;
    }

    crc = calc_crc24(buf, len);
    ecrc = ib_get(&info_container, IB_CRC_FW);
    if (ecrc == 0 || ecrc == -1 || ecrc != crc)
    {
        DBG_INFO(UPDI_DEBUG, "Info Block read fw crc24 mismatch %06x(%06x), force save flash data with size %d", ecrc, crc, len);
        /*
        result = -4;
        goto out;
        */
    }

    result = save_content_to_segment(nvm_ptr, NVM_FLASH, &dhex_info, 0, buf, len);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "save_flash_content_to_segment failed %d", result);
        result = -5;
        goto out;
    }
    free(buf);
    buf = NULL;

    // Info block content
    len = ib_get(&info_container, B_HEAD_SIZE);
    if (len <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "get eeprom size = %d failed", len);
        result = -6;
        goto out;
    }
    nvm_type = get_storage_type(BLOCK_INFO);
    offset = get_storage_offset(BLOCK_INFO);
    result = save_content_to_segment(nvm_ptr, nvm_type, &dhex_info, offset, (char *)info_container.head, len);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "save_eeprom_content_to_segment failed %d", result);
        result = -7;
        goto out;
    }

    for (i = 0; i < NUM_NVM_TYPES; i++)
    {
        if (i == NVM_FLASH || i == MEM_SRAM || i == nvm_type)
            continue;

        result = nvm_get_block_info(nvm_ptr, i, &iblock);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
            result = -3;
            break;
        }

        buf = malloc(iblock.nvm_size);
        if (!buf)
        {
            DBG_INFO(UPDI_DEBUG, "malloc nvm buf size = %d failed", iblock.nvm_size);
            result = -4;
            break;
        }

        start = _block_start_address(&iblock, FLAG_ADDR_MAPPED);
        result = nvm_read_auto(nvm_ptr, start, buf, iblock.nvm_size);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "nvm_read_auto type %d failed %d", i, result);
            result = -5;
            free(buf);
            break;
        }

        sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
        seg = set_segment_data_by_id_addr(&dhex_info, sid, 0, iblock.nvm_size, buf, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
        if (!seg)
        {
            DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr type %d failed %d", i, result);
            result = -5;
            free(buf);
            break;
        }

        free(buf);
    }
    buf = NULL;

    // save hex content to file
    save_file = trim_name_with_extesion(file, '.', 1, SAVE_FILE_EXTENSION_NAME);
    if (!save_file)
    {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", SAVE_FILE_EXTENSION_NAME, result);
        result = -9;
        goto out;
    }

    result = dev_hex_save(dev, save_file, ipe_format ? SEG_EX_LINEAR_ADDRESS : SEG_EX_SEGMENT_ADDRESS, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_hex_save failed %d", result);
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
    @dev: device info structure, get by get_chip_info()
    @ipe_format: using mplab ipe format hex file
    @returns 0 - success, other value failed code
*/
int updi_dump(void *nvm_ptr, const char *file, const device_info_t *dev, bool ipe_format)
{
    hex_data_t dhex_info;
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    char *buf;
    char *save_file = NULL;

    int i, result = 0;
    unsigned int start;

    memset(&dhex_info, 0, sizeof(dhex_info));
    for (i = 0; i < NUM_NVM_TYPES; i++)
    {
        result = nvm_get_block_info(nvm_ptr, i, &iblock);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "nvm_get_block_info failed %d", result);
            result = -3;
            break;
        }

        buf = malloc(iblock.nvm_size);
        if (!buf)
        {
            DBG_INFO(UPDI_DEBUG, "malloc nvm buf size = %d failed", iblock.nvm_size);
            result = -4;
            break;
        }

        start = _block_start_address(&iblock, FLAG_ADDR_MAPPED);
        result = nvm_read_auto(nvm_ptr, start, buf, iblock.nvm_size);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "nvm_read_auto type %d failed %d", i, result);
            result = -5;
            free(buf);
            break;
        }

        sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
        seg = set_segment_data_by_id_addr(&dhex_info, sid, 0, iblock.nvm_size, buf, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
        if (!seg)
        {
            DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr type %d failed %d", i, result);
            result = -5;
            free(buf);
            break;
        }

        free(buf);
    }

    save_file = trim_name_with_extesion(file, '.', 1, DUMP_FILE_EXTENSION_NAME);
    if (!save_file)
    {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", DUMP_FILE_EXTENSION_NAME, result);
        return -2;
    }

    result = dev_hex_save(dev, save_file, ipe_format ? SEG_EX_LINEAR_ADDRESS : SEG_EX_SEGMENT_ADDRESS, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_hex_save failed %d", result);
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
    Load version segment from vcs file
    @dev: device info structure, get by get_chip_info()
    @file: vcs file name
    @dhex: hex data structure
    @pack: pack type
    return flash segment if sucessful, else NULL
*/
segment_buffer_t *load_version_segment_from_file(const device_info_t *dev, const char *file, hex_data_t *dhex, int pack)
{
    nvm_info_t iblock;
    segment_buffer_t *seg = NULL;
    ihex_segment_t sid;
    NVM_TYPE_T nvm_type;
    int offset, i, size;
    information_container_t info_container;
    information_content_params_t info_params;
    const char *version_files[] = BOARD_FILES;
    char *vcs_file = NULL;
    char *map_file = NULL;
    int check = 0, result;

    memset(&info_container, 0, sizeof(info_container));
    memset(&info_params, 0, sizeof(info_params));

    // Version
    for (i = 0; i < ARRAY_SIZE(version_files); i++)
    {
        vcs_file = trim_name_with_extesion(file, '\\', 2, version_files[i]);
        if (!vcs_file)
        {
            DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed", version_files[i]);
            goto out;
        }

        result = load_version_value_from_file(vcs_file, &info_params.fw_version);
        free(vcs_file);
        if (result == 0)
        {
            DBG_INFO(UPDI_DEBUG, "load_version_value_from_file (%s) successfully", version_files[i]);
            break;
        }
    }

    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "load_version_value_from_file failed %d", result);
        result = -2;
        goto out;
    }

    // Varibles
    map_file = trim_name_with_extesion(file, '.', 1, MAP_FILE_EXTENSION_NAME);
    if (!map_file)
    {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", MAP_FILE_EXTENSION_NAME, result);
        result = -3;
        goto out;
    }

    result = load_varible_address_from_file(map_file, &info_params.var_addr);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "load_varible_address_from_file(failed %d), SKipped", result);
        // result = -4;
        // goto out;
    }

    // Fuse
    result = dev_get_nvm_info(dev, NVM_FUSES, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -5;
        goto out;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    seg = get_segment_by_id(dhex, sid);
    if (!seg)
    {
        DBG_INFO(UPDI_DEBUG, "failed to pack the FUSES data");
        result = -6;
        goto out;
    }
    else
    {
        info_params.fuse.data.crc = calc_crc8(seg->data, seg->len);
        info_params.fuse.data.size = (unsigned char)seg->len;
        seg = NULL;
    }

    // config
    if (TEST_BIT(pack, PACK_BUILD_SELFTEST))
    {
        nvm_type = get_storage_type(BLOCK_CFG);
        result = dev_get_nvm_info(dev, nvm_type, &iblock);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
            result = -7;
            goto out;
        }

        sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
        seg = get_segment_by_id(dhex, sid);
        if (!seg || seg->len < sizeof(config_information_t))
        {
            DBG_INFO(UPDI_DEBUG, "Skip to pack the CONFIG data");
            result = -8;
            goto out;
        }
        else
        {
            info_params.config.value = ((config_information_t *)seg->data)->value;
        }
        seg = NULL;
    }
    else if (TEST_BIT(pack, PACK_BUILD_CFG_VER))
    {
        info_params.config.data.size = 0;
        info_params.config.data.version.value = CONFIG_BLOCK_C0_VERSION;
    }

    // Firmware
    result = dev_get_nvm_info(dev, NVM_FLASH, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -7;
        goto out;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    seg = get_segment_by_id(dhex, sid);
    if (!seg)
    {
        DBG_INFO(UPDI_DEBUG, "get_segment_by_id(%d) failed %d", sid);
        result = -8;
        goto out;
    }
    info_params.fw_crc24 = calc_crc24(seg->data, seg->len);
    info_params.fw_size = seg->len;
    seg = NULL;

    // Build Info block
    result = ext_create_data_block(&info_container, &info_params, sizeof(info_params), BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "ib_create_information_block failed %d", result);
        result = -9;
        goto out;
    }

    size = ext_get(&info_container, B_HEAD_SIZE);
    if (size <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "get head size = %d failed %d", size, result);
        result = -10;
        goto out;
    }

    // Save to hex
    nvm_type = get_storage_type(BLOCK_INFO);
    result = dev_get_nvm_info(dev, nvm_type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info(info) failed %d", result);
        result = -11;
        goto out;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    unload_segment_by_sid(dhex, sid);
    offset = get_storage_offset(BLOCK_INFO);
    seg = set_segment_data_by_id_addr(dhex, sid, offset, size, (char *)info_container.head, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
    if (!seg)
    {
        DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed");
        result = -12;
        goto out;
    }

out:
    ib_destory(&info_container);
    return seg;
}

/*
    Load fuse segment from vcs file
    @dev: device info structure, get by get_chip_info()
    @file: vcs file name
    @dhex: hex data structure
    return 0 means successful, other value failed
*/
int load_fuse_content_from_file(const device_info_t *dev, const char *file, hex_data_t *dhex)
{
    nvm_info_t iblock, iblock_lockbits;
    segment_buffer_t *seg = NULL;
    ihex_segment_t sid, sid_lockbits;
    unsigned char val;
    unsigned int *fuses_config = NULL;
    const unsigned int invalid_value = 0x800;
    const char *version_files[] = BOARD_FILES;
    char *vcs_file = NULL;
    int i, count = 0, result = 0;

    result = dev_get_nvm_info(dev, NVM_FUSES, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info(%d) failed %d", NVM_FUSES, result);
        return -2;
    }

    fuses_config = malloc(iblock.nvm_size * sizeof(*fuses_config));
    if (!fuses_config)
    {
        DBG_INFO(UPDI_DEBUG, "malloc fuses_config failed %d");
        result = -3;
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(version_files); i++)
    {
        vcs_file = trim_name_with_extesion(file, '\\', 2, version_files[i]);
        if (!vcs_file)
        {
            DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", version_files[i], result);
            result = -4;
            goto out;
        }

        result = search_defined_array_int_from_file(vcs_file, "FUSES_CONTENT", fuses_config, iblock.nvm_size, invalid_value, HEX_FORMAT);
        free(vcs_file);
        if (result == 0)
        {
            DBG_INFO(UPDI_DEBUG, "No fuse content defined at '%s'", version_files[i]);
            goto out;
        }
        else if (result < 0 || (u32)result > iblock.nvm_size)
        {
            DBG_INFO(OTHER_ERROR, "search_defined_array_int_from_file failed %d", result);
            result = -5;
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "search_defined_array_int_from_file (%s) successfully", version_files[i]);
            count = result;
            break;
        }
    }

    if (result < 0 || (u32)result > iblock.nvm_size)
    {
        DBG_INFO(OTHER_ERROR, "search_defined_array_int_from_file failed %d", result);
        result = -6;
        goto out;
    }

    if (count == 0)
    {
        goto out;
    }
    // Unload fuse
    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    unload_segment_by_sid(dhex, sid);

    // Unload lockbits
    result = dev_get_nvm_info(dev, NVM_LOCKBITS, &iblock_lockbits);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info(%d) failed %d", NVM_LOCKBITS, result);
        result = -7;
        goto out;
    }
    else
    {
        sid_lockbits = _block_segment_id(&iblock_lockbits, SEG_EX_SEGMENT_ADDRESS);
        unload_segment_by_sid(dhex, sid_lockbits);
    }

    for (i = 0; i < count; i++)
    {
        if (fuses_config[i] < invalid_value)
        {
            val = fuses_config[i] & 0xff;
            seg = set_segment_data_by_id_addr(dhex, sid, i, 1, &val, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
            if (!seg)
            {
                DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed %d val 0x%02x", i, val);
                result = -7;
                goto out;
            }
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "Fuse[%d]: %02x is not supported", i, val);
            result = -8;
            goto out;
        }
    }
out:

    if (fuses_config)
        free(fuses_config);

    return result;
}

int _load_fuse_lockbits_content_from_file(const device_info_t *dev, int type, char* name, const char *file, hex_data_t *dhex)
{
    nvm_info_t iblock, iblock_lockbits;
    segment_buffer_t *seg = NULL;
    ihex_segment_t sid, sid_lockbits;
    unsigned char val;
    unsigned int *fuses_config = NULL;
    const unsigned int invalid_value = 0x800;
    const char *version_files[] = BOARD_FILES;
    char *vcs_file = NULL;
    int i, count = 0, result = 0;

    result = dev_get_nvm_info(dev, type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info(%d) failed %d", type, result);
        return -2;
    }

    fuses_config = malloc(iblock.nvm_size * sizeof(*fuses_config));
    if (!fuses_config)
    {
        DBG_INFO(UPDI_DEBUG, "malloc fuses_config failed %d");
        result = -3;
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(version_files); i++)
    {
        vcs_file = trim_name_with_extesion(file, '\\', 2, version_files[i]);
        if (!vcs_file)
        {
            DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", version_files[i], result);
            result = -4;
            goto out;
        }

        result = search_defined_array_int_from_file(vcs_file, "FUSES_CONTENT", fuses_config, iblock.nvm_size, invalid_value, HEX_FORMAT);
        free(vcs_file);
        if (result == 0)
        {
            DBG_INFO(UPDI_DEBUG, "No fuse content defined at '%s'", version_files[i]);
            goto out;
        }
        else if (result < 0 || (u32)result > iblock.nvm_size)
        {
            DBG_INFO(OTHER_ERROR, "search_defined_array_int_from_file failed %d", result);
            result = -5;
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "search_defined_array_int_from_file (%s) successfully", version_files[i]);
            count = result;
            break;
        }
    }

    if (result < 0 || (u32)result > iblock.nvm_size)
    {
        DBG_INFO(OTHER_ERROR, "search_defined_array_int_from_file failed %d", result);
        result = -6;
        goto out;
    }

    if (count == 0)
    {
        goto out;
    }
    // Unload fuse
    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    unload_segment_by_sid(dhex, sid);

    // Unload lockbits
    result = dev_get_nvm_info(dev, NVM_LOCKBITS, &iblock_lockbits);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info(%d) failed %d", NVM_LOCKBITS, result);
        result = -7;
        goto out;
    }
    else
    {
        sid_lockbits = _block_segment_id(&iblock_lockbits, SEG_EX_SEGMENT_ADDRESS);
        unload_segment_by_sid(dhex, sid_lockbits);
    }

    for (i = 0; i < count; i++)
    {
        if (fuses_config[i] < invalid_value)
        {
            val = fuses_config[i] & 0xff;
            seg = set_segment_data_by_id_addr(dhex, sid, i, 1, &val, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
            if (!seg)
            {
                DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed %d val 0x%02x", i, val);
                result = -7;
                goto out;
            }
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "Fuse[%d]: %02x is not supported", i, val);
            result = -8;
            goto out;
        }
    }
out:

    if (fuses_config)
        free(fuses_config);

    return result;
}

/*
    Load selftest segment from vcs file
    @dev: device info structure, get by get_chip_info()
    @file: vcs file name
    @dhex: hex data structure
    return selftest segment count if success, NULL mean failed
*/
segment_buffer_t *load_selftest_content_from_file(const device_info_t *dev, const char *file, hex_data_t *dhex)
{
    nvm_info_t iblock;
    segment_buffer_t *seg = NULL;
    NVM_TYPE_T nvm_type;
    ihex_segment_t sid;
    config_container_t cfg_container;
    unsigned int *selftest_config = NULL;
    s_elem_t *data = NULL;
    const char *version_files[] = BOARD_FILES;
    char *vcs_file = NULL;
    int i, size, offset, count, len, result = 0;

    nvm_type = get_storage_type(BLOCK_CFG);
    result = dev_get_nvm_info(dev, nvm_type, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        return NULL;
    }

    selftest_config = malloc(iblock.nvm_size * sizeof(*selftest_config));
    if (!selftest_config)
    {
        DBG_INFO(UPDI_DEBUG, "malloc fuses_config failed %d");
        result = -3;
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(version_files); i++)
    {
        vcs_file = trim_name_with_extesion(file, '\\', 2, version_files[i]);
        if (!vcs_file)
        {
            DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", version_files[i], result);
            result = -4;
            goto out;
        }

        result = search_defined_array_int_from_file(vcs_file, "SELFTEST_CONTENT", selftest_config, iblock.nvm_size, 0, DEC_FORMAT);
        free(vcs_file);
        if (result == 0)
        {
            DBG_INFO(UPDI_DEBUG, "No selftest content defined at '%s'", version_files[i]);
            goto out;
        }
        else if (result < 0 || (u32)result > iblock.nvm_size)
        {
            DBG_INFO(OTHER_ERROR, "search_defined_array_int_from_file failed %d", result);
            result = -5;
        }
        else
        {
            DBG_INFO(UPDI_DEBUG, "search_defined_array_int_from_file (%s) successfully", version_files[i]);
            break;
        }
    }

    if (result < 0 || (u32)result > iblock.nvm_size)
    {
        DBG_INFO(OTHER_ERROR, "search_defined_array_int_from_file failed %d", result);
        result = -6;
        goto out;
    }

    // Transfer int to byte
    count = result;
    len = count / NUM_SIGLIM_TYPES * sizeof(config_body_elem_c1_t);
    data = malloc(len);
    if (!data)
    {
        DBG_INFO(UPDI_DEBUG, "malloc selftest data memory failed");
        result = -7;
        goto out;
    }

    for (i = 0; i < count; i++)
    {
        data[i] = (s_elem_t)selftest_config[i];
    }

    // Create configure block
    result = ext_create_data_block(&cfg_container, data, len, BLOCK_CFG);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "ext_create_data_block(cfg) failed %d", result);
        result = -8;
        goto out;
    }

    size = ext_get(&cfg_container, B_HEAD_SIZE);
    if (size <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "get head size = %d failed %d", size, result);
        result = -9;
        goto out;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    unload_segment_by_sid(dhex, sid);
    offset = get_storage_offset(BLOCK_INFO);
    seg = set_segment_data_by_id_addr(dhex, sid, offset, size, (char *)cfg_container.head, HEX_TYPE(HEX_ALLOC_MEMORY, SEG_EX_SEGMENT_ADDRESS));
    if (!seg)
    {
        DBG_INFO(UPDI_DEBUG, "set_segment_data_by_id_addr failed");
        result = -10;
        goto out;
    }

    // successfully

out:
    if (data)
        free(data);

    if (selftest_config)
        free(selftest_config);

    cb_destory(&cfg_container);

    return seg;
}

/*
    Device pack raw Hex content to a vcs Hex file
    @dev: device info structure, get by get_chip_info()
    @file: raw Hex file path for input
    @pack: pack type
    @ipe_format: using mplab ipe format hex file
    @returns 0 - success, other value failed code
*/
int dev_pack_to_vcs_hex_file(const device_info_t *dev, const char *file, int pack, bool ipe_format)
{
    hex_data_t dhex_info;
    segment_buffer_t *seg;
    char *ihex_file = NULL;
    char crc8 = 0;
    int crc24 = 0;
    int result = 0;

    memset(&dhex_info, 0, sizeof(dhex_info));

    // get first flash segment from file
    result = dev_hex_load(dev, file, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_hex_load failed %d", result);
        result = -3;
        goto out;
    }

    if (!TEST_BIT(pack, PACK_REBUILD))
    {
        result = load_fuse_content_from_file(dev, file, &dhex_info);
        if (result)
        {
            DBG_INFO(UPDI_ERROR, "load_fuse_content_from_file(error=%d), skipped", result);
            // result = -4;
            // goto out;
        }

        if (TEST_BIT(pack, PACK_BUILD_SELFTEST))
        {
            seg = load_selftest_content_from_file(dev, file, &dhex_info);
            if (!seg)
            {
                DBG_INFO(UPDI_DEBUG, "Failed load_selftest_content_from_file(error=%d)", result);
                result = -5;
                goto out;
            }
        }

        seg = load_version_segment_from_file(dev, file, &dhex_info, pack);
        if (!seg)
        {
            DBG_INFO(UPDI_ERROR, "load_version_segment_from_file, Skipped");
            // result = -7;
            // goto out;
        }
    }

    ihex_file = trim_name_with_extesion(file, '.', 1,
                                        ipe_format ? VCS_IPE_HEX_FILE_EXTENSION_NAME : TEST_BIT(pack, PACK_REBUILD) ? VCS_STD_HEX_FILE_EXTENSION_NAME
                                                                                                                    : VCS_HEX_FILE_EXTENSION_NAME);
    if (!ihex_file)
    {
        DBG_INFO(UPDI_DEBUG, "trim_name_with_extesion %s failed %d", VCS_HEX_FILE_EXTENSION_NAME, result);
        return -8;
    }

    result = dev_hex_save(dev, ihex_file, ipe_format ? SEG_EX_LINEAR_ADDRESS : SEG_EX_SEGMENT_ADDRESS, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_hex_save failed %d", result);
        result = -9;
        goto out;
    }

    // Show the result
    if (!TEST_BIT(pack, PACK_REBUILD))
    {
        dev_vcs_hex_file_show_info(dev, ihex_file);
    }

    DBG_INFO(UPDI_DEBUG, "\nSaved Hex to \"%s\"", ihex_file);

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
int dev_vcs_hex_file_show_info(const device_info_t *dev, const char *file)
{
    hex_data_t dhex_info;
    information_container_t file_info_container;
    config_container_t file_conf_container;
    information_header_t header, conf_header;
    nvm_info_t iblock;
    segment_buffer_t *seg;
    ihex_segment_t sid;
    int fw_size, fw_crc, crc;
    int result = 0;

    memset(&dhex_info, 0, sizeof(dhex_info));
    memset(&file_info_container, 0, sizeof(file_info_container));
    memset(&file_conf_container, 0, sizeof(file_conf_container));

    // Load hex file
    result = dev_hex_load(dev, file, &dhex_info);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "load_hex_file '%s' failed %d", file, result);
        return -2;
    }

    // Get Info block
    result = get_ext_data_from_hex_dev(dev, &dhex_info, &file_info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(OTHER_ERROR, "get_infoblock_from_hex_info(info) failed %d", result);
        result = -3;
        goto out;
    }

    // Get flash block information
    result = dev_get_nvm_info(dev, NVM_FLASH, &iblock);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -4;
        goto out;
    }

    sid = _block_segment_id(&iblock, SEG_EX_SEGMENT_ADDRESS);
    seg = get_segment_by_id(&dhex_info, sid);
    if (!seg)
    {
        DBG_INFO(UPDI_DEBUG, "dev_get_nvm_info failed %d", result);
        result = -4;
        goto out;
    }

    fw_size = ext_get(&file_info_container, IB_FW_SIZE);
    if (fw_size <= 0 || seg->len < fw_size)
    {
        DBG_INFO(UPDI_DEBUG, "seg size not enough, seg = %d, infoblock size %d", seg->len, fw_size);
        result = -5;
        goto out;
    }

    fw_crc = ext_get(&file_info_container, IB_CRC_FW);
    crc = calc_crc24(seg->data, fw_size);
    if (fw_crc < 0 || crc < 0 || fw_crc != crc)
    {
        DBG_INFO(UPDI_DEBUG, "Info Block read file crc24 mismatch %06x(%06x)", fw_crc, crc);
        result = -6;
        goto out;
    }
    ext_show(&file_info_container);

    // Show cfg area
    header.value = ext_get(&file_info_container, B_HEAD);
    if (HEADER_MINOR(header.data, INFO_BLOCK_S3_VER_MINOR))
    {
        conf_header.value = ext_get(&file_info_container, IB_CFG);
        if (HEADER_MINOR(conf_header.data, CONFIG_BLOCK_C1_VER_MINOR))
        {
            result = get_ext_data_from_hex_dev(dev, &dhex_info, &file_conf_container, BLOCK_CFG);
            if (result)
            {
                DBG_INFO(OTHER_ERROR, "get_infoblock_from_hex_info(cfg) failed %d", result);
                result = -7;
                goto out;
            }
            ext_show(&file_conf_container);
        }
    }

    // Fuse area
    dev_hex_show(dev, NVM_FUSES, &dhex_info);
out:
    ext_destory(&file_info_container);
    ext_destory(&file_conf_container);
    unload_segments(&dhex_info);

    return result;
}

/*
    Page Erase
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and count. Format: [addr];[count]
    @returns 0 - success, other value failed code
*/
int _updi_page_erase(void *nvm_ptr, char *cmd)
{
    char **tk_s, **tk_w; // token section, token words
#define UPDI_PAGE_ERASE_STROKEN_COUNT 128
    int address, count;
    int i, k, result = 0;

    tk_s = str_split(cmd, '|');
    for (k = 0; tk_s && tk_s[k]; k++)
    {
        tk_w = str_split(tk_s[k], ':');
        for (i = 0, address = ERROR_PTR; tk_w && tk_w[i]; i++)
        {
            if (result == 0)
            { // only work when no error occur
                if (i == 0)
                    address = (int)strtol(tk_w[i], NULL, 0);
                else if (i == 1)
                {
                    if (VALID_PTR(address))
                    {
                        count = (int)(strtol(tk_w[i], NULL, 0)); // Max size 255 once
                        if (count > UPDI_PAGE_ERASE_STROKEN_COUNT)
                        {
                            count = UPDI_PAGE_ERASE_STROKEN_COUNT;
                            DBG_INFO(UPDI_DEBUG, "Page erase count %d over max, set to", count, count);
                        }

                        result = nvm_erase_auto(nvm_ptr, address, count);
                        if (result)
                        {
                            DBG_INFO(UPDI_DEBUG, "nvm_page_erase failed %d", result);
                            result = -4;
                        }
                        else
                        {
                            // Successful
                        }
                    }
                }
            }
            free(tk_w[i]);
        }

        if (!tk_w)
        {
            DBG_INFO(UPDI_DEBUG, "Parse page erase str tk_w: %s failed", tk_s[k]);
        }
        else
            free(tk_w);
        free(tk_s[k]);
    }

    if (!tk_s)
    {
        DBG_INFO(UPDI_DEBUG, "Parse page erase str tk_s: %s failed", cmd);
    }
    else
        free(tk_s);

    return result;
}

/*
    UPDI Page erase
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and count. Format: [addr0:page_count]|[addre1:page_count]...
    @returns 0 - success, other value failed code
*/
int updi_page_erase(void *nvm_ptr, char *cmd)
{
    return _updi_page_erase(nvm_ptr, cmd);
}

/*
    Memory Read
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and count. 
        Format: [addr0:size]|[addre1:size]...
            addr is hex type dig
            size is auto type dig
    @returns 0 - success, other value failed code
*/
int _updi_read_mem(void *nvm_ptr, char *cmd, u8 *outbuf, int outlen)
{
    char **tk_s, **tk_w; // token section, token words
#define UPDI_READ_STROKEN_WORDS_LEN 1024
    int address, len, copylen, outlen_left = outlen;
    char *buf;
    int i, k, result = 0;

    tk_s = str_split(cmd, '|');
    for (k = 0; tk_s && tk_s[k]; k++)
    {
        tk_w = str_split(tk_s[k], ':');
        for (i = 0, address = ERROR_PTR; tk_w && tk_w[i]; i++)
        {
            if (result == 0)
            { // only work when no error occur
                if (i == 0)
                {
                    address = (int)strtol(tk_w[i], NULL, 16);
                }
                else if (i == 1)
                {
                    len = (int)(strtol(tk_w[i], NULL, 0)); // Max size 255 once
                    if (len > UPDI_READ_STROKEN_WORDS_LEN)
                    {
                        DBG_INFO(UPDI_DEBUG, "Read memory len %d over max, set to", len, UPDI_READ_STROKEN_WORDS_LEN);
                        len = UPDI_READ_STROKEN_WORDS_LEN;
                    }

                    buf = malloc(len);
                    if (!buf)
                    {
                        DBG_INFO(UPDI_DEBUG, "mallloc memory %d failed", len);
                        result = -3;
                        continue;
                    }

                    result = nvm_read_auto(nvm_ptr, address, buf, len);
                    if (result)
                    {
                        DBG_INFO(UPDI_DEBUG, "nvm_read_auto failed %d", result);
                        result = -4;
                    }
                    else
                    {
                        if (outbuf && outlen_left > 0)
                        {
                            copylen = min(outlen, len);
                            memcpy(outbuf, buf, copylen);
                            outbuf += copylen;
                            outlen_left -= copylen;
                        }
                        else
                        {
                            // Debug output:
                            DBG(DEFAULT_DEBUG, "Read tk[%d]:", buf, len, "%02x ", i);
                        }
                    }

                    free(buf);
                }
            }
            free(tk_w[i]);
        }

        if (!tk_w)
        {
            DBG_INFO(UPDI_DEBUG, "Parse read str tk_w: %s failed", tk_s[k]);
        }
        else
            free(tk_w);
        free(tk_s[k]);
    }

    if (!tk_s)
    {
        DBG_INFO(UPDI_DEBUG, "Parse read str tk_s: %s failed", cmd);
    }
    else
        free(tk_s);

    if (result)
        return result;

    return outlen - outlen_left;
}

/*
    UPDI Memory Read
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and count. 
        Format: [addr0:size]|[addre1:size]...
            addr is hex type dig
            size is auto type dig
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
    @check: whether readback data for double checking
    @returns 0 - success, other value failed code
*/
int _updi_write(void *nvm_ptr, char *cmd, nvm_wop opw, bool check)
{
    char **tk_s, **tk_w, **tokens;
    int address;
#define UPDI_WRITE_STROKEN_LEN 512
    char buf[UPDI_WRITE_STROKEN_LEN];
    int i, j, k, m, result = 0;
    bool dirty = false;

    tk_s = str_split(cmd, '|');
    for (k = 0; tk_s && tk_s[k]; k++)
    {
        tk_w = str_split(tk_s[k], ':');
        for (m = 0, address = ERROR_PTR; tk_w && tk_w[m]; m++)
        {
            if (result == 0)
            { // only work when no error occur
                if (m == 0)
                {
                    address = (int)strtol(tk_w[m], NULL, 16);
                }
                else if (m == 1)
                {
                    tokens = str_split(tk_w[m], ';');
                    for (i = 0; tokens && tokens[i]; i++)
                    {
                        DBG_INFO(UPDI_DEBUG, "Write[%d]: %s", i, tokens[i]);
                        j = i % UPDI_WRITE_STROKEN_LEN;
                        buf[j] = (char)(strtol(tokens[i], NULL, 16) & 0xff);
                        dirty = true;
                        if (j + 1 == UPDI_WRITE_STROKEN_LEN)
                        {
                            result = opw(nvm_ptr, address + i - j, buf, j + 1, check);
                            if (result)
                            {
                                DBG_INFO(UPDI_DEBUG, "opw failed %d", result);
                                result = -4;
                            }
                            dirty = false;
                        }
                        free(tokens[i]);
                    }
                    // write the left data
                    if (dirty && result == 0)
                    {
                        result = opw(nvm_ptr, address + i - j - 1, buf, j + 1, check);
                        if (result)
                        {
                            DBG_INFO(UPDI_DEBUG, "opw failed %d", result);
                            result = -5;
                        }
                    }
                    DBG_INFO(DEFAULT_DEBUG, "Write address %x(%d), result %d", address, i, result);

                    if (!tokens)
                    {
                        DBG_INFO(UPDI_DEBUG, "Parse write str: %s failed", tk_w[m]);
                    }
                    else
                        free(tokens);
                }
            }
            free(tk_w[m]);
        }
        if (!tk_w)
        {
            DBG_INFO(UPDI_DEBUG, "Parse write str: %s failed", tk_s[k]);
        }
        else
            free(tk_w);

        free(tk_s[k]);
    }

    if (!tk_s)
    {
        DBG_INFO(UPDI_DEBUG, "Parse write str: %s failed", cmd);
    }
    else
        free(tk_s);

    return result;
}

/*
UPDI Memory Write
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and data. 
        Format: [addr0]:[dat0];[dat1];[dat2]|[addr1]:...
            addr is hex type dig
            dat is hex type dig
    @check: whether readback data for double checking
    @returns 0 - success, other value failed code
*/
int updi_write(void *nvm_ptr, char *cmd, bool check)
{
    return _updi_write(nvm_ptr, cmd, nvm_write_auto, check);
}

/*
    UPDI Reset chip
    @nvm_ptr: updi_nvm_init() device handle
    @returns 0 - success, other value failed code
*/
int updi_reset(void *nvm_ptr)
{
    return nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET, true);
}

/*
    Debug view
    @nvm_ptr: updi_nvm_init() device handle
    @cmd: cmd string use for address and data. Format: ds=[ptr]|dr[ptr]|loop=[dat1]|keys[dat2]
            ds pointer: ptc_qtlib_node_stat1 address, which store structure qtm_acq_node_data_t, we get signal and cc value in it
            dr pointer: qtlib_key_data_set1 address, which store qtm_touch_key_data_t, we get ref value in it
    @returns 0 - success, other value failed code. this function will print the log in console
*/

static void _verbar_token_parse(char *cmd, const char *tag[], int *params, int count)
{
    char **tk_s, **tk_w; // token section, token words
    int i, j;

    // Parse tokens
    tk_s = str_split(cmd, '|');
    if (tk_s)
    {
        for (i = 0; tk_s[i]; i++)
        {
            tk_w = str_split(tk_s[i], '=');
            if (tk_w)
            {
                if (tk_w && tk_w[0] && tk_w[1])
                {
                    for (j = 0; j < count; j++)
                    {
                        if (!strcmp(tag[j], tk_w[0]))
                        {
                            params[j] = (int)strtol(tk_w[1], NULL, 0);
                            break;
                        }
                    }
                }

                for (j = 0; tk_w[j]; j++)
                    free(tk_w[j]);
            }

            free(tk_s[i]);
        }
        free(tk_s);
    }
}

// Use 1/1000 pf as unit, the max value 675 * 8 = 54000, less than 16bit, but will show negative value in studio if more thant 32767
#define CALCULATE_CAP_DIV_1000(_v) (((_v)&0x0F) * 7 + (((_v) >> 4) & 0x0F) * 68 + (((_v) >> 8) & 0x0F) * 675 + ((((_v) >> 12) & 0x03) + (((_v) >> 14) & 0x03)) * 6750)

// Use 1/100 pf as unit
#define CALCULATE_CAP_DIV_100(_v) ((((_v) >> 2) & 0x03) * 3 + (((_v) >> 4) & 0x0F) * 7 + (((_v) >> 8) & 0x0F) * 68 + ((((_v) >> 12) & 0x03) + (((_v) >> 14) & 0x03)) * 675)

// Use 1/10 pf as unit
#define CALCULATE_CAP_DIV_10(_v) ((((_v) >> 3) & 0x01) * 1 + (((_v) >> 2) & 0x03) * 3 + (((_v) >> 8) & 0x0F) * 7 + ((((_v) >> 12) & 0x03) + (((_v) >> 14) & 0x03)) * 68)

// Use pf(float)
#define CALCULATE_CAP_DIV(_v) ((_v)&0x0F) * 0.00675 + (((_v) >> 4) & 0x0F) * 0.0675 + (((_v) >> 8) & 0x0F) * 0.675 + ((((_v) >> 12) & 0x03) + (((_v) >> 14) & 0x03)) * 6.75;

static int _get_var_addr_data(void *nvm_ptr, varible_address_t *vaddr)
{
    information_container_t info_container;
    varible_address_array_t var_arr;

    int i, val, result = 0;

    memset(&info_container, 0, sizeof(info_container));
    result = get_ext_info_from_storage(nvm_ptr, &info_container, BLOCK_INFO);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "get_ext_info_from_storage failed", result);
        result = -2;
        goto out;
    }

    for (i = IB_REG_ST; i < IB_REG_END; i++)
    {
        val = ext_get(&info_container, i);
        if (!VALID_PTR(val))
        {
            DBG_INFO(UPDI_DEBUG, "ext_get `IB_REG(%d)` failed 0x%x", i, val);
            result = -3;
            goto out;
        }
        else
        {
            var_arr.data[i - IB_REG_ST] = (u16)val;
        }
    }

    if (vaddr)
    {
        memcpy(vaddr, &var_arr.var_addr, sizeof(vaddr));
    }

out:
    ext_destory(&info_container);

    return result;
}

static int _get_mem_data(void *nvm_ptr, int idx, int addr, void *output, int len)
{
    int result;

    // Node varible
    result = nvm_read_mem(nvm_ptr, (u16)(addr + idx * len), output, len);
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_read_mem addr(0x%x) - %d failed %d", addr, idx, result);
        return -2;
    }

    return 0;
}

/* storage param tag */
enum
{
    STORAGE_INFOBLOCK_ID,
    STORAGE_USERROW_OFFSET,
    STORAGE_EEPROM_OFFSET,
    STORAGE_MAX_PARAM_NUM
};

enum
{
    INFO_USERROW,
    INFO_EEPROM
};

const char *storage_token_tag[STORAGE_MAX_PARAM_NUM] = {
    "infoblock",
    "uoff",
    "eoff",
};

int storage_params[STORAGE_MAX_PARAM_NUM] = {
    /* STORAGE_INFOBLOCK_ID */ INFO_USERROW,
    /* STORAGE_USERROW_OFFSET */ 0,
    /* STORAGE_EEPROM_OFFSET */ 0};

int get_storage_type(B_BLOCK_TYPE btype)
{
    int nvm_type = NUM_NVM_TYPES;

    if (btype == BLOCK_INFO)
    {
        if (storage_params[STORAGE_INFOBLOCK_ID] == INFO_EEPROM)
        {
            nvm_type = NVM_EEPROM;
        }
        else
        {
            nvm_type = NVM_USERROW;
        }
    }
    else if (btype == BLOCK_CFG)
    {
        if (storage_params[STORAGE_INFOBLOCK_ID] == INFO_EEPROM)
        {
            nvm_type = NVM_USERROW;
        }
        else
        {
            nvm_type = NVM_EEPROM;
        }
    }
    else
    {
        /* Not support */
    }

    return nvm_type;
}

int get_storage_offset(B_BLOCK_TYPE btype)
{
    int id = STORAGE_USERROW_OFFSET;

    if (btype == BLOCK_INFO)
    {
        if (storage_params[STORAGE_INFOBLOCK_ID] == INFO_EEPROM)
        {
            id = STORAGE_EEPROM_OFFSET;
        }
        else
        {
            id = STORAGE_USERROW_OFFSET;
        }
    }
    else if (btype == BLOCK_CFG)
    {
        if (storage_params[STORAGE_INFOBLOCK_ID] == INFO_EEPROM)
        {
            id = STORAGE_USERROW_OFFSET;
        }
        else
        {
            id = STORAGE_EEPROM_OFFSET;
        }
    }
    else
    {
        // Not support
    }

    return storage_params[id];
}

int updi_storage(void *nvm_ptr, char *cmd)
{
    int result = 0;

    _verbar_token_parse(cmd, storage_token_tag, storage_params, STORAGE_MAX_PARAM_NUM);

    return result;
}

typedef struct cap_sample_value
{
    u16 reference;
    u16 signal;
    u16 cccap;

    double cc_value;
    u16 comcap;
    u8 sensor_state;
    u8 node_acq_status;
} cap_sample_value_t;

static int _get_rsd_data(void *nvm_ptr, int idx, int ds, int dr, cap_sample_value_t *rsd)
{
    qtm_acq_node_data_t ptc_signal;
    qtm_touch_key_data_t ptc_ref;

    double cc_value;
    int16_t val, ref_value, signal_value, delta_value;
    int result;

    memset(&ptc_signal, 0, sizeof(ptc_signal));
    memset(&ptc_ref, 0, sizeof(ptc_ref));

    // Signal varible
    result = nvm_read_mem(nvm_ptr, (u16)(ds + idx * sizeof(ptc_signal)), (void *)&ptc_signal, sizeof(ptc_signal));
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_read_mem signal failed %d", result);
        return -2;
    }

    // Ref varible
    result = nvm_read_mem(nvm_ptr, (u16)(dr + idx * sizeof(ptc_ref)), (void *)&ptc_ref, sizeof(ptc_ref));
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "nvm_read_mem reference failed %d", result);
        return -3;
    }

    val = (int16_t)lt_int16_to_cpu(ptc_signal.node_comp_caps);
    cc_value = CALCULATE_CAP_DIV(val);
    ref_value = (int16_t)lt_int16_to_cpu(ptc_ref.channel_reference);
    signal_value = (int16_t)lt_int16_to_cpu(ptc_signal.node_acq_signals);
    delta_value = signal_value - ref_value;

    if (rsd)
    {
        rsd->cccap = CALCULATE_CAP_DIV_10(val); // CALCULATE_CAP_DIV_100
        rsd->cc_value = cc_value;
        rsd->reference = ref_value;
        rsd->signal = signal_value;
        rsd->comcap = val;
        rsd->sensor_state = ptc_ref.sensor_state;
        rsd->node_acq_status = ptc_signal.node_acq_status;
    }

    /*
    DBG(DEFAULT_DEBUG, "signal raw:", (unsigned char *)&ptc_signal, sizeof(ptc_signal), "0x%02x ");
    DBG(DEFAULT_DEBUG, "ref raw:", (unsigned char *)&ptc_ref, sizeof(ptc_signal), "0x%02x ");
    */

    return 0;
}

/* param tag */
enum
{
    DBG_SIGNAL_ADDR,
    DBG_REFERENCE_ADDR,
    DBG_LOOP_CNT,
    DBG_KEY_START,
    DBG_KEY_CNT,
    DBG_MAX_PARAM_NUM
};

const char *dbg_token_tag[DBG_MAX_PARAM_NUM] = {
    "ds" /*ptc_qtlib_node_stat1->signal and cc value*/,
    "dr" /*qtlib_key_data_set1->ref value*/,
    "loop",
    "st",
    "keys"};

int updi_debugview(void *nvm_ptr, char *cmd)
{
    cap_sample_value_t rsd_data;
    varible_address_t var_addr;

    // time varible
    time_t timer;
    char timebuf[26];
    struct tm *tm_info;

    int i, j, channel, result = 0;

    int params[DBG_MAX_PARAM_NUM] = {0 /*DBG_SIGNAL_ADDR*/, 0 /*DBG_REFERENCE_ADDR*/, 0 /*DBG_LOOP_CNT*/, 0 /*DBG_KEY_START*/, 1 /*DBG_KEY_CNT*/}; // loop value default set to 1, keys default set to 1

    _verbar_token_parse(cmd, dbg_token_tag, params, DBG_MAX_PARAM_NUM);

    // Verify the input parameters
    if (!params[DBG_SIGNAL_ADDR] || !params[DBG_REFERENCE_ADDR])
    {
        // No valid address, load from info block
        result = _get_var_addr_data(nvm_ptr, &var_addr);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "_get_var_addr_data failed 0x%x", result);
            return -2;
        }
        else
        {
            params[DBG_SIGNAL_ADDR] = var_addr.dsdr.data.ds;
            params[DBG_REFERENCE_ADDR] = var_addr.dsdr.data.dr;
        }

        // Reset that mcu can continue to run
        nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET, true);
    }

    // if DBG_LOOP_CNT less than or qual 0: loop forever
    for (i = 0; params[DBG_LOOP_CNT] <= 0 || i < params[DBG_LOOP_CNT]; i++)
    {
        for (j = 0; j < params[DBG_KEY_CNT]; j++)
        {
            channel = params[DBG_KEY_START] + j;
            result = _get_rsd_data(nvm_ptr, channel, params[DBG_SIGNAL_ADDR], params[DBG_REFERENCE_ADDR], &rsd_data);
            if (result)
            {
                DBG_INFO(UPDI_DEBUG, "_get_rsd_data failed 0x%x", result);
            }
            else
            {
                time(&timer);
                tm_info = localtime(&timer);
                strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm_info);

                DBG_INFO(DEFAULT_DEBUG, "T[%s][%d-%d]: delta,%hd, ref,%hd, signal,%hd, cc,%hd(%.2f), sensor_state,%02xH, node_state,%02xH", timebuf, i, j,
                         (int16_t)(rsd_data.signal - rsd_data.reference),
                         rsd_data.reference,
                         rsd_data.signal,
                         rsd_data.cccap,
                         rsd_data.cc_value,
                         rsd_data.sensor_state,
                         rsd_data.node_acq_status);
            }
        }
    }

    return result;
}

static int _get_cfg_body_data(void *nvm_ptr, signal_limit_data_t *siglim_ptr, int len)
{
    config_container_t config_container;
    int lim_count = 0, size, result;
    signal_limit_data_t *siglim;

    // No test parameters, load from config block
    memset(&config_container, 0, sizeof(config_container));
    result = get_ext_info_from_storage(nvm_ptr, &config_container, BLOCK_CFG);
    if (result)
    {
        DBG_INFO(UPDI_ERROR, "get_ext_info_from_storage(cfg) failed", result);
        goto out;
    }

    siglim = ext_read(&config_container, CB_CFG_BODY_ELEM_DATA, 0);
    if (!siglim)
    {
        DBG_INFO(UPDI_DEBUG, "ext_get `CB_CFG_BODY_ELEM_DATA` failed");
        goto out;
    }

    // Get body size
    size = ext_get(&config_container, CB_CFG_BODY_SIZE);
    if (size <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "ext_get `CB_CFG_BODY_SIZE` failed 0x%x", size);
        goto out;
    }

    // Get count
    lim_count = ext_get(&config_container, CB_CFG_BODY_ELEM_COUNT);
    if (lim_count <= 0)
    {
        DBG_INFO(UPDI_DEBUG, "ext_get `CB_CFG_BODY_ELEM_COUNT` failed 0x%x", lim_count);
        goto out;
    }

    if (siglim_ptr && len >= size)
    {
        memcpy(siglim_ptr, siglim, size);
    }

out:
    ext_destory(&config_container);

    return lim_count;
}

/* param tag */
enum
{
    SLTEST_SIGLIM_LO,
    SLTEST_SIGLIM_HI,
    SLTEST_REF_RANGE,
    SLTEST_DS_DR_ADDR,
    SLTEST_ACQ_ND_ADDR,
    SLTEST_KEY_START,
    SLTEST_KEY_CNT,
    SLTEST_MAX_PARAM_NUM
};

const char *sltest_token_tag[SLTEST_MAX_PARAM_NUM] = {
    "siglo",
    "sighi",
    "range",
    "dsdr",
    "acqnd",
    "st",
    "keys"};

int updi_selftest(void *nvm_ptr, char *cmd)
{
    cap_sample_value_t rsd_data;
    varible_address_t var_addr;
    int16_t val;
    int i, j, k, size, lim_count, channel, start, result = 0;
    signal_limit_data_t *siglim = NULL, limit_setting;
    bool reset = false;
    bool siglim_alloc = false;

    // debug varible
    qtm_acq_node_config_t ptc_node;
    qtm_acq_node_group_config_t ptc_acq;

    int params[SLTEST_MAX_PARAM_NUM] = {0 /*SLTEST_SIGLIM_LO*/, 0 /*SLTEST_SIGLIM_HI*/, 0 /*SLTEST_DS_DR_RANGE*/, 0 /*SLTEST_ACQ_ND_ADDR*/, 0 /*SLTEST_KEY_START*/, 1 /*SLTEST_KEY_CNT*/}; // keys default set to 1
    _verbar_token_parse(cmd, sltest_token_tag, params, SLTEST_MAX_PARAM_NUM);

    // Verify the input limit parameters
    if (!params[SLTEST_SIGLIM_LO] && !params[SLTEST_SIGLIM_HI] && !params[SLTEST_REF_RANGE])
    {
        lim_count = _get_cfg_body_data(nvm_ptr, NULL, 0);
        if (lim_count <= 0)
        {
            DBG_INFO(UPDI_DEBUG, "_get_cfg_body_data failed(NULL) 0x%x", result);
            return -2;
        }
        else
        {
            size = lim_count * sizeof(signal_limit_data_t);
            siglim = malloc(size);
            if (!siglim)
            {
                return -3;
            }
            else
            {
                siglim_alloc = true;
                lim_count = _get_cfg_body_data(nvm_ptr, siglim, size);
                if (lim_count <= 0)
                {
                    DBG_INFO(UPDI_DEBUG, "_get_cfg_body_data failed(%d) 0x%x", size, result);
                    result = -4;
                    goto out;
                }
            }
        }
        start = 0;
        reset = true;
    }
    else
    {
        limit_setting.limit.count = (uint8_t)params[SLTEST_KEY_CNT];
        limit_setting.limit.siglo = (uint16_t)params[SLTEST_SIGLIM_LO];
        limit_setting.limit.sighi = (uint16_t)params[SLTEST_SIGLIM_HI];
        limit_setting.limit.range = (uint8_t)params[SLTEST_REF_RANGE];
        siglim = &limit_setting;
        start = params[SLTEST_KEY_START];
        lim_count = 1;
    }

    // Verify the input addr parameters
    if (!params[SLTEST_DS_DR_ADDR] || !params[SLTEST_ACQ_ND_ADDR])
    {
        // No valid address, load from info block
        result = _get_var_addr_data(nvm_ptr, &var_addr);
        if (result)
        {
            DBG_INFO(UPDI_DEBUG, "_get_var_addr_data failed 0x%x", result);
            result = -5;
            goto out;
        }
        reset = true;
    }
    else
    {
        var_addr.dsdr.value = (unsigned int)params[SLTEST_DS_DR_ADDR];
        var_addr.acqnd.value = (unsigned int)params[SLTEST_ACQ_ND_ADDR];
    }

    if (/*reset*/ 0)
    {
        // Reset that mcu can continue to run
        nvm_reset(nvm_ptr, TIMEOUT_WAIT_CHIP_RESET, true);
    }

    // ptc_acq
    result = _get_mem_data(nvm_ptr, 0, var_addr.acqnd.data.acq, &ptc_acq, sizeof(ptc_acq));
    if (result)
    {
        DBG_INFO(UPDI_DEBUG, "_get_mem_data(acq) failed 0x%x", result);
        result = -6;
        goto out;
    }

    DBG_INFO(UPDI_DEBUG, "==========================");
    DBG_INFO(UPDI_DEBUG, "Selftest Parameters:");
    for (i = 0, j = start; i < lim_count; i++)
    {
        DBG_INFO(UPDI_DEBUG, "Group(%d): K%d(n%d): [%d - %d / %d]",
                 i,
                 start,
                 siglim[i].limit.count,
                 siglim[i].limit.siglo,
                 siglim[i].limit.sighi,
                 siglim[i].limit.range);
        j += siglim[i].limit.count;
    }
    DBG_INFO(UPDI_DEBUG, "");

    // Get range
    for (i = 0, k = start; i < lim_count; i++)
    {
        for (j = 0; j < siglim[i].limit.count; j++)
        {
            channel = k + j;
            if (channel < ptc_acq.num_sensor_nodes)
            { // Check node count
                result = _get_rsd_data(nvm_ptr, channel, var_addr.dsdr.data.ds, var_addr.dsdr.data.dr, &rsd_data);
                if (result)
                {
                    DBG_INFO(UPDI_DEBUG, "_get_rsd_data failed 0x%x", result);
                }
                else
                {
                    result = _get_mem_data(nvm_ptr, channel, var_addr.acqnd.data.node, &ptc_node, sizeof(ptc_node));
                    if (result)
                    {
                        DBG_INFO(UPDI_DEBUG, "_get_mem_data(node) failed 0x%x", result);
                    }
                    else
                    {
                        if (rsd_data.cccap > siglim[i].limit.sighi || rsd_data.cccap < siglim[i].limit.siglo)
                        {
                            DBG_INFO(UPDI_ERROR, "Group[%d]: key(%d) signal(%d) out of range (%d~%d) ", i, channel, rsd_data.cccap, siglim[i].limit.siglo, siglim[i].limit.sighi);
                            result = -7;
                            goto out;
                        }
                        else
                        {
                            val = rsd_data.reference >> NODE_GAIN_DIG(ptc_node.node_gain);
                            if (val > NODE_BASE_LINE + siglim[i].limit.range || val < NODE_BASE_LINE - siglim[i].limit.range)
                            {
                                DBG_INFO(UPDI_ERROR, "Group[%d]: key(%d) ref(%d %d) variance out of range (%d) ", i, channel, rsd_data.reference, val, siglim[i].limit.range);
                                result = -8;
                                goto out;
                            }
                            else
                            {
                                // The node is passed
                                DBG_INFO(UPDI_DEBUG, "Key(%d) Ref %d Cap %d is OK", channel, rsd_data.reference, rsd_data.cccap);
                            }
                        }
                    }
                }
            }
        }

        k += j;
    }
out:
    if (siglim && siglim_alloc)
    {
        free(siglim);
    }

    if (result)
    {
        DBG_INFO(UPDI_ERROR, "Failed");
    }
    else
    {
        DBG_INFO(UPDI_ERROR, "Passed");
    }

    return result;
}