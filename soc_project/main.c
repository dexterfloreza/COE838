#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>

/* ========================= CONFIG ========================= */

/*
Lightweight HPS-to-FPGA bridge base address.
This is the memory region used to communicate with FPGA peripherals.
*/
#define LW_BRIDGE_BASE 0xFF200000
#define LW_BRIDGE_SPAN 0x00200000

/*
Base addresses of your Qsys slaves
*/
#define MD5_DATA_BASE  0x00000000   // data slave (message + digest interface)
#define MD5_CTRL_BASE  0x00000800   // control slave (start/reset/done/wen)

/*
System parameters
*/
#define NUM_ENGINES 32      // number of parallel MD5 engines
#define MSG_WORDS   16      // 512-bit message = 16 x 32-bit words
#define DIGEST_WORDS 4      // 128-bit MD5 digest = 4 x 32-bit words

/*
Data slave register mapping:
- write DATA -> sets md5_datain
- write ADDR -> sets md5_addrin
- read DATA -> returns digest word selected by addr
*/
#define DATA_REG_DATA 0
#define DATA_REG_ADDR 1

/*
Control slave register mapping:
- START → starts engines (bitmask)
- RESET → resets engines (bitmask)
- WEN → write enable pulse
- DONE → done status (bitmask)
*/
#define CTRL_REG_START 0
#define CTRL_REG_RESET 1
#define CTRL_REG_WEN   2
#define CTRL_REG_DONE  2   // read and write share same offset

/*
Pointers to memory-mapped FPGA registers
*/
volatile uint32_t *data_ptr;
volatile uint32_t *ctrl_ptr;

/* ========================= TEST VECTOR ========================= */

/*
Pre-padded MD5 message for "abc"
- little-endian format
- includes padding + length
*/
static const uint32_t msg[16] = {
    0x80636261,0,0,0,0,0,0,0,
    0,0,0,0,0,0,24,0
};

/*
Expected MD5("abc") result in little-endian words
*/
static const uint32_t expected[4] = {
    0x98500190,
    0xb04fd23c,
    0x7d3f96d6,
    0x727fe128
};

/* ========================= TIME ========================= */

/*
Returns current time in seconds (double precision)
Used for performance measurement
*/
double now()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec + tv.tv_usec/1e6;
}

/* ========================= CORE WRITE ========================= */

/*
Writes one 32-bit word into a specific engine’s message memory

Address encoding (VERY IMPORTANT):
((engine >> 1) << 5) | ((engine & 1) << 4) | word_index

This matches your hardware's internal memory mapping.
*/
void write_word(int engine, int i, uint32_t val)
{
    uint32_t addr = ((engine >> 1) << 5) | ((engine & 1) << 4) | i;

    data_ptr[DATA_REG_DATA] = val;   // set data
    data_ptr[DATA_REG_ADDR] = addr;  // set address

    /*
    Pulse write enable (WEN)
    Required to commit the write into FPGA memory
    */
    ctrl_ptr[CTRL_REG_WEN] = 1;
    usleep(1);   // ensure pulse width (hardware timing)
    ctrl_ptr[CTRL_REG_WEN] = 0;
}

/* ========================= LOAD ========================= */

/*
Loads full 512-bit message into one engine
*/
void load_message(int engine)
{
    int i;
    for(i=0;i<16;i++)
        write_word(engine,i,msg[i]);
}

/* ========================= READ ========================= */

/*
Reads 128-bit digest from a given engine

Address mapping:
(engine << 2) | word_index

Each engine has 4 digest registers
*/
void read_digest(int engine, uint32_t d[4])
{
    int i;
    for(i=0;i<4;i++)
    {
        uint32_t addr = (engine << 2) | i;

        data_ptr[DATA_REG_ADDR] = addr;
        d[i] = data_ptr[DATA_REG_DATA];
    }
}

/* ========================= CHECK ========================= */

/*
Compares computed digest against expected MD5 result
*/
int correct(uint32_t d[4])
{
    int i;
    for(i=0;i<4;i++)
        if(d[i] != expected[i]) return 0;
    return 1;
}

/* ========================= SERIAL ========================= */

/*
Serial mode:
- Only engine 0 is used
- Compute one hash at a time
*/
void run_serial(double sec)
{
    uint64_t total=0, ok=0;
    double t0=now();

    while(now()-t0 < sec)
    {
        uint32_t d[4];

        /*
        Reset engine 0
        */
        ctrl_ptr[CTRL_REG_RESET] = 1;
        usleep(1);
        ctrl_ptr[CTRL_REG_RESET] = 0;

        /*
        Load message into engine 0
        */
        load_message(0);

        /*
        Start computation
        */
        ctrl_ptr[CTRL_REG_START] = 1;
        usleep(1);
        ctrl_ptr[CTRL_REG_START] = 0;

        /*
        Wait until engine finishes
        */
        while((ctrl_ptr[CTRL_REG_DONE] & 1)==0);

        /*
        Read digest result
        */
        read_digest(0,d);

        /*
        Validate correctness
        */
        if(correct(d)) ok++;
        total++;
    }

    printf("\nSERIAL RESULTS\n");
    printf("Total: %llu\n",total);
    printf("Correct: %llu\n",ok);
    printf("Accuracy: %.2f%%\n",100.0*ok/total);
    printf("Rate: %.2f H/s\n",total/sec);
}

/* ========================= PARALLEL ========================= */

/*
Parallel mode:
- Uses all 32 engines
- Computes 32 hashes simultaneously
*/
void run_parallel(double sec)
{
    uint64_t total=0, ok=0;
    double t0=now();

    while(now()-t0 < sec)
    {
        uint32_t d[4];
        int e;

        /*
        Reset ALL engines at once
        */
        ctrl_ptr[CTRL_REG_RESET] = 0xFFFFFFFF;
        usleep(1);
        ctrl_ptr[CTRL_REG_RESET] = 0;

        /*
        Load message into all engines
        */
        for(e=0;e<32;e++)
            load_message(e);

        /*
        Start all engines simultaneously
        */
        ctrl_ptr[CTRL_REG_START] = 0xFFFFFFFF;
        usleep(1);
        ctrl_ptr[CTRL_REG_START] = 0;

        /*
        Wait until ALL engines are done
        */
        while(ctrl_ptr[CTRL_REG_DONE] != 0xFFFFFFFF);

        /*
        Read all digests
        */
        for(e=0;e<32;e++)
        {
            read_digest(e,d);
            if(correct(d)) ok++;
            total++;
        }
    }

    printf("\nPARALLEL RESULTS\n");
    printf("Total: %llu\n",total);
    printf("Correct: %llu\n",ok);
    printf("Accuracy: %.2f%%\n",100.0*ok/total);
    printf("Rate: %.2f H/s\n",total/sec);
}

/* ========================= MAIN ========================= */

/*
Program entry point
Usage:
./MD5sSW s 5 → serial mode
./MD5sSW p 5 → parallel mode
*/
int main(int argc,char*argv[])
{
    if(argc<3)
    {
        printf("Usage: %s <s|p> <seconds>\n",argv[0]);
        return 0;
    }

    double sec = atof(argv[2]);

    /*
    Open physical memory access
    */
    int fd = open("/dev/mem",O_RDWR|O_SYNC);

    /*
    Map FPGA bridge into virtual memory
    */
    void *base = mmap(NULL,LW_BRIDGE_SPAN,
                      PROT_READ|PROT_WRITE,
                      MAP_SHARED,fd,LW_BRIDGE_BASE);

    /*
    Assign pointers to slaves
    */
    data_ptr = (uint32_t*)(base + MD5_DATA_BASE);
    ctrl_ptr = (uint32_t*)(base + MD5_CTRL_BASE);

    /*
    Select execution mode
    */
    if(argv[1][0]=='p')
        run_parallel(sec);
    else
        run_serial(sec);

    /*
    Cleanup
    */
    munmap(base,LW_BRIDGE_SPAN);
    close(fd);
}
