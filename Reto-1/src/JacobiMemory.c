#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
 * JACOBI ITERATION
 * ========================================================================= */

int jacobi(double *u, double *f, int N, double h, double tol, int max_iter)
{
    double *u_old;        /* snapshot of u before each sweep */
    double h2;            /* h²                              */
    double rms;           /* RMS residual at current iterate */
    int    it;            /* iteration counter               */
    int    j;

    h2 = h * h;

    /*
     * Allocate a working array for the "old" values.
     * At each sweep we copy u -> u_old first, then compute all new u[j]
     * from u_old. This ensures every update uses only pre-sweep values.
     */
    u_old = (double *)malloc(N * sizeof(double));
    if (!u_old)
    {
        fprintf(stderr, "ERROR: could not allocate u_old buffer.\n");
        exit(1);
    }

    /*
     * FIX 2 — pointer swapping instead of array copying.
     * Rather than copying N doubles from u into u_old at the start of
     * every sweep (an entire extra O(N) memory pass), we keep two pointers
     * and swap them after each sweep. u_cur always points to the current
     * iterate; u_nxt always points to the buffer where the next iterate
     * is written. No data ever moves — only two pointer variables swap.
     *
     * The boundary nodes of u_nxt must be initialised once to the Dirichlet
     * values (0 here). They are never touched by the sweep loop, so they
     * remain correct after every swap.
     */
    double *u_cur = u;      /* current iterate — starts as the zero vector  */
    double *u_nxt = u_old;  /* next iterate buffer — alternates with u_cur  */

    u_nxt[0]     = u_cur[0];      /* boundary: u(0) = 0 */
    u_nxt[N - 1] = u_cur[N - 1]; /* boundary: u(1) = 0 */

    /*
     * FIX 3 — residual check frequency.
     * In the original code the full RMS residual was computed after every
     * single sweep — one extra O(N) pass per iteration. For large k, where
     * tens of thousands of sweeps are needed, this dominates runtime.
     * We instead check only every CHECK_INTERVAL sweeps. Convergence is
     * detected at most CHECK_INTERVAL sweeps late, which is negligible.
     * Between checks, rms retains its value from the last check.
     */
    const int CHECK_INTERVAL = 50;

    /*
     * FIX 1 — fused sweep + residual loop.
     * On check iterations we compute the residual contribution of node j
     * in the SAME loop as the Jacobi update. The three values u_cur[j-1],
     * u_cur[j], u_cur[j+1] are already loaded from memory to compute
     * u_nxt[j]; we reuse them immediately for the residual formula at zero
     * extra memory cost. This eliminates the second full array traversal
     * that compute_rms_residual() previously caused.
     *
     * Note: the residual is evaluated on u_cur (the iterate BEFORE this
     * sweep). It is therefore the residual of the previous iterate, which
     * is a valid and standard convergence measure.
     */

    /*
    printf("\n");
    printf("  %8s  %14s  %14s\n", "Step", "RMS residual", "Change norm");
    printf("  %8s  %14s  %14s\n", "--------", "--------------", "--------------");
    */ 

    it  = 0;
    rms = compute_rms_residual(u_cur, f, N, h); /* residual of initial guess */

    while (1)
    {

        if (it % CHECK_INTERVAL == 0)
        {
            /*
             * FIX 1 + 3 — check iteration: fused sweep and residual.
             * Single loop reads u_cur[j-1..j+1] once, uses those values
             * for both the Jacobi update and the residual accumulation,
             * then writes u_nxt[j]. One memory pass replaces two.
             */
            double sum_sq = 0.0;
            double inv_h2 = 1.0 / h2;
            double r_j;

            /* interior nodes: fused update + residual */
            for (j = 1; j < N - 1; j++)
            {
                /* residual of u_cur at node j — data already in cache */
                r_j    = (-u_cur[j-1] + 2.0*u_cur[j] - u_cur[j+1]) * inv_h2 - f[j];
                sum_sq += r_j * r_j;

                /* Jacobi update — same three values, no extra load */
                u_nxt[j] = (f[j] * h2 + u_cur[j-1] + u_cur[j+1]) / 2.0;
            }

            /* boundary contributions to the residual sum */
            r_j    = u_cur[0]     - f[0];       /* row 0:   u[0]   = 0 */
            sum_sq += r_j * r_j;
            r_j    = u_cur[N-1]   - f[N-1];     /* row N-1: u[N-1] = 0 */
            sum_sq += r_j * r_j;

            rms = sqrt(sum_sq / (double)N);
        }
        else
        {
            /*
             * FIX 3 — non-check iteration: sweep only, no residual.
             * The array is traversed exactly once per sweep on these
             * iterations, with no extra passes for residual or change norm.
             */
            for (j = 1; j < N - 1; j++)
            {
                /*
                 * Solve the j-th equation for u[j]:
                 *   u_new[j] = (f[j]*h² + u_old[j-1] + u_old[j+1]) / 2
                 *
                 * Boundary nodes (j=0 and j=N-1) are never touched;
                 * they stay at zero throughout the entire iteration.
                 */
                u_nxt[j] = (f[j] * h2 + u_cur[j-1] + u_cur[j+1]) / 2.0;
            }
        }

        /*
         * FIX 2 — pointer swap.
         * What was previously a full O(N) memcpy (u -> u_old) is now
         * just two pointer assignments. The roles of the two buffers flip:
         * the buffer we just wrote becomes the new current iterate, and
         * the old current iterate becomes the workspace for the next sweep.
         */
        double *tmp = u_cur;
        u_cur       = u_nxt;
        u_nxt       = tmp;

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

    /*
     * FIX 2 — copy-back guard.
     * After an odd number of sweeps, u_cur points to u_old (the allocated
     * buffer), not to the original u array that the caller passed in.
     * We copy back once so the caller sees the result in the expected place.
     * This is a single O(N) copy that happens once at the end, regardless
     * of how many thousands of iterations were performed.
     */
    if (u_cur != u)
    {
        for (j = 0; j < N; j++)
            u[j] = u_cur[j];
    }

    free(u_old);

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

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int k = atoi(argv[1]);
  
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

    if (!x || !f || !u || !ue)
    {
        //fprintf(stderr, "ERROR: memory allocation failed.\n");
        return 1;
    }

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
    int it_num = jacobi(u, f, N, h, tol, max_iter);

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

  double elapsed = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("%.6f,%d",elapsed, it_num);

    return 0;
}
