/* Actual PC12-byte initialized scale and4-byte loader-zero cache cell.
 * No geometry buffer, LUT reserve or interface aliases are defined here. */
typedef char mesh_storage_long32[(sizeof(long)==4)?1:-1];
long g_mesh_scale_5ceb0[3]={0x2000,0x2000,0x2000};
long g_mesh_cached_column_6ffb8=0;
