#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "grid.h"
#include "tinfo.h"
#include "mybarrier.h"

// Initiate a barrier object
mybarrier barr;

int counter = 0;
int generation = 0;

// This function counts the neighbors of a point in our grid.
//
int count_neighbors(grid *G, int x, int y) {
    int i, j, count = 0;
    for (i = x - 1; i <= x + 1; i++) {
        for (j = y - 1;j <= y + 1; j++) {
            if ((i == x && j == y) || (i < 0 || j < 0) || (i >= G->rows || j >= G->cols)) {
                continue;
            }
            if(G->val[i][j] == 1) {
                count++;
            }
        }
    }
    return count;
}


// evolve function looks at a section of the grid G (determined by the thread), and
// proceeds to count the neighbors for each entry. Based on the neighbor
// count, the function passes a value indicating cell death (0) to T, or
// cell birth (1) to T.
//
void evolve(grid *G, grid *T, int height, int part) {

    int i, j, neighbors;

    // Examine a specific part of G
    //
    for (i = part; i < part + height; i++) {
        for (j = 0; j < G->cols; j++) {

            neighbors = count_neighbors(G, i, j);

            // Determine which cells are born and which die.
            //
            if (G->val[i][j] == 1 && (neighbors < 2 || neighbors > 3)) {
                T->val[i][j] = 0;
            } else if (G->val[i][j] == 0 && neighbors == 3) {
                T->val[i][j] = 1;
            }
        }
    }
}


// Looks at a specific part of our temp grid, and transfers
// the values into our permanent grid. The values transferred
// depend on the thread that calls the function.
//
void update_grid(grid *main, grid *temp, int height, int part) {
    for (int i = part; i < part + height; i++) {
        for (int j = 0; j < main->cols; j++) {
            main->val[i][j] = temp->val[i][j];
        }
    }
}

// We use sleep before printing the grid in the above loop. By
// making the program wait for a specified amount of time before
// printing, we get a clearer evolution process. Without the
// 'sleep(2000)' above, the program prints each evolved state too quickly.
//
void sleep(unsigned int mill) {
    clock_t start = clock();
    while (clock() - start < mill) { }
}

// Prints the grid -- relatively straightforward implementation.
// Replaces the 1's and 0's within the grid by X's and blank
// spaces, respectively.
//
void print_grid(grid *G) {
    printf("Welcome to the game of life! Generation: %d.\n", generation);

    for (int i = 0; i < G->rows; i++) {
        for (int j = 0; j < G->cols; j++) {
            switch (G->val[i][j]) {
                case 0:
                    putchar(' ');
                    break;
                case 1:
                    putchar('X');
                    break;
                default:
                    break;
            }
        }
        putchar('\n');
    }

}

// thread_func is the general function passed to each thread. It is responsible
// for computing the evolved values of a certain section of G (mEvolve),
// and then updating G to contain these values (mgridUpdate).
//
void *thread_func(void *arguments) {
    tinfo *I = (tinfo *)arguments;

    grid *G = I->in;
    grid *T = I->out;
    int div = I->divide;

    int height = (int)(G->rows / div);
    int part = height * I->section;

    // multEvolve looks at the entries in G and, based on those
    // entries, enters new values into T. Therefore, we can have
    // simultaneous threads accessing multEvolve with no problem
    // (assuming we have allocated independent parts of G for
    // each thread to work on). We place a barrier before and after
    // running mgridUpdate, as mgridUpdate edits the values in G.
    // We need each thread to be working with the same G when multEvolve
    // runs.
    //
    for (int i = 0; i < I->gen; i++) {
        evolve(G, T, height, part);
        barrier_wait(&barr);
        update_grid(G, T, height, part);
        counter++;

        // This is the only relatively tricky part of the loop.
        // The last thread to complete the above computations
        // should [theoretically] satisfy count % div == 0, and
        // then print the grid. If we didn't have the 'if' condition,
        // our grid would print 'div' times for each generation, which
        // is undesirable.
        //
        if (counter % div == 0) {
            sleep(2000);
            generation++;
            print_grid(G);
        }
        barrier_wait(&barr);
        i++;
    }
}

int main() {
    int i, g, rows, cols;
    int threads_number;
    char mode;

    printf("Welcome to the Game of Life.\n");
    printf("How many generations would you like to watch? ");
    scanf("%d", &g);
    printf("Enter the width of the board: ");
    scanf("%d", &cols);
    printf("Enter the height of the board: ");
    scanf("%d", &rows);

    // Define our grids: G is our main grid, and T is our
    // temp grid.
    grid *G = init_grid(rows, cols);
    grid *T = init_grid(rows, cols);

    printf("Please enter grid populating mode ('M' for manual insert and 'R' for random populating): ");
    scanf(" %c", &mode);
    while (mode != 'M' && mode != 'R') {
        printf("I'm sorry, %c is not available input. Please choose correct mode: ", mode);
        scanf(" %c", &mode);
    }
    if (mode == 'R') {
        random_populate(G);
    } else {
        manual_populate(G);
    }
    print_grid(G);
    update_grid(T, G, G->rows, 0);

    // Gets the desired number of threads from the user -- we repeatedly
    // ask for a number until we get a divisor of rows. Once we know how
    // many threads there will be, we initialize the barrier.
    //
    printf("Please enter a divisor of %d to determine the number of threads: ", rows);
    scanf("%d", &threads_number);
    while (rows % threads_number != 0) {
        printf("I'm sorry, %d does not divide %d. Please choose a divisor of %d: ", threads_number, rows, rows);
        scanf("%d", &threads_number);
    }

    barrier_init(&barr, threads_number);

    // Creates an array of tinfo structs and
    // pthreads. We then place the necessary
    // info into each tinfo struct.
    //
    tinfo **I = malloc(threads_number * sizeof(tinfo));
    pthread_t threads[threads_number];

    for (i = 0; i < threads_number; i++) {
        I[i] = initTinfo();
        I[i]->in = G;
        I[i]->out = T;
        I[i]->section = i;
        I[i]->divide = threads_number;
        I[i]->gen = g;
    }

    // Initialize a number of threads. Each thread works on a portion of our
    // grid -- which portion it works on is decided by the I[i] tinfo struct.
    //
    for (i = 0; i < threads_number; i++) {
        pthread_create(&threads[i], NULL, &thread_func, (void *)I[i]);
        pthread_join(threads[i], NULL);
    }

    barrier_destroy(&barr);

    return 0;
}
