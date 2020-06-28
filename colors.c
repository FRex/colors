#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _MSC_VER
#define COLORS_ON_WINDOWS
#endif

#ifdef COLORS_ON_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

static int eanbleConsoleColor(void)
{
#ifndef COLORS_ON_WINDOWS
    return 1; /* outside windows just assume it will work */
#else
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0u;

    if(console == INVALID_HANDLE_VALUE)
    {
        fprintf(
            stderr,
            "GetStdHandle(STD_OUTPUT_HANDLE) returned INVALID_HANDLE_VALUE, GetLastError() = %u\n",
            GetLastError()
        );
        return 0;
    }

    if(console == NULL)
    {
        fprintf(stderr, "GetStdHandle(STD_OUTPUT_HANDLE) returned NULL\n");
        return 0;
    }

    if(!GetConsoleMode(console, &mode))
    {
        fprintf(stderr, "GetConsoleMode(console, &mode) returned fales, GetLastError() = %d\n",
            GetLastError());
        return 0;
    }

    /* ENABLE_VIRTUAL_TERMINAL_PROCESSING, by value in case its missing from header... */
    mode |= 0x0004;

    if(!SetConsoleMode(console, mode))
    {
        fprintf(stderr, "SetConsoleMode(console, mode) returned fales, GetLastError() = %d\n",
            GetLastError());
        return 0;
    }

    return 1;
#endif /* COLORS_ON_WINDOWS */
}

const unsigned kWhite = 0xffffff;

const unsigned kColors[] = {
    0xffffff, 0xff0000, 0x00ff00, 0x7f0000,
    0x007f00, 0x00ffff, 0xff00ff, 0xffff00,
    0x007fff, 0x3fff3f, 0x007f3f, 0x7f7f00,
};

const int kColorCount = sizeof(kColors) / sizeof(kColors[0]);

static void setColor(unsigned rgb)
{
    const int r = (rgb >> 16) & 0xff;
    const int g = (rgb >> 8) & 0xff;
    const int b = (rgb >> 0) & 0xff;
    printf("\033[38;2;%d;%d;%dm", r, g, b);
}

/* 32-bit fnv1, not 1a */
static unsigned fnv(const char * str)
{
    unsigned ret = 2166136261u;
    while(*str)
    {
        ret ^= (unsigned char)(*str);
        ret *= 16777619u;
        ++str;
    } /* while *str */

    return ret;
}

static void printColoredByHash(const char * str)
{
    const int idx = fnv(str) % kColorCount;
    setColor(kColors[idx]);
    fputs(str, stdout); /* not puts to not get a newline */
    setColor(kWhite); /* todo: save and restore original color if its not white? */
}

static int readline(char * buff, int len)
{
    /* for files without final newline last fgets read line without \n and set eof */
    if(feof(stdin))
        return 0;

    buff[0] = '\0'; /* to make sure we got 0 len str if we're about to hit eof */
    fgets(buff, 1000, stdin);

    /* this happens for files that do have newline at end of file too */
    if(strlen(buff) == 0 && feof(stdin))
        return 0;

    /* if we have a newline at end of line - remove it */
    if(strchr(buff, '\n'))
        *strchr(buff, '\n') = '\0';

    return 1;
}

int main(int argc, char ** argv)
{
    char buff[8 * 1024 + 5];

    if(!eanbleConsoleColor())
    {
        /* todo: act like straight stdin>stdout cat here? */
        return 1;
    }

    /* todo: somehow handle if line is too long? print error? */
    while(readline(buff, 8 * 1024))
    {
        const char * lastwordstart = buff;
        char * cur = buff;

        while(*cur)
        {
            if(isspace((unsigned char)*cur))
            {
                const char c = *cur; /* save the space char */
                *cur = '\0'; /* make word so far terminated by nul */
                printColoredByHash(lastwordstart);
                lastwordstart = cur + 1; /* next word starts at next char at least */
                fputc(c, stdout); /* print the space char we hit and replaced */
            }

            ++cur;
        } /* while *cur */

        /* print any leftover word */
        printColoredByHash(lastwordstart);
        fputc('\n', stdout);
    } /* while readline */

    return 0;
}
