## Abstract:

EXAMPLE: VADDR = 0xFFFFFFFF.80000000
 
                   9 Bit     9 Bit      9 Bit     9 Bit    12 Bit
    Unused          PML4      PDP        PD        PT       Frame
1111111111111111  111111111 111111111 111111111 111111111 111111111111
64                                                                   0
<------------------------ 64 Bit Address ----------------------------> 


PML4  =>  (VADDR >> 39) & 0x1FF   ----> 511
PDP   =>  (VADDR >> 30) & 0x1FF   ----> 510   ------------->  NEXT = 511
PD    =>  (VADDR >> 22) & 0x1FF   ---->   0                           |
PT    =>  (VADDR >> 12) & 0x1FF   ---->   0                           |______PD [ 0 - 511 ]
Frame =>  VADDR & 0xFFF           ----->  0                           |       |
                                                                      |       0 -> 0x40000000  -  ((0x40000000 + 0x200000) - 1)
                                                                      |       1 -> 0x40200000  -  so on ...
PD [ 0 - 511 ]                                                        |
0 ->     0        - (0x200000 - 1)                                    |
1 ->     0x200000 - (0x400000 - 1)                                    |
2 ->     0x400000 - (0x600000 - 1)                                    |
N ->     N * 0x200000 - (((N + 1) * 0x200000) -1)                     |
                                                                      |
511      0x3FE00000   -  0x3FFFFFFF     => 0xFFFFFFFF.BFFFFFFF        |
                                                     |                |
                                                     |                |
                                                     V                |
                                                    NEXT              |
                                           0xFFFFFFFF.C0000000  ------+ ----> = (128 * 0x200000) Physical Address referenced by 0xFFFFFFFF.D0000000
                                                     |_________________ 
                                                     |                 |
                                           0xFFFFFFFF.D0000000      Distance = 128 (0x80) pages
                                                     |                       = 256 MB / 0x200000   => 2 MB pages
                                                     |                  OR   = 256 MB / 0x1000     => 4K pages => 0x10000 (65536) pages = 10 Page Tables
                                                     |
                                          If assigned to 0xFD000000
                                            then 0xFFFFFFFF.C0000000
                                            should be -> (0xFD000000 - Distance)


I guess the easiest way is to use 0xFFFFFFFF80000000 and /or 0xFFFFFFFFC0000000 as the base,
because we will have Page Directory at index 0.
