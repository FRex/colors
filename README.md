# colors

A small command that works as a pipe and colors words in random colors
(consistently coloring same words same colors), to make it easier to spot
same words in the output. E.g. to see potentially same values of some hash,
(example is my other tool [pixelsum](https://github.com/FRex/pixelsum)):

![screenshot](screenshot.png)

Run with `--addsep=chars` to add `chars` as extra word separators and with
`-h` or `--help` to see help. With `--cat` it acts like `cat` (I use this for
benchmarking - this mode is the ideal and coloring is slower due to overhead).

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
1. 45869c10e3dbb38fa15330ac6f98e3e91c0dd78d
2. 8a6bffbee8e9e9e2af318d61dd44ef0679dfe510
3. 9965d6e146611da73d17a1aacfc2c3c9c4c73767
4. d77de798c443efa010a7c3d1627233944fb3d9ab

It's about 15-20x slower than GNU cat on my laptop, but it always does line
buffering, is written in portable C, and adds colors, so that's plenty fast
even for large files. I might try to optimize it further in the future but I
feel like most reasonable gains were achieved by the above 4 commits already.
