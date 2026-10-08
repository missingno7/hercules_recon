/* Real host setters/invokers and DLL forwarding/gate functions, separate TUs.
 * Observing game callbacks and initial storage are synthetic fixture boundaries. */
#include <stdio.h>
#include <string.h>
#include "../calibration/engine_interface.h"
#include "../calibration/host_callback_protocol_externs.h"

EngineInterface g_engine_interface;
HostCallbackStoragePrefix g_host_callback_prefix;
unsigned long g_dispatch_callback_flag_71ec0;
extern EngineNoArgCallback dispatch_engine_2cc40(EngineNoArgCallback);
extern void dispatch_completion_gate(void);
static int checks,failures,first_calls,second_calls,alternate_calls;
static EngineNoArgCallback nested_previous;
#define EXPECT(x) do {++checks;if(!(x)){++failures;printf("line %d\n",__LINE__);}}while(0)
static void __cdecl second(void) {++second_calls;}
static void __cdecl alternate(void) {++alternate_calls;}
static void __cdecl first(void) {
    ++first_calls;
    nested_previous=dispatch_engine_2cc40(second);
    host_disable_callback_gate();
}
int main(void) {
    memset(&g_engine_interface,0,sizeof(g_engine_interface));
    memset(&g_host_callback_prefix,0,sizeof(g_host_callback_prefix));
    g_engine_interface.dispatch_2cc40_3e4=host_set_callback_one;
    g_engine_interface.dispatch_2caa0_330=host_invoke_callback_zero;
    EXPECT(host_enable_clear_callbacks()==0 && g_host_callback_prefix.delivery_gate_008==1);
    EXPECT(host_set_callback_zero(first)==0);
    EXPECT(dispatch_engine_2cc40(dispatch_completion_gate)==0);
    g_dispatch_callback_flag_71ec0=0;
    host_invoke_callback_one();
    EXPECT(first_calls==0 && second_calls==0);
    g_dispatch_callback_flag_71ec0=1;
    host_invoke_callback_one();
    EXPECT(first_calls==1 && second_calls==0);
    EXPECT(nested_previous==dispatch_completion_gate);
    EXPECT(g_host_callback_prefix.callback_zero_000==first && g_host_callback_prefix.callback_one_004==second);
    EXPECT(g_host_callback_prefix.delivery_gate_008==0);
    host_invoke_callback_one();
    EXPECT(first_calls==1 && second_calls==0);
    EXPECT(host_enable_clear_callbacks()==0);
    EXPECT(g_host_callback_prefix.callback_zero_000==0 && g_host_callback_prefix.callback_one_004==0 && g_host_callback_prefix.delivery_gate_008==1);
    EXPECT(host_set_callback_zero(second)==0);
    EXPECT(dispatch_engine_2cc40(dispatch_completion_gate)==0);
    host_invoke_callback_one();
    EXPECT(first_calls==1 && second_calls==1);
    g_engine_interface.dispatch_2caa0_330=alternate;
    host_invoke_callback_one();
    EXPECT(alternate_calls==1 && second_calls==1);
    EXPECT(dispatch_engine_2cc40(0)==dispatch_completion_gate);
    host_invoke_callback_one();
    EXPECT(alternate_calls==1 && second_calls==1);
    printf("callback pipeline: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
