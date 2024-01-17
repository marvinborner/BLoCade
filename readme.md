# BLoCade

[BLoC](https://github.com/marvinborner/bloc) is a file format for shared
binary lambda calculus (BLC).

BLoCade, the BLoC-aid, turns BLoC files back into executable files
(targets). This is useful for [bruijn](https://bruijn.marvinborner.de)
(see [wiki](https://bruijn.marvinborner.de/wiki/coding/compilation/)),
benchmarking, or general term optimization.

## Targets

-   BLC (sharing by abstraction): Terms having BLoC entry indices get
    abstracted and applied to the respective term. The indices get
    converted to De Bruijn indices. The dependency graph is resolved by
    a topological sort. Flag `bblc` (bits) and `blc` (ASCII 0/1).
-   BLC (unshared): Every BLoC entry gets reinserted into the original
    term. Do not use this if you want efficiency or small files. Flag
    `unbblc` (bits) and `unblc` (ASCII 0/1).
-   Planned: [Effekt](https://effekt-lang.org), Scala, HVM, C, NASM,
    LLVM, JS, Haskell

## Benchmarks

To be evaluated.
