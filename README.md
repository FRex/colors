# colors

A small command that works as a pipe and colors words in random colors
(consistently coloring same words same colors), to make it easier to spot
same words in the output. E.g. to see potentially same values of some hash,
(example is my other tool [pixelsum](https://github.com/FRex/pixelsum)):

![screenshot](screenshot.png)

Run with `--addsep=chars` to add `chars` as extra word separators and with
`-h` or `--help` to see help.

Go to releases for a 32-bit Windows exe (keep in mind 24-bit ANSI colors in
console require quite a recent Windows 10 version:
[link](https://devblogs.microsoft.com/commandline/24-bit-color-in-the-windows-console/)).

For optimal performance (the above exe is from a niche C compiler and not
optimized) you should compile it yourself with
`gcc -O3 -march=native colors.c -o colors` or similar. Please let me know if it
doesn't compile or doesn't work on your OS, Distro, GCC default settings, etc.
