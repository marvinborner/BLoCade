# BLoCade

[BLoC](https://github.com/marvinborner/bloc) is an optimized file format
for binary lambda calculus (BLC).

BLoCade is the BLoC-aid and turns BLoC files back into executable files
(targets). This is useful for [bruijn](https://bruijn.marvinborner.de),
benchmarking, or general term optimization.

## Targets

-   BLC (sharing by abstraction): Terms having BLoC entry indices get
    abstracted and applied to the respective term. The indices get
    converted to De Bruijn indices. Flag `bblc` (bits) and `blc` (ASCII
    0/1).
-   BLC (unshared): Every BLoC entry gets reinserted into the original
    term. Do not use this if you want efficiency or small files. Flag
    `unbblc` (bits) and `unblc` (ASCII 0/1).
-   [Effekt](https://effekt-lang.org): Because, why not? Flag `effekt`.
-   Planned: Scala, HVM, C, LLVM, JS, Haskell

## Benchmarks

To be evaluated.
