/* Copyright (c) NetHack Atari port contributors.  */
/* NetHack may be freely redistributed.  See license for details. */

/* Replace the TOS "bombs" with a register/stack-frame dump.
 *
 * crash_install() (run as a constructor) hooks the CPU exception
 * vectors via Setexc.  On a crash the stub saves all registers and the
 * supervisor stack frame, switches to a private stack and prints a
 * dump to the console, then appends it to CRASH.LOG in the game
 * directory.  The 68030 bus/address-error frames (format $A/$B) carry
 * the fault address, so the dump shows vector, PC, fault address,
 * registers, the basepage text base (for offline symbol lookup), the
 * last checkpoint reached, and raw frame/stack words.
 *
 * crash_checkpoint(tag) remembers the tag for the crash dump; if a
 * file NHBOOT.LOG already exists in the game directory the tag is also
 * appended there immediately, giving a boot-progress trace that
 * survives a crash or reset.
 *
 * crash_selftest() raises an illegal instruction when a file CRASHTST
 * exists, to verify the whole reporting chain on hardware.
 */

#include <mint/osbind.h>
#include <mint/basepage.h>
#include <stdlib.h>
#include <string.h>

#include "crashlog.h"

#define BOOTLOG "NHBOOT.LOG"
#define CRASHLOG_FILE "CRASH.LOG"

/* referenced from the asm stubs below - not static on purpose */
short crash_active;
short crash_vecnum;
long crash_regs[16];
long crash_usp;
long crash_frameptr;
char crash_stack[4096];

void crash_report(void);

static const char *crash_ckpt = "start";
static short bootlog_mode = -1;

/* one stub per vector: record the vector number, join common code */
#define CRASH_STUB(v)                          \
    __asm__("\t.text\n"                        \
            "\t.globl _crash_stub_" #v "\n"    \
            "_crash_stub_" #v ":\n"            \
            "\tmove.w #" #v ",_crash_vecnum\n" \
            "\tbra _crash_common\n");

CRASH_STUB(2)  CRASH_STUB(3)  CRASH_STUB(4)  CRASH_STUB(5)
CRASH_STUB(6)  CRASH_STUB(7)  CRASH_STUB(10) CRASH_STUB(11)
CRASH_STUB(16) CRASH_STUB(17) CRASH_STUB(18)
CRASH_STUB(19) CRASH_STUB(20) CRASH_STUB(21) CRASH_STUB(22)
CRASH_STUB(23)

/* save registers and frame pointer, run the reporter on a fresh stack;
   a second fault while reporting just parks the CPU */
__asm__("\t.text\n"
        "\t.globl _crash_common\n"
        "_crash_common:\n"
        "\ttst.w _crash_active\n"
        "\tbne 1f\n"
        "\tmove.w #1,_crash_active\n"
        "\tmovem.l %d0-%d7/%a0-%a7,_crash_regs\n"
        "\tmove.l %sp,_crash_frameptr\n"
        "\tmove.l %usp,%a0\n"
        "\tmove.l %a0,_crash_usp\n"
        "\tlea _crash_stack+4096,%sp\n"
        "\tjsr _crash_report\n"
        "1:\tbra 1b\n");

extern void crash_stub_2(void), crash_stub_3(void), crash_stub_4(void),
    crash_stub_5(void), crash_stub_6(void), crash_stub_7(void),
    crash_stub_10(void), crash_stub_11(void),
    crash_stub_16(void), crash_stub_17(void), crash_stub_18(void),
    crash_stub_19(void), crash_stub_20(void), crash_stub_21(void),
    crash_stub_22(void), crash_stub_23(void);

/* Vector 8 (privilege) is left to the OS: MiNT emulates move sr and other
   privileged instructions there for user code.  10 and 11 (line-A, line-F)
   stay hooked; nethack never emits those, so any trap is a real fault. */
static const short crash_vec[] = { 2, 3, 4, 5, 6, 7, 10, 11,
                                   16, 17, 18, 19, 20, 21, 22, 23 };
static void (*const crash_stub[])(void) = {
    crash_stub_2,  crash_stub_3,  crash_stub_4,  crash_stub_5,
    crash_stub_6,  crash_stub_7,  crash_stub_10,
    crash_stub_11, crash_stub_16, crash_stub_17, crash_stub_18,
    crash_stub_19, crash_stub_20, crash_stub_21, crash_stub_22,
    crash_stub_23
};
#define NVEC (sizeof(crash_vec) / sizeof(crash_vec[0]))

static long crash_oldvec[NVEC];

static void
crash_restore(void)
{
    unsigned short i;

    for (i = 0; i < NVEC; i++)
        if (crash_oldvec[i])
            (void) Setexc(crash_vec[i], (void (*)(void)) crash_oldvec[i]);
}

void
crash_install(void)
{
    unsigned short i;

    for (i = 0; i < NVEC; i++)
        crash_oldvec[i] = (long) Setexc(crash_vec[i], crash_stub[i]);
    atexit(crash_restore);
}

static void __attribute__((constructor))
crash_ctor(void)
{
    crash_install();
}

void
crash_checkpoint(const char *tag)
{
    long fd;

    crash_ckpt = tag;
    if (bootlog_mode < 0) {
        fd = Fopen(BOOTLOG, 2);
        bootlog_mode = (fd >= 0) ? 1 : 0;
        if (fd >= 0)
            (void) Fclose((short) fd);
    }
    if (bootlog_mode > 0 && (fd = Fopen(BOOTLOG, 2)) >= 0) {
        (void) Fseek(0L, (short) fd, 2);
        (void) Fwrite((short) fd, (long) strlen(tag), tag);
        (void) Fwrite((short) fd, 2L, "\r\n");
        (void) Fclose((short) fd);
    }
}

void
crash_selftest(void)
{
    long fd = Fopen("CRASHTST", 0);

    if (fd >= 0) {
        (void) Fclose((short) fd);
        __asm__ volatile("\t.word 0x4afc"); /* ILLEGAL */
    }
}

/* ---- crash-time reporting: no libc, only BIOS/GEMDOS traps ---- */

static char rbuf[3072];
static long rlen;

static void
outc(char c)
{
    if (rlen < (long) sizeof(rbuf) - 1)
        rbuf[rlen++] = c;
}

static void
outs(const char *s)
{
    while (*s)
        outc(*s++);
}

static void
outhex(unsigned long v, short digits)
{
    static const char hd[] = "0123456789ABCDEF";
    short i;

    for (i = digits - 1; i >= 0; i--)
        outc(hd[(v >> (4 * i)) & 0xf]);
}

/* memory the dump may safely read: even address inside ST-RAM below
   phystop or inside registered/likely TT-RAM */
static int
addr_ok(unsigned long a)
{
    unsigned long phystop = *(volatile unsigned long *) 0x42EUL;
    unsigned long ramtop = *(volatile unsigned long *) 0x5A4UL;

    if (a & 1)
        return 0;
    if (a >= 8 && a + 4 <= phystop)
        return 1;
    if (a >= 0x01000000UL && ramtop > 0x01000000UL && a + 4 <= ramtop)
        return 1;
    return 0;
}

void
crash_report(void)
{
    volatile unsigned short *f = (volatile unsigned short *) crash_frameptr;
    unsigned long pc = ((unsigned long) f[1] << 16) | f[2];
    unsigned short sr = f[0], fmt = (unsigned short) (f[3] >> 12);
    unsigned long fault = 0;
    BASEPAGE *bp = _base;
    long i;

    rlen = 0;
    outs("\r\n== NETHACK CRASH ==\r\nVEC=");
    outhex((unsigned long) crash_vecnum, 2);
    outs(" CKPT=");
    outs(crash_ckpt);
    outs("\r\nSR=");
    outhex(sr, 4);
    outs(" PC=");
    outhex(pc, 8);
    outs(" FMT=");
    outhex(fmt, 1);
    if (fmt == 0xA || fmt == 0xB) {
        fault = ((unsigned long) f[8] << 16) | f[9];
        outs(" FAULT=");
        outhex(fault, 8);
    }
    outs("\r\n");
    for (i = 0; i < 16; i++) {
        outs(i < 8 ? "D" : "A");
        outc((char) ('0' + (i & 7)));
        outc('=');
        outhex((unsigned long) crash_regs[i], 8);
        outs((i & 3) == 3 ? "\r\n" : " ");
    }
    outs("USP=");
    outhex((unsigned long) crash_usp, 8);
    outs(" SSP=");
    outhex((unsigned long) crash_frameptr, 8);
    if (bp) {
        outs("\r\nTBASE=");
        outhex((unsigned long) bp->p_tbase, 8);
        outs(" TLEN=");
        outhex((unsigned long) bp->p_tlen, 8);
        outs(" PC-TBASE=");
        outhex(pc - (unsigned long) bp->p_tbase, 8);
    }
    outs("\r\nFRAME:");
    for (i = 0; i < 16; i++) {
        outc(' ');
        outhex(f[i], 4);
    }
    if (addr_ok(pc - 8)) {
        outs("\r\nCODE@PC-8:");
        for (i = 0; i < 12; i++) {
            outc(' ');
            outhex(((volatile unsigned short *) (pc - 8))[i], 4);
        }
    }
    if (addr_ok((unsigned long) crash_usp)) {
        outs("\r\nUSTK:");
        for (i = 0; i < 16; i++) {
            outc(' ');
            outhex(((volatile unsigned long *) crash_usp)[i], 8);
        }
    }
    /* For each address register that points to RAM, dump a few longs. */
    for (i = 8; i < 15; i++) {          /* A0..A6 */
        unsigned long a = (unsigned long) crash_regs[i];
        long j;

        if (!addr_ok(a))
            continue;
        outs("\r\nA");
        outc((char) ('0' + (i - 8)));
        outs(":");
        for (j = 0; j < 6; j++) {
            outc(' ');
            outhex(((volatile unsigned long *) a)[j], 8);
        }
    }
    outs("\r\n== END - photo/keep CRASH.LOG - press key ==\r\n");
    rbuf[rlen] = '\0';

    /* console first (BIOS only), file second (GEMDOS may be dead) */
    for (i = 0; i < rlen; i++)
        (void) Bconout(2, rbuf[i]);
    {
        long fd = Fcreate(CRASHLOG_FILE, 0);

        if (fd >= 0) {
            (void) Fwrite((short) fd, rlen, rbuf);
            (void) Fclose((short) fd);
        }
    }
    (void) Bconin(2);
    Pterm(255);
}
