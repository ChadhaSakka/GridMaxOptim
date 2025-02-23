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

#include <stdio.h>  // printf, fopen etc.
#include <stdlib.h> // atoi, qsort, malloc etc.

// Abstract values, entry for a values_array_t array
typedef struct {
   float v1, v2;
} value_t;

// Dynamic array of values
typedef struct {
   unsigned nx, ny;  // number of values along X, Y
   value_t **entries; // array of pointers to values (flat)
} value_grid_t;

// Structure to relate values and position in the grid
typedef struct {
   unsigned x, y; // position in the 2D grid
   float v1, v2;
} pos_val_t;

// Dynamic array of pos_val_t entries
typedef struct {
   unsigned nx, ny; // number of values along X, Y
   pos_val_t **entries; // array of pointers to pos_val_t structures (flat)
} pos_val_grid_t;

size_t sum_bytes;       // Cumulated sum of allocated bytes (malloc, realloc)

// Pseudo-randomly generates 'n' values and writes them to a text file
int generate_random_values (const char *file_name, unsigned nx, unsigned ny)
{
   printf ("Generate %u x %u values and dump them to %s...\n", nx, ny, file_name);

   // Open/create output file
   FILE *fp = fopen (file_name, "w");
   if (!fp) {
      fprintf (stderr, "Cannot write %s\n", file_name);
      return -1;
   }

   // Save nx and ny on the first line
   if (fprintf (fp, "%u %u\n", nx, ny) <= 0)
      return -2;

   // Generate values (one per line)
   unsigned i, j;
   for (i=0; i<nx; i++) {
      for (j=0; j<ny; j++) {
         const float v1 = (float) rand() / RAND_MAX;
         const float v2 = (float) rand() / RAND_MAX;

         if (fprintf (fp, "%lf %lf\n", v1, v2) <= 0)
            return -2;
      }
   }

   // Close output file
   fclose (fp);

   return 0;
}

// Loads values from a file written by generate_random_values() to the grid
int load_values (const char *file_name, value_grid_t *val_grid)
{
   printf ("Load values from %s...\n", file_name);

   // Open input file (containing one coordinate per line)
   FILE *fp = fopen (file_name, "r");
   if (!fp) {
      fprintf (stderr, "Cannot read %s\n", file_name);
      return -1;
   }

   char buf [100];

   // Load grid size from input file (first line)
   unsigned nx, ny;
   if (fgets (buf, sizeof buf, fp) != NULL &&
       sscanf (buf, "%u %u", &nx, &ny) != 2) {
      fprintf (stderr, "Failed to parse the first line from the input file\n");
      fclose (fp);
      return 1;
   }

   // Update output array length
   val_grid->nx = nx;
   val_grid->ny = ny;

   // Initialize output array as empty
   val_grid->entries = NULL;

   // Load pairs from input file (one per line)
   unsigned nb_inserted_values = 0;
   while (fgets (buf, sizeof buf, fp) != NULL) {
      // Parse current line (v1, v2)
      float v1, v2;
      if (sscanf (buf, "%f %f", &v1, &v2) != 2) {
         fprintf (stderr, "Failed to parse a line from the input file\n");
         fclose (fp);
         return 1;
      }

      // Create a new pair: allocate memory and set it from the current line
      value_t *new_value = malloc (sizeof *new_value);
      sum_bytes += sizeof *new_value;
      new_value->v1 = v1;
      new_value->v2 = v2;

      // Enlarge (reallocate) the output array to save one more point
      val_grid->entries = realloc (val_grid->entries,
                                   (nb_inserted_values + 1) * sizeof val_grid->entries[0]);
      sum_bytes += sizeof val_grid->entries[0];

      // Append the new point (pointer to) to the output array
      val_grid->entries [nb_inserted_values++] = new_value;
   }

   // Close input file
   fclose (fp);

   if (nb_inserted_values != (nx * ny)) {
      fprintf (stderr, "Mismatch between the number of parsed values (%u) and the grid size (%u x %u)\n",
               nb_inserted_values, nx, ny);
      return 1;
   }

   return 0;
}

// Relate pairs to coordinates
void load_positions (value_grid_t src, pos_val_grid_t *dst)
{
   dst->nx = src.nx;
   dst->ny = src.ny;

   sum_bytes += src.nx * src.ny * sizeof dst->entries[0];
   dst->entries = malloc (src.nx * src.ny * sizeof dst->entries[0]);

   unsigned i, j;
   for (i=0; i<src.nx; i++) {
      for (j=0; j<src.ny; j++) {
         const value_t *src_val = src.entries [i * src.ny + j];

         pos_val_t *new_pos_val = malloc (sizeof new_pos_val[0]);
         sum_bytes += sizeof *new_pos_val;
         new_pos_val->x  = i;
         new_pos_val->y  = j;
         new_pos_val->v1 = src_val->v1;
         new_pos_val->v2 = src_val->v2;

         dst->entries [i * src.ny + j] = new_pos_val;
      }
   }
}

// Compares two pos_val entries (to be used with qsort)
int cmp_pv_entries_v1 (const void *a, const void *b)
{
   const pos_val_t *pva = *((const pos_val_t **) a);
   const pos_val_t *pvb = *((const pos_val_t **) b);

   const float a_v1 = pva->v1;
   const float b_v1 = pvb->v1;

   if (a_v1 < b_v1) return -1;
   if (a_v1 > b_v1) return 1;
   return 0;
}

// Compares two pos_val entries (to be used with qsort)
int cmp_pv_entries_v2 (const void *a, const void *b)
{
   const pos_val_t *pva = *((const pos_val_t **) a);
   const pos_val_t *pvb = *((const pos_val_t **) b);

   const float a_v2 = pva->v2;
   const float b_v2 = pvb->v2;

   if (a_v2 < b_v2) return -1;
   if (a_v2 > b_v2) return 1;
   return 0;
}

// Computes maximum v1+v2 (and save related points) from a row of pairs
pos_val_t *find_max_v1 (const pos_val_grid_t *pv_grid)
{
   printf ("Compute maximum v1...\n");

   qsort (pv_grid->entries, pv_grid->nx * pv_grid->ny,
          sizeof (pv_grid->entries[0]), cmp_pv_entries_v1);

   return pv_grid->entries [pv_grid->nx * pv_grid->ny - 1];
}

// Computes maximum v1+v2 (and save related points) from a row of pairs
pos_val_t *find_max_v2 (const pos_val_grid_t *pv_grid)
{
   printf ("Compute maximum v2...\n");

   qsort (pv_grid->entries, pv_grid->nx * pv_grid->ny,
          sizeof (pv_grid->entries[0]), cmp_pv_entries_v2);

   return pv_grid->entries [pv_grid->nx * pv_grid->ny - 1];
}

// Frees memory that was allocated to save distances
void free_pos_val_grid (pos_val_grid_t pv_grid)
{
   printf ("Free memory allocated for positions+values (%u x %u entries)...\n",
           pv_grid.nx, pv_grid.ny);

   unsigned i, j;
   for (i=0; i<pv_grid.nx; i++) {
      for (j=0; j<pv_grid.ny; j++) {
         free (pv_grid.entries [i * pv_grid.ny + j]);
         sum_bytes -= sizeof *(pv_grid.entries[0]);
      }
   }

   free (pv_grid.entries);
   sum_bytes -= pv_grid.nx * pv_grid.ny * sizeof pv_grid.entries[0];
}

// Frees memory that was allocated to save points/coordinates
void free_value_grid (value_grid_t val_grid)
{
   printf ("Free memory allocated for coordinates (%u x %u entries)...\n",
           val_grid.nx, val_grid.ny);

   unsigned i, j;
   for (i=0; i<val_grid.nx; i++) {
      for (j=0; j<val_grid.ny; j++) {
         free (val_grid.entries[i * val_grid.ny + j]);
         sum_bytes -= sizeof *(val_grid.entries[0]);
      }
   }

   free (val_grid.entries);
   sum_bytes -= val_grid.nx * val_grid.ny * sizeof val_grid.entries[0];
}

// Program entry point: CF comments on top of this file + README
int main (int argc, char *argv[])
{
   // Check arguments number
   if (argc < 4) {
      fprintf (stderr, "Usage: %s <nb repetitions> <nb points X> <nb points Y>\n", argv[0]);
      return EXIT_FAILURE;
   }

   // Read arguments from command line
   unsigned nrep = (unsigned) atoi (argv[1]);
   unsigned nx   = (unsigned) atoi (argv[2]);
   unsigned ny   = (unsigned) atoi (argv[3]);

   // Generate points and save them to a text file named "values.txt"
   const char *input_file_name = "values.txt";
   if (generate_random_values (input_file_name, nx, ny) != 0) {
      fprintf (stderr, "Failed to write %u x %u coordinates to %s\n",
               nx, ny, input_file_name);
      return EXIT_FAILURE;
   }

   sum_bytes = 0;

   // Main part: repeated nrep times
   unsigned r;
   for (r=0; r<nrep; r++) {
      value_grid_t value_grid;
      pos_val_grid_t pos_val_grid;

      // Load coordinates from disk to memory
      if (load_values (input_file_name, &value_grid) != 0) {
         fprintf (stderr, "Failed to load coordinates\n");
         return EXIT_FAILURE;
      }

      // Relate pairs to coordinates
      load_positions (value_grid, &pos_val_grid);

      // Compute maximum (v1 and v2)
      const pos_val_t *pos_v1_max = find_max_v1 (&pos_val_grid);
      const pos_val_t *pos_v2_max = find_max_v2 (&pos_val_grid);

      // Print maximum (v1 and v2)
      printf ("Max v1: x=%u, y=%u, v1=%f\n",
              pos_v1_max->x, pos_v1_max->y, pos_v1_max->v1);
      printf ("Max v2: x=%u, y=%u, v2=%f\n",
              pos_v2_max->x, pos_v2_max->y, pos_v2_max->v2);

      // Free allocated memory
      free_pos_val_grid (pos_val_grid);
      free_value_grid (value_grid);
   }

   // Delete text file
   remove (input_file_name);

   return EXIT_SUCCESS;
}
