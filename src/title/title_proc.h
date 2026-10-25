/* TITLE process record, shared by the scheduler unit and every screen handler.
   A handler is the process callback: the scheduler calls fn_00(proc) when delay_08 counts down to
   zero, and the handler sets state_04 (its phase) and delay_08 (ticks until its next call). */
#ifndef TITLE_PROC_H
#define TITLE_PROC_H

typedef struct TitleProc TitleProc;
struct TitleProc {
    void (*fn_00)(TitleProc *);
    unsigned long state_04;
    long delay_08;
    unsigned long unknown_0c;
    TitleProc *next_10;
    unsigned long unknown_14;
};

#endif
