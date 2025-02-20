/* Author: Emmanuel OSERET, University of Versailles Saint-Quentin-en-Yvelines, France
 * This program pseudo-randomly generates pairs of values (v1,v2) in a 2D grid
 * and then computes and prints maximum (v1) and maximum (v2) and x-y position
 * Execution:
 * - Usage: ./exe <nb repetitions> <nb points X> <nb points Y>
 * - Nb repetitions: number of times the experiment is repeated (from file loading to memory release), allows to increase runtime for more precise sampling-based profiling
 * - Nb points X: input size along X
 * - Nb points Y: input size along Y
 * - Recommended: with the baseline implementation, good starting point is 2000 x 3000
 */
/* Author: Emmanuel OSERET, University of Versailles Saint-Quentin-en-Yvelines, France
 * Optimized version: Removed unnecessary malloc calls using flat arrays.
 * Usage: ./exe <nb repetitions> <nb points X> <nb points Y>
 */

#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h> // For AVX2/SIMD intrinsics (optional, remove if not needed)

// Abstract values
typedef struct {
    float v1, v2;
} value_t;

// Dynamic array of values (flat)
typedef struct {
    unsigned nx, ny;  // number of values along X, Y
    value_t *entries; // Flat array (no pointers to pointers)
} value_grid_t;

// Structure to relate values and position
typedef struct {
    unsigned x, y; // position in the 2D grid
    float v1, v2;
} pos_val_t;

// Dynamic array of pos_val_t entries (flat)
typedef struct {
    unsigned nx, ny; // number of values along X, Y
    pos_val_t *entries; // Flat array (no pointers to pointers)
} pos_val_grid_t;

size_t sum_bytes = 0; // Cumulated sum of allocated bytes

// Buffered pseudo-random value generation
int generate_random_values(const char *file_name, unsigned nx, unsigned ny) {
    printf("Generate %u x %u values and dump them to %s...\n", nx, ny, file_name);

    FILE *fp = fopen(file_name, "w");
    if (!fp) {
        fprintf(stderr, "Cannot write %s\n", file_name);
        return -1;
    }

    // Write grid size
    if (fprintf(fp, "%u %u\n", nx, ny) <= 0)
        return -2;

    char buffer[1024 * 1024]; // 1MB buffer
    size_t buf_pos = 0;

    for (unsigned i = 0; i < nx; i++) {
        for (unsigned j = 0; j < ny; j++) {
            float v1 = rand() / (float)RAND_MAX;
            float v2 = rand() / (float)RAND_MAX;
            buf_pos += sprintf(buffer + buf_pos, "%f %f\n", v1, v2);
            if (buf_pos > sizeof(buffer) - 100) {
                fwrite(buffer, 1, buf_pos, fp);
                buf_pos = 0;
            }
        }
    }
    if (buf_pos > 0) {
        fwrite(buffer, 1, buf_pos, fp);
    }

    fclose(fp);
    return 0;
}

// Load values with flat array, removing per-element malloc
int load_values(const char *file_name, value_grid_t *val_grid) {
    printf("Load values from %s...\n", file_name);

    FILE *fp = fopen(file_name, "r");
    if (!fp) {
        fprintf(stderr, "Cannot read %s\n", file_name);
        return -1;
    }

    char buf[100];
    unsigned nx = 0, ny = 0;

    // Load grid size
    if (fgets(buf, sizeof(buf), fp) != NULL && sscanf(buf, "%u %u", &nx, &ny) != 2) {
        fprintf(stderr, "Failed to parse grid size\n");
        fclose(fp);
        return 1;
    }

    val_grid->nx = nx;
    val_grid->ny = ny;

    // Allocate single flat array (aligned for vectorization)
    val_grid->entries = aligned_alloc(64, nx * ny * sizeof(value_t));
    if (!val_grid->entries) {
        fclose(fp);
        return 1;
    }
    sum_bytes += nx * ny * sizeof(value_t);

    unsigned idx = 0;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (sscanf(buf, "%f %f", &val_grid->entries[idx].v1, &val_grid->entries[idx].v2) != 2) {
            fprintf(stderr, "Failed to parse a line\n");
            fclose(fp);
            free(val_grid->entries);
            return 1;
        }
        idx++;
    }

    fclose(fp);
    if (idx != nx * ny) {
        fprintf(stderr, "Mismatch in number of values\n");
        free(val_grid->entries);
        return 1;
    }

    return 0;
}

// Load positions with flat array, removing per-element malloc
void load_positions(value_grid_t src, pos_val_grid_t *dst) {
    dst->nx = src.nx;
    dst->ny = src.ny;

    // Allocate single flat array (aligned)
    dst->entries = aligned_alloc(64, src.nx * src.ny * sizeof(pos_val_t));
    if (!dst->entries) return;
    sum_bytes += src.nx * src.ny * sizeof(pos_val_t);

    for (unsigned i = 0; i < src.nx; i++) {
        for (unsigned j = 0; j < src.ny; j++) {
            unsigned idx = i * src.ny + j;
            dst->entries[idx].x = i;
            dst->entries[idx].y = j;
            dst->entries[idx].v1 = src.entries[idx].v1;
            dst->entries[idx].v2 = src.entries[idx].v2;
        }
    }
}

// Linear max-finding for v1 (corrected for flat array)
pos_val_t *find_max_v1(const pos_val_grid_t *pv_grid) {
    printf("Compute max v1...\n");
    if (pv_grid->nx == 0 || pv_grid->ny == 0) {
        return NULL;
    }
    // Initialize max_pv as a pointer to the first element of the flat array
    pos_val_t *max_pv = &pv_grid->entries[0];
    unsigned total = pv_grid->nx * pv_grid->ny;
    for (unsigned i = 1; i < total; i++) {
        // Use . instead of -> since entries[i] is now a pos_val_t, not a pointer
        if (pv_grid->entries[i].v1 > max_pv->v1) {
            max_pv = &pv_grid->entries[i];
        }
    }
    return max_pv;
}

// Linear max-finding for v2 (corrected for flat array)
pos_val_t *find_max_v2(const pos_val_grid_t *pv_grid) {
    printf("Compute max v2...\n");
    if (pv_grid->nx == 0 || pv_grid->ny == 0) {
        return NULL;
    }
    // Initialize max_pv as a pointer to the first element of the flat array
    pos_val_t *max_pv = &pv_grid->entries[0];
    unsigned total = pv_grid->nx * pv_grid->ny;
    for (unsigned i = 1; i < total; i++) {
        // Use . instead of -> since entries[i] is now a pos_val_t, not a pointer
        if (pv_grid->entries[i].v2 > max_pv->v2) {
            max_pv = &pv_grid->entries[i];
        }
    }
    return max_pv;
}

// Free memory for flat arrays (single free per grid)
void free_pos_val_grid(pos_val_grid_t pv_grid) {
    printf("Free memory for positions+values (%u x %u entries)...\n", pv_grid.nx, pv_grid.ny);
    free(pv_grid.entries);
    sum_bytes -= pv_grid.nx * pv_grid.ny * sizeof(pos_val_t);
}

void free_value_grid(value_grid_t val_grid) {
    printf("Free memory for values (%u x %u entries)...\n", val_grid.nx, val_grid.ny);
    free(val_grid.entries);
    sum_bytes -= val_grid.nx * val_grid.ny * sizeof(value_t);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <nb repetitions> <nb points X> <nb points Y>\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned nrep = (unsigned)atoi(argv[1]);
    unsigned nx = (unsigned)atoi(argv[2]);
    unsigned ny = (unsigned)atoi(argv[3]);

    const char *input_file_name = "values.txt";
    if (generate_random_values(input_file_name, nx, ny) != 0) {
        fprintf(stderr, "Failed to generate values\n");
        return EXIT_FAILURE;
    }

    for (unsigned r = 0; r < nrep; r++) {
        value_grid_t value_grid = {0};
        pos_val_grid_t pos_val_grid = {0};

        if (load_values(input_file_name, &value_grid) != 0) {
            fprintf(stderr, "Failed to load values\n");
            return EXIT_FAILURE;
        }

        load_positions(value_grid, &pos_val_grid);

        const pos_val_t *pos_v1_max = find_max_v1(&pos_val_grid);
        const pos_val_t *pos_v2_max = find_max_v2(&pos_val_grid);

        printf("Max v1: x=%u, y=%u, v1=%f\n", pos_v1_max->x, pos_v1_max->y, pos_v1_max->v1);
        printf("Max v2: x=%u, y=%u, v2=%f\n", pos_v2_max->x, pos_v2_max->y, pos_v2_max->v2);

        free_pos_val_grid(pos_val_grid);
        free_value_grid(value_grid);
    }

    remove(input_file_name);
    printf("Total bytes allocated: %zu\n", sum_bytes);
    return EXIT_SUCCESS;
}
