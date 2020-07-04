#include <string.h>
#include <stdio.h>

#ifdef _MSC_VER
#define COLORS_ON_WINDOWS
#endif

#ifdef COLORS_ON_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#endif

static int eanbleConsoleColor(void)
{
#ifndef COLORS_ON_WINDOWS
    return 1; /* outside windows just assume it will work */
#else
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0u;

    /* using 'Console' winapi func fails if stdout isn't a tty/is redirected so
     * assume we just want to dump ANSI color sequnces to file in that case */
    if(!_isatty(_fileno(stdout)))
        return 1;

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

static int mygetline(char * buff, int len, int * toomuch)
{
    /* for files without final newline last fgets read line without \n and set eof */
    if(feof(stdin))
        return 0;

    buff[0] = '\0'; /* to make sure we got 0 len str if we're about to hit eof */

    /* fgets returns null if it hits eof before reading any chars too, try that too */
    if(!fgets(buff, len, stdin))
        return 0;

    /* this happens for files that do have newline at end of file too */
    if(strlen(buff) == 0 && feof(stdin))
        return 0;

    /* if we have a newline at end of line - remove it */
    if(strchr(buff, '\n'))
    {
        *strchr(buff, '\n') = '\0';
    }
    else
    {
        /* if we have no newline and its not eof then its too long line */
        if(!feof(stdin))
            *toomuch = 1;
    }

    return 1;
}

static int startswith(const char * s, const char * n)
{
    return 0 == strncmp(s, n, strlen(n));
}

/* set must have enough zeroed space after last nul to fit all new unique elems */
static void addtoset(char * set, const char * newelements)
{
    while(*newelements)
    {
        if(!strchr(set, *newelements))
            set[strlen(set)] = *newelements;

        ++newelements;
    } /* while */
}

static const char * basename(const char * fpath)
{
    const char * ret = fpath;

    while(strchr(ret, '/'))
        ret = strchr(ret, '/') + 1;

    while(strchr(ret, '\\'))
        ret = strchr(ret, '\\') + 1;

    return ret;
}

#define buffsize 8192

int main(int argc, char ** argv)
{
    char separators[260]; /* the set of separators, no repetitions*/
    char buff[buffsize];
    int toomuch, i;

    /* check if -h or --help is present */
    for(i = 1; i < argc; ++i)
    {
        if(startswith(argv[i], "--help") || startswith(argv[i], "-h"))
        {
            const char * exename = basename(argv[0]);
            printf("%s - pipe to color same words same random colors\n", exename);
            printf("Help: %s -h or %s --help\n", exename, exename);
            printf("Usage: %s [--addsep=chars]...\n", exename);
            printf("    --addsep=chars - adds chars to list of word separators\n");
            /* print all colors we have available here too? */
            return 0;
        }
    }

    if(!eanbleConsoleColor())
    {
        while(fgets(buff, buffsize, stdin))
            fputs(buff, stdout);

        return 1;
    } /* if not enable console color */

    /* prepare the separators set */
    memset(separators, 0x0, 260);
    strcpy(separators, " \f\n\r\t\v");
    for(i = 1; i < argc; ++i)
    {
        if(startswith(argv[i], "--addsep="))
            addtoset(separators, argv[i] + strlen("--addsep="));
        else
            fprintf(stderr, "unknown option: %s", argv[i]);
    }

    toomuch = 0;
    while(mygetline(buff, buffsize, &toomuch))
    {
        const char * lastwordstart = buff;
        char * cur = buff;

        if(toomuch)
        {
            /* todo: also print error in color if stderr is tty? */
            fprintf(stderr, "warning: more than %d chars in line - degrading to plain cat\n", buffsize - 2);
            fputs(buff, stdout);
            while(fgets(buff, buffsize, stdin))
                fputs(buff, stdout);

            return 1;
        } /* if too much */

        while(*cur)
        {
            if(strchr(separators, *cur))
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
    } /* while mygetline */

    return 0;
}
