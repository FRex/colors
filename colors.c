#ifdef _MSC_VER
#define COLORS_ON_WINDOWS
#define _CRT_SECURE_NO_WARNINGS /* to allow using strcpy */
#endif

#include <stdlib.h>
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
     * assume we just want to dump ANSI color sequences to file in that case */
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

/* linebuffsize must be smaller than mybuffsize to not process words bigger than
   mybuffsize because that would make mybuff_add write past the end of its buffer */
#define linebuffsize (4096)
#define mybuffsize (256 * 1024)

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
    if(datalen == 0)
        return;

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

/* 32-bit fnv1a */
static unsigned fnv(const char * str, int length)
{
    unsigned ret = 2166136261u;
    while(length > 0)
    {
        ret ^= (unsigned char)(*str);
        ret *= 16777619u;
        --length;
        ++str;
    } /* while length > 0 */

    return ret;
}

#define COLOR_RESET_STRING "\033[0m"
#define COLOR_RESET_STRING_LENGTH (sizeof(COLOR_RESET_STRING) - 1)

static void addColoredByHash(mybuff * outbuff, const char * str, int length)
{
    int idx;

    /* don't print color codes around empty string */
    if(length == 0)
        return;

    idx = fnv(str, length) % kColorCount;
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
    buff[len - 1] = '@'; /* nul term goes here if the line takes entire buffer */

    /* fgets returns null if it hits eof before reading any chars too, try that too */
    if(!fgets(buff, len, stdin))
        return 0;

    /* this happens for files that do have newline at end of file too */
    /* first cond is optimized strlen(buff) == 0, compilers usually do that too */
    if(buff[0] == '\0' && feof(stdin))
        return 0;

    /* newline is treated as a normal separator and written out so keep it */
    /* if entire buff is full and last char is not newline and its not eof then the line is too long */
    /* NOTE: this assumes fgets doesnt modify characters past nul term it wrote */
    if(buff[len - 1] != '@' && buff[len - 2] != '\n' && !feof(stdin))
        *toomuch = 1;

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
    printf("    --wordlen=NUM    - limit words to NUM bytes but keep UTF-8 intact\n");
    printf("    --no-flush       - don't flush the stdout after each line\n");
    printf("    --noflush        - alias for --no-flush\n");

    /* print colors in their color, if possible, else in default color */
    ok = enableConsoleColor();
    printf("Available color format strings (%s):\n", ok ? "in that color each" : "values only");
    for(i = 0; i < kColorCount; ++i)
    {
        const char * c = kColors[i].string;
        if(ok)
            fputs(c, stdout);

        fputs(c + 1, stdout); /* skip the ESC char to not interpret this as control sequence */
        if(ok)
            fputs(COLOR_RESET_STRING, stdout);

        putc('\n', stdout);
    } /* for each color */

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

static int isNoflushOption(const char * a)
{
    return sameString("--no-flush", a) || sameString("--noflush", a);
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

static int isUtf8ContinuationByte(unsigned char c)
{
    /* detect 10xxxxxx - get the top two bits then check they are 1 and 0 */
    return (c & (3 << 6)) == (2 << 6);
}

int main(int argc, char ** argv)
{
    char separatorset[260]; /* the set of separators, no repetitions*/
    char indexedseparators[260]; /* c is separator iff indexedseparators[c] */
    char buff[linebuffsize];
    char separatorsbuff[linebuffsize];
    int toomuch, i, verbose, separatorsbufflen, cat, wordlen, noflush;
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

    wordlen = 0;
    noflush = 0;
    for(i = 1; i < argc; ++i)
    {
        if(isVerboseOption(argv[i]) || isCatOption(argv[i]))
            continue;

        if(isNoflushOption(argv[i]))
        {
            noflush = 1;
            continue;
        }

        /* catch --addsep option with no value or even no = */
        if(sameString(argv[i], "--addsep") || sameString(argv[i], "--addsep="))
        {
            fprintf(stderr, "--addsep argument requires a separators after =\n");
            continue;
        }

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

            continue;
        } /* if --addsep */

        /* catch --wordlen option with no value or even no = */
        if(sameString(argv[i], "--wordlen") || sameString(argv[i], "--wordlen="))
        {
            fprintf(stderr, "--wordlen argument requires a number value after =\n");
            continue;
        }

        if(startswith(argv[i], "--wordlen="))
        {
            const char * argnum = argv[i] + strlen("--wordlen=");
            if(strlen(argnum) > 7)
            {
                fprintf(stderr, "max 7 digits for --wordlen=\n");
                continue;
            }

            wordlen = atoi(argnum);
            if(wordlen <= 0)
            {
                fprintf(stderr, "argument for --wordlen= must be positive, '%s' evaluated to %d\n", argnum, wordlen);
                wordlen = 0;
                continue;
            }

            if(verbose)
                fprintf(stderr, "wordlen successfully set to %d\n", wordlen);

            continue;
        } /* if --wordlen= */

        fprintf(stderr, "unknown option: %s\n", argv[i]);
    }

    /* prepare the separators table from set */
    memset(indexedseparators, 0x0, 260);
    for(i = 0; i < 256; ++i)
        indexedseparators[i] = (NULL != strchr(separatorset, i));

    /* set to 0 here and not in loop body to buffer more than 1 line in no flush mode */
    outbuff.usage = 0;
    toomuch = 0;

    /* set wordlen to a huge value if its not used, so curwordlen > wordlen never passes, no need to first do worlden > 0 */
    if(wordlen == 0)
        wordlen = 1 << 30;

    while(mygetline(buff, linebuffsize, &toomuch))
    {
        const char * lastwordstart = buff;
        char * cur = buff;

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
            /* if we hit a non-continuation byte it means (assuming valid UTF-8
               input) that we just ended some codepoint's encoding, so we can
               flush the word out if it's long enough, flushing at just == wordlen
               would result in in invalid UTF-8 output and totally break the text
               no need to check separator first, if it was a separator char we'd
               flush the word anyway, and if not then it'd run this exact check */
            if((int)(cur - lastwordstart) >= wordlen && !isUtf8ContinuationByte(*cur))
            {
                addColoredByHash(&outbuff, lastwordstart, (int)(cur - lastwordstart));
                lastwordstart = cur;
            }

            if(indexedseparators[(unsigned char)*cur])
            {
                separatorsbuff[separatorsbufflen++] = *cur; /* save the space char */
                addColoredByHash(&outbuff, lastwordstart, (int)(cur - lastwordstart));
                lastwordstart = cur + 1; /* next word starts at next char at least */
            }
            else
            {
                mybuff_add(&outbuff, separatorsbuff, separatorsbufflen);
                separatorsbufflen = 0;
            }

            ++cur;
        } /* while *cur */

        /* print any leftover word or separators (only one will be non-empty so order doesn't matter here) */
        mybuff_add(&outbuff, separatorsbuff, separatorsbufflen);
        addColoredByHash(&outbuff, lastwordstart, (int)(cur - lastwordstart));

        if(!noflush)
        {
            mybuff_flush(&outbuff); /* buffer more than one line in noflush mode */
            fflush(stdout); /* to make sure even with pipes or redirections the lines appear as soon as possible */
        }
    } /* while mygetline */

    /* make sure to flush buffered output at the end - matters for no flush mode */
    mybuff_flush(&outbuff);
    fflush(stdout);

    return 0;
}
