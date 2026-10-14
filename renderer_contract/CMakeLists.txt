# Shared reference utilities. No historical matching flags or GPU dependencies.
add_library(hercules_renderer_contract STATIC
    hercules_cif.c
    hercules_edge_steps.c
    hercules_frame.c
    hercules_host_abi.c
    hercules_packed_uv.c
    hercules_packets.c
    hercules_pixels.c
    hercules_projection.c
    hercules_rectangle.c
    hercules_reprojection.c
    hercules_route_uv_models.c
    hercules_sprite_geometry.c
    hercules_sprite_raster.c
    hercules_staging.c
    hercules_state.c
    hercules_textures.c)
target_include_directories(hercules_renderer_contract PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(hercules_renderer_contract PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
