
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>      /* fork(), pid_t                               */
#include <sys/wait.h>    /* wait()                                      */
#include <sys/mman.h>    /* mmap(), munmap(), MAP_SHARED, MAP_ANONYMOUS */

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
 * FORKED SWEEP
 *
 * Mirrors multiply_matrices_forked() from MultiprocessingMatrixSolver exactly:
 *   - FIX 1: cap processes to interior node count
 *   - fork num_procs children, each handles a contiguous slice of j
 *   - each child does its work and calls exit(0)
 *   - parent spawns all children then collects with wait()
 *
 * The fork/wait pattern provides all synchronisation needed without any
 * pthread or semaphore:
 *
 *   BEFORE fork: parent writes u_old completely — every child inherits a
 *                consistent snapshot via the shared mmap region.
 *
 *   DURING fork: each child writes only to its own disjoint slice of u,
 *                so there is no race between children.
 *
 *   AFTER wait:  parent has collected all children before it reads u for
 *                the residual, so u is fully updated when the parent sees it.
 *
 * u and u_old must be mmap(MAP_SHARED | MAP_ANONYMOUS) so that child writes
 * to u survive wait() and are visible to the parent. f is read-only after
 * initialisation so children can use the parent's copy-on-write malloc copy.
 * ========================================================================= */

static void forked_sweep(double *u, double *u_old, double *f,
                         int N, double h2, int num_procs)
{
    int interior = N - 2; /* interior nodes: j = 1 .. N-2 */

    /* FIX 1: cap processes to interior node count to avoid empty or
     * redundant children — mirrors MultiprocessingMatrixSolver */
    if (num_procs > interior) num_procs = interior;

    int base_chunk = interior / num_procs;
    int remainder  = interior % num_procs;
    int current    = 1; /* first interior node index */

    /* Spawn all children first, then collect — identical pattern to
     * multiply_matrices_forked() in MultiprocessingMatrixSolver */
    for (int p = 0; p < num_procs; p++)
    {
        int chunk   = base_chunk + (p < remainder ? 1 : 0);
        int start_j = current;
        int end_j   = current + chunk;
        current    += chunk;

        /* FIX 2: check fork() return value —
         * mirrors MultiprocessingMatrixSolver's error check */
        pid_t pid = fork();
        if (pid < 0) { perror("fork failed"); exit(1); }

        if (pid == 0)
        {
            /* ── Child process: sweep owned interior nodes ──
             *
             * Reads u_old[j-1] and u_old[j+1]. For boundary nodes within
             * a child's range (e.g. child 1 reading u_old[start_j-1] which
             * belongs to child 0's slice), this is safe because u_old was
             * fully written by the parent before any fork() was called.
             * No child ever writes to u_old.
             *
             * Writes only to u[start_j .. end_j-1] — a disjoint slice
             * from every other child's writes, so there is no race.
             *
             * Boundary nodes (j=0 and j=N-1) are never touched;
             * they stay at zero throughout the entire iteration.
             */
            for (int j = start_j; j < end_j; j++)
            {
                /*
                 * Solve the j-th equation for u[j]:
                 *   u_new[j] = (f[j]*h² + u_old[j-1] + u_old[j+1]) / 2
                 *
                 * Boundary nodes (j=0 and j=N-1) are never touched;
                 * they stay at zero throughout the entire iteration.
                 */
                u[j] = (f[j] * h2 + u_old[j-1] + u_old[j+1]) / 2.0;
            }

            exit(0); /* child exits cleanly; only parent continues */
        }
        /* Parent continues to spawn the next child immediately */
    }

    /* Parent: collect all children before returning.
     * wait(NULL) blocks until any child exits. Calling it num_procs times
     * ensures every child has finished writing its slice of u before the
     * caller reads u for residual computation — mirrors the wait loop in
     * MultiprocessingMatrixSolver. */
    for (int p = 0; p < num_procs; p++)
        wait(NULL);
}

/* =========================================================================
 * JACOBI ITERATION
 *
 * The iteration loop runs entirely in the parent process. Each sweep
 * delegates the arithmetic to child processes via forked_sweep(). The
 * residual and convergence check are performed by the parent after all
 * children have exited — no shared state or barriers needed for this part.
 *
 * Synchronisation is implicit in the fork/wait sequence:
 *   parent copies u -> u_old      (serial, before fork)
 *   forked_sweep() spawns children (parallel sweep)
 *   forked_sweep() wait() loop    (parent blocks until all done)
 *   parent computes residual      (serial, after all children gone)
 *   parent checks convergence     (serial)
 * ========================================================================= */

int jacobi(double *u, double *f, int N, double h, double tol, int max_iter,
           int num_procs)
{
    double h2  = h * h;
    double rms;
    int    it  = 0;

    /*
     * Allocate a working array for the "old" values.
     * At each sweep we copy u -> u_old first, then compute all new u[j]
     * from u_old. This ensures every update uses only pre-sweep values.
     *
     * Both u_shared and u_old must be in mmap(MAP_SHARED) so that child
     * writes to u_shared are visible to the parent after wait(), and child
     * reads of u_old reflect the values the parent wrote before each fork.
     * Using MAP_ANONYMOUS creates pages backed by no file, identical in
     * purpose to the C_data region in MultiprocessingMatrixSolver.
     */
    double *u_shared = (double *)mmap(NULL, N * sizeof(double),
                                       PROT_READ | PROT_WRITE,
                                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (u_shared == MAP_FAILED) { perror("mmap failed (u_shared)"); exit(1); }

    double *u_old = (double *)mmap(NULL, N * sizeof(double),
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (u_old == MAP_FAILED) { perror("mmap failed (u_old)"); exit(1); }

    /* Copy the caller's initial u (all zeros) into the shared region */
    for (int j = 0; j < N; j++)
        u_shared[j] = u[j];

    /*
    printf("\n");
    printf("  %8s  %14s  %14s\n", "Step", "RMS residual", "Change norm");
    printf("  %8s  %14s  %14s\n", "--------", "--------------", "--------------");
    */ 

    while (1)
    {
        /* ── Copy u -> u_old in the parent BEFORE any fork ──
         *
         * This serial step replaces the copy-phase barrier from the threads
         * version. Because every child is forked AFTER this loop finishes,
         * every child reads a fully consistent u_old with no race condition.
         * Boundary nodes are included so children can safely read
         * u_old[start_j - 1] and u_old[end_j] at range edges.
         */
        for (int j = 0; j < N; j++)
            u_old[j] = u_shared[j];

        /* ── Parallel sweep: fork num_procs children ──
         *
         * Each child updates its slice of u_shared using u_old, then exits.
         * forked_sweep() returns only after all children have been collected
         * by wait() — equivalent to the sweep barrier and copy barrier from
         * the threads version, but with no explicit synchronisation primitive.
         */
        forked_sweep(u_shared, u_old, f, N, h2, num_procs);

        /* ── Compute RMS residual with the new iterate ──
         *
         * Runs entirely in the parent after all children have exited.
         * u_shared now holds the fully updated iterate; no child is alive
         * to race with this read.
         */
        rms = compute_rms_residual(u_shared, f, N, h);

        it++;

        /* ── Check convergence ── */
        
      if (rms <= tol)
        {
            //printf("\n  Converged after %d iterations (RMS = %.4e <= tol = %.4e)\n",
              //     it, rms, tol);
            break;
        }

        /* ── Failsafe: stop if max_iter exceeded ── */
        if (it >= max_iter)
        {
            printf("\n  WARNING: reached max_iter = %d without converging.\n", max_iter);
            printf("  Final RMS residual = %.4e, tolerance = %.4e\n", rms, tol);
            break;
        }
    }

    /* Copy the final result back into the caller's u array so main() sees
     * the solution in its original malloc'd buffer — same as the sequential
     * version where u is updated in place throughout. */
    for (int j = 0; j < N; j++)
        u[j] = u_shared[j];

    munmap(u_shared, N * sizeof(double));
    munmap(u_old,    N * sizeof(double));

    return it;
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
    int num_procs   = atoi(argv[2]); /* number of child processes */
  
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
    int it_num = jacobi(u, f, N, h, tol, max_iter, num_procs);

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

    printf("%.6f,%d,",elapsed, it_num);

    return 0;
}