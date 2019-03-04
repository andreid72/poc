#include "common.h"
#include "msc_lock.h"

//#define MSC_LOCK
#ifdef MSC_LOCK
msc_lock_t lock;
static void lock_init()
{
    msc_init(&lock);
}

static void lock_destroy()
{
    msc_destroy(&lock);
}

static void lock_lock(int id)
{
    msc_lock(&lock, id);
}

static void lock_touch(int id)
{
    msc_touch(&lock, id);
}

static void lock_unlock(int id)
{
    msc_unlock(&lock, id);
}
#else
pthread_spinlock_t lock;
static void lock_init()
{
    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
}

static void lock_destroy()
{
    pthread_spin_destroy(&lock);
}

static void lock_lock(int id)
{
    pthread_spin_lock(&lock);
}

#define lock_touch(id)

static void lock_unlock(int id)
{
    pthread_spin_unlock(&lock);
}
#endif

pthread_barrier_t tbarrier;
int global_counter = 0;
// NOTE we limit experiment to 1K threads
// This is a POC so it doesn't matter
double performance_data[1024] = { 0 };

enum {
    REQ_LOCK,
    ACQ_LOCK,
    REL_LOCK,
    DONE_LOCK,
    ALL_LOCK
};

char *get_type(int id)
{
    switch( id ) {
    case REQ_LOCK: return "REQ_LOCK";
    case ACQ_LOCK: return "AQC_LOCK";
    case REL_LOCK: return "REL_LOCK";
    case DONE_LOCK:return "DONE_LOCK";
    }
}

uint64_t timestamps[32][1024*1024][ALL_LOCK] = { 0 };

inline uint64_t rdtsc() {
    uint64_t ts;
    asm volatile ( 
        "rdtsc\n\t"    // Returns the time in EDX:EAX.
        "shl $32, %%rdx\n\t"  // Shift the upper bits left.
        "or %%rdx, %0"        // 'Or' in the lower bits.
        : "=a" (ts)
        : 
        : "rdx");
    return ts;
}

#define min(a,b) ( (a > b) ? b : a )

void *worker(void *_id)
{
    int i, tid = *((int*)_id);
    volatile int local_counter = 0;
    bind_to_core(tid);
    uint64_t wl_start;

    pthread_barrier_wait(&tbarrier);

    double start = GET_TS();
    for(i=0; i < niter; i++) {
        wl_start = rdtsc();
        timestamps[tid][i][REQ_LOCK] = rdtsc();
        lock_lock(tid);
        timestamps[tid][i][ACQ_LOCK] = rdtsc();

        if( verify_mode ) {
            // In the verification mode we want to make sure that
            // global counter won't miss additions
            global_counter++;
        } else {
            // In the performance measurement mode we don't want additional
            // cache invalidations, so deal with the local variable.
            int k;
            wl_start = rdtsc();
            for(k=0; k < workload; k++) {
                if( !(k & ((1<<8) - 1)) ) {
                        lock_touch(tid);
                }
                asm volatile (
                    "incl (%[ptr])\n" 
                    :
                    : [ptr] "r" (&local_counter)
                    : "memory");
            }
        }
        timestamps[tid][i][REL_LOCK] = rdtsc();
        lock_unlock(tid);
        timestamps[tid][i][DONE_LOCK] = rdtsc();
    }
    performance_data[tid] = GET_TS() - start;
}

int main(int argc, char **argv)
{
    int i;

    process_args(argc,argv);
//    bind_to_core(0);

    pthread_barrier_init(&tbarrier, NULL, nthreads + 1);
    int tids[nthreads];
    pthread_t id[nthreads];

    lock_init();

    /* setup and create threads */
    for (i=0; i<nthreads; i++) {
        tids[i] = i;
        pthread_create(&id[i], NULL, worker, (void *)&tids[i]);
    }

    pthread_barrier_wait(&tbarrier);

    for (i=0; i<nthreads; i++) {
        pthread_join(id[i], NULL);
    }

    lock_destroy();

    double sum = 0;
    for(i=0; i < nthreads; i++) {
        sum += performance_data[i];
    }

    printf("Average latency / lock acquire: %lf us\n", 1E6 * sum / (nthreads * niter));
    
    if( verify_mode ){
	if( global_counter != nthreads * niter ) {
	    printf("Verification: FAILED!\n");
	} else {
	    printf("Verification: SUCCESS!\n");
	}
    } else {
        int k;
        uint64_t *t0 = (uint64_t*)timestamps[0], t0_cnt=0;
        uint64_t *t1 = (uint64_t*)timestamps[1], t1_cnt=0;
        uint64_t prev = min(t0[0], t1[0]);
        uint64_t begin = prev;
        uint64_t stat[ALL_LOCK] = { 0 };
        
        for(k = 0; k < ALL_LOCK * niter; k++) {
            int thr_id = -1;
            int type = -1;
            uint64_t val;
            if( t0[t0_cnt] < t1[t1_cnt] ) {
                val = t0[t0_cnt];
                type = t0_cnt % ALL_LOCK;
                thr_id = 0;
                t0_cnt++;
            } else {
                val = t1[t1_cnt];
                type = t1_cnt % ALL_LOCK;
                thr_id = 1;
                t1_cnt++;
            }
            stat[type] += val - prev;
            char tmp[256];
            sprintf(tmp, "(!!!: %lu", (val - begin));
            printf("%lu: %d [%s] %s\n", 
                    val - prev, thr_id, get_type(type),
                    ((type == ACQ_LOCK) && ((val - prev) > 10000)) ? tmp : "" );
            prev = val;
        }
        
        for(k = 0; k < ALL_LOCK; k++) {
            printf("[%s]: %lf\n", get_type(k), (double)stat[k] / (2 * niter));
        }
    }

    

}
