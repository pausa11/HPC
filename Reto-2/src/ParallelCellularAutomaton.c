#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>


static void init_road(int *road, int n, double density, unsigned int *seed)
{
    int    i;
    double r;

    for (i = 0; i < n; i++) {
        r = (double)rand_r(seed) / ((double)RAND_MAX + 1.0);
        road[i] = (r < density) ? 1 : 0;k
    }
}

/* -------------------------------------------------------------------------
 * simulate
 *
 * road and road_new are private to the calling thread (allocated per-thread
 * in main), so pointer swaps inside this function are safe.
 *
 * OPTION 1 is applied to:
 *   (a) the car-count loop
 *   (b) the warmup cell loop
 *   (c) the measurement cell loop  (with a reduction on moved_this_step)
 *
 * All three inner loops read only from `road` (old state) and write only to
 * `road_new` (new state), so there are no write-write or read-write conflicts
 * between iterations — parallelism is safe.
 * ------------------------------------------------------------------------- */
static double simulate(int *road, int *road_new, int n,
                       int warmup, int measure)
{
    int    step, i;
    int    left, right;
    int    total_cars  = 0;
    long   moved_total = 0;
    int   *tmp;

    /* ---- Count total cars -------------------------------------------- */
    /* OPTION 1: parallel reduction — each thread sums its chunk of cells  */
    #pragma omp parallel for \
        private(i) \
        reduction(+:total_cars) \
        schedule(static)
    for (i = 0; i < n; i++) {
        total_cars += road[i];
    }

    if (total_cars == 0) return 1.0;

    /* ------------------------------------------------------------------ */
    /* WARM-UP phase (Option 1 applied)                                    */
    /* ------------------------------------------------------------------ */
    for (step = 0; step < warmup; step++) {

        /*
         * OPTION 1: each thread handles a contiguous chunk of cells.
         * Reads are from road[] (old), writes go to road_new[] (new).
         * Neighbours i-1 and i+1 are read-only, so no conflict even at
         * chunk boundaries.
         */
        #pragma omp parallel for \
            private(i, left, right) \
            schedule(static)
        for (i = 0; i < n; i++) {
            left  = (i - 1 + n) % n;
            right = (i + 1)     % n;

            if (road[i] == 0) {
                road_new[i] = road[left];
            } else {
                road_new[i] = road[right];
            }
        }
        /* Pointer swap is done by a single thread after the parallel region */
        tmp      = road;
        road     = road_new;
        road_new = tmp;
    }

    /* ------------------------------------------------------------------ */
    /* MEASUREMENT phase (Option 1 applied)                                */
    /* ------------------------------------------------------------------ */
    for (step = 0; step < measure; step++) {
        int moved_this_step = 0;

        /*
         * OPTION 1: reduction(+:moved_this_step) lets each thread
         * accumulate its local count of moved cars privately, then
         * OpenMP sums them all into moved_this_step after the loop.
         * This avoids any atomic or critical section overhead.
         */
        #pragma omp parallel for \
            private(i, left, right) \
            reduction(+:moved_this_step) \
            schedule(static)
        for (i = 0; i < n; i++) {
            left  = (i - 1 + n) % n;
            right = (i + 1)     % n;

            if (road[i] == 0) {
                road_new[i] = road[left];
            } else {
                road_new[i] = road[right];
                if (road[right] == 0) {
                    moved_this_step++;   /* private per-thread, reduced after */
                }
            }
        }

        moved_total += moved_this_step;

        tmp      = road;
        road     = road_new;
        road_new = tmp;
    }

    return (double)moved_total / ((double)total_cars * (double)measure);
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(int argc, char *argv[])
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <N> <density_steps>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n             = atoi(argv[1]);
    int density_steps = atoi(argv[2]);

    int warmup_steps  = n / 10;
    int measure_steps = n / 10;

    unsigned int master_seed = (unsigned int)time(NULL);

    /*
     * NESTED PARALLELISM SETUP:
     *
     * Enable nested parallel regions so that Option 1 (inner cell loop)
     * can run inside Option 2 (outer density loop).
     *
     * We explicitly set the inner level to use 1 thread to avoid spawning
     * T*T threads on a T-core machine. If you only want Option 1 (and want
     * cells parallelised with all cores), set outer threads to 1 and inner
     * threads to omp_get_max_threads().
     */
    omp_set_nested(1);
    omp_set_max_active_levels(2);   /* allow 2 levels of parallel nesting */

    int outer_threads = omp_get_max_threads();  /* Option 2 uses all cores  */
    int inner_threads = 1;                       /* Option 1: 1 per outer thread
                                                    to avoid over-subscription */

    /*
     * ----------------------------------------------------------------
     * OPTION 2 — Parallel density loop
     *
     * Each thread gets:
     *   - Its own road and road_new arrays (no sharing, no false sharing)
     *   - Its own seed derived from master_seed + d  (reproducible and
     *     independent — different d values give different random sequences)
     *   - Its own local copies of density and avg_vel
     *
     * schedule(dynamic) is used because different density values converge
     * at different rates, so workload per iteration is unequal. Dynamic
     * scheduling lets faster threads pick up the next available density
     * value rather than waiting at a static boundary.
     * ----------------------------------------------------------------
     */
    #pragma omp parallel for \
        schedule(dynamic) \
        num_threads(outer_threads) \
        default(none) \
        shared(n, density_steps, warmup_steps, measure_steps, master_seed, inner_threads, stderr)
    for (int d = 0; d <= density_steps; d++) {

        /* ---- Per-thread private state -------------------------------- */

        /*
         * Each thread allocates its own road arrays on the heap.
         * Stack allocation would risk stack overflow for large N.
         */
        int *road     = (int *)malloc(n * sizeof(int));
        int *road_new = (int *)malloc(n * sizeof(int));

        if (!road || !road_new) {
            fprintf(stderr, "Thread %d: malloc failed for d=%d\n",
                    omp_get_thread_num(), d);
            free(road);
            free(road_new);
            /* Cannot return inside an OpenMP parallel for; skip iteration */
            continue;
        }

        /*
         * Derive a unique seed from master_seed and d.
         * Adding d ensures every density point starts from a different
         * random sequence, making results independent across threads.
         */
        unsigned int seed = master_seed + (unsigned int)d;

        double density = (double)d / (double)density_steps;
        double avg_vel;

        /* Set the number of threads for the nested Option 1 regions */
        omp_set_num_threads(inner_threads);

        /* Initialise road */
        init_road(road, n, density, &seed);

        /* Run simulation (Option 1 parallelism lives inside here) */
        avg_vel = simulate(road, road_new, n, warmup_steps, measure_steps);

        /* Uncomment to print results:
         * printf("%.6f  %.6f\n", density, avg_vel);
         */
        (void)avg_vel;  /* suppress unused-variable warning when print is off */

        free(road);
        free(road_new);
    }
    /* All density values finished — implicit barrier here */

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec  - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.6f,", elapsed);

    return EXIT_SUCCESS;
}