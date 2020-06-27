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

#include <stdio.h>
#include <iterator>

const unsigned kColors[] = {
    0xffffff,

    0xff0000,
    0x00ff00,
    0x0000ff,

    0xff00ff,
    0xffff00,
    0x00ffff,

    0xff7fff,
    0xffff7f,
    0x7fffff,

    0xff7f7f,
    0x7fff7f,
    0x7f7fff,

    0x3f7fff,
    0x7f3fff,

    0x7fff3f,
    0x3fff7f,

    0xff3f7f,
    0xff7f3f,
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

static void printColoredByHash(const std::string& str)
{
    std::hash<std::string> h;
    const int idx = h(str) % kColorCount;
    setColor(kColors[idx]);
    std::cout << str;
}

inline unsigned rgb(int r, int g, int b)
{
    return (r << 16) + (g << 8) + b;
}

static void add6(std::vector<unsigned>& colors, int r, int g, int b)
{
    colors.push_back(rgb(r, g, b));
    colors.push_back(rgb(g, r, b));

    colors.push_back(rgb(r, b, g));
    colors.push_back(rgb(g, b, r));

    colors.push_back(rgb(b, r, g));
    colors.push_back(rgb(b, g, r));
}

#include <set>

int main(int argc, char ** argv)
{
    doit();


    if(argc > 1 && 0 == strcmp("--colors", argv[1]))
    {
        int sofar = 0;
        std::vector<unsigned> colors;
        const int components[] = { 0x0, 0x1f, 0x3f, 0x7f, 0xff };
        for(const int r : components)
            for(const int g : components)
                for(const int b : components)
                    add6(colors, r, g, b);

        {
            std::set<unsigned> set(colors.begin(), colors.end());
            colors.assign(set.begin(), set.end());
            std::sort(colors.begin(), colors.end());
        }

        for(const unsigned c : colors)
        {
            setColor(c);
            printf("0x%06x ", c);

            ++sofar;
            if(sofar % 8 == 0)
                puts("");
        } //for each

        setColor(0xffffff);
        puts("");
        puts("-------------------");

        sofar = 0;

        colors = {
            0xffffff,
            0xff0000, 0x00ff00,
            0x7f0000, 0x007f00,
            0x00ffff, 0xff00ff, 0xffff00,

            0x007fff,

            0x3fff3f,

            0x007f3f,
            0x7f7f00,

        };

        for(const unsigned c : colors)
        {
            setColor(c);
            printf("0x%06x, ", c);

            ++sofar;
            if(sofar % 4 == 0)
                puts("");
        } //for each
        puts("");

        return 0;
    }


    std::string str;
    while(std::getline(std::cin, str))
    {
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
        sofar.clear();
        std::cout << std::endl;
    }//while
}
