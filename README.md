# colors

A small command that works as a pipe and colors words in random colors
(consistently coloring same words same colors), to make it easier to spot
same words in the output. E.g. to see potentially same values of some hash,
(example is my other tool [pixelsum](https://github.com/FRex/pixelsum)):

It does not correctly handle files with `NUL` (ASCII/UTF-8 byte of
value 0, `00` in hex) in them. An option to handle this might be added later.

![screenshot](screenshot.png)

Run with `--addsep=chars` to add `chars` as extra word separators and with
`-h` or `--help` to see help. With `--cat` it acts like `cat` (I use this for
benchmarking - this mode is the ideal and coloring is slower due to overhead).
Option `--wordlen=x` where x is a positive number will split words into parts
of that length at most, to make it clear in case there is many long words that
get colored the same (in that case with this option each long word will have
few colors which will reduce the chance of 100% color collision). It's somewhat
UTF-8 aware - it counts bytes, not codepoints, but it will not break encoding
sequence of a single codepoint in two (so if the threshold is reached
mid-codepoint it will continue taking codeunits until that codepoint ends).
Use `--no-flush` or `--noflush` to not work line-by-line (faster but will no
longer work nicely with slow outputting programs, interactive use, etc.).
Option `--alnum` is a shortcut for setting all printable ASCII non-alnum as separators.
Option `--seed=x` sets the seed, from 1 to 255 inclusive, to modify what
colors are assigned to what words, in case you want to rerun because the words
you tried to tell apart appeared as the same color originally.

Go to releases to find:
1. Unotptimized 32-bit Windows exe built with Pelles C (a niche C
compiler: [link](http://www.smorgasbordet.com/pellesc/download.htm)).
2. Optimized 64-bit Windows exe built with GCC -O3 (from
[w64devkit](https://nullprogram.com/blog/2020/09/25/)).

Keep in mind 24-bit ANSI colors in console require quite a recent Windows 10 version:
[link](https://devblogs.microsoft.com/commandline/24-bit-color-in-the-windows-console/).

Please let me know if it doesn't compile or work on your terminal, OS, Distro,
GCC default settings, etc. or if you find any bugs or have any improvement ideas.

Speed should not be an issue ever since these commits were made:
1. https://github.com/FRex/colors/commit/45869c10e3dbb38fa15330ac6f98e3e91c0dd78d
2. https://github.com/FRex/colors/commit/8a6bffbee8e9e9e2af318d61dd44ef0679dfe510
3. https://github.com/FRex/colors/commit/9965d6e146611da73d17a1aacfc2c3c9c4c73767
4. https://github.com/FRex/colors/commit/d77de798c443efa010a7c3d1627233944fb3d9ab

It's about 15-20x slower than GNU cat on my laptop, but it always does line
buffering, is written in portable C, and adds colors, so that's plenty fast
even for large files. I might try to optimize it further in the future but I
feel like most reasonable gains were achieved by the above 4 commits already.
