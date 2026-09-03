#ifndef DOOMC_MAP_H
#define DOOMC_MAP_H

/* Static level layouts. Legend:
 *   '#','B','C'  wall (texture 1,2,3)   'D' door   'X' exit (walkable)
 *   '.'          floor                   ' ' void = wall (treat as solid)
 *   'P' player start  'G' grunt  'S' spitter  'a' ammo  'h' medkit
 */
#define MAP_COUNT 2

int map_width(int level);
int map_height(int level);
/* Raw cell char at (x,y); out of bounds returns '#'. Never NULL. */
char map_cell(int level, int x, int y);

#endif
