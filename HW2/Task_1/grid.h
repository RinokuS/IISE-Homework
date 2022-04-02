#ifndef _GRID_H
#define _GRID_H

// Our grid struct is very simple: it keeps
// track of the dimension of the grid, and
// the value in each section. It is essentially
// a matrix.
//
typedef struct {
    int rows;
    int cols;
    int **val;
} grid;

grid *init_grid(int rows, int cols);
void random_populate(grid *G);
void manual_populate(grid *G);

#endif
