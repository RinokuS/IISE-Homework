#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "grid.h"
#include "tinfo.h"
#include "barrier.h"

// Initiate a barrier object
barrier barr;

int generation = 0;

// This function counts the neighbors of a point in our grid.
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


// evolve function looks at a section of the main grid, and
// proceeds to count the neighbors for each entry. Based on the neighbor
// count, the function passes a value indicating cell death or
// cell birth to temp grid.
void evolve(grid *main, grid *temp, int height, int part) {

    int i, j, neighbors;

    // Examine a specific part of G
    for (i = part; i < part + height; i++) {
        for (j = 0; j < main->cols; j++) {

            neighbors = count_neighbors(main, i, j);

            // Determine which cells are born and which die.
            if (main->val[i][j] == 1 && (neighbors < 2 || neighbors > 3)) {
                temp->val[i][j] = 0;
            } else if (main->val[i][j] == 0 && neighbors == 3) {
                temp->val[i][j] = 1;
            }
        }
    }
}


// Looks at a specific part of our temp grid, and transfers
// the values into our permanent grid. The values transferred
// depend on the thread that calls the function.
//
void update_grid(grid *x, grid *y, int height, int part) {
    for (int i = part; i < part + height; i++) {
        for (int j = 0; j < x->cols; j++) {
            x->val[i][j] = y->val[i][j];
        }
    }
}

void print_grid(grid *G, char *label) {
    printf("%s\n", label);

    for (int i = 0; i < G->rows; i++) {
        for (int j = 0; j < G->cols; j++) {
            switch (G->val[i][j]) {
                case 0:
                    putchar('0');
                    break;
                case 1:
                    putchar('1');
                    break;
                default:
                    break;
            }
        }
        putchar('\n');
    }
    putchar('\n');
}

// thread_func is the general function passed to each thread. It is responsible
// for computing the evolved values of a certain section of grid,
// and then updating main grid to contain these values.
void *thread_func(void *arguments) {
    tinfo *info = (tinfo *)arguments;

    grid *main = info->in;
    grid *temp = info->out;
    int div = info->divide;

    int height = (int)(main->rows / div);
    int part = height * info->section;

    // we need to wait other threads before we start to update the main grid
    // and before we start another evolve loop
    for (int i = 0; i < info->gen; i++) {
        evolve(main, temp, height, part);
        barrier_wait(&barr);
        update_grid(main, temp, height, part);
        barrier_wait(&barr);
    }
}

int main() {
    int g, rows, cols;
    int threads_number;
    char mode;
    struct timespec mt1, mt2;
    long int timestamp;

    printf("Welcome to the Multithreaded Game of Life.\n");
    printf("Enter the height of the board: ");
    scanf("%d", &rows);
    printf("Enter the width of the board: ");
    scanf("%d", &cols);
    printf("Enter the number of generations: ");
    scanf("%d", &g);
    printf("Please enter a divisor of %d to determine the number of threads: ", rows);
    scanf("%d", &threads_number);
    while (rows % threads_number != 0) {
        printf("I'm sorry, %d does not divide %d. Please choose a divisor of %d: ", threads_number, rows, rows);
        scanf("%d", &threads_number);
    }

    printf("Please enter grid populating mode ('M' for manual insert and 'R' for random populating): ");
    scanf(" %c", &mode);
    while (mode != 'M' && mode != 'R') {
        printf("I'm sorry, %c is not available input. Please choose correct mode: ", mode);
        scanf(" %c", &mode);
    }

    grid *main = init_grid(rows, cols);
    grid *temp = init_grid(rows, cols);
    if (mode == 'R') {
        random_populate(main, 132 /*(unsigned int) time(NULL)*/);
    } else {
        manual_populate(main);
    }
    print_grid(main, "Populated grid at the start of the game: ");
    update_grid(temp, main, main->rows, 0);
    // start our profile session
    clock_gettime(CLOCK_MONOTONIC, &mt1);

    barrier_init(&barr, threads_number);

    // Creates an array of tinfo structs and
    // pthreads. We then place the necessary
    // info into each tinfo struct.
    tinfo **thread_infos = malloc(threads_number * sizeof(tinfo));
    pthread_t threads[threads_number];

    for (int i = 0; i < threads_number; i++) {
        thread_infos[i] = init_tinfo();
        thread_infos[i]->in = main;
        thread_infos[i]->out = temp;
        thread_infos[i]->section = i;
        thread_infos[i]->divide = threads_number;
        thread_infos[i]->gen = g;
    }

    // Initialize a number of threads. Each thread works on a portion of our
    // grid
    for (int i = 0; i < threads_number; i++) {
        pthread_create(&threads[i], NULL, &thread_func, (void *)thread_infos[i]);
    }
    for (int i = 0; i < threads_number; i++) {
        pthread_join(threads[i], NULL);
    }

    barrier_destroy(&barr);

    print_grid(main, "Final grid: ");
    clock_gettime (CLOCK_MONOTONIC, &mt2);

    timestamp = 1000000000 * (mt2.tv_sec - mt1.tv_sec) + (mt2.tv_nsec - mt1.tv_nsec);

    printf("Elapsed time: %ld", timestamp);

    destroy_grid(main);
    destroy_grid(temp);
    free(thread_infos);

    return 0;
}
