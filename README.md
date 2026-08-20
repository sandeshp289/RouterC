# RouterC

Description:

    A Router in C/C++

Input Files:

    router.c                         : C/C++ code

    bench1.grid                      : Input grid file (for e.g. bench1) in text format which specifies the physical grid

                                       on which your maze router will search

    bench1.nl                        : Input netlist file (for e.g. bench1) in text format which specifies the nets you
    
                                       need to route

Output Files:

    bench1.router.output             : Output file in text format, which tells what happened with each net you tried to route,
                                       and what path it took

   
Compilation:

    g++ router.c
    
Execution:

    ./a.out bench1
