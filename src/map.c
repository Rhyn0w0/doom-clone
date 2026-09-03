#include "map.h"
#include <stddef.h>

/* Level 1:入门 rooms, one door, a few enemies, exit east. */
static const char *L1[] = {
    "########################",
    "#P.....#.......#.......#",
    "#......#.......#...h...#",
    "#..G...#...a...#.......#",
    "#......D.......D....S..#",
    "#......#.......#.......#",
    "#......#.......#.......#",
    "###.####.#######.#####.#",
    "#......#.......#.....#.#",
    "#..a...#...G...#..a..#.#",
    "#......#.......#.....#.#",
    "#..#####.#####.####..#.#",
    "#..#.........G......##.#",
    "#..#..h......a.......#X#",
    "#..#.................#.#",
    "########################",
};

/* Level 2: bigger, two doors, more of both enemy types. */
static const char *L2[] = {
    "################################",
    "#P.......#..........#..........#",
    "#........#....G.....#.....S....#",
    "#...a....#..........#..........#",
    "#........D..........D.....h....#",
    "#........#..........#..........#",
    "#...S....#....#####.####.####..#",
    "#........#....#.........#....#.#",
    "####.#####....#....a....#....#.#",
    "#........#....#.........#.h..#.#",
    "#...G....#....####.######....#.#",
    "#........#....#.....S........#.#",
    "#...h....#....#.............##.#",
    "#........D....D....#####.....#X#",
    "#...a.........#....#...#.....#.#",
    "#........#....#....#...#..G..#.#",
    "#...S....#....#....#...#.....#.#",
    "################################",
};

static const char **levels[MAP_COUNT] = { L1, L2 };
static const int heights[MAP_COUNT] = { 16, 18 };

int map_width(int level)
{
    if (level < 0 || level >= MAP_COUNT)
        level = 0;
    int w = 0;
    for (int y = 0; y < heights[level]; y++) {
        int len = 0;
        while (levels[level][y][len] != '\0')
            len++;
        if (len > w)
            w = len;
    }
    return w;
}

int map_height(int level)
{
    if (level < 0 || level >= MAP_COUNT)
        level = 0;
    return heights[level];
}

char map_cell(int level, int x, int y)
{
    if (level < 0 || level >= MAP_COUNT)
        level = 0;
    if (y < 0 || y >= heights[level])
        return '#';
    const char *row = levels[level][y];
    int len = 0;
    while (row[len] != '\0')
        len++;
    if (x < 0 || x >= len)
        return '#';
    char c = row[x];
    if (c == ' ')
        return '#';
    return c;
}
