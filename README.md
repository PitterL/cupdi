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
    or: Flash hex file: cupdi -c COM2 -d tiny817 -f c:/817.hex



A brief description of what the program does and how it works.

    -h, --help            show this help message and exit
Basic options

    -d, --device=<str>    Target device
    -c, --comport=<str>   Com port to use (Windows: COMx | *nix: /dev/ttyX) (Linux platform not finished yet)
    -b, --baudrate=<int>  Baud rate, default=115200
    -f, --file=<str>      Intel HEX file: to program, save or verification 
    -u, --unlock          Perform a chip unlock (implied with --unlock)
    -e, --erase           Perform a chip erase (implied with --flash)
    -p, --program         Program Intel HEX file to flash
    -k, --check           Compare Intel HEX file with flash content
    -s, --save            Save flash to a Intel HEX file
    -u, --fuses=<str>     Fuse to set (syntax: fuse_nr:0xvalue)
    -r, --read=<str>      Direct read from memory [addr];[n]
    -w, --write=<str>     Direct write to memory [addr];[dat0];[dat1];[dat2]...
    -v, --verbose=<int>   Set verbose mode (SILENCE|UPDI|NVM|APP|LINK|PHY|SER): [0~6], default 0, suggest 2 for status information
    -t, --test            Test UPDI device
  
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
        cupdi.exe -c COM7 -d tiny817 -v 2 -k -f tiny817.hex
    
    Save Flash content:
        cupdi.exe -c COM7 -d tiny817 -v 2 -s -f tiny817.hex     (The content will save to tiny817.hex.out)
    
  
