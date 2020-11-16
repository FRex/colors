#ifdef _MSC_VER
#define COLORS_ON_WINDOWS
#define _CRT_SECURE_NO_WARNINGS /* to allow using strcpy */
#endif

#include <string.h>
#include <stdio.h>

#ifdef COLORS_ON_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

static void ensureNoWindowsLineConversions(void)
{
#ifdef COLORS_ON_WINDOWS
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
#endif /* COLORS_ON_WINDOWS */
}

static int enableConsoleColor(void)
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
            (unsigned)GetLastError()
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
        fprintf(stderr, "GetConsoleMode(console, &mode) returned false, GetLastError() = %u\n",
            (unsigned)GetLastError());
        return 0;
    }

    /* ENABLE_VIRTUAL_TERMINAL_PROCESSING, by value in case its missing from header... */
    mode |= 0x0004;

    if(!SetConsoleMode(console, mode))
    {
        fprintf(stderr, "SetConsoleMode(console, mode) returned false, GetLastError() = %u\n",
            (unsigned)GetLastError());
        return 0;
    }

    return 1;
#endif /* COLORS_ON_WINDOWS */
}

/* linebuffsize must be bigger than mybuffsize to not allow words bigger than
   mybuffsize because that would make mybuff_add do a buffer overwrite */
#define mybuffsize 4096
#define linebuffsize (2 * mybuffsize)

typedef struct mybuff {
    int usage;
    char data[mybuffsize];
} mybuff;

static void mybuff_flush(mybuff * self)
{
    fwrite(self->data, 1, self->usage, stdout);
    self->usage = 0;
}

static void mybuff_add(mybuff * self, const char * data, int datalen)
{
    if(self->usage + datalen > mybuffsize)
        mybuff_flush(self);

    memcpy(self->data + self->usage, data, datalen);
    self->usage += datalen;
}

typedef struct mystring {
    const char * string;
    int length;
} mystring;

const mystring kColors[] = {
#define FORMAT_COLOR_HELPER(literal) {literal, sizeof(literal) - 1}
#define FORMAT_COLOR(r, g, b) FORMAT_COLOR_HELPER("\033[38;2;"#r";"#g";"#b"m")
    FORMAT_COLOR(255, 255, 255),
    FORMAT_COLOR(255, 0, 0),
    FORMAT_COLOR(0, 255, 0),
    FORMAT_COLOR(127, 0, 0),
    FORMAT_COLOR(0, 127, 0),
    FORMAT_COLOR(0, 255, 255),
    FORMAT_COLOR(255, 0, 255),
    FORMAT_COLOR(255, 255, 0),
    FORMAT_COLOR(0, 127, 255),
    FORMAT_COLOR(63, 255, 63),
    FORMAT_COLOR(0, 127, 63),
    FORMAT_COLOR(127, 127, 0),
#undef FORMAT_COLOR
#undef FORMAT_COLOR_HELPER
};

const int kColorCount = sizeof(kColors) / sizeof(kColors[0]);

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

#define COLOR_RESET_STRING "\033[0m"
#define COLOR_RESET_STRING_LENGTH (sizeof(COLOR_RESET_STRING) - 1)

static void printColoredByHash(mybuff * outbuff, const char * str, int length)
{
    const int idx = fnv(str) % kColorCount;

    /* don't print color codes around empty string */
    if(str[0] == '\0')
        return;

    mybuff_add(outbuff, kColors[idx].string, kColors[idx].length);
    mybuff_add(outbuff, str, length);
    mybuff_add(outbuff, COLOR_RESET_STRING, COLOR_RESET_STRING_LENGTH);
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
static int addtoset(char * set, const char * newelements)
{
    int ret = 0;
    while(*newelements)
    {
        if(!strchr(set, *newelements))
        {
            set[strlen(set)] = *newelements;
            ++ret;
        } /* if not strchr */

        ++newelements;
    } /* while */

    return ret;
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

static int printhelp(const char * argv0)
{
    const char * exename = basename(argv0);
    int i, ok;

    printf("%s - pipe to color same words same random colors\n", exename);
    printf("Help: %s -h or %s --help\n", exename, exename);
    printf("Usage: %s [-v] [--verbose] [--addsep=chars]...\n", exename);
    printf("    --addsep=chars   - adds chars to list of word separators\n");
    printf("    --verbose or -v  - print internal and diagnostic info to stderr\n");

    /* print colors in their color, if possible, else in default color */
    ok = enableConsoleColor();
    printf("Available color format strings (%s):\n", ok ? "in that color each" : "values only");
    for(i = 0; i < kColorCount; ++i)
    {
        const char * c = kColors[i].string;
        if(ok)
            fputs(c, stdout);

        puts(c + 1); /* skip the ESC char to not interpret this as control sequence */
    } /* for each color */

    fputs(COLOR_RESET_STRING, stdout);
    return 0;
}

static int sameString(const char * a, const char * b)
{
    return 0 == strcmp(a, b);
}

static int isVerboseOption(const char * a)
{
    return sameString("-v", a) || sameString("--verbose", a);
}

static int hasVerboseOption(int argc, char ** argv)
{
    int i;
    for(i = 1; i < argc; ++i)
        if(isVerboseOption(argv[i]))
            return 1;

    return 0;
}

static int isCatOption(const char * a)
{
    return sameString(a, "--cat");
}

static int hasCatOption(int argc, char ** argv)
{
    int i;
    for(i = 1; i < argc; ++i)
        if(isCatOption(argv[i]))
            return 1;

    return 0;
}

static void printEscaped(FILE * f, const char * str)
{
    while(*str)
    {
        switch(*str)
        {
        case '\n':
            fputs("\\n", f);
            break;
        case '\v':
            fputs("\\v", f);
            break;
        case '\t':
            fputs("\\t", f);
            break;
        case '\r':
            fputs("\\r", f);
            break;
        case '\f':
            fputs("\\f", f);
            break;
        case '\\':
            fputs("\\\\", f);
            break;
        default:
            if(' ' <= *str && *str <= '~')
                fputc(*str, f);
            else
                fprintf(f, "\\x%x%x", (*str) >> 4, (*str) & 0xf);
            break;
        } /* switch */

        ++str;
    } /* while */
}

int main(int argc, char ** argv)
{
    char separatorset[260]; /* the set of separators, no repetitions*/
    char indexedseparators[260]; /* c is separator iff indexedseparators[c] */
    char buff[linebuffsize];
    char separatorsbuff[linebuffsize];
    int toomuch, i, verbose, separatorsbufflen, cat;
    mybuff outbuff;

    /* enable binary stdin/stdout/stderr as soon as possible */
    ensureNoWindowsLineConversions();

    /* check if -h or --help is present */
    for(i = 1; i < argc; ++i)
        if(startswith(argv[i], "--help") || startswith(argv[i], "-h"))
            return printhelp(argv[0]);

    /* do the cat option first, before even trying to enable console color */
    cat = hasCatOption(argc, argv);
    if(cat || !enableConsoleColor())
    {
        while(fgets(buff, linebuffsize, stdin))
            fputs(buff, stdout);

        return cat ? 0 : 1; /* return 0 if cat was requested, 1 if its error */
    } /* if not enable console color */

    verbose = hasVerboseOption(argc, argv);

    /* prepare the separators set */
    memset(separatorset, 0x0, 260);
    strcpy(separatorset, " \f\n\r\t\v");
    if(verbose)
    {
        fprintf(stderr, "starting with separator set '");
        printEscaped(stderr, separatorset);
        fprintf(stderr, "'\n");
    }

    for(i = 1; i < argc; ++i)
    {
        if(isVerboseOption(argv[i]) || isCatOption(argv[i]))
            continue;

        if(startswith(argv[i], "--addsep="))
        {
            const char * newchars = argv[i] + strlen("--addsep=");
            int added;

            if(verbose)
            {
                fprintf(stderr, "adding '");
                printEscaped(stderr, newchars);
                fprintf(stderr, "' to separator set '");
                printEscaped(stderr, separatorset);
                fprintf(stderr, "'");
            } /* verbose */

            added = addtoset(separatorset, newchars);
            if(verbose)
            {
                fprintf(stderr, ", result is '");
                printEscaped(stderr, separatorset);
                fprintf(stderr, "' (%d chars added)\n", added);
            }
        }
        else
            fprintf(stderr, "unknown option: %s", argv[i]);
    }

    /* prepare the separators table from set */
    memset(indexedseparators, 0x0, 260);
    for(i = 0; i < 256; ++i)
        indexedseparators[i] = (NULL != strchr(separatorset, i));

    toomuch = 0;
    while(mygetline(buff, linebuffsize, &toomuch))
    {
        const char * lastwordstart = buff;
        char * cur = buff;

        outbuff.usage = 0;
        if(toomuch)
        {
            /* todo: also print error in color if stderr is tty? */
            fprintf(stderr, "warning: more than %d chars in line - degrading to plain cat\n", linebuffsize - 2);
            fputs(buff, stdout);
            while(fgets(buff, linebuffsize, stdin))
                fputs(buff, stdout);

            return 1;
        } /* if too much */

        separatorsbufflen = 0;
        while(*cur)
        {
            if(indexedseparators[(unsigned char)*cur])
            {
                separatorsbuff[separatorsbufflen++] = *cur; /* save the space char */
                *cur = '\0'; /* make word so far terminated by nul */
                printColoredByHash(&outbuff, lastwordstart, (int)(cur - lastwordstart));
                lastwordstart = cur + 1; /* next word starts at next char at least */
            }
            else
            {
                separatorsbuff[separatorsbufflen] = '\0';
                if(separatorsbufflen > 0)
                    mybuff_add(&outbuff, separatorsbuff, separatorsbufflen);

                separatorsbufflen = 0;
            }

            ++cur;
        } /* while *cur */

        /* print any leftover word or separators (only one will be non-empty so order doesn't matter here) */
        separatorsbuff[separatorsbufflen] = '\0';
        if(separatorsbufflen > 0)
            mybuff_add(&outbuff, separatorsbuff, separatorsbufflen);

        printColoredByHash(&outbuff, lastwordstart, (int)strlen(lastwordstart));
        mybuff_add(&outbuff, "\n", 1);
        mybuff_flush(&outbuff);
    } /* while mygetline */

    return 0;
}
