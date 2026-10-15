// Original PC bitmap resampling hypothesis; exact source identity is unproved.
// Float ratios and x87 accumulation preserve the observed sampling boundaries.
extern "C" void __cdecl graphics_native_06530(unsigned char *source,
    int source_width,int source_height,int source_pitch,unsigned char *target,
    int target_width,int target_height,int target_pitch)
{
    float step_x,step_y,x,y;
    int row,column,offset;
    if (!source || source_width<1 || source_height<1 || source_pitch<source_width ||
        !target || target_width<1 || target_height<1 || target_pitch<target_width)
        return;
    step_x=(float)source_width/target_width;
    step_y=(float)source_height/target_height;
    y=0.0f;
    for (row=target_height;row>0;--row) {
        x=0.0f;
        offset=(int)y*source_pitch;
        for (column=0;column<target_width;++column) {
            target[column]=source[offset+(int)x];
            x+=step_x;
        }
        target+=target_pitch;
        y+=step_y;
    }
}
