#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>

/*
===============================================================================
MD5 CONTROLLER APPLICATION — FULL WORKFLOW EXPLANATION
CREATED BY: DEXTER RYAN FLOREZA
STUDENT NUMBER: 500946679
CONTACT: dexter.floreza@torontomu.ca
MAY THIS EXPLANATION HELP ALL FUTURE ELECTRICAL AND COMPUTER ENGINEERING STUDENTS SO THAT THEY MAY NEVER SUFFER LIKE I DID  
===============================================================================

SYSTEM OVERVIEW
---------------
This application runs on the Hard Processor System (HPS) under Yocto Linux and
controls an FPGA-based MD5 hashing core through the lightweight HPS-to-FPGA
(lwh2f) memory-mapped bridge.

The HPS acts as a controller:
    - Sends 512-bit messages (as 16 x 32-bit words)
    - Triggers MD5 computation on FPGA engines
    - Reads back 128-bit digests (4 x 32-bit words)
    - Verifies correctness
    - Measures performance metrics

All communication between HPS and FPGA uses 32-bit memory-mapped registers.


-------------------------------------------------------------------------------
MEMORY-MAPPED INTERFACE
-------------------------------------------------------------------------------
Two Avalon-MM slaves are exposed by the FPGA:

1. DATA SLAVE
   - DATA_REG_DATA : read/write data
   - DATA_REG_ADDR : selects internal address

   Used for:
     - Writing message words
     - Reading digest words

2. CONTROL SLAVE
   - CTRL_REG_RESET : resets engines
   - CTRL_REG_START : starts computation
   - CTRL_REG_WEN   : write-enable pulse (for message writes)
   - CTRL_REG_DONE  : status (indicates computation complete)

These are accessed via pointers mapped from /dev/mem using mmap().


-------------------------------------------------------------------------------
MESSAGE FORMAT (MD5 REQUIREMENT)
-------------------------------------------------------------------------------
Each MD5 computation requires a 512-bit padded message block.

In this application:
    - The message "abc" is pre-padded according to MD5 standard
    - Stored as 16 x 32-bit words (little-endian)
    - Written sequentially into each engine’s memory

Expected digest:
    MD5("abc") = 900150983cd24fb0d6963f7d28e17f72


-------------------------------------------------------------------------------
CORE WORKFLOW (PER HASH COMPUTATION)
-------------------------------------------------------------------------------

For each engine (serial or parallel):

1. RESET ENGINE(S)
   - Write to CTRL_REG_RESET
   - Ensures clean state before computation

2. LOAD MESSAGE
   - For each of the 16 words:
       • Write value to DATA_REG_DATA
       • Write address to DATA_REG_ADDR
       • Pulse CTRL_REG_WEN (write enable)
   - This transfers the full 512-bit message into FPGA memory

3. START COMPUTATION
   - Pulse CTRL_REG_START
   - FPGA begins MD5 hashing internally

4. WAIT FOR COMPLETION
   - Poll CTRL_REG_DONE until:
       • Serial: bit 0 = 1
       • Parallel: all bits = 1 (0xFFFFFFFF)

5. READ DIGEST
   - Read 4 x 32-bit words from DATA_REG_DATA
   - These form the 128-bit MD5 digest

6. VERIFY CORRECTNESS
   - Compare against expected digest
   - Increment correct_hashes if match

7. UPDATE METRICS
   - total_hashes++
   - track compute time and total elapsed time


-------------------------------------------------------------------------------
SERIAL VS PARALLEL MODES
-------------------------------------------------------------------------------

SERIAL MODE:
    - Only engine 0 is used
    - One hash computed at a time
    - Lower throughput, minimal resource usage

PARALLEL MODE:
    - All 32 engines are used simultaneously
    - Same message loaded into all engines
    - 32 hashes computed per cycle
    - Much higher throughput


-------------------------------------------------------------------------------
PERFORMANCE METRICS COMPUTED
-------------------------------------------------------------------------------

1. TOTAL HASHES
   - Total number of MD5 computations completed

2. CORRECT HASHES
   - Number of hashes matching expected output

3. ACCURACY (%)
   - (correct_hashes / total_hashes) × 100
   - Must be 100% to confirm correctness

4. ELAPSED TIME
   - Total runtime of test loop

5. COMPUTE TIME
   - Time spent waiting for FPGA to finish (pure hashing)

6. OVERHEAD TIME
   - Time spent on:
       • resets
       • message loading
       • data transfers
       • software delays

7. HASH RATE (H/s)
   - total_hashes / elapsed_time

8. COMPUTE RATE (H/s)
   - total_hashes / compute_time
   - Represents true hardware throughput

9. LATENCY (µs/hash)
   - elapsed_time / total_hashes

10. SPEEDUP (PARALLEL vs SERIAL)
   - parallel_rate / serial_rate

11. EFFICIENCY
   - speedup / NUM_ENGINES
   - Indicates parallel scaling effectiveness


-------------------------------------------------------------------------------
KEY DESIGN INSIGHTS
-------------------------------------------------------------------------------

The system demonstrates a classic hardware acceleration trade-off:

    1. SERIAL: compute-bound (limited by single engine)
    2. PARALLEL: communication-bound (limited by HPS-FPGA interface)

While parallel execution increases throughput significantly, performance is
ultimately constrained by memory-mapped I/O bandwidth and control overhead.

This highlights the importance of optimizing data movement in SoC systems.

===============================================================================
END OF WORKFLOW DESCRIPTION
===============================================================================
*/

/*
===============================================================================
FUNCTION-BY-FUNCTION EXPLANATION
===============================================================================

This section explains the purpose, inputs, outputs, and behavior of each
function used in the MD5 controller application.

-------------------------------------------------------------------------------
now_sec()
-------------------------------------------------------------------------------
Purpose:
    Returns the current system time in seconds (double precision).

How it works:
    - Uses gettimeofday() to obtain seconds + microseconds
    - Converts to a floating-point value

Why it matters:
    - Used to measure execution time, compute time, and performance metrics

-------------------------------------------------------------------------------
write_word(int engine, int word_index, uint32_t value)
-------------------------------------------------------------------------------
Purpose:
    Writes a single 32-bit word into a specific MD5 engine’s message memory.

Inputs:
    engine      → which MD5 engine (0–31)
    word_index  → index within 512-bit message (0–15)
    value       → 32-bit message word

How it works:
    1. Computes address based on FPGA memory mapping
    2. Writes value to DATA register
    3. Writes address to ADDR register
    4. Pulses WEN (write enable) to commit the write

Why it matters:
    - This is the ONLY way to transfer message data into FPGA
    - Must strictly follow control protocol

-------------------------------------------------------------------------------
load_message(int engine)
-------------------------------------------------------------------------------
Purpose:
    Loads the full 512-bit MD5 message into a given engine.

Inputs:
    engine → target MD5 engine

How it works:
    - Calls write_word() 16 times
    - Transfers all 16 message words sequentially

Why it matters:
    - Ensures correct message is loaded before computation
    - Required before every hash

-------------------------------------------------------------------------------
read_digest(int engine, uint32_t digest[4])
-------------------------------------------------------------------------------
Purpose:
    Reads the 128-bit MD5 digest from a given engine.

Inputs:
    engine  → which MD5 engine
    digest  → array to store result (4 x 32-bit words)

How it works:
    1. Sets address for each digest word
    2. Reads from DATA register
    3. Stores result in array

Why it matters:
    - Retrieves final MD5 result from FPGA
    - Needed for correctness verification

-------------------------------------------------------------------------------
digest_is_correct(const uint32_t digest[4])
-------------------------------------------------------------------------------
Purpose:
    Verifies whether the computed digest matches expected MD5("abc").

Inputs:
    digest → computed digest from FPGA

Returns:
    1 if correct, 0 otherwise

How it works:
    - Compares each of the 4 words with expected values

Why it matters:
    - Ensures system is functioning correctly
    - Required by assignment (must show 100% accuracy)

-------------------------------------------------------------------------------
run_serial(double seconds, stats_t *stats)
-------------------------------------------------------------------------------
Purpose:
    Executes MD5 hashing using ONE engine repeatedly for a fixed duration.

Inputs:
    seconds → runtime duration
    stats   → structure to store performance results

Workflow:
    Loop until time expires:
        1. Reset engine 0
        2. Load message into engine 0
        3. Start computation
        4. Wait for DONE signal
        5. Read digest
        6. Verify correctness
        7. Update counters

Metrics collected:
    - total hashes
    - correct hashes
    - compute time (DONE wait)
    - total elapsed time

Why it matters:
    - Baseline performance (no parallelism)
    - Used to compare against parallel mode

-------------------------------------------------------------------------------
run_parallel(double seconds, stats_t *stats)
-------------------------------------------------------------------------------
Purpose:
    Executes MD5 hashing using ALL 32 engines simultaneously.

Inputs:
    seconds → runtime duration
    stats   → structure to store performance results

Workflow:
    Loop until time expires:
        1. Reset ALL engines
        2. Load message into all 32 engines
        3. Start all engines simultaneously
        4. Wait until ALL engines are done
        5. Read all 32 digests
        6. Verify each digest
        7. Update counters

Key difference from serial:
    - Processes 32 hashes per iteration instead of 1

Why it matters:
    - Demonstrates hardware parallelism
    - Produces significantly higher throughput

-------------------------------------------------------------------------------
print_stats(const stats_t *s)
-------------------------------------------------------------------------------
Purpose:
    Displays detailed performance metrics for one mode.

Outputs:
    - total hashes
    - correct hashes
    - accuracy (%)
    - hash rate (H/s)
    - compute rate (H/s)
    - latency (µs/hash)
    - compute vs overhead time

Why it matters:
    - Provides all required metrics for analysis and report

-------------------------------------------------------------------------------
print_comparison(const stats_t *serial, const stats_t *parallel)
-------------------------------------------------------------------------------
Purpose:
    Compares serial and parallel performance.

Outputs:
    - hash rates
    - speedup (parallel / serial)
    - efficiency (speedup / number of engines)

Why it matters:
    - Quantifies benefit of parallelization
    - Helps explain scaling limitations

-------------------------------------------------------------------------------
main(int argc, char *argv[])
-------------------------------------------------------------------------------
Purpose:
    Entry point of the program.

Inputs:
    argv[1] → mode:
        's' = serial
        'p' = parallel
        'b' = both
    argv[2] → runtime (seconds)

Workflow:
    1. Parse arguments
    2. Open /dev/mem (physical memory access)
    3. Map FPGA bridge using mmap()
    4. Assign pointers to DATA and CONTROL registers
    5. Run selected mode(s)
    6. Print results
    7. Unmap memory and close file

Why it matters:
    - Connects software to hardware
    - Controls full program execution


===============================================================================
END OF FUNCTION EXPLANATION
===============================================================================
*/



/* ========================= CONFIG ========================= */

#define LW_BRIDGE_BASE  0xFF200000
#define LW_BRIDGE_SPAN  0x00200000

#define MD5_DATA_BASE   0x00000000
#define MD5_CTRL_BASE   0x00000800

#define NUM_ENGINES     32
#define MSG_WORDS       16
#define DIGEST_WORDS    4

/* data slave register mapping */
#define DATA_REG_DATA   0
#define DATA_REG_ADDR   1

/* control slave register mapping */
#define CTRL_REG_START  0
#define CTRL_REG_RESET  1
#define CTRL_REG_WEN    2
#define CTRL_REG_DONE   2   /* read shares offset with WEN write */

/* ========================= GLOBAL MMIO POINTERS ========================= */

volatile uint32_t *data_ptr = NULL;
volatile uint32_t *ctrl_ptr = NULL;

/* ========================= TEST MESSAGE ========================= */
/*
 * Pre-padded 512-bit block for "abc"
 * MD5("abc") = 900150983cd24fb0d6963f7d28e17f72
 *
 * Stored as 16 x 32-bit words for lwh2f / alt_* compatibility.
 */
static const uint32_t msg[MSG_WORDS] = {
    0x80636261, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000018, 0x00000000
};

/*
 * Expected digest in little-endian 32-bit words.
 */
static const uint32_t expected[DIGEST_WORDS] = {
    0x98500190,
    0xb04fd23c,
    0x7d3f96d6,
    0x727fe128
};

/* ========================= STATS STRUCT ========================= */


typedef struct {
    const char *mode_name;

    uint64_t total_hashes;
    uint64_t correct_hashes;

    double requested_seconds;
    double elapsed_seconds;

    double compute_time;     // time spent ONLY waiting for DONE
    double overhead_time;    // everything else

    double hash_rate;        // total / elapsed
    double compute_rate;     // total / compute_time
    double latency_us;       // per hash
} stats_t;


/* ========================= TIME ========================= */
/* -------------------------------------------------------------------------------
THE now_sec() FUNCTION
-------------------------------------------------------------------------------
Purpose:
    Returns the current system time in seconds (double precision).

How it works:
    - Uses gettimeofday() to obtain seconds + microseconds
    - Converts to a floating-point value

Why it matters:
    - Used to measure execution time, compute time, and performance metrics
*/

static double now_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

/* ========================= CORE ACCESS HELPERS ========================= */

/*
 * Write one 32-bit message word into one engine.
 * Address encoding must match the FPGA design.
 */


/*-------------------------------------------------------------------------------
write_word(int engine, int word_index, uint32_t value)
-------------------------------------------------------------------------------
Purpose:
    Writes a single 32-bit word into a specific MD5 engine’s message memory.

Inputs:
    engine      → which MD5 engine (0–31)
    word_index  → index within 512-bit message (0–15)
    value       → 32-bit message word

How it works:
    1. Computes address based on FPGA memory mapping
    2. Writes value to DATA register
    3. Writes address to ADDR register
    4. Pulses WEN (write enable) to commit the write

Why it matters:
    - This is the ONLY way to transfer message data into FPGA
    - Must strictly follow control protocol
*/
static void write_word(int engine, int word_index, uint32_t value)
{
    uint32_t addr = ((engine >> 1) << 5) | ((engine & 1) << 4) | word_index;

    data_ptr[DATA_REG_DATA] = value;
    data_ptr[DATA_REG_ADDR] = addr;

    /* pulse WEN */
    ctrl_ptr[CTRL_REG_WEN] = 1;
    usleep(1);
    ctrl_ptr[CTRL_REG_WEN] = 0;
}


/* -------------------------------------------------------------------------------
load_message(int engine)
-------------------------------------------------------------------------------
Purpose:
    Loads the full 512-bit MD5 message into a given engine.

Inputs:
    engine → target MD5 engine

How it works:
    - Calls write_word() 16 times
    - Transfers all 16 message words sequentially

Why it matters:
    - Ensures correct message is loaded before computation
    - Required before every hash
*/


static void load_message(int engine)
{
    for (int i = 0; i < MSG_WORDS; i++) {
        write_word(engine, i, msg[i]);
    }
}

/*
 * Read 128-bit digest from one engine as 4 x 32-bit words.
 */
static void read_digest(int engine, uint32_t digest[DIGEST_WORDS])
{
    for (int i = 0; i < DIGEST_WORDS; i++) {
        uint32_t addr = ((uint32_t)engine << 2) | (uint32_t)i;
        data_ptr[DATA_REG_ADDR] = addr;
        digest[i] = data_ptr[DATA_REG_DATA];
    }
}

static int digest_is_correct(const uint32_t digest[DIGEST_WORDS])
{
    for (int i = 0; i < DIGEST_WORDS; i++) {
        if (digest[i] != expected[i]) {
            return 0;
        }
    }
    return 1;
}

/* ========================= DEBUG PRINT ========================= */

static void print_digest_hex(const uint32_t digest[DIGEST_WORDS])
{
    const uint8_t *b = (const uint8_t *)digest;
    for (int i = 0; i < 16; i++) {
        printf("%02x", b[i]);
    }
}

/* ========================= SERIAL MODE ========================= */
/*
 * One engine at a time for a specified duration.
 */
static void run_serial(double seconds, stats_t *stats)
{
    double t0 = now_sec();
    uint32_t digest[4];

    stats->mode_name = "SERIAL";
    stats->requested_seconds = seconds;
    stats->total_hashes = 0;
    stats->correct_hashes = 0;
    stats->compute_time = 0;

    while ((now_sec() - t0) < seconds) {

        ctrl_ptr[CTRL_REG_RESET] = 1;
        usleep(1);
        ctrl_ptr[CTRL_REG_RESET] = 0;

        load_message(0);

        double t_start = now_sec();

        ctrl_ptr[CTRL_REG_START] = 1;
        ctrl_ptr[CTRL_REG_START] = 0;

        while ((ctrl_ptr[CTRL_REG_DONE] & 1) == 0);

        double t_end = now_sec();
        stats->compute_time += (t_end - t_start);

        read_digest(0, digest);

        stats->total_hashes++;
        if (digest_is_correct(digest))
            stats->correct_hashes++;
    }

    stats->elapsed_seconds = now_sec() - t0;
    stats->overhead_time = stats->elapsed_seconds - stats->compute_time;

    stats->hash_rate = stats->total_hashes / stats->elapsed_seconds;
    stats->compute_rate = stats->total_hashes / stats->compute_time;
    stats->latency_us = (stats->elapsed_seconds / stats->total_hashes) * 1e6;
}
/* ========================= PARALLEL MODE ========================= */
/*
 * All 32 engines compute concurrently for a specified duration.
 */
static void run_parallel(double seconds, stats_t *stats)
{
    double t0 = now_sec();
    uint32_t digest[4];

    stats->mode_name = "PARALLEL";
    stats->requested_seconds = seconds;
    stats->total_hashes = 0;
    stats->correct_hashes = 0;
    stats->compute_time = 0;

    while ((now_sec() - t0) < seconds) {

        ctrl_ptr[CTRL_REG_RESET] = 0xFFFFFFFF;
        ctrl_ptr[CTRL_REG_RESET] = 0;

        for (int e = 0; e < NUM_ENGINES; e++)
            load_message(e);

        double t_start = now_sec();

        ctrl_ptr[CTRL_REG_START] = 0xFFFFFFFF;
        ctrl_ptr[CTRL_REG_START] = 0;

        while (ctrl_ptr[CTRL_REG_DONE] != 0xFFFFFFFF);

        double t_end = now_sec();
        stats->compute_time += (t_end - t_start);

        for (int e = 0; e < NUM_ENGINES; e++) {
            read_digest(e, digest);

            stats->total_hashes++;
            if (digest_is_correct(digest))
                stats->correct_hashes++;
        }
    }

    stats->elapsed_seconds = now_sec() - t0;
    stats->overhead_time = stats->elapsed_seconds - stats->compute_time;

    stats->hash_rate = stats->total_hashes / stats->elapsed_seconds;
    stats->compute_rate = stats->total_hashes / stats->compute_time;
    stats->latency_us = (stats->elapsed_seconds / stats->total_hashes) * 1e6;
}

/* ========================= REPORTING ========================= */

static void print_stats(const stats_t *s)
{
    double accuracy = 100.0 * s->correct_hashes / s->total_hashes;

    printf("\n========================================\n");
    printf("Mode:            %s\n", s->mode_name);
    printf("Elapsed Time:    %.4f s\n", s->elapsed_seconds);

    printf("Total Hashes:    %llu\n", (unsigned long long)s->total_hashes);
    printf("Correct Hashes:  %llu\n", (unsigned long long)s->correct_hashes);
    printf("Accuracy:        %.2f%%\n", accuracy);

    printf("\n--- Performance ---\n");
    printf("Hash Rate:       %.2f H/s\n", s->hash_rate);
    printf("Compute Rate:    %.2f H/s\n", s->compute_rate);
    printf("Latency:         %.2f us/hash\n", s->latency_us);

    printf("\n--- Time Breakdown ---\n");
    printf("Compute Time:    %.4f s\n", s->compute_time);
    printf("Overhead Time:   %.4f s\n", s->overhead_time);

    printf("========================================\n");
}

static void print_comparison(const stats_t *s, const stats_t *p)
{
    double speedup = p->hash_rate / s->hash_rate;
    double efficiency = speedup / NUM_ENGINES;

    printf("\n===============================================================\n");
    printf("                 ADVANCED PERFORMANCE SUMMARY\n");
    printf("===============================================================\n");

    printf("+-----------+-----------+-----------+-----------+-----------+\n");
    printf("| Mode      | Rate H/s  | Latency   | Speedup   | Efficiency|\n");
    printf("+-----------+-----------+-----------+-----------+-----------+\n");

    printf("| Serial    | %-9.0f | %-9.2f | %-9.2f | %-9.2f |\n",
           s->hash_rate, s->latency_us, 1.0, 1.0);

    printf("| Parallel  | %-9.0f | %-9.2f | %-9.2f | %-9.2f |\n",
           p->hash_rate, p->latency_us, speedup, efficiency);

    printf("+-----------+-----------+-----------+-----------+-----------+\n");

    printf("\nKey Observations:\n");
    printf("- Parallel speedup: %.2fx\n", speedup);
    printf("- Efficiency vs ideal (32x): %.2f%%\n", efficiency * 100);

    printf("===============================================================\n");
}

/* ========================= MAIN ========================= */

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s <s|p|b> <seconds>\n", argv[0]);
        printf("  s = serial\n");
        printf("  p = parallel\n");
        printf("  b = both\n");
        return 1;
    }

    double seconds = atof(argv[2]);
    if (seconds <= 0.0) {
        printf("ERROR: seconds must be > 0\n");
        return 1;
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

    void *base = mmap(NULL, LW_BRIDGE_SPAN, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (base == MAP_FAILED) {
        perror("mmap()");
        close(fd);
        return 1;
    }

    data_ptr = (volatile uint32_t *)((volatile uint8_t *)base + MD5_DATA_BASE);
    ctrl_ptr = (volatile uint32_t *)((volatile uint8_t *)base + MD5_CTRL_BASE);

    stats_t serial_stats = {0};
    stats_t parallel_stats = {0};

    if (argv[1][0] == 's') {
        run_serial(seconds, &serial_stats);
        print_stats(&serial_stats);
    }
    else if (argv[1][0] == 'p') {
        run_parallel(seconds, &parallel_stats);
        print_stats(&parallel_stats);
    }
    else if (argv[1][0] == 'b') {
        run_serial(seconds, &serial_stats);
        run_parallel(seconds, &parallel_stats);
        print_stats(&serial_stats);
        print_stats(&parallel_stats);
        print_comparison(&serial_stats, &parallel_stats);
    }
    else {
        printf("ERROR: mode must be s, p, or b\n");
        munmap(base, LW_BRIDGE_SPAN);
        close(fd);
        return 1;
    }

    munmap(base, LW_BRIDGE_SPAN);
    close(fd);
    return 0;
}
