# tiny-regex-c
# A small regex implementation in C
### Description
Forked from [kokke/tiny-regex-c](https://github.com/kokke/tiny-regex-c). 

### Current Status
supported syntax:  
`^ $ . \s \S \d \D \w \W [^ABC0-9] [ABC0-9] * + ? *? +? ?? {m,n} {m,n}?`  
No static variables.  
Matching functions return a pointer to the matching position, also tells the end position if requested.  

### Example Usage
See tests directory

### License
All material in this repository is in the public domain with the exception of [scripts](scripts) folder.
([exrex](asciimoo/exrex) is under AGPLv3 license, since regex_test.py and regex_test_neg.py use exrex they fall under the same AGPLv3 license.)



 
