#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

/*
 * exact(): Evaluates the exact solution u(x) = x*(x-1)*exp(x) of the continuous
 */
double exact(double x)
{
    return x * (x - 1.0) * exp(x);
}

/*
 * force(): Evaluates the forcing function f(x) = -x*(x+3)*exp(x).
 */
double force(double x)
{
    return -x * (x + 3.0) * exp(x);
}

/* =========================================================================
 * RESIDUAL COMPUTATION
 * ========================================================================= */

double compute_rms_residual(double *u, double *f, int N, double h)
{
    double h2     = h * h;           /* h²                             */
    double inv_h2 = 1.0 / h2;       /* 1/h² — the unscaled coefficient */
    double sum_sq = 0.0;
    double r_j;
    int j;

    for (j = 0; j < N; j++)
    {
        if (j == 0 || j == N - 1)
        {
            /*
             * Boundary equation: u[j] = 0.
             * The row of A is [ 0 ... 1 ... 0 ] and f[j]=0, so
             * residual = u[j] - f[j] = 0. We include it anyway so the
             * RMS is computed over all N equations, matching the PDF formula.
             */
            r_j = u[j] - f[j];
        }
        else
        {
            /*
             * Interior equation (unscaled form):
             *   (A*u)[j] = (-u[j-1] + 2*u[j] - u[j+1]) / h²
             * residual  = (A*u)[j] - f[j]
             */
            r_j = (-u[j-1] + 2.0*u[j] - u[j+1]) * inv_h2 - f[j];
        }

        sum_sq += r_j * r_j;
    }

    return sqrt(sum_sq / (double)N);
}

/* =========================================================================
 * SHARED DATA
 * All threads receive a pointer to this single struct so they can access
 * the solution arrays, convergence state, and synchronisation primitives.
 * Mirrors the ThreadArgs approach in ThreadsMatrixSolver but extended with
 * barriers, because Jacobi requires multiple synchronised phases per sweep
 * rather than a single independent work unit per thread.
 * ========================================================================= */

typedef struct
{
    /* problem data — read-only after initialisation */
    int     N;
    double  h2;          /* h²                          */
    double  inv_h2;      /* 1/h²                        */
    double *u;           /* current solution vector     */
    double *u_old;       /* previous iterate            */
    double *f;           /* right-hand side / forcing   */
    double  tol;
    int     max_iter;

    /* convergence state — written only by the serial thread */
    int     it;
    int     converged;
    double  rms;

    /* per-thread partial residual sums, one entry per thread.
     * Each thread writes only its own slot; the serial thread reads all. */
    int     num_threads;
    double *partial_sum_sq;

    /* four barriers, one per phase of each Jacobi sweep:
     *   barrier_copy     — all copies done before any sweep begins
     *   barrier_sweep    — all sweeps done before residual begins
     *   barrier_residual — all partial sums written; serial thread reduces
     *   barrier_check    — converged flag written before any thread re-loops */
    pthread_barrier_t barrier_copy;
    pthread_barrier_t barrier_sweep;
    pthread_barrier_t barrier_residual;
    pthread_barrier_t barrier_check;
} SharedData;

/* =========================================================================
 * THREAD ARGUMENT STRUCT
 * Follows the ThreadsMatrixSolver pattern: one struct instance per thread,
 * carrying the thread's work range and a pointer to the shared data.
 * ========================================================================= */

typedef struct
{
    int         thread_id;  /* index into partial_sum_sq array  */
    int         start_j;    /* first interior node for this thread (inclusive) */
    int         end_j;      /* last interior node for this thread  (exclusive) */
    SharedData *shared;
} ThreadArgs;

/* =========================================================================
 * JACOBI WORKER FUNCTION
 * Each thread executes four phases per iteration, synchronised by barriers:
 *
 *   Phase 1 — copy:     u_old[j] = u[j]  for owned nodes
 *   Phase 2 — sweep:    u[j] = (f[j]*h² + u_old[j-1] + u_old[j+1]) / 2
 *   Phase 3 — residual: accumulate partial sum_sq for owned nodes
 *   Phase 4 — check:    serial thread reduces, sets converged; all read it
 *
 * The barriers guarantee that:
 *   - no thread begins the sweep before all copies are done
 *     (prevents reading a neighbour's stale u_old value)
 *   - no thread begins the residual before all sweeps are done
 *     (prevents reading a neighbour's not-yet-updated u value)
 *   - no thread reads s->converged before the serial thread has written it
 * ========================================================================= */

void* jacobi_worker(void *arg)
{
    ThreadArgs *t = (ThreadArgs *)arg;
    SharedData *s = t->shared;
    int j;

    while (1)
    {
        /* ── Phase 1: copy u -> u_old for this thread's range ──
         *
         * Boundary nodes (j=0 and j=N-1) are seeded once before the thread
         * loop and never modified by the sweep, so they never need re-copying.
         */
        for (j = t->start_j; j < t->end_j; j++)
            s->u_old[j] = s->u[j];

        /* All threads must finish copying before any begins the sweep.
         * Without this barrier a thread could read a neighbour's u_old[j]
         * that the neighbour has not yet copied, producing wrong results. */
        pthread_barrier_wait(&s->barrier_copy);

        /* ── Jacobi sweep: update every interior node ── */
        for (j = t->start_j; j < t->end_j; j++)
        {
            /*
             * Solve the j-th equation for u[j]:
             *   u_new[j] = (f[j]*h² + u_old[j-1] + u_old[j+1]) / 2
             *
             * Boundary nodes (j=0 and j=N-1) are never touched;
             * they stay at zero throughout the entire iteration.
             *
             * Reads of u_old[start_j-1] and u_old[end_j] (neighbours across
             * thread boundaries) are safe because barrier_copy guarantees
             * those slots were filled by their owners before we arrived here.
             */
            s->u[j] = (s->f[j] * s->h2 + s->u_old[j-1] + s->u_old[j+1]) / 2.0;
        }

        /* All threads must finish the sweep before any begins the residual.
         * Without this barrier a thread could read a neighbour's u[j] that
         * the neighbour has not yet updated, corrupting the residual. */
        pthread_barrier_wait(&s->barrier_sweep);

        /* ── Compute RMS residual with the new iterate ──
         *
         * Each thread accumulates the squared residual for its own nodes and
         * stores the result in its private slot of partial_sum_sq.
         * The serial thread will sum all slots after the next barrier.
         */
        {
            double sum_sq = 0.0;
            double r_j;
            for (j = t->start_j; j < t->end_j; j++)
            {
                /*
                 * Interior equation (unscaled form):
                 *   (A*u)[j] = (-u[j-1] + 2*u[j] - u[j+1]) / h²
                 * residual  = (A*u)[j] - f[j]
                 */
                r_j    = (-s->u[j-1] + 2.0*s->u[j] - s->u[j+1]) * s->inv_h2 - s->f[j];
                sum_sq += r_j * r_j;
            }
            s->partial_sum_sq[t->thread_id] = sum_sq;
        }

        /* pthread_barrier_wait returns PTHREAD_BARRIER_SERIAL_THREAD for
         * exactly one thread. That designated thread performs the global
         * reduction, increments the iteration counter, and sets the
         * convergence flag. All other threads return 0 and proceed directly
         * to barrier_check where they will wait for the flag to be written. */
        int rc = pthread_barrier_wait(&s->barrier_residual);
        if (rc == PTHREAD_BARRIER_SERIAL_THREAD)
        {
            /* Sum the boundary residual contributions (always zero here since
             * u[0]=u[N-1]=0 and f[0]=f[N-1]=0, but kept for correctness with
             * the PDF formula which accounts for all N equations).
             *
             * Boundary equation: u[j] = 0.
             * The row of A is [ 0 ... 1 ... 0 ] and f[j]=0, so
             * residual = u[j] - f[j] = 0. We include it anyway so the
             * RMS is computed over all N equations, matching the PDF formula.
             */
            double total = 0.0;
            double r0    = s->u[0]       - s->f[0];
            double rN    = s->u[s->N-1]  - s->f[s->N-1];
            total += r0 * r0 + rN * rN;

            /* Sum all per-thread contributions */
            for (int t2 = 0; t2 < s->num_threads; t2++)
                total += s->partial_sum_sq[t2];

            s->rms = sqrt(total / (double)s->N);
            s->it++;

            /* ── Check convergence ── */

          if (s->rms <= s->tol)
            {
                //printf("\n  Converged after %d iterations (RMS = %.4e <= tol = %.4e)\n",
                  //     s->it, s->rms, s->tol);
                s->converged = 1;
            }

        }

        /* All threads wait here until the serial thread has finished writing
         * s->converged. Only after this barrier is it safe to read it. */
        pthread_barrier_wait(&s->barrier_check);

        if (s->converged) break;
    }

    return NULL;
}

/* =========================================================================
 * JACOBI ITERATION
 * ========================================================================= */

int jacobi(double *u, double *f, int N, double h, double tol, int max_iter,
           int num_threads)
{
    double h2;            /* h²                              */

    h2 = h * h;

    /*
     * Allocate a working array for the "old" values.
     * At each sweep we copy u -> u_old first, then compute all new u[j]
     * from u_old. This ensures every update uses only pre-sweep values.
     */
    double *u_old = (double *)malloc(N * sizeof(double));
    if (!u_old)
    {
        fprintf(stderr, "ERROR: could not allocate u_old buffer.\n");
        exit(1);
    }

    /* Boundary nodes are fixed at 0 and never modified by the sweep.
     * Seeding them once here means they never need to be re-copied. */
    u_old[0]     = u[0];
    u_old[N - 1] = u[N - 1];

    /*
    printf("\n");
    printf("  %8s  %14s  %14s\n", "Step", "RMS residual", "Change norm");
    printf("  %8s  %14s  %14s\n", "--------", "--------------", "--------------");
    */ 

    /* ── FIX 1: cap threads to number of interior nodes ──
     * Mirrors ThreadsMatrixSolver's cap of threads to number of rows.
     * Having more threads than interior nodes would leave some threads with
     * nothing to do while still consuming barrier slots. */
    int interior = N - 2;
    if (num_threads > interior) num_threads = interior;

    /* ── Populate shared data ── */
    SharedData shared;
    shared.N            = N;
    shared.h2           = h2;
    shared.inv_h2       = 1.0 / h2;
    shared.u            = u;
    shared.u_old        = u_old;
    shared.f            = f;
    shared.tol          = tol;
    shared.max_iter     = max_iter;
    shared.it           = 0;
    shared.converged    = 0;
    shared.rms          = 0.0;
    shared.num_threads  = num_threads;

    /* ── FIX 2: heap-allocate thread arrays to avoid stack VLAs ──
     * Mirrors ThreadsMatrixSolver's approach. For large num_threads a VLA
     * on the stack could silently overflow it. */
    shared.partial_sum_sq = (double *)malloc(num_threads * sizeof(double));
    if (!shared.partial_sum_sq)
    {
        fprintf(stderr, "ERROR: could not allocate partial_sum_sq.\n");
        exit(1);
    }

    pthread_t  *threads = (pthread_t  *)malloc(num_threads * sizeof(pthread_t));
    ThreadArgs *args    = (ThreadArgs *)malloc(num_threads * sizeof(ThreadArgs));
    if (!threads || !args)
    {
        fprintf(stderr, "ERROR: could not allocate thread arrays.\n");
        exit(1);
    }

    /* Initialise all four barriers; each requires exactly num_threads
     * participants to release — one arrival per worker thread per phase. */
    pthread_barrier_init(&shared.barrier_copy,     NULL, num_threads);
    pthread_barrier_init(&shared.barrier_sweep,    NULL, num_threads);
    pthread_barrier_init(&shared.barrier_residual, NULL, num_threads);
    pthread_barrier_init(&shared.barrier_check,    NULL, num_threads);

    /* ── Distribute interior nodes among threads ──
     * Interior nodes run from j=1 to j=N-2 (total: N-2 nodes).
     * Remainder nodes (interior % num_threads) are handed to the first
     * threads one by one so no thread has more than one extra node. */
    int base_chunk  = interior / num_threads;
    int remainder   = interior % num_threads;
    int current     = 1; /* first interior node index */

    for (int t = 0; t < num_threads; t++)
    {
        int chunk       = base_chunk + (t < remainder ? 1 : 0);
        args[t].thread_id = t;
        args[t].start_j   = current;
        args[t].end_j     = current + chunk;
        args[t].shared    = &shared;
        current          += chunk;

        /* ── FIX 3: check pthread_create return value ──
         * Mirrors ThreadsMatrixSolver's error check on thread creation. */
        int rc = pthread_create(&threads[t], NULL, jacobi_worker, &args[t]);
        if (rc != 0)
        {
            fprintf(stderr, "pthread_create failed: %d\n", rc);
            exit(1);
        }
    }

    /* Wait for all threads to complete */
    for (int t = 0; t < num_threads; t++)
        pthread_join(threads[t], NULL);

    /* Destroy barriers and free all heap-allocated thread structures */
    pthread_barrier_destroy(&shared.barrier_copy);
    pthread_barrier_destroy(&shared.barrier_sweep);
    pthread_barrier_destroy(&shared.barrier_residual);
    pthread_barrier_destroy(&shared.barrier_check);

    free(shared.partial_sum_sq);
    free(threads);
    free(args);
    free(u_old);

    return shared.it;
}

/* =========================================================================
 * MAIN DRIVER
 * ========================================================================= */

int main(int argc, char *argv[])
{
    /*
     * Parse the grid index k from the command line.
     * k determines the grid size: N = 2^k + 1
     */

    struct timespec start, end; /* Wall-clock snapshots */
    clock_gettime(CLOCK_MONOTONIC, &start);

    int k           = atoi(argv[1]);
    int num_threads = atoi(argv[2]); /* number of worker threads */
  
  /* if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <k>\n", argv[0]);
        fprintf(stderr, "  k = grid index, N = 2^k + 1 nodes.\n");
        fprintf(stderr, "  Example: %s 5   (reproduces PDF sample with N=33)\n", argv[0]);
        return 1;
    }*/ 

    k = atoi(argv[1]);
    if (k < 0)
    {
        fprintf(stderr, "ERROR: k must be >= 0.\n");
        return 1;
    }

    /* ── Grid parameters ── */
    int    N  = (1 << k) + 1;    /* N = 2^k + 1 (bitshift is exact for integer powers of 2) */
    double a  = 0.0;              /* left endpoint                                            */
    double b  = 1.0;              /* right endpoint                                           */
    double ua = 0.0;              /* Dirichlet BC at x=a                                      */
    double ub = 0.0;              /* Dirichlet BC at x=b                                      */
    double h  = (b - a) / (double)(N - 1); /* mesh spacing h = 1/(N-1)                       */

    /*
     * Convergence tolerance on the RMS residual.
     * The PDF uses tol = 1e-6 for the sample calculation (Section 10)
     * and tol = 1e-10 for the convergence study table (Section 11).
     * We use 1e-6 here to match the sample output.
     */
    double tol      = 1.0e-6;
    int    max_iter = 10000000;   /* safety cap — not a convergence criterion */

    /* ── Allocate solution and RHS vectors ── */
    double *x  = (double *)malloc(N * sizeof(double)); /* grid node positions */
    double *f  = (double *)malloc(N * sizeof(double)); /* RHS / forcing       */
    double *u  = (double *)malloc(N * sizeof(double)); /* Jacobi solution     */
    double *ue = (double *)malloc(N * sizeof(double)); /* exact solution      */
    /*
    if (!x || !f || !u || !ue)
    {
        //fprintf(stderr, "ERROR: memory allocation failed.\n");
        return 1;
    }
    */
    /* ── Build the grid ── */
    /*
     * Node j is at x[j] = a + j*h, for j = 0, 1, ..., N-1.
     * Using the formula from Section 3 of the PDF (with 0-based indexing):
     *   x[j] = ((N-1-j)*a + j*b) / (N-1)
     * which simplifies to x[j] = j*h when a=0, b=1.
     */
    int j;
    for (j = 0; j < N; j++)
        x[j] = ((double)(N - 1 - j) * a + (double)j * b) / (double)(N - 1);

    /* ── Evaluate the forcing function at every node ── */
    /*
     * For interior nodes, f[j] = force(x[j]).
     * For boundary nodes the equation is just u = BC value, so we set
     * f[0] = ua and f[N-1] = ub. The Jacobi loop never writes to u[0]
     * or u[N-1], so the boundary values are preserved automatically.
     */
    f[0]     = ua;
    f[N - 1] = ub;
    for (j = 1; j < N - 1; j++)
        f[j] = force(x[j]);

    /* ── Evaluate the exact solution for later comparison ── */
    for (j = 0; j < N; j++)
        ue[j] = exact(x[j]);

    /* ── Initialize the Jacobi iterate to zero ── */
    /*
     * u⁰ = 0 is the natural starting point when no better guess exists
     * (Section 8 of the PDF). The boundary values u[0]=ua=0 and
     * u[N-1]=ub=0 are already correct and will never be overwritten.
     */
    for (j = 0; j < N; j++)
        u[j] = 0.0;

    /* ── Print problem summary ── */
    
    /*
    printf("\n");
    printf("JACOBI_POISSON_1D\n");
    printf("  Jacobi iterative solution of the 1D Poisson equation.\n");
    printf("\n");
    printf("  Grid index k      = %d\n",   k);
    printf("  Number of nodes N = %d\n",   N);
    printf("  Mesh spacing h    = %g\n",   h);
    printf("  Interval          = [%g, %g]\n", a, b);
    printf("  Boundary values   = u(%g)=%g,  u(%g)=%g\n", a, ua, b, ub);
    printf("  RMS residual tol  = %g\n",   tol);
    */

    /* ── Print summary statistics ── */
    double rms_jacobi_vs_exact  = 0.0;
    for (j = 0; j < N; j++)
    {
        rms_jacobi_vs_exact  += (u[j]  - ue[j]) * (u[j]  - ue[j]);
    }
    rms_jacobi_vs_exact  = sqrt(rms_jacobi_vs_exact  / (double)N);
  
  /* ── Run the Jacobi iteration ── */
    int it_num = jacobi(u, f, N, h, tol, max_iter, num_threads);

    /*
    printf("\n");
    printf("  Using grid index k = %d\n",             k);
    printf("  System size N      = %d\n",             N);
    printf("  Tolerance          = %g\n",             tol);
    printf("  Jacobi iterations  = %d\n",             it_num);
    printf("\n");
    printf("  RMS error  |u_jacobi  - u_exact|  = %g\n", rms_jacobi_vs_exact);
    */
    /* ── Free all allocated memory ── */
    free(x);
    free(f);
    free(u);
    free(ue);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed = (end.tv_sec  - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("%.6f,%d,", elapsed, it_num);

    return 0;
}