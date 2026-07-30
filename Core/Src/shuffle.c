#include "shuffle.h"
#include "main.h"
#include "fat32.h"

/* A generously sufficient upper bound for audio files; if exceeded, only the
   first SHUFFLE_MAX_FILES files are shuffled. */
#define SHUFFLE_MAX_FILES 256

static int      bag[SHUFFLE_MAX_FILES];
static int      bag_count   = 0;
static int      bag_pos     = 0;
static int      enabled     = 0;
static uint32_t rng_state   = 0;

static uint32_t next_rand(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static void build_bag(int current_index) {
    if (rng_state == 0)
        rng_state = HAL_GetTick() ^ DWT->CYCCNT ^ 0x9E3779B9u;

    int total = FAT32_CountFiles();
    if (total > SHUFFLE_MAX_FILES) total = SHUFFLE_MAX_FILES;
    bag_count = total;
    if (bag_count <= 0) { bag_pos = 0; return; }

    for (int i = 0; i < bag_count; i++) bag[i] = i;
    for (int i = bag_count - 1; i > 0; i--) {
        int j = (int)(next_rand() % (uint32_t)(i + 1));
        int tmp = bag[i];
        bag[i] = bag[j];
        bag[j] = tmp;
    }

    bag_pos = 0;
    for (int i = 0; i < bag_count; i++) {
        if (bag[i] == current_index) { bag_pos = i; break; }
    }
}

void Shuffle_SetEnabled(int on, int current_index) {
    if (on && !enabled)
        build_bag(current_index);
    enabled = on ? 1 : 0;
}

int Shuffle_IsEnabled(void) {
    return enabled;
}

void Shuffle_Reseed(int current_index) {
    build_bag(current_index);
}

int Shuffle_Next(void) {
    if (bag_count <= 0) return 0;
    bag_pos++;
    if (bag_pos >= bag_count) {
        build_bag(-1); /* new round: reshuffle, start over (-1 never matches an index) */
    }
    return bag[bag_pos];
}

int Shuffle_Prev(void) {
    if (bag_count <= 0) return 0;
    bag_pos--;
    if (bag_pos < 0) bag_pos = bag_count - 1;
    return bag[bag_pos];
}
