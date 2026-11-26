#ifndef ENG1_ACTOR_PIPELINE_STORAGE_H
#define ENG1_ACTOR_PIPELINE_STORAGE_H

/* One private backing object for the measured actor-pipeline byte interval.
 * The union supplies ordinary byte, WORD, and DWORD interior views. It does
 * not claim that the original program declared this exact C aggregate. */
typedef union ENG1ActorPipelineStorage71440 ENG1ActorPipelineStorage71440;
union ENG1ActorPipelineStorage71440 {
    unsigned char byte_cells[0x8c0];
    unsigned short word_cells[0x460];
    unsigned long dword_cells[0x230];
};

typedef char eng1_actor_pipeline_storage_is_0x8c0[
    (sizeof(ENG1ActorPipelineStorage71440) == 0x8c0) ? 1 : -1];
typedef char eng1_actor_pipeline_pointer_is_32[(sizeof(void *) == 4) ? 1 : -1];
typedef char eng1_actor_pipeline_ulong_is_32[(sizeof(unsigned long) == 4) ? 1 : -1];
typedef char eng1_actor_pipeline_ushort_is_16[(sizeof(unsigned short) == 2) ? 1 : -1];

extern ENG1ActorPipelineStorage71440 g_actor_pipeline_storage_71440;

/* Existing camera-storage owners; actor-pipeline names are aliases only. */
extern long g_camera_x_70ca0;
extern long g_camera_center_x_70cdc;

#endif
