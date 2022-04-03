#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct cache_line {
    char array[64];
};

void profile_cputime(unsigned int arr_size) {
    unsigned int iterations1 = 1024 * 1024 * 1024;
    unsigned int iterations2 = 128 * 1024 * 1024;
    volatile struct cache_line arr[arr_size];
    struct timespec mt1, mt2;
    double timestamp = 0;
    //printf("cache_line structure size: %lu\n", sizeof(struct cache_line));

    for (unsigned int i = 0; i < iterations1; i++) {
        arr[i % arr_size].array[0]++;
    }
    for (int i = 0; i < 10; ++i) {
        clock_gettime (CLOCK_REALTIME, &mt1);
        for (unsigned int j = 0; j < iterations2; j++) {
            arr[j % arr_size].array[0]++;
        }

        clock_gettime (CLOCK_REALTIME, &mt2);
        timestamp += (1000000000. * (mt2.tv_sec - mt1.tv_sec) + (mt2.tv_nsec - mt1.tv_nsec)) / 1000000000;
    }

    timestamp /= 10;
    printf("size: %lu Kb, time: %1.4f \n", arr_size * sizeof(struct cache_line) / 1024, timestamp);
}

int main() {
    unsigned int l1_cache_check[] = {128, 256, 384, 512, 768, 1024, 1566, 2048 };
    unsigned int l2_cache_check[] = {1566, 2048, 2560, 3072, 3584, 4096, 4608, 5120, 5632, 6144 };

    for (int i = 0; i < 8; ++i) {
        profile_cputime(l1_cache_check[i]);
    }

    for (int i = 0; i < 10; ++i) {
        profile_cputime(l2_cache_check[i]);
    }

    return 0;
}