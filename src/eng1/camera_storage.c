/* Access-width ENG1 camera provider. These are real loader-zero cells and
 * initialized PC values, not guessed pointees or reserve capacity. Ownership,
 * original aggregate/TU and native placement are not established. */
typedef char camera_storage_long32[(sizeof(long)==4)?1:-1];
typedef char camera_storage_short16[(sizeof(short)==2)?1:-1];

/* All32 bytes are read by2DA90's eight-DWORD copy.2F840 writes seven cells,
 * preserving DWORD+14; the source object exposes that observed prefix only. */
long g_render_transform_70c60[8]={0};
/* Scalar width is native DWORD; static storage still starts at zero. */
long g_camera_x_70ca0;
long g_camera_y_70ca4, g_camera_z_70ca8;
long g_camera_grid_x_70cac, g_camera_grid_y_70cb0, g_camera_grid_z_70cb4;
long g_camera_offset_x_70cb8, g_camera_offset_y_70cbc, g_camera_offset_z_70cc0;
long g_camera_prior_x_70cc4, g_camera_prior_y_70cc8;
long g_camera_angle_x_70cd0, g_camera_angle_y_70cd4;
long g_camera_center_x_70cdc, g_camera_center_y_70ce0, g_camera_focal_70ce4;
unsigned char g_camera_iterations_70cfd, g_camera_bounds_id_70cff;
short g_camera_adjust_x_70d02, g_camera_adjust_y_70d04, g_camera_adjust_z_70d06;
short g_camera_depth_low_70d0c, g_camera_depth_high_70d0e;
/* +12 is independently supplied selected-width WORD; no overlapping DWORD. */
unsigned short g_camera_flags_70d10;
long g_camera_z_offset_70d18, g_camera_z_base_70d1c;

/* Each native lookup terminates at the firstFFFF WORD. The17-WORD prefix is
 * sufficient for every16-bit magnitude; six following zero bytes are padding/
 * ownership unknown and are excluded. All three tables are writable in PE. */
unsigned short g_camera_threshold_x_5e058[17]={
    0,20,39,57,74,90,106,123,140,158,176,195,214,234,254,296,65535
};
unsigned short g_camera_threshold_y_5e080[17]={
    0,20,39,57,74,90,106,123,140,158,176,195,214,234,254,296,65535
};
unsigned short g_camera_threshold_z_5e0a8[17]={
    0,20,39,57,74,90,106,123,140,158,176,195,214,234,254,296,65535
};
/* Exactly32 bytes copied twice to the interface-owned transform state. The
 * native coefficients are2000, not an assumed standard SDK identity1000. */
long g_render_identity_5ce28[8]={0x2000,0,0x2000,0,0x2000,0,0,0};
