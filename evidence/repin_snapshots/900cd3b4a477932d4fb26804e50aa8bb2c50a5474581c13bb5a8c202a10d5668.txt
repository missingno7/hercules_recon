#include "actor_pipeline_storage.h"

/* Loader-zero native data; this source supplies one measured physical span. */
ENG1ActorPipelineStorage71440 g_actor_pipeline_storage_71440 = {0};

/* Native actor pointer at 0x7200c, separate from the larger worklist span. */
typedef struct ENG1ActorPipelineActor ENG1ActorPipelineActor;
ENG1ActorPipelineActor *g_pipeline_actor_7200c = 0;

/* Separately addressed WORD cells used by the measured pipeline. */
unsigned short g_pipeline_word_6fec0, g_pipeline_word_6fec4;
unsigned short g_pipeline_word_71376, g_pipeline_word_713ae;
