#ifndef _GRID_H
#define _GRID_H

typedef struct {
    int rows;
    int cols;
    int **val;
} grid;

grid *init_grid(int rows, int cols);
void destroy_grid(grid* G);
void random_populate(grid *G, unsigned int seed);
void manual_populate(grid *G);

#endif
