#ifndef HERCULES_FRAME_LIFECYCLE_BRIDGES_H
#define HERCULES_FRAME_LIFECYCLE_BRIDGES_H
#include "engine_interface.h"

void __cdecl dispatch_engine_2c280(void);
void __cdecl dispatch_engine_2c2d0(void *);
void __cdecl dispatch_engine_2c2e0(unsigned long);
void __cdecl dispatch_engine_2c5e0(const void *, unsigned long, unsigned long, unsigned long);
void __cdecl dispatch_engine_2c500(void *, void *);
void __cdecl dispatch_engine_2c980(void *, unsigned long, unsigned long, unsigned long, unsigned long);
void __cdecl dispatch_engine_2c8f0(void *);
void __cdecl dispatch_engine_2c8e0(void *);
void __cdecl dispatch_engine_2c7e0(void);
EngineNoArgCallback __cdecl dispatch_engine_2c640(EngineNoArgCallback);
#endif
