#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static void doit()
{
    //if color setting fails just act like cat does?
    //todo: error handling, reporting, etc.
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(console, &dwMode);
    dwMode |= 0x0004; //ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(console, dwMode);
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

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

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

static void printColoredByHash(const std::string& str)
{
    const int idx = fnv(str.c_str()) % kColorCount;
    setColor(kColors[idx]);
    std::cout << str;
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
    char buff[1024];

    doit();

    while(readline(buff, 1000))
    {
        std::string str = buff;
        std::string sofar;
        for(const char c : str)
        {
            if(std::isspace(c))
            {
                printColoredByHash(sofar);
                sofar.clear();
                std::cout << c;
            }
            else
            {
                sofar.push_back(c);
            }
        }//for

        printColoredByHash(sofar);
        std::cout << std::endl;
    }//while
}
