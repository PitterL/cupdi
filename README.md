# cupdi

This is C version of UPDI interface achievement, you could refer to the Python version:

    https://github.com/mraardvark/pyupdi.git

My purpose is you could use UPDI to flash the new TinyAVR at any enviroment like Computer / MCU Embedded system, just transplant the code the there.

    I used other vendor library, thanks:
        argparse: https://github.com/cofyc/argparse.git
        ihex:       https://github.com/arkku/ihex.git
        sercom:     https://github.com/dpogue/Terminal-Emulator.git
    If you have any question, please query Pater to mailbox atpboy444@hotmail.com
    If you like it please help to mark STAR at https://github.com/PitterL/cupdi.git



Usage: Simple command line interface for UPDI programming:
   or: cupdi [options] [[--] args]
   or: Erase chip: cupdi -c COM2 -d tiny817 -e
   or: Flash hex file: cupdi -c COM2 -d tiny817 --program -f c:/817.hex

A brief description of what the program does and how it works.

    -h, --help            show this help message and exit

Basic options
    -d, --device=<str>    Target device
    -c, --comport=<str>   Com port to use (Windows: COMx | *nix: /dev/ttyX)
    -b, --baudrate=<int>  Baud rate, default=115200
    -f, --file=<str>      Intel HEX file to flash
    -u, --unlock          Perform a chip unlock (implied with --unlock)
    -e, --erase           Perform a chip erase (implied with --flash)
    --, --program         Program Intel HEX file to flash
    --, --update          Program infoblock update to eeprom(need map file)
    --, --check           Check flash content with infoblock CRC
    --, --compare         Compare vcs HEX file with infoblock and fuses content
    --, --verify          check and compare
    -i, --info            Get Infoblock infomation of firmware
    --, --save            Save flash to a VCS HEX file
    --, --dump            Dump flash to a Intel HEX file
    --, --fuses=<str>     Fuse to set [addr0]:[dat0];[dat1];|[addr1]...
    -r, --read=<str>      Direct read from memory [addr1]:[n1]|[addr2]:[n2]...
    -w, --write=<str>     Direct write to memory [addr0]:[dat0];[dat1];|[addr1]...
    --, --dbgview=<str>   get ref/delta/cc value operation ds=[ptc_qtlib_node_stat1]|dr=[qtlib_key_data_set1]|loop=[n]|keys=[n] (loop(Hex) set to 0 loop forvever, default 1, keys default 1)
    -v, --verbose=<int>   Set verbose mode (SILENCE|UPDI|NVM|APP|LINK|PHY|SER): [0~6], default 0, suggest 2 for status information
    --, --reset           UPDI reset device
    --, --disable         UPDI disable
    -t, --test            Test UPDI device
    --, --version         Show version
    --, --pack-build      Pack info block to Intel HEX file, (macro FIRMWARE_VERSION at 'touch.h')save with extension'.ihex'
    --, --pack-info       Shwo packed file(ihex) info
  
Example:

    Write memory:
        cupdi.exe -c COM7 -d tiny817 -v 2 -u -w 3f00;01;02;03;04;05
    
    Read memory:
        cupdi.exe -c COM7 -d tiny817 -v 2 -u -r 3f00;5
    
    Erase Flash:
        cupdi.exe -c COM7 -d tiny817 -v 2 -e
    
    Program:
        cupdi.exe -c COM7 -d tiny817 -v 2 -f tiny817.hex    or
        cupdi.exe -c COM7 -d tiny817 -v 2 -p -f tiny817.hex     (if no other operation for file, default programming)
    
    Verify Flash:
        cupdi.exe -c COM7 -d tiny817 -v 2 --verify -f tiny817.hex
    
    Save Flash content:
        cupdi.exe -c COM7 -d tiny817 -v 2 -s -f tiny817.hex     (The content will save to tiny817.hex.out)
	
	pack image with fuse and version number (with pack.h file in ../qtouch/pack.h):
		cupdi.exe -d tiny817 --pack-build
	
    
  
