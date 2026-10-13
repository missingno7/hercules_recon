#include "hercules_state.h"

static renderer_u16 read_u16_le(const renderer_u8 *bytes)
{
    return (renderer_u16)((renderer_u16)bytes[0] |
                          ((renderer_u16)bytes[1] << 8));
}

static renderer_u32 read_u32_le(const renderer_u8 *bytes)
{
    return (renderer_u32)bytes[0] |
           ((renderer_u32)bytes[1] << 8) |
           ((renderer_u32)bytes[2] << 16) |
           ((renderer_u32)bytes[3] << 24);
}

static renderer_i32 sign_extend_u16(renderer_u16 value)
{
    if (value & 0x8000U) {
        return (renderer_i32)value - 0x10000L;
    }
    return (renderer_i32)value;
}

static renderer_i32 clamp_i32(renderer_i32 value, renderer_i32 maximum)
{
    if (value < 0) {
        return 0;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

HerculesStateStatus hercules_state_decode_e1(
    const renderer_u8 *wire, size_t length, HerculesE1StateUpdate *out)
{
    HerculesE1StateUpdate result;
    renderer_u32 payload_offset;
    renderer_u32 value;

    if (wire == 0 || out == 0) {
        return HERCULES_STATE_INVALID_ARGUMENT;
    }
    payload_offset = (renderer_u32)offsetof(RendererE1ProducerPacket, mode_payload);
    if (length < (size_t)(payload_offset + 4U)) {
        return HERCULES_STATE_TRUNCATED;
    }
    if (wire[payload_offset + 3U] != 0xE1U) {
        return HERCULES_STATE_WRONG_TAG;
    }

    value = read_u32_le(wire + payload_offset);
    result.page_index = value & 0x1FU;
    result.table_selector = (value >> 5) & 3U;
    result.alternate_bank = ((value & 0x180U) == 0x180U) ? 1U : 0U;
    *out = result;
    return HERCULES_STATE_OK;
}

HerculesStateStatus hercules_state_decode_f3(
    const renderer_u8 *wire, size_t length, HerculesF3ClipBounds *out)
{
    HerculesF3ClipBounds result;
    renderer_i32 x;
    renderer_i32 y;
    renderer_i32 right_delta;
    renderer_i32 bottom_delta;
    renderer_u32 x_offset;
    renderer_u32 y_offset;
    renderer_u32 dx_offset;
    renderer_u32 dy_offset;

    if (wire == 0 || out == 0) {
        return HERCULES_STATE_INVALID_ARGUMENT;
    }
    if (length < sizeof(HrcF3WireView)) {
        return HERCULES_STATE_TRUNCATED;
    }
    if (wire[offsetof(HrcF3WireView, tag)] != 0xF3U) {
        return HERCULES_STATE_WRONG_TAG;
    }

    x_offset = (renderer_u32)offsetof(HrcF3WireView, x);
    y_offset = (renderer_u32)offsetof(HrcF3WireView, encoded_y);
    dx_offset = (renderer_u32)offsetof(HrcF3WireView, right_delta);
    dy_offset = (renderer_u32)offsetof(HrcF3WireView, bottom_delta);
    x = sign_extend_u16(read_u16_le(wire + x_offset));
    y = wire[y_offset];
    if (y >= 0xF0L) {
        y -= 0xF0L;
    }
    right_delta = sign_extend_u16(read_u16_le(wire + dx_offset));
    bottom_delta = sign_extend_u16(read_u16_le(wire + dy_offset));

    result.left = clamp_i32(x, 320L);
    result.top = clamp_i32(y, 240L);
    result.right = clamp_i32(x + right_delta, 320L);
    result.bottom = clamp_i32(y + bottom_delta, 240L);
    *out = result;
    return HERCULES_STATE_OK;
}

HerculesStateStatus hercules_state_decode_command01(
    const renderer_u8 *wire, size_t length, HerculesCommand01Move *out)
{
    HerculesCommand01Move result;
    renderer_u32 destination_x_offset;
    renderer_u32 destination_y_offset;
    renderer_u32 source_x_offset;

    if (wire == 0 || out == 0) {
        return HERCULES_STATE_INVALID_ARGUMENT;
    }
    if (length < sizeof(HrcCommand01WireView)) {
        return HERCULES_STATE_TRUNCATED;
    }
    if (wire[offsetof(HrcCommand01WireView, tag)] != 0x01U) {
        return HERCULES_STATE_WRONG_TAG;
    }

    destination_x_offset = (renderer_u32)offsetof(HrcCommand01WireView, destination_x);
    destination_y_offset = (renderer_u32)offsetof(HrcCommand01WireView, destination_y);
    source_x_offset = (renderer_u32)offsetof(HrcCommand01WireView, source_x);
    result.destination_x = sign_extend_u16(read_u16_le(wire + destination_x_offset));
    result.destination_y = sign_extend_u16(read_u16_le(wire + destination_y_offset));
    result.source_x = sign_extend_u16(read_u16_le(wire + source_x_offset));
    result.source_y = sign_extend_u16(read_u16_le(wire + source_x_offset + 2U));
    result.width = sign_extend_u16(read_u16_le(wire + source_x_offset + 4U));
    result.height = sign_extend_u16(read_u16_le(wire + source_x_offset + 6U));
    *out = result;
    return HERCULES_STATE_OK;
}
