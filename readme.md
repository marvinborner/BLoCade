# BLoCade

[BLoC](https://github.com/marvinborner/bloc) is a file format for shared
binary lambda calculus (BLC).

BLoCade, the BLoC-aid, turns BLoC files back into executable files
(targets). This is useful for [bruijn](https://bruijn.marvinborner.de)
(see [wiki](https://bruijn.marvinborner.de/wiki/coding/compilation/)),
benchmarking, or general term optimization.

## Targets

- BLC (sharing by abstraction): Terms having BLoC entry indices get
  abstracted and applied to the respective term. The indices get
  converted to De Bruijn indices. The dependency graph is resolved by a
  topological sort. Flag `bblc` (bits) and `blc` (ASCII 0/1).
- BLC (unshared): Every BLoC entry gets reinserted into the original
  term. Do not use this if you want efficiency or small files. Flag
  `unbblc` (bits) and `unblc` (ASCII 0/1).
- Planned: “normal” programming languages

## Shared `bblc` benchmarks

Some deliberately unoptimized test cases from `test/`, evaluated using
`./run` and measured in bits:

| file          | bloc | unbblc/orig | bblc  |
|:--------------|:-----|:------------|:------|
| echo          | 56   | 4           | 4     |
| reverse       | 248  | 172         | 172   |
| churchadd1    | 296  | 390         | 159   |
| churchadd2    | 368  | 433         | 207   |
| churcharith   | 560  | 1897        | 339   |
| uni           | 576  | 459         | 448   |
| ternaryvarmod | 1248 | 2330        | 1824  |
| collatz       | 1904 | 2884        | 1986  |
| ternaryadd    | 2240 | 19422       | 2708  |
| ternaryfac    | 2792 | 9599        | 3091  |
| aoc           | 7480 | 50062       | 10861 |

*Huge* and convoluted programs like
[@woodrush](https:://github.com/woodrush)’s
[lambda-8cc](https://github.com/woodrush/lambda-8cc) will not get
smaller by compiling them to shared BLC. The *many* sharable terms get
abstracted to so many levels that the (unary) BLC indices (in case of
8cc) can literally be 50 kilobit long. One solution would obviously be a
BLC variant with binary indices, or just storing and interpreting such
files in the BLoC format directly (which also uses binary reference
indices). A better algorithm than standard topological sort for
resolving the dependencies would also help.

8cc, measured in megabit:

| file       | bloc | unbblc/orig | bblc    |
|:-----------|:-----|:------------|:--------|
| lambda-8cc | 5.13 | 40.72       | 3267.88 |
