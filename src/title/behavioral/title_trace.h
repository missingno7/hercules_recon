#ifndef TITLE_BEHAVIORAL_TRACE_H
#define TITLE_BEHAVIORAL_TRACE_H
/* Optional diagnostics, enabled only by HERCULES_TITLE_TRACE file path.
   This support routine has no original TITLE RVA. */
#ifdef HERCULES_TITLE_NO_TRACE
/* Our instrumentation is absent from the matching diagnostic, including its
   argument evaluation and runtime-library dependencies. Not a game callback. */
#define title_trace(phase, a, b, c) ((void)0)
#else
void title_trace(const char *phase, unsigned long a, unsigned long b, unsigned long c);
#endif
#endif
