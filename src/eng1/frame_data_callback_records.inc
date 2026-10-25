/* Per-configuration ENG1 callback-record prefixes with actual spans. */
/* Each owner ends at the next configuration pointee root; observed limits are explicit. */
/* Record ABI and any runtime index above the consumer-observed minimum remain unproved. */
typedef void (__cdecl *Eng1FrameObservedCallback)(/* ABI unresolved */);
typedef char eng1_frame_callback_ptr_is_32bit[(sizeof(Eng1FrameObservedCallback)==4)?1:-1];
typedef struct Eng1FrameCallbackTriple12 { Eng1FrameObservedCallback callback_000; Eng1FrameObservedCallback callback_004; Eng1FrameObservedCallback callback_008; } Eng1FrameCallbackTriple12;
typedef char eng1_frame_callback_triple_size[(sizeof(Eng1FrameCallbackTriple12)==12)?1:-1];
typedef struct Eng1FrameCallbackPair8 { Eng1FrameObservedCallback callback_000; Eng1FrameObservedCallback callback_004; } Eng1FrameCallbackPair8;
typedef char eng1_frame_callback_pair_size[(sizeof(Eng1FrameCallbackPair8)==8)?1:-1];
typedef struct Eng1FrameFactoryPair8 { Eng1FrameObservedCallback callback_000; Eng1FrameObservedCallback callback_004; } Eng1FrameFactoryPair8;
typedef char eng1_frame_factory_pair_size[(sizeof(Eng1FrameFactoryPair8)==8)?1:-1];

extern void __cdecl eng1_native_code_target_001010();
extern void __cdecl eng1_native_code_target_001040();
extern void __cdecl eng1_native_code_target_0010f0();
extern void __cdecl eng1_native_code_target_001310();
extern void __cdecl eng1_native_code_target_0013c0();
extern void __cdecl init_fields_1d50();
extern void __cdecl eng1_native_code_target_001d80();
extern void __cdecl eng1_native_code_target_001e00();
extern void __cdecl eng1_native_code_target_001e30();
extern void __cdecl eng1_native_code_target_008660();
extern void __cdecl eng1_native_code_target_0086f0();
extern void __cdecl eng1_native_code_target_008dd0();
extern void __cdecl eng1_native_code_target_008e10();
extern void __cdecl eng1_native_code_target_0090c0();
extern void __cdecl eng1_native_code_target_009100();
extern void __cdecl eng1_native_code_target_009190();
extern void __cdecl eng1_native_code_target_0092c0();
extern void __cdecl eng1_native_code_target_009850();
extern void __cdecl eng1_native_code_target_009890();
extern void __cdecl eng1_native_code_target_0099e0();
extern void __cdecl eng1_native_code_target_009a20();
extern void __cdecl eng1_native_code_target_009fb0();
extern void __cdecl eng1_native_code_target_00a010();
extern void __cdecl eng1_native_code_target_00a420();
extern void __cdecl eng1_native_code_target_00a490();
extern void __cdecl eng1_native_code_target_00a600();
extern void __cdecl eng1_native_code_target_00a670();
extern void __cdecl eng1_native_code_target_00a870();
extern void __cdecl eng1_native_code_target_00a910();
extern void __cdecl eng1_native_code_target_00ae00();
extern void __cdecl eng1_native_code_target_00b3f0();
extern void __cdecl eng1_native_code_target_00baf0();
extern void __cdecl eng1_native_code_target_00bb10();
extern void __cdecl eng1_native_code_target_00bc50();
extern void __cdecl eng1_empty_c720();
extern void __cdecl eng1_native_code_target_00c780();
extern void __cdecl eng1_native_code_target_00c7e0();
extern void __cdecl eng1_native_code_target_00c8c0();
extern void __cdecl eng1_native_code_target_00c910();
extern void __cdecl eng1_native_code_target_00c920();
extern void __cdecl eng1_native_code_target_00c940();
extern void __cdecl eng1_native_code_target_00cb80();
extern void __cdecl eng1_native_code_target_00cc30();
extern void __cdecl eng1_native_code_target_00d190();
extern void __cdecl eng1_native_code_target_00d210();
extern void __cdecl eng1_native_code_target_00d240();
extern void __cdecl eng1_native_code_target_00d2c0();
extern void __cdecl eng1_native_code_target_00d350();
extern void __cdecl eng1_native_code_target_00d380();
extern void __cdecl eng1_native_code_target_00d390();
extern void __cdecl eng1_native_code_target_00d3d0();
extern void __cdecl eng1_native_code_target_00d410();
extern void __cdecl eng1_native_code_target_00d440();
extern void __cdecl eng1_native_code_target_00d460();
extern void __cdecl eng1_native_code_target_00d760();
extern void __cdecl eng1_native_code_target_00d7d0();
extern void __cdecl eng1_native_code_target_00ddd0();
extern void __cdecl eng1_native_code_target_00de50();
extern void __cdecl eng1_native_code_target_00e170();
extern void __cdecl eng1_native_code_target_00e200();
extern void __cdecl eng1_native_code_target_00e3b0();
extern void __cdecl eng1_native_code_target_00e470();
extern void __cdecl eng1_native_code_target_00e520();
extern void __cdecl eng1_native_code_target_00e560();
extern void __cdecl set_limit_e610();
extern void __cdecl eng1_native_code_target_00e620();
extern void __cdecl eng1_native_code_target_00ee70();
extern void __cdecl eng1_native_code_target_00eec0();
extern void __cdecl eng1_native_code_target_00eee0();
extern void __cdecl eng1_native_code_target_00efa0();
extern void __cdecl eng1_native_code_target_00efe0();
extern void __cdecl init_flags_f070();
extern void __cdecl eng1_native_code_target_00f0a0();
extern void __cdecl eng1_native_code_target_00f180();
extern void __cdecl eng1_native_code_target_00f380();
extern void __cdecl eng1_native_code_target_00f3b0();
extern void __cdecl eng1_native_code_target_00f430();
extern void __cdecl eng1_native_code_target_00f4f0();
extern void __cdecl eng1_native_code_target_00f910();
extern void __cdecl eng1_native_code_target_00fa00();
extern void __cdecl eng1_native_code_target_00fd00();
extern void __cdecl eng1_native_code_target_00fe60();
extern void __cdecl eng1_native_code_target_0102c0();
extern void __cdecl load_coords_10380();
extern void __cdecl eng1_native_code_target_0103c0();
extern void __cdecl eng1_native_code_target_010540();
extern void __cdecl eng1_native_code_target_0105a0();
extern void __cdecl eng1_native_code_target_0115f0();
extern void __cdecl eng1_native_code_target_011690();
extern void __cdecl init_defaults_117d0();
extern void __cdecl eng1_native_code_target_011810();
extern void __cdecl eng1_native_code_target_011950();
extern void __cdecl eng1_native_code_target_0119a0();
extern void __cdecl eng1_native_code_target_011a80();
extern void __cdecl eng1_native_code_target_011ac0();
extern void __cdecl eng1_native_code_target_011c10();
extern void __cdecl eng1_native_code_target_011c70();
extern void __cdecl eng1_native_code_target_011cd0();
extern void __cdecl eng1_native_code_target_011df0();
extern void __cdecl eng1_native_code_target_012a10();
extern void __cdecl eng1_native_code_target_012b00();
extern void __cdecl eng1_native_code_target_012c00();
extern void __cdecl eng1_native_code_target_012d80();
extern void __cdecl eng1_native_code_target_012ed0();
extern void __cdecl eng1_native_code_target_013570();
extern void __cdecl eng1_native_code_target_013840();
extern void __cdecl eng1_native_code_target_013880();
extern void __cdecl eng1_native_code_target_013990();
extern void __cdecl eng1_native_code_target_0139f0();
extern void __cdecl set_mode_13c90();
extern void __cdecl eng1_native_code_target_013ca0();
extern void __cdecl eng1_native_code_target_013d70();
extern void __cdecl eng1_native_code_target_014210();
extern void __cdecl eng1_native_code_target_0142d0();
extern void __cdecl eng1_native_code_target_014790();
extern void __cdecl eng1_native_code_target_014850();
extern void __cdecl eng1_native_code_target_014c50();
extern void __cdecl eng1_native_code_target_014cf0();
extern void __cdecl copy_pair_14de0();
extern void __cdecl eng1_native_code_target_014e10();
extern void __cdecl eng1_native_code_target_014fb0();
extern void __cdecl eng1_native_code_target_015000();
extern void __cdecl eng1_native_code_target_015030();
extern void __cdecl eng1_native_code_target_015060();
extern void __cdecl eng1_native_code_target_019f50();
extern void __cdecl eng1_native_code_target_01a0f0();
extern void __cdecl eng1_native_code_target_01a1d0();
extern void __cdecl eng1_native_code_target_01a2b0();
extern void __cdecl eng1_native_code_target_01a390();
extern void __cdecl eng1_native_code_target_01a440();
extern void __cdecl eng1_native_code_target_01a5a0();
extern void __cdecl eng1_native_code_target_01a950();
extern void __cdecl eng1_native_code_target_01b230();
extern void __cdecl eng1_native_code_target_01b2a0();
extern void __cdecl eng1_native_code_target_01b7f0();
extern void __cdecl eng1_native_code_target_01bc20();
extern void __cdecl eng1_native_code_target_01bca0();
extern void __cdecl eng1_native_code_target_01c200();
extern void __cdecl eng1_native_code_target_01c220();
extern void __cdecl eng1_native_code_target_01c2a0();
extern void __cdecl eng1_native_code_target_01cf50();
extern void __cdecl eng1_native_code_target_01d0b0();
extern void __cdecl initialize_wide_effect();
extern void __cdecl eng1_native_code_target_01daf0();
extern void __cdecl initialize_wider_effect();
extern void __cdecl eng1_native_code_target_01db70();
extern void __cdecl eng1_native_code_target_01dbc0();
extern void __cdecl eng1_native_code_target_01dc00();
extern void __cdecl initialize_low_backdrop();
extern void __cdecl select_side_sprite();
extern void __cdecl eng1_native_code_target_01dd00();
extern void __cdecl start_cycle_effect();
extern void __cdecl advance_cycle_effect();
extern void __cdecl eng1_native_code_target_01dfc0();
extern void __cdecl eng1_native_code_target_01e020();
extern void __cdecl start_sequence_effect();
extern void __cdecl advance_sequence_effect();
extern void __cdecl eng1_native_code_target_01e320();
extern void __cdecl eng1_native_code_target_01e3f0();
extern void __cdecl eng1_native_code_target_01e500();
extern void __cdecl eng1_native_code_target_01e600();
extern void __cdecl initialize_flagged_effect();
extern void __cdecl follow_attachment_scale();
extern void __cdecl eng1_native_code_target_01e870();
extern void __cdecl eng1_native_code_target_01e8b0();
extern void __cdecl clear_effect_animation();
extern void __cdecl eng1_native_code_target_01e910();
extern void __cdecl eng1_native_code_target_01e950();
extern void __cdecl eng1_native_code_target_01e9d0();
extern void __cdecl initialize_high_backdrop();
extern void __cdecl eng1_native_code_target_01ea90();
extern void __cdecl eng1_native_code_target_01eba0();
extern void __cdecl eng1_native_code_target_01ebf0();
extern void __cdecl eng1_native_code_target_01ed00();
extern void __cdecl eng1_native_code_target_01ed30();
extern void __cdecl eng1_native_code_target_01fea0();
extern void __cdecl eng1_native_code_target_01ff50();
extern void __cdecl set_wide_scale();
extern void __cdecl eng1_native_code_target_021320();
extern void __cdecl eng1_native_code_target_0213c0();
extern void __cdecl eng1_native_code_target_021400();
extern void __cdecl initialize_backdrop();
extern void __cdecl initialize_particle();
extern void __cdecl eng1_native_code_target_021710();
extern void __cdecl eng1_native_code_target_021740();
extern void __cdecl eng1_native_code_target_021780();
extern void __cdecl initialize_burst();
extern void __cdecl eng1_native_code_target_021940();
extern void __cdecl eng1_native_code_target_021980();
extern void __cdecl eng1_native_code_target_0219e0();
extern void __cdecl eng1_native_code_target_021c00();
extern void __cdecl eng1_native_code_target_021d10();
extern void __cdecl initialize_linked_particle();
extern void __cdecl follow_owner_scale();
extern void __cdecl reset_placement();
extern void __cdecl eng1_empty_22180();
extern void __cdecl eng1_native_code_target_022b90();
extern void __cdecl eng1_native_code_target_022c20();
extern void __cdecl eng1_native_code_target_022d20();
extern void __cdecl eng1_native_code_target_022d60();
extern void __cdecl eng1_native_code_target_022da0();
extern void __cdecl eng1_native_code_target_022dd0();
extern void __cdecl set_flags_two();
extern void __cdecl eng1_native_code_target_023120();
extern void __cdecl show_sprite_82();
extern void __cdecl eng1_native_code_target_023460();
extern void __cdecl show_sprite_7a();
extern void __cdecl eng1_native_code_target_023660();
extern void __cdecl eng1_native_code_target_023680();
extern void __cdecl eng1_native_code_target_0236e0();
extern void __cdecl eng1_native_code_target_0237d0();
extern void __cdecl eng1_native_code_target_023840();
extern void __cdecl eng1_native_code_target_023a50();
extern void __cdecl eng1_native_code_target_023b30();
extern void __cdecl eng1_native_code_target_024270();
extern void __cdecl eng1_native_code_target_024320();
extern void __cdecl eng1_native_code_target_0246c0();
extern void __cdecl eng1_native_code_target_024780();
extern void __cdecl eng1_native_code_target_024b60();
extern void __cdecl eng1_native_code_target_024f50();
extern void __cdecl reset_render_parameter();
extern void __cdecl eng1_native_code_target_0253d0();
extern void __cdecl eng1_native_code_target_025580();
extern void __cdecl eng1_native_code_target_0255c0();
extern void __cdecl eng1_native_code_target_02cc50();
extern void __cdecl eng1_native_code_target_02cd30();
extern void __cdecl eng1_native_code_target_02d9b0();
extern void __cdecl eng1_native_code_target_02d9e0();
extern void __cdecl eng1_native_code_target_02da10();
extern void __cdecl eng1_native_code_target_02da40();
extern void __cdecl eng1_native_code_target_0337f0();
extern void __cdecl eng1_native_code_target_033830();
extern void __cdecl eng1_native_code_target_033910();
extern void __cdecl eng1_native_code_target_033990();
extern void __cdecl eng1_native_code_target_033cc0();
extern void __cdecl eng1_native_code_target_033cf0();
extern void __cdecl eng1_native_code_target_033d70();
extern void __cdecl eng1_native_code_target_033db0();
extern void __cdecl eng1_native_code_target_033e10();
extern void __cdecl eng1_native_code_target_033e80();
extern void __cdecl eng1_native_code_target_033ee0();
extern void __cdecl eng1_native_code_target_033f30();
extern void __cdecl eng1_native_code_target_033fc0();
extern void __cdecl eng1_native_code_target_034000();
extern void __cdecl eng1_native_code_target_0341a0();
extern void __cdecl eng1_native_code_target_034210();
extern void __cdecl eng1_native_code_target_0342f0();
extern void __cdecl eng1_native_code_target_034350();
extern void __cdecl eng1_native_code_target_0343a0();
extern void __cdecl eng1_native_code_target_0343e0();
extern void __cdecl eng1_native_code_target_034650();
extern void __cdecl eng1_native_code_target_034750();
extern void __cdecl eng1_native_code_target_0348a0();
extern void __cdecl eng1_native_code_target_034910();
extern void __cdecl eng1_native_code_target_0349f0();
extern void __cdecl eng1_native_code_target_034a30();
extern void __cdecl eng1_native_code_target_034c00();
extern void __cdecl eng1_native_code_target_034c30();
extern void __cdecl eng1_native_code_target_034c80();
extern void __cdecl eng1_native_code_target_034d00();
extern void __cdecl eng1_native_code_target_034de0();
extern void __cdecl eng1_native_code_target_034e20();
extern void __cdecl eng1_native_code_target_035200();
extern void __cdecl eng1_native_code_target_0352f0();
extern void __cdecl eng1_native_code_target_035a30();
extern void __cdecl eng1_native_code_target_035b20();
extern void __cdecl eng1_native_code_target_035c60();
extern void __cdecl eng1_native_code_target_035cf0();
extern void __cdecl eng1_native_code_target_035d70();
extern void __cdecl eng1_native_code_target_035fb0();
extern void __cdecl eng1_native_code_target_035fe0();
extern void __cdecl eng1_native_code_target_036030();
extern void __cdecl eng1_native_code_target_0360a0();
extern void __cdecl eng1_native_code_target_0360f0();
extern void __cdecl set_mode_flags_36120();
extern void __cdecl eng1_native_code_target_036150();
extern void __cdecl eng1_native_code_target_0361a0();
extern void __cdecl eng1_native_code_target_0363e0();
extern void __cdecl eng1_native_code_target_0365f0();
extern void __cdecl reset_extent_36b60();
extern void __cdecl eng1_native_code_target_036b90();
extern void __cdecl eng1_native_code_target_036cb0();
extern void __cdecl eng1_native_code_target_036ce0();
extern void __cdecl eng1_native_code_target_036d10();
extern void __cdecl eng1_native_code_target_036d70();
extern void __cdecl eng1_native_code_target_036df0();
extern void __cdecl eng1_native_code_target_036e10();
extern void __cdecl eng1_native_code_target_038090();
extern void __cdecl eng1_native_code_target_038100();
extern void __cdecl eng1_native_code_target_038310();
extern void __cdecl eng1_native_code_target_0383b0();
extern void __cdecl eng1_native_code_target_0383d0();
extern void __cdecl eng1_native_code_target_038590();
extern void __cdecl eng1_native_code_target_038610();
extern void __cdecl eng1_native_code_target_038740();
extern void __cdecl eng1_native_code_target_0387f0();
extern void __cdecl eng1_native_code_target_038950();
extern void __cdecl eng1_native_code_target_0389c0();
extern void __cdecl eng1_native_code_target_038b00();
extern void __cdecl eng1_native_code_target_038b30();
extern void __cdecl eng1_native_code_target_038d10();
extern void __cdecl eng1_native_code_target_038d70();
extern void __cdecl eng1_native_code_target_038de0();
extern void __cdecl eng1_native_code_target_038e30();
extern void __cdecl eng1_native_code_target_038f50();
extern void __cdecl eng1_native_code_target_039090();
extern void __cdecl eng1_native_code_target_039360();
extern void __cdecl eng1_native_code_target_0393b0();
extern void __cdecl eng1_native_code_target_039460();
extern void __cdecl eng1_native_code_target_039530();
extern void __cdecl eng1_native_code_target_0396e0();
extern void __cdecl eng1_native_code_target_039750();
extern void __cdecl eng1_native_code_target_039870();
extern void __cdecl eng1_native_code_target_0398b0();
extern void __cdecl eng1_native_code_target_0399c0();
extern void __cdecl eng1_native_code_target_0399f0();
extern void __cdecl eng1_native_code_target_039a70();
extern void __cdecl eng1_native_code_target_039ae0();
extern void __cdecl eng1_native_code_target_039b00();
extern void __cdecl eng1_native_code_target_039bf0();
extern void __cdecl eng1_native_code_target_039c00();
extern void __cdecl eng1_native_code_target_039c20();
extern void __cdecl eng1_native_code_target_039c50();
extern void __cdecl eng1_native_code_target_039ca0();
extern void __cdecl eng1_native_code_target_039cf0();
extern void __cdecl eng1_native_code_target_039f20();
extern void __cdecl eng1_native_code_target_03a040();
extern void __cdecl eng1_native_code_target_03a420();
extern void __cdecl eng1_native_code_target_03a4a0();
extern void __cdecl eng1_native_code_target_03a530();
extern void __cdecl eng1_native_code_target_03a550();
extern void __cdecl eng1_native_code_target_03a610();
extern void __cdecl eng1_native_code_target_03a700();
extern void __cdecl eng1_native_code_target_03a730();
extern void __cdecl eng1_native_code_target_03a770();
extern void __cdecl eng1_native_code_target_03a7a0();
extern void __cdecl eng1_native_code_target_03a810();
extern void __cdecl eng1_native_code_target_03a8a0();
extern void __cdecl eng1_native_code_target_03a8e0();
extern void __cdecl eng1_native_code_target_03a910();
extern void __cdecl eng1_native_code_target_03a950();
extern void __cdecl eng1_native_code_target_03a980();
extern void __cdecl eng1_native_code_target_03aa10();
extern void __cdecl eng1_native_code_target_03aa70();
extern void __cdecl eng1_native_code_target_03aaf0();
extern void __cdecl eng1_native_code_target_03ab50();
extern void __cdecl eng1_native_code_target_03abd0();
extern void __cdecl eng1_native_code_target_03ac80();
extern void __cdecl eng1_native_code_target_03adc0();
extern void __cdecl eng1_native_code_target_03aef0();
extern void __cdecl eng1_native_code_target_03af20();
extern void __cdecl eng1_native_code_target_03b050();
extern void __cdecl eng1_native_code_target_03b080();
extern void __cdecl eng1_native_code_target_03b0b0();
extern void __cdecl eng1_native_code_target_03b0e0();
extern void __cdecl eng1_native_code_target_03b110();
extern void __cdecl eng1_native_code_target_03b140();
extern void __cdecl eng1_native_code_target_03b170();
extern void __cdecl eng1_native_code_target_03b190();
extern void __cdecl eng1_native_code_target_03b310();
extern void __cdecl eng1_native_code_target_03b530();
extern void __cdecl eng1_native_code_target_03b640();
extern void __cdecl eng1_native_code_target_03b730();
extern void __cdecl eng1_native_code_target_03b7f0();
extern void __cdecl eng1_native_code_target_03b8b0();
extern void __cdecl eng1_native_code_target_03b970();
extern void __cdecl eng1_native_code_target_03b980();
extern void __cdecl eng1_native_code_target_03b9d0();
extern void __cdecl eng1_native_code_target_03ba00();
extern void __cdecl eng1_native_code_target_03bbe0();
extern void __cdecl eng1_native_code_target_03bc00();
extern void __cdecl eng1_native_code_target_03bc20();
extern void __cdecl eng1_native_code_target_03bde0();
extern void __cdecl eng1_native_code_target_03c050();
extern void __cdecl eng1_native_code_target_03c070();
extern void __cdecl eng1_native_code_target_03ca80();
extern void __cdecl eng1_native_code_target_03cb90();
extern void __cdecl eng1_native_code_target_03ce10();
extern void __cdecl eng1_native_code_target_03cf90();
extern void __cdecl eng1_native_code_target_03d310();
extern void __cdecl eng1_native_code_target_03d440();
extern void __cdecl eng1_native_code_target_03d780();
extern void __cdecl eng1_native_code_target_03d7c0();
extern void __cdecl eng1_native_code_target_03d800();
extern void __cdecl eng1_native_code_target_03d930();
extern void __cdecl eng1_native_code_target_03db90();
extern void __cdecl eng1_native_code_target_03dcd0();
extern void __cdecl eng1_native_code_target_03e160();
extern void __cdecl eng1_native_code_target_03e220();
extern void __cdecl eng1_native_code_target_03e280();
extern void __cdecl eng1_native_code_target_03e370();
extern void __cdecl eng1_native_code_target_03e3d0();
extern void __cdecl eng1_native_code_target_03e580();
extern void __cdecl eng1_native_code_target_03e5d0();
extern void __cdecl eng1_native_code_target_03e6d0();
extern void __cdecl eng1_native_code_target_03e770();
extern void __cdecl set_render_flags_3e9b0();
extern void __cdecl eng1_native_code_target_03e9d0();
extern void __cdecl eng1_native_code_target_03ea00();
extern void __cdecl eng1_native_code_target_03ea40();
extern void __cdecl eng1_native_code_target_03ea70();
extern void __cdecl set_collide_flags_3eaa0();
extern void __cdecl set_collide_flags_3eac0();
extern void __cdecl eng1_native_code_target_03eaf0();
extern void __cdecl eng1_native_code_target_03ebc0();
extern void __cdecl eng1_native_code_target_03ecb0();
extern void __cdecl eng1_native_code_target_03edf0();
extern void __cdecl eng1_native_code_target_03eec0();
extern void __cdecl set_flags_3f040();
extern void __cdecl adjust_bounds_3f070();
extern void __cdecl eng1_native_code_target_03f0b0();
extern void __cdecl eng1_native_code_target_03f140();
extern void __cdecl eng1_native_code_target_03f1c0();
extern void __cdecl eng1_native_code_target_03f460();
extern void __cdecl eng1_native_code_target_03f4b0();
extern void __cdecl eng1_native_code_target_03f620();
extern void __cdecl eng1_native_code_target_03f720();
extern void __cdecl eng1_native_code_target_03f8d0();
extern void __cdecl eng1_native_code_target_03fa50();
extern void __cdecl eng1_native_code_target_03fb00();
extern void __cdecl eng1_native_code_target_03fcf0();
extern void __cdecl eng1_native_code_target_03fe00();
extern void __cdecl eng1_native_code_target_03fe40();
extern void __cdecl eng1_native_code_target_03ff30();
extern void __cdecl eng1_native_code_target_0407b0();
extern void __cdecl eng1_native_code_target_0408b0();
extern void __cdecl eng1_native_code_target_040950();
extern void __cdecl eng1_native_code_target_040980();
extern void __cdecl eng1_native_code_target_0409b0();
extern void __cdecl eng1_native_code_target_040a00();
extern void __cdecl eng1_native_code_target_040a30();
extern void __cdecl eng1_native_code_target_040a50();
extern void __cdecl apply_scaled_offsets_40ab0();
extern void __cdecl eng1_native_code_target_040b00();
extern void __cdecl eng1_native_code_target_040b40();
extern void __cdecl eng1_native_code_target_040ba0();
extern void __cdecl eng1_native_code_target_040c10();
extern void __cdecl eng1_native_code_target_040c90();
extern void __cdecl eng1_native_code_target_040de0();
extern void __cdecl eng1_native_code_target_040e20();
extern void __cdecl eng1_native_code_target_040e50();
extern void __cdecl eng1_native_code_target_0410b0();
extern void __cdecl eng1_native_code_target_0410e0();
extern void __cdecl set_state_flags_41300();
extern void __cdecl eng1_native_code_target_041320();
extern void __cdecl eng1_native_code_target_041460();
extern void __cdecl eng1_native_code_target_041490();
extern void __cdecl eng1_native_code_target_0417e0();
extern void __cdecl eng1_native_code_target_041810();
extern void __cdecl eng1_native_code_target_041820();
extern void __cdecl eng1_native_code_target_0418a0();
extern void __cdecl eng1_native_code_target_041a50();
extern void __cdecl eng1_native_code_target_041ad0();
extern void __cdecl eng1_native_code_target_041b70();
extern void __cdecl eng1_native_code_target_041c30();
extern void __cdecl eng1_native_code_target_041f00();
extern void __cdecl eng1_native_code_target_041f50();
extern void __cdecl eng1_native_code_target_041f80();
extern void __cdecl copy_link_state_41fa0();
extern void __cdecl eng1_native_code_target_042020();
extern void __cdecl eng1_native_code_target_042090();
extern void __cdecl eng1_native_code_target_042220();
extern void __cdecl eng1_native_code_target_042280();
extern void __cdecl set_kind_422d0();
extern void __cdecl eng1_native_code_target_0422f0();
extern void __cdecl eng1_native_code_target_042390();
extern void __cdecl eng1_native_code_target_042540();
extern void __cdecl eng1_native_code_target_042710();
extern void __cdecl eng1_native_code_target_042770();
extern void __cdecl eng1_native_code_target_0429c0();
extern void __cdecl eng1_native_code_target_042bd0();
extern void __cdecl eng1_native_code_target_043330();
extern void __cdecl eng1_native_code_target_0433d0();
extern void __cdecl eng1_native_code_target_043440();
extern void __cdecl eng1_native_code_target_043ea0();
extern void __cdecl eng1_native_code_target_0445a0();
extern void __cdecl eng1_native_code_target_0447c0();
extern void __cdecl eng1_native_code_target_045930();
extern void __cdecl eng1_native_code_target_045960();
extern void __cdecl set_flags_459e0();
extern void __cdecl eng1_native_code_target_045a00();
extern void __cdecl reset_links_45a50();
extern void __cdecl eng1_native_code_target_045a80();
extern void __cdecl eng1_native_code_target_045cc0();
extern void __cdecl eng1_native_code_target_045d50();
extern void __cdecl eng1_native_code_target_045e20();
extern void __cdecl eng1_native_code_target_045ef0();
extern void __cdecl set_flags_461d0();
extern void __cdecl eng1_native_code_target_0461f0();
extern void __cdecl eng1_native_code_target_046230();
extern void __cdecl eng1_native_code_target_046320();

typedef struct Eng1FrameCallbackBlock_067608 {
    Eng1FrameCallbackTriple12 records[79];
    unsigned char zero_intertable_padding[4];
} Eng1FrameCallbackBlock_067608;
typedef char Eng1FrameCallbackBlock_067608_size[(sizeof(Eng1FrameCallbackBlock_067608) == 952) ? 1 : -1];
Eng1FrameCallbackBlock_067608 g_eng1_callback_record_block_067608 = {
  {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x67608 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x67614 */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x67620 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x6762c */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x67638 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x67644 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x67650 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x6765c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x67668 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x67674 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x67680 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x6768c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x67698 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x676a4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x676b0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x676bc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x676c8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x676d4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x676e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x676ec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x676f8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x67704 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x67710 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x6771c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x67728 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x67734 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x67740 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x6774c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x67758 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x67764 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x67770 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x6777c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x67788 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x67794 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x677a0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x677ac */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x677b8 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x677c4 */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x677d0 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x677dc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 40; RVA 0x677e8 */
    { eng1_native_code_target_040b00, eng1_empty_c720, eng1_native_code_target_040b40 }, /* record 41; RVA 0x677f4 */
    { eng1_native_code_target_03e370, eng1_empty_c720, eng1_native_code_target_03e3d0 }, /* record 42; RVA 0x67800 */
    { eng1_native_code_target_03fe40, eng1_empty_c720, eng1_native_code_target_03ff30 }, /* record 43; RVA 0x6780c */
    { eng1_native_code_target_03fe40, eng1_empty_c720, eng1_native_code_target_03ff30 }, /* record 44; RVA 0x67818 */
    { eng1_native_code_target_03fe40, eng1_empty_c720, eng1_native_code_target_03ff30 }, /* record 45; RVA 0x67824 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 46; RVA 0x67830 */
    { eng1_native_code_target_040e20, eng1_empty_c720, eng1_native_code_target_040e50 }, /* record 47; RVA 0x6783c */
    { eng1_native_code_target_03e220, eng1_empty_c720, eng1_native_code_target_03e160 }, /* record 48; RVA 0x67848 */
    { eng1_native_code_target_02cc50, eng1_empty_c720, eng1_native_code_target_02cd30 }, /* record 49; RVA 0x67854 */
    { eng1_native_code_target_03ce10, eng1_native_code_target_03cf90, eng1_native_code_target_03cf90 }, /* record 50; RVA 0x67860 */
    { eng1_native_code_target_03ca80, eng1_empty_c720, eng1_native_code_target_03cb90 }, /* record 51; RVA 0x6786c */
    { set_render_flags_3e9b0, eng1_empty_c720, eng1_native_code_target_03e9d0 }, /* record 52; RVA 0x67878 */
    { eng1_native_code_target_03d800, eng1_empty_c720, eng1_native_code_target_03d930 }, /* record 53; RVA 0x67884 */
    { eng1_native_code_target_03e6d0, eng1_empty_c720, eng1_native_code_target_03e770 }, /* record 54; RVA 0x67890 */
    { eng1_native_code_target_03f140, eng1_empty_c720, eng1_native_code_target_03f1c0 }, /* record 55; RVA 0x6789c */
    { eng1_native_code_target_0433d0, eng1_empty_c720, eng1_native_code_target_043440 }, /* record 56; RVA 0x678a8 */
    { eng1_native_code_target_03d310, eng1_empty_c720, eng1_native_code_target_03d440 }, /* record 57; RVA 0x678b4 */
    { eng1_native_code_target_03e580, eng1_empty_c720, eng1_native_code_target_03e5d0 }, /* record 58; RVA 0x678c0 */
    { eng1_native_code_target_0409b0, eng1_empty_c720, eng1_native_code_target_040a00 }, /* record 59; RVA 0x678cc */
    { eng1_native_code_target_0410b0, eng1_empty_c720, eng1_native_code_target_0410e0 }, /* record 60; RVA 0x678d8 */
    { eng1_native_code_target_041460, eng1_empty_c720, eng1_native_code_target_041490 }, /* record 61; RVA 0x678e4 */
    { eng1_native_code_target_03db90, eng1_native_code_target_03dcd0, eng1_native_code_target_03dcd0 }, /* record 62; RVA 0x678f0 */
    { eng1_native_code_target_03ebc0, eng1_empty_c720, eng1_native_code_target_03ecb0 }, /* record 63; RVA 0x678fc */
    { eng1_native_code_target_00f910, eng1_empty_c720, eng1_native_code_target_00fa00 }, /* record 64; RVA 0x67908 */
    { eng1_native_code_target_041820, eng1_empty_c720, eng1_native_code_target_0418a0 }, /* record 65; RVA 0x67914 */
    { eng1_native_code_target_041f50, eng1_empty_c720, eng1_native_code_target_041f80 }, /* record 66; RVA 0x67920 */
    { eng1_native_code_target_03d780, eng1_empty_c720, eng1_native_code_target_03d7c0 }, /* record 67; RVA 0x6792c */
    { eng1_native_code_target_03edf0, eng1_empty_c720, eng1_native_code_target_03eec0 }, /* record 68; RVA 0x67938 */
    { eng1_native_code_target_03edf0, eng1_empty_c720, eng1_native_code_target_03eec0 }, /* record 69; RVA 0x67944 */
    { eng1_native_code_target_042020, eng1_empty_c720, eng1_native_code_target_042090 }, /* record 70; RVA 0x67950 */
    { eng1_native_code_target_041820, eng1_empty_c720, eng1_native_code_target_0418a0 }, /* record 71; RVA 0x6795c */
    { eng1_native_code_target_03f460, eng1_empty_c720, eng1_native_code_target_03f4b0 }, /* record 72; RVA 0x67968 */
    { eng1_native_code_target_03f620, eng1_empty_c720, eng1_native_code_target_03f720 }, /* record 73; RVA 0x67974 */
    { eng1_native_code_target_03f460, eng1_empty_c720, eng1_native_code_target_03f8d0 }, /* record 74; RVA 0x67980 */
    { eng1_native_code_target_0409b0, eng1_empty_c720, eng1_native_code_target_040a30 }, /* record 75; RVA 0x6798c */
    { load_coords_10380, eng1_empty_c720, eng1_native_code_target_0103c0 }, /* record 76; RVA 0x67998 */
    { eng1_native_code_target_03e220, eng1_empty_c720, eng1_native_code_target_03e280 }, /* record 77; RVA 0x679a4 */
    { eng1_native_code_target_042710, eng1_empty_c720, eng1_native_code_target_042770 }, /* record 78; RVA 0x679b0 */
  }, { 0U, 0U, 0U, 0U }
};
/* callback12_field_01c: 79 complete records; native span 0x67608..0x679c0; 0 null pointer cells. */
/* Four observed zero bytes retained as explicit intertable padding. */

typedef struct Eng1FrameCallbackBlock_04e268 {
    Eng1FrameCallbackTriple12 records[87];
    unsigned char zero_intertable_padding[4];
} Eng1FrameCallbackBlock_04e268;
typedef char Eng1FrameCallbackBlock_04e268_size[(sizeof(Eng1FrameCallbackBlock_04e268) == 1048) ? 1 : -1];
Eng1FrameCallbackBlock_04e268 g_eng1_callback_record_block_04e268 = {
  {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x4e268 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x4e274 */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x4e280 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x4e28c */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x4e298 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x4e2a4 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x4e2b0 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x4e2bc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x4e2c8 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x4e2d4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x4e2e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x4e2ec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x4e2f8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x4e304 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x4e310 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x4e31c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x4e328 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x4e334 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x4e340 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x4e34c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x4e358 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x4e364 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x4e370 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x4e37c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x4e388 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x4e394 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x4e3a0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x4e3ac */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x4e3b8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x4e3c4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x4e3d0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x4e3dc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x4e3e8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x4e3f4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x4e400 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x4e40c */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x4e418 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x4e424 */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x4e430 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x4e43c */
    { eng1_native_code_target_008660, eng1_empty_c720, eng1_native_code_target_0086f0 }, /* record 40; RVA 0x4e448 */
    { eng1_native_code_target_009190, eng1_empty_c720, eng1_native_code_target_0092c0 }, /* record 41; RVA 0x4e454 */
    { eng1_native_code_target_00a870, eng1_empty_c720, eng1_native_code_target_00a910 }, /* record 42; RVA 0x4e460 */
    { eng1_native_code_target_008dd0, eng1_empty_c720, eng1_native_code_target_008e10 }, /* record 43; RVA 0x4e46c */
    { eng1_native_code_target_009850, eng1_empty_c720, eng1_native_code_target_009890 }, /* record 44; RVA 0x4e478 */
    { eng1_native_code_target_00a600, eng1_empty_c720, eng1_native_code_target_00a670 }, /* record 45; RVA 0x4e484 */
    { eng1_native_code_target_00ae00, eng1_empty_c720, eng1_native_code_target_00b3f0 }, /* record 46; RVA 0x4e490 */
    { eng1_native_code_target_00bb10, eng1_empty_c720, eng1_native_code_target_00bc50 }, /* record 47; RVA 0x4e49c */
    { eng1_native_code_target_00d210, eng1_empty_c720, eng1_native_code_target_00d240 }, /* record 48; RVA 0x4e4a8 */
    { eng1_native_code_target_00d2c0, eng1_empty_c720, eng1_native_code_target_00d350 }, /* record 49; RVA 0x4e4b4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 50; RVA 0x4e4c0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 51; RVA 0x4e4cc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 52; RVA 0x4e4d8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 53; RVA 0x4e4e4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 54; RVA 0x4e4f0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 55; RVA 0x4e4fc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 56; RVA 0x4e508 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 57; RVA 0x4e514 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 58; RVA 0x4e520 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 59; RVA 0x4e52c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 60; RVA 0x4e538 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 61; RVA 0x4e544 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 62; RVA 0x4e550 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 63; RVA 0x4e55c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 64; RVA 0x4e568 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 65; RVA 0x4e574 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 66; RVA 0x4e580 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 67; RVA 0x4e58c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 68; RVA 0x4e598 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 69; RVA 0x4e5a4 */
    { eng1_native_code_target_00d760, eng1_empty_c720, eng1_native_code_target_00d7d0 }, /* record 70; RVA 0x4e5b0 */
    { eng1_native_code_target_00ddd0, eng1_empty_c720, eng1_native_code_target_00de50 }, /* record 71; RVA 0x4e5bc */
    { eng1_native_code_target_00d410, eng1_empty_c720, eng1_native_code_target_00d440 }, /* record 72; RVA 0x4e5c8 */
    { eng1_native_code_target_00d410, eng1_empty_c720, eng1_native_code_target_00d440 }, /* record 73; RVA 0x4e5d4 */
    { eng1_native_code_target_0099e0, eng1_empty_c720, eng1_native_code_target_009a20 }, /* record 74; RVA 0x4e5e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_native_code_target_00d460 }, /* record 75; RVA 0x4e5ec */
    { eng1_native_code_target_009fb0, eng1_empty_c720, eng1_native_code_target_00a010 }, /* record 76; RVA 0x4e5f8 */
    { eng1_native_code_target_00e170, eng1_empty_c720, eng1_native_code_target_00e200 }, /* record 77; RVA 0x4e604 */
    { eng1_native_code_target_00a420, eng1_empty_c720, eng1_native_code_target_00a490 }, /* record 78; RVA 0x4e610 */
    { eng1_native_code_target_00f910, eng1_empty_c720, eng1_native_code_target_00fa00 }, /* record 79; RVA 0x4e61c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 80; RVA 0x4e628 */
    { eng1_native_code_target_00e520, eng1_empty_c720, eng1_native_code_target_00e470 }, /* record 81; RVA 0x4e634 */
    { eng1_native_code_target_00e520, eng1_empty_c720, eng1_native_code_target_00e560 }, /* record 82; RVA 0x4e640 */
    { eng1_native_code_target_00c780, eng1_empty_c720, eng1_native_code_target_00c7e0 }, /* record 83; RVA 0x4e64c */
    { eng1_native_code_target_00c8c0, eng1_empty_c720, eng1_native_code_target_00c910 }, /* record 84; RVA 0x4e658 */
    { eng1_native_code_target_00e3b0, eng1_empty_c720, eng1_native_code_target_00c910 }, /* record 85; RVA 0x4e664 */
    { eng1_native_code_target_0090c0, eng1_empty_c720, eng1_native_code_target_009100 }, /* record 86; RVA 0x4e670 */
  }, { 0U, 0U, 0U, 0U }
};
/* callback12_field_01c: 87 complete records; native span 0x4e268..0x4e680; 0 null pointer cells. */
/* Four observed zero bytes retained as explicit intertable padding. */

typedef struct Eng1FrameCallbackBlock_060778 {
    Eng1FrameCallbackTriple12 records[75];
    unsigned char zero_intertable_padding[4];
} Eng1FrameCallbackBlock_060778;
typedef char Eng1FrameCallbackBlock_060778_size[(sizeof(Eng1FrameCallbackBlock_060778) == 904) ? 1 : -1];
Eng1FrameCallbackBlock_060778 g_eng1_callback_record_block_060778 = {
  {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x60778 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x60784 */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x60790 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x6079c */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x607a8 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x607b4 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x607c0 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x607cc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x607d8 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x607e4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x607f0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x607fc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x60808 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x60814 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x60820 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x6082c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x60838 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x60844 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x60850 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x6085c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x60868 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x60874 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x60880 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x6088c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x60898 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x608a4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x608b0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x608bc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x608c8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x608d4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x608e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x608ec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x608f8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x60904 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x60910 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x6091c */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x60928 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x60934 */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x60940 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x6094c */
    { eng1_native_code_target_033fc0, eng1_empty_c720, eng1_native_code_target_034000 }, /* record 40; RVA 0x60958 */
    { eng1_native_code_target_0337f0, eng1_empty_c720, eng1_native_code_target_033830 }, /* record 41; RVA 0x60964 */
    { eng1_native_code_target_033910, eng1_empty_c720, eng1_native_code_target_033990 }, /* record 42; RVA 0x60970 */
    { eng1_native_code_target_0343a0, eng1_empty_c720, eng1_native_code_target_0343e0 }, /* record 43; RVA 0x6097c */
    { eng1_native_code_target_0348a0, eng1_empty_c720, eng1_native_code_target_034910 }, /* record 44; RVA 0x60988 */
    { eng1_native_code_target_033ee0, eng1_empty_c720, eng1_native_code_target_033f30 }, /* record 45; RVA 0x60994 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 46; RVA 0x609a0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 47; RVA 0x609ac */
    { eng1_native_code_target_0342f0, eng1_empty_c720, eng1_native_code_target_034350 }, /* record 48; RVA 0x609b8 */
    { eng1_native_code_target_0342f0, eng1_empty_c720, eng1_native_code_target_034350 }, /* record 49; RVA 0x609c4 */
    { eng1_native_code_target_0342f0, eng1_empty_c720, eng1_native_code_target_034350 }, /* record 50; RVA 0x609d0 */
    { eng1_native_code_target_034c80, eng1_empty_c720, eng1_native_code_target_034d00 }, /* record 51; RVA 0x609dc */
    { eng1_native_code_target_034de0, eng1_empty_c720, eng1_native_code_target_034e20 }, /* record 52; RVA 0x609e8 */
    { eng1_native_code_target_034c00, eng1_empty_c720, eng1_native_code_target_034c30 }, /* record 53; RVA 0x609f4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 54; RVA 0x60a00 */
    { eng1_native_code_target_024270, eng1_native_code_target_024320, eng1_native_code_target_024320 }, /* record 55; RVA 0x60a0c */
    { eng1_native_code_target_010540, eng1_empty_c720, eng1_native_code_target_0105a0 }, /* record 56; RVA 0x60a18 */
    { eng1_native_code_target_011c10, eng1_empty_c720, eng1_native_code_target_011df0 }, /* record 57; RVA 0x60a24 */
    { eng1_native_code_target_011c70, eng1_empty_c720, eng1_native_code_target_011df0 }, /* record 58; RVA 0x60a30 */
    { eng1_native_code_target_011cd0, eng1_empty_c720, eng1_native_code_target_011df0 }, /* record 59; RVA 0x60a3c */
    { eng1_native_code_target_011950, eng1_native_code_target_0119a0, eng1_empty_c720 }, /* record 60; RVA 0x60a48 */
    { eng1_native_code_target_011a80, eng1_native_code_target_011ac0, eng1_empty_c720 }, /* record 61; RVA 0x60a54 */
    { eng1_native_code_target_035200, eng1_empty_c720, eng1_native_code_target_0352f0 }, /* record 62; RVA 0x60a60 */
    { eng1_native_code_target_035c60, eng1_empty_c720, eng1_native_code_target_035d70 }, /* record 63; RVA 0x60a6c */
    { eng1_native_code_target_035cf0, eng1_empty_c720, eng1_native_code_target_035d70 }, /* record 64; RVA 0x60a78 */
    { eng1_native_code_target_0246c0, eng1_empty_c720, eng1_native_code_target_024780 }, /* record 65; RVA 0x60a84 */
    { eng1_native_code_target_0349f0, eng1_empty_c720, eng1_native_code_target_034a30 }, /* record 66; RVA 0x60a90 */
    { eng1_native_code_target_025580, eng1_empty_c720, eng1_native_code_target_0255c0 }, /* record 67; RVA 0x60a9c */
    { eng1_native_code_target_0341a0, eng1_empty_c720, eng1_native_code_target_034210 }, /* record 68; RVA 0x60aa8 */
    { eng1_native_code_target_033cc0, eng1_empty_c720, eng1_native_code_target_033cf0 }, /* record 69; RVA 0x60ab4 */
    { eng1_native_code_target_00f910, eng1_empty_c720, eng1_native_code_target_00fa00 }, /* record 70; RVA 0x60ac0 */
    { eng1_native_code_target_0115f0, eng1_empty_c720, eng1_native_code_target_011690 }, /* record 71; RVA 0x60acc */
    { load_coords_10380, eng1_empty_c720, eng1_native_code_target_0103c0 }, /* record 72; RVA 0x60ad8 */
    { eng1_native_code_target_036150, eng1_empty_c720, eng1_native_code_target_0361a0 }, /* record 73; RVA 0x60ae4 */
    { eng1_native_code_target_033e10, eng1_empty_c720, eng1_native_code_target_033e80 }, /* record 74; RVA 0x60af0 */
  }, { 0U, 0U, 0U, 0U }
};
/* callback12_field_01c: 75 complete records; native span 0x60778..0x60b00; 0 null pointer cells. */
/* Four observed zero bytes retained as explicit intertable padding. */

Eng1FrameCallbackTriple12 g_eng1_callback_record_prefix_057f10[54] = {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x57f10 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x57f1c */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x57f28 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x57f34 */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x57f40 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x57f4c */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x57f58 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x57f64 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x57f70 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x57f7c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x57f88 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x57f94 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x57fa0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x57fac */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x57fb8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x57fc4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x57fd0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x57fdc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x57fe8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x57ff4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x58000 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x5800c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x58018 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x58024 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x58030 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x5803c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x58048 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x58054 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x58060 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x5806c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x58078 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x58084 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x58090 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x5809c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x580a8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x580b4 */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x580c0 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x580cc */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x580d8 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x580e4 */
    { eng1_native_code_target_01dbc0, eng1_empty_c720, eng1_native_code_target_01dc00 }, /* record 40; RVA 0x580f0 */
    { eng1_native_code_target_01cf50, eng1_empty_c720, eng1_native_code_target_01d0b0 }, /* record 41; RVA 0x580fc */
    { select_side_sprite, eng1_empty_c720, eng1_empty_22180 }, /* record 42; RVA 0x58108 */
    { eng1_native_code_target_01bc20, eng1_empty_c720, eng1_native_code_target_01c200 }, /* record 43; RVA 0x58114 */
    { eng1_native_code_target_01ed00, eng1_empty_c720, eng1_native_code_target_01ed30 }, /* record 44; RVA 0x58120 */
    { eng1_native_code_target_01b2a0, eng1_empty_c720, eng1_native_code_target_01b7f0 }, /* record 45; RVA 0x5812c */
    { eng1_native_code_target_01c220, eng1_empty_c720, eng1_native_code_target_01c2a0 }, /* record 46; RVA 0x58138 */
    { eng1_native_code_target_01e320, eng1_empty_c720, eng1_native_code_target_01e3f0 }, /* record 47; RVA 0x58144 */
    { start_cycle_effect, eng1_empty_c720, advance_cycle_effect }, /* record 48; RVA 0x58150 */
    { eng1_native_code_target_01dd00, eng1_empty_c720, eng1_empty_22180 }, /* record 49; RVA 0x5815c */
    { start_sequence_effect, eng1_empty_c720, advance_sequence_effect }, /* record 50; RVA 0x58168 */
    { eng1_native_code_target_01e500, eng1_empty_c720, eng1_native_code_target_01e600 }, /* record 51; RVA 0x58174 */
    { initialize_wide_effect, eng1_empty_c720, eng1_native_code_target_01daf0 }, /* record 52; RVA 0x58180 */
    { initialize_wider_effect, eng1_empty_c720, eng1_native_code_target_01db70 }, /* record 53; RVA 0x5818c */
};
/* callback12_field_01c: 54 complete records; native span 0x57f10..0x58198; 0 null pointer cells. */

Eng1FrameCallbackTriple12 g_eng1_callback_record_prefix_059720[54] = {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x59720 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x5972c */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x59738 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x59744 */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x59750 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x5975c */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x59768 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x59774 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x59780 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x5978c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x59798 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x597a4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x597b0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x597bc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x597c8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x597d4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x597e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x597ec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x597f8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x59804 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x59810 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x5981c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x59828 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x59834 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x59840 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x5984c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x59858 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x59864 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x59870 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x5987c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x59888 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x59894 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x598a0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x598ac */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x598b8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x598c4 */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x598d0 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x598dc */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x598e8 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x598f4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 40; RVA 0x59900 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 41; RVA 0x5990c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 42; RVA 0x59918 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 43; RVA 0x59924 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 44; RVA 0x59930 */
    { eng1_native_code_target_022da0, eng1_empty_c720, eng1_native_code_target_022dd0 }, /* record 45; RVA 0x5993c */
    { set_wide_scale, eng1_empty_c720, eng1_native_code_target_021320 }, /* record 46; RVA 0x59948 */
    { reset_placement, eng1_empty_c720, eng1_empty_22180 }, /* record 47; RVA 0x59954 */
    { set_flags_two, eng1_empty_c720, eng1_native_code_target_023120 }, /* record 48; RVA 0x59960 */
    { eng1_native_code_target_0213c0, eng1_empty_c720, eng1_native_code_target_021400 }, /* record 49; RVA 0x5996c */
    { eng1_native_code_target_023a50, eng1_empty_c720, eng1_native_code_target_023b30 }, /* record 50; RVA 0x59978 */
    { show_sprite_82, eng1_empty_c720, eng1_native_code_target_023460 }, /* record 51; RVA 0x59984 */
    { eng1_native_code_target_021c00, eng1_empty_c720, eng1_native_code_target_021d10 }, /* record 52; RVA 0x59990 */
    { initialize_burst, eng1_empty_c720, eng1_native_code_target_021940 }, /* record 53; RVA 0x5999c */
};
/* callback12_field_01c: 54 complete records; native span 0x59720..0x599a8; 0 null pointer cells. */

typedef struct Eng1FrameCallbackBlock_061f80 {
    Eng1FrameCallbackTriple12 records[143];
    unsigned char zero_intertable_padding[4];
} Eng1FrameCallbackBlock_061f80;
typedef char Eng1FrameCallbackBlock_061f80_size[(sizeof(Eng1FrameCallbackBlock_061f80) == 1720) ? 1 : -1];
Eng1FrameCallbackBlock_061f80 g_eng1_callback_record_block_061f80 = {
  {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x61f80 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x61f8c */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x61f98 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x61fa4 */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x61fb0 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x61fbc */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x61fc8 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x61fd4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x61fe0 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x61fec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x61ff8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x62004 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x62010 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x6201c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x62028 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x62034 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x62040 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x6204c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x62058 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x62064 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x62070 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x6207c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x62088 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x62094 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x620a0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x620ac */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x620b8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x620c4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x620d0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x620dc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x620e8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x620f4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x62100 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x6210c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x62118 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x62124 */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x62130 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x6213c */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x62148 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x62154 */
    { eng1_native_code_target_038090, eng1_empty_c720, eng1_native_code_target_038100 }, /* record 40; RVA 0x62160 */
    { eng1_native_code_target_038590, eng1_empty_c720, eng1_native_code_target_038610 }, /* record 41; RVA 0x6216c */
    { eng1_native_code_target_038f50, eng1_empty_c720, eng1_native_code_target_039090 }, /* record 42; RVA 0x62178 */
    { eng1_native_code_target_039f20, eng1_empty_c720, eng1_native_code_target_03a040 }, /* record 43; RVA 0x62184 */
    { eng1_native_code_target_03abd0, eng1_empty_c720, eng1_native_code_target_03ac80 }, /* record 44; RVA 0x62190 */
    { eng1_native_code_target_03abd0, eng1_empty_c720, eng1_native_code_target_03ac80 }, /* record 45; RVA 0x6219c */
    { eng1_native_code_target_03abd0, eng1_empty_c720, eng1_native_code_target_03ac80 }, /* record 46; RVA 0x621a8 */
    { eng1_native_code_target_03abd0, eng1_empty_c720, eng1_native_code_target_03ac80 }, /* record 47; RVA 0x621b4 */
    { eng1_native_code_target_03abd0, eng1_empty_c720, eng1_native_code_target_03ac80 }, /* record 48; RVA 0x621c0 */
    { eng1_native_code_target_039360, eng1_empty_c720, eng1_native_code_target_0393b0 }, /* record 49; RVA 0x621cc */
    { eng1_native_code_target_039460, eng1_empty_c720, eng1_native_code_target_039530 }, /* record 50; RVA 0x621d8 */
    { eng1_native_code_target_0396e0, eng1_empty_c720, eng1_native_code_target_039750 }, /* record 51; RVA 0x621e4 */
    { eng1_native_code_target_039870, eng1_empty_c720, eng1_native_code_target_0398b0 }, /* record 52; RVA 0x621f0 */
    { eng1_native_code_target_0399c0, eng1_empty_c720, eng1_native_code_target_0399f0 }, /* record 53; RVA 0x621fc */
    { eng1_native_code_target_03aef0, eng1_empty_c720, eng1_native_code_target_03af20 }, /* record 54; RVA 0x62208 */
    { eng1_empty_c720, eng1_empty_c720, eng1_native_code_target_039a70 }, /* record 55; RVA 0x62214 */
    { eng1_native_code_target_03a420, eng1_empty_c720, eng1_native_code_target_03a4a0 }, /* record 56; RVA 0x62220 */
    { eng1_native_code_target_036d10, eng1_empty_c720, eng1_native_code_target_036d70 }, /* record 57; RVA 0x6222c */
    { eng1_native_code_target_036df0, eng1_empty_c720, eng1_native_code_target_036e10 }, /* record 58; RVA 0x62238 */
    { eng1_native_code_target_039bf0, eng1_empty_c720, eng1_empty_c720 }, /* record 59; RVA 0x62244 */
    { eng1_native_code_target_039c00, eng1_empty_c720, eng1_empty_c720 }, /* record 60; RVA 0x62250 */
    { eng1_native_code_target_039ae0, eng1_empty_c720, eng1_native_code_target_039b00 }, /* record 61; RVA 0x6225c */
    { eng1_native_code_target_03a610, eng1_empty_c720, eng1_native_code_target_03a700 }, /* record 62; RVA 0x62268 */
    { eng1_native_code_target_03a530, eng1_empty_c720, eng1_native_code_target_03a550 }, /* record 63; RVA 0x62274 */
    { eng1_native_code_target_03a730, eng1_empty_c720, eng1_native_code_target_03a770 }, /* record 64; RVA 0x62280 */
    { eng1_native_code_target_03a7a0, eng1_empty_c720, eng1_native_code_target_03a810 }, /* record 65; RVA 0x6228c */
    { eng1_native_code_target_03a8a0, eng1_empty_c720, eng1_native_code_target_03a8e0 }, /* record 66; RVA 0x62298 */
    { eng1_native_code_target_03a910, eng1_empty_c720, eng1_native_code_target_03a950 }, /* record 67; RVA 0x622a4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_native_code_target_03a980 }, /* record 68; RVA 0x622b0 */
    { eng1_native_code_target_03abd0, eng1_empty_c720, eng1_native_code_target_03adc0 }, /* record 69; RVA 0x622bc */
    { eng1_native_code_target_03b050, eng1_empty_c720, eng1_native_code_target_03b170 }, /* record 70; RVA 0x622c8 */
    { eng1_native_code_target_03b080, eng1_empty_c720, eng1_native_code_target_03b170 }, /* record 71; RVA 0x622d4 */
    { eng1_native_code_target_03b0b0, eng1_empty_c720, eng1_native_code_target_03b170 }, /* record 72; RVA 0x622e0 */
    { eng1_native_code_target_03b0e0, eng1_empty_c720, eng1_native_code_target_03b170 }, /* record 73; RVA 0x622ec */
    { eng1_native_code_target_03b110, eng1_empty_c720, eng1_native_code_target_03b170 }, /* record 74; RVA 0x622f8 */
    { eng1_native_code_target_03b140, eng1_empty_c720, eng1_native_code_target_03b170 }, /* record 75; RVA 0x62304 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 76; RVA 0x62310 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 77; RVA 0x6231c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 78; RVA 0x62328 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 79; RVA 0x62334 */
    { eng1_native_code_target_036cb0, eng1_empty_c720, eng1_native_code_target_036ce0 }, /* record 80; RVA 0x62340 */
    { eng1_native_code_target_038b00, eng1_empty_c720, eng1_native_code_target_038b30 }, /* record 81; RVA 0x6234c */
    { eng1_native_code_target_038950, eng1_empty_c720, eng1_native_code_target_0389c0 }, /* record 82; RVA 0x62358 */
    { eng1_native_code_target_038950, eng1_empty_c720, eng1_native_code_target_0389c0 }, /* record 83; RVA 0x62364 */
    { eng1_native_code_target_0383b0, eng1_empty_c720, eng1_native_code_target_038310 }, /* record 84; RVA 0x62370 */
    { eng1_native_code_target_038d10, eng1_empty_c720, eng1_native_code_target_038d70 }, /* record 85; RVA 0x6237c */
    { eng1_native_code_target_039c20, eng1_empty_c720, eng1_native_code_target_039c50 }, /* record 86; RVA 0x62388 */
    { eng1_native_code_target_03aa10, eng1_empty_c720, eng1_native_code_target_03aa70 }, /* record 87; RVA 0x62394 */
    { eng1_native_code_target_0383b0, eng1_empty_c720, eng1_native_code_target_0383d0 }, /* record 88; RVA 0x623a0 */
    { eng1_native_code_target_038de0, eng1_empty_c720, eng1_native_code_target_038e30 }, /* record 89; RVA 0x623ac */
    { eng1_native_code_target_039ca0, eng1_empty_c720, eng1_native_code_target_039cf0 }, /* record 90; RVA 0x623b8 */
    { eng1_native_code_target_03aaf0, eng1_empty_c720, eng1_native_code_target_03ab50 }, /* record 91; RVA 0x623c4 */
    { eng1_native_code_target_03b190, eng1_empty_c720, eng1_native_code_target_03b310 }, /* record 92; RVA 0x623d0 */
    { eng1_native_code_target_03bc20, eng1_empty_c720, eng1_native_code_target_03bde0 }, /* record 93; RVA 0x623dc */
    { eng1_native_code_target_03c050, eng1_empty_c720, eng1_native_code_target_03c070 }, /* record 94; RVA 0x623e8 */
    { eng1_native_code_target_03c050, eng1_empty_c720, eng1_native_code_target_03c070 }, /* record 95; RVA 0x623f4 */
    { eng1_native_code_target_03c050, eng1_empty_c720, eng1_native_code_target_03c070 }, /* record 96; RVA 0x62400 */
    { eng1_native_code_target_03b9d0, eng1_empty_c720, eng1_native_code_target_03ba00 }, /* record 97; RVA 0x6240c */
    { eng1_native_code_target_03b7f0, eng1_empty_c720, eng1_native_code_target_03b980 }, /* record 98; RVA 0x62418 */
    { eng1_native_code_target_03b530, eng1_empty_c720, eng1_native_code_target_03b980 }, /* record 99; RVA 0x62424 */
    { eng1_native_code_target_03b640, eng1_empty_c720, eng1_native_code_target_03b980 }, /* record 100; RVA 0x62430 */
    { eng1_native_code_target_03b730, eng1_empty_c720, eng1_native_code_target_03b980 }, /* record 101; RVA 0x6243c */
    { eng1_native_code_target_03b8b0, eng1_empty_c720, eng1_native_code_target_03b980 }, /* record 102; RVA 0x62448 */
    { eng1_native_code_target_03b970, eng1_empty_c720, eng1_native_code_target_03b980 }, /* record 103; RVA 0x62454 */
    { eng1_native_code_target_038740, eng1_empty_c720, eng1_native_code_target_0387f0 }, /* record 104; RVA 0x62460 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 105; RVA 0x6246c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 106; RVA 0x62478 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 107; RVA 0x62484 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 108; RVA 0x62490 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 109; RVA 0x6249c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 110; RVA 0x624a8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 111; RVA 0x624b4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 112; RVA 0x624c0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 113; RVA 0x624cc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 114; RVA 0x624d8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 115; RVA 0x624e4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 116; RVA 0x624f0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 117; RVA 0x624fc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 118; RVA 0x62508 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 119; RVA 0x62514 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 120; RVA 0x62520 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 121; RVA 0x6252c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 122; RVA 0x62538 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 123; RVA 0x62544 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 124; RVA 0x62550 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 125; RVA 0x6255c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 126; RVA 0x62568 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 127; RVA 0x62574 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 128; RVA 0x62580 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 129; RVA 0x6258c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 130; RVA 0x62598 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 131; RVA 0x625a4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 132; RVA 0x625b0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 133; RVA 0x625bc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 134; RVA 0x625c8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 135; RVA 0x625d4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 136; RVA 0x625e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 137; RVA 0x625ec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 138; RVA 0x625f8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 139; RVA 0x62604 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 140; RVA 0x62610 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 141; RVA 0x6261c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 142; RVA 0x62628 */
  }, { 0U, 0U, 0U, 0U }
};
/* callback12_field_01c: 143 complete records; native span 0x61f80..0x62638; 0 null pointer cells. */
/* Four observed zero bytes retained as explicit intertable padding. */

typedef struct Eng1FrameCallbackBlock_06a968 {
    Eng1FrameCallbackTriple12 records[43];
    unsigned char zero_intertable_padding[4];
} Eng1FrameCallbackBlock_06a968;
typedef char Eng1FrameCallbackBlock_06a968_size[(sizeof(Eng1FrameCallbackBlock_06a968) == 520) ? 1 : -1];
Eng1FrameCallbackBlock_06a968 g_eng1_callback_record_block_06a968 = {
  {
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x6a968 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 1; RVA 0x6a974 */
    { eng1_native_code_target_013ca0, eng1_empty_c720, eng1_native_code_target_013d70 }, /* record 2; RVA 0x6a980 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 3; RVA 0x6a98c */
    { eng1_native_code_target_014790, eng1_empty_c720, eng1_native_code_target_014850 }, /* record 4; RVA 0x6a998 */
    { eng1_native_code_target_014210, eng1_empty_c720, eng1_native_code_target_0142d0 }, /* record 5; RVA 0x6a9a4 */
    { eng1_native_code_target_014c50, eng1_empty_c720, eng1_native_code_target_014cf0 }, /* record 6; RVA 0x6a9b0 */
    { eng1_native_code_target_014fb0, eng1_empty_c720, eng1_native_code_target_015000 }, /* record 7; RVA 0x6a9bc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x6a9c8 */
    { copy_pair_14de0, eng1_empty_c720, eng1_native_code_target_014e10 }, /* record 9; RVA 0x6a9d4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 10; RVA 0x6a9e0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 11; RVA 0x6a9ec */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x6a9f8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x6aa04 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x6aa10 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x6aa1c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 16; RVA 0x6aa28 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 17; RVA 0x6aa34 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 18; RVA 0x6aa40 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x6aa4c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x6aa58 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x6aa64 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x6aa70 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x6aa7c */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x6aa88 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x6aa94 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x6aaa0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x6aaac */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x6aab8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x6aac4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x6aad0 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x6aadc */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x6aae8 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x6aaf4 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x6ab00 */
    { eng1_empty_c720, eng1_empty_c720, eng1_empty_c720 }, /* record 35; RVA 0x6ab0c */
    { eng1_native_code_target_001010, eng1_empty_c720, eng1_empty_c720 }, /* record 36; RVA 0x6ab18 */
    { eng1_native_code_target_00f430, eng1_empty_c720, eng1_native_code_target_00f4f0 }, /* record 37; RVA 0x6ab24 */
    { eng1_native_code_target_001040, eng1_empty_c720, eng1_native_code_target_0010f0 }, /* record 38; RVA 0x6ab30 */
    { eng1_native_code_target_001310, eng1_empty_c720, eng1_native_code_target_0013c0 }, /* record 39; RVA 0x6ab3c */
    { eng1_native_code_target_0445a0, eng1_empty_c720, eng1_native_code_target_0447c0 }, /* record 40; RVA 0x6ab48 */
    { reset_links_45a50, eng1_empty_c720, eng1_native_code_target_045a80 }, /* record 41; RVA 0x6ab54 */
    { eng1_native_code_target_045e20, eng1_empty_c720, eng1_native_code_target_045ef0 }, /* record 42; RVA 0x6ab60 */
  }, { 0U, 0U, 0U, 0U }
};
/* callback12_field_01c: 43 complete records; native span 0x6a968..0x6ab70; 0 null pointer cells. */
/* Four observed zero bytes retained as explicit intertable padding. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_067bd0[66] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x67bd0 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x67bd8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x67be0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x67be8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x67bf0 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x67bf8 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x67c00 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x67c08 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x67c10 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x67c18 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x67c20 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x67c28 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x67c30 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x67c38 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x67c40 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x67c48 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x67c50 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x67c58 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x67c60 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x67c68 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x67c70 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x67c78 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x67c80 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x67c88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x67c90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x67c98 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x67ca0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x67ca8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x67cb0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x67cb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x67cc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x67cc8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x67cd0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x67cd8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x67ce0 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x67ce8 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x67cf0 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x67cf8 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x67d00 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x67d08 */
    { eng1_native_code_target_03fa50, eng1_native_code_target_03fb00 }, /* record 40; RVA 0x67d10 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 41; RVA 0x67d18 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 42; RVA 0x67d20 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 43; RVA 0x67d28 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 44; RVA 0x67d30 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 45; RVA 0x67d38 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 46; RVA 0x67d40 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 47; RVA 0x67d48 */
    { eng1_native_code_target_00d380, eng1_native_code_target_043ea0 }, /* record 48; RVA 0x67d50 */
    { eng1_native_code_target_040a50, apply_scaled_offsets_40ab0 }, /* record 49; RVA 0x67d58 */
    { set_collide_flags_3eaa0, eng1_empty_c720 }, /* record 50; RVA 0x67d60 */
    { set_collide_flags_3eac0, eng1_native_code_target_03eaf0 }, /* record 51; RVA 0x67d68 */
    { eng1_native_code_target_040a50, apply_scaled_offsets_40ab0 }, /* record 52; RVA 0x67d70 */
    { eng1_native_code_target_041a50, eng1_native_code_target_041ad0 }, /* record 53; RVA 0x67d78 */
    { eng1_native_code_target_041c30, eng1_empty_c720 }, /* record 54; RVA 0x67d80 */
    { eng1_empty_c720, eng1_native_code_target_041f00 }, /* record 55; RVA 0x67d88 */
    { eng1_native_code_target_040ba0, eng1_native_code_target_040c10 }, /* record 56; RVA 0x67d90 */
    { set_flags_3f040, eng1_empty_c720 }, /* record 57; RVA 0x67d98 */
    { eng1_native_code_target_03ea00, eng1_native_code_target_03ea40 }, /* record 58; RVA 0x67da0 */
    { eng1_native_code_target_03ea70, eng1_empty_c720 }, /* record 59; RVA 0x67da8 */
    { eng1_native_code_target_0407b0, eng1_native_code_target_0408b0 }, /* record 60; RVA 0x67db0 */
    { eng1_native_code_target_03fcf0, eng1_native_code_target_03fe00 }, /* record 61; RVA 0x67db8 */
    { eng1_native_code_target_041a50, eng1_native_code_target_041b70 }, /* record 62; RVA 0x67dc0 */
    { eng1_native_code_target_042390, eng1_native_code_target_042540 }, /* record 63; RVA 0x67dc8 */
    { eng1_native_code_target_0429c0, eng1_native_code_target_042bd0 }, /* record 64; RVA 0x67dd0 */
    { eng1_empty_c720, eng1_native_code_target_043330 }, /* record 65; RVA 0x67dd8 */
};
/* callback8_field_024: 66 complete records; native span 0x67bd0..0x67de0; 0 null pointer cells. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_04e8c8[73] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x4e8c8 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x4e8d0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x4e8d8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x4e8e0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x4e8e8 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x4e8f0 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x4e8f8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x4e900 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x4e908 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x4e910 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x4e918 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x4e920 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x4e928 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x4e930 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x4e938 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x4e940 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x4e948 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x4e950 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x4e958 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x4e960 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x4e968 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x4e970 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x4e978 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x4e980 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x4e988 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x4e990 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x4e998 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x4e9a0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x4e9a8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x4e9b0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x4e9b8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x4e9c0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x4e9c8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x4e9d0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x4e9d8 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x4e9e0 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x4e9e8 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x4e9f0 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x4e9f8 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x4ea00 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 40; RVA 0x4ea08 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 41; RVA 0x4ea10 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 42; RVA 0x4ea18 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 43; RVA 0x4ea20 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 44; RVA 0x4ea28 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 45; RVA 0x4ea30 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 46; RVA 0x4ea38 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 47; RVA 0x4ea40 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 48; RVA 0x4ea48 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 49; RVA 0x4ea50 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 50; RVA 0x4ea58 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 51; RVA 0x4ea60 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 52; RVA 0x4ea68 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 53; RVA 0x4ea70 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 54; RVA 0x4ea78 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 55; RVA 0x4ea80 */
    { eng1_native_code_target_00cb80, eng1_native_code_target_00cc30 }, /* record 56; RVA 0x4ea88 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00d390 }, /* record 57; RVA 0x4ea90 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00d3d0 }, /* record 58; RVA 0x4ea98 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 59; RVA 0x4eaa0 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 60; RVA 0x4eaa8 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 61; RVA 0x4eab0 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 62; RVA 0x4eab8 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 63; RVA 0x4eac0 */
    { eng1_native_code_target_00c920, eng1_native_code_target_00c940 }, /* record 64; RVA 0x4eac8 */
    { eng1_native_code_target_00d190, eng1_native_code_target_00c940 }, /* record 65; RVA 0x4ead0 */
    { eng1_native_code_target_00d190, eng1_native_code_target_00c940 }, /* record 66; RVA 0x4ead8 */
    { eng1_native_code_target_00d190, eng1_native_code_target_00c940 }, /* record 67; RVA 0x4eae0 */
    { eng1_native_code_target_00d190, eng1_native_code_target_00c940 }, /* record 68; RVA 0x4eae8 */
    { eng1_native_code_target_00d190, eng1_native_code_target_00c940 }, /* record 69; RVA 0x4eaf0 */
    { eng1_native_code_target_00d190, eng1_native_code_target_00c940 }, /* record 70; RVA 0x4eaf8 */
    { eng1_native_code_target_00baf0, eng1_native_code_target_03bc00 }, /* record 71; RVA 0x4eb00 */
    { set_limit_e610, eng1_native_code_target_00e620 }, /* record 72; RVA 0x4eb08 */
};
/* callback8_field_024: 73 complete records; native span 0x4e8c8..0x4eb10; 0 null pointer cells. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_060c70[46] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x60c70 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x60c78 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x60c80 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x60c88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x60c90 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x60c98 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x60ca0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x60ca8 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x60cb0 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x60cb8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x60cc0 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x60cc8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x60cd0 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x60cd8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x60ce0 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x60ce8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x60cf0 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x60cf8 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x60d00 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x60d08 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x60d10 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x60d18 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x60d20 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x60d28 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x60d30 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x60d38 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x60d40 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x60d48 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x60d50 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x60d58 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x60d60 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x60d68 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x60d70 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x60d78 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x60d80 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x60d88 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x60d90 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x60d98 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x60da0 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x60da8 */
    { eng1_empty_c720, eng1_native_code_target_024b60 }, /* record 40; RVA 0x60db0 */
    { eng1_empty_c720, eng1_native_code_target_024f50 }, /* record 41; RVA 0x60db8 */
    { eng1_native_code_target_034650, eng1_native_code_target_034750 }, /* record 42; RVA 0x60dc0 */
    { eng1_native_code_target_035a30, eng1_native_code_target_035b20 }, /* record 43; RVA 0x60dc8 */
    { eng1_native_code_target_036030, eng1_empty_c720 }, /* record 44; RVA 0x60dd0 */
    { eng1_native_code_target_0363e0, eng1_native_code_target_0365f0 }, /* record 45; RVA 0x60dd8 */
};
/* callback8_field_024: 46 complete records; native span 0x60c70..0x60de0; 0 null pointer cells. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_0582e0[41] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x582e0 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x582e8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x582f0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x582f8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x58300 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x58308 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x58310 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x58318 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x58320 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x58328 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x58330 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x58338 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x58340 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x58348 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x58350 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x58358 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x58360 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x58368 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x58370 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x58378 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x58380 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x58388 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x58390 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x58398 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x583a0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x583a8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x583b0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x583b8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x583c0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x583c8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x583d0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x583d8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x583e0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x583e8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x583f0 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x583f8 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x58400 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x58408 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x58410 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x58418 */
    { eng1_native_code_target_01b230, eng1_empty_c720 }, /* record 40; RVA 0x58420 */
};
/* callback8_field_024: 41 complete records; native span 0x582e0..0x58428; 0 null pointer cells. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_059af8[42] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x59af8 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x59b00 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x59b08 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x59b10 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x59b18 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x59b20 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x59b28 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x59b30 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x59b38 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x59b40 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x59b48 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x59b50 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x59b58 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x59b60 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x59b68 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x59b70 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x59b78 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x59b80 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x59b88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x59b90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x59b98 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x59ba0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x59ba8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x59bb0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x59bb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x59bc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x59bc8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x59bd0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x59bd8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x59be0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x59be8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x59bf0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x59bf8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x59c00 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x59c08 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x59c10 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x59c18 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x59c20 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x59c28 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x59c30 */
    { 0, 0 }, /* record 40; RVA 0x59c38 */
    { eng1_native_code_target_0237d0, eng1_native_code_target_023840 }, /* record 41; RVA 0x59c40 */
};
/* callback8_field_024: 42 complete records; native span 0x59af8..0x59c48; 2 null pointer cells. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_062790[50] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x62790 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x62798 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x627a0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x627a8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x627b0 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x627b8 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x627c0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x627c8 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x627d0 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x627d8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x627e0 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x627e8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x627f0 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x627f8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x62800 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x62808 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x62810 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x62818 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x62820 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x62828 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x62830 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x62838 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x62840 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x62848 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x62850 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x62858 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x62860 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x62868 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x62870 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x62878 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x62880 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x62888 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x62890 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x62898 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x628a0 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x628a8 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x628b0 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x628b8 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x628c0 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x628c8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 40; RVA 0x628d0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 41; RVA 0x628d8 */
    { eng1_native_code_target_03bbe0, eng1_native_code_target_03bc00 }, /* record 42; RVA 0x628e0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 43; RVA 0x628e8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 44; RVA 0x628f0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 45; RVA 0x628f8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 46; RVA 0x62900 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 47; RVA 0x62908 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 48; RVA 0x62910 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 49; RVA 0x62918 */
};
/* callback8_field_024: 50 complete records; native span 0x62790..0x62920; 0 null pointer cells. */

Eng1FrameCallbackPair8 g_eng1_callback_record_prefix_06acb0[40] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x6acb0 */
    { eng1_native_code_target_01a440, eng1_native_code_target_01a5a0 }, /* record 1; RVA 0x6acb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 2; RVA 0x6acc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 3; RVA 0x6acc8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 4; RVA 0x6acd0 */
    { eng1_native_code_target_019f50, eng1_native_code_target_01a0f0 }, /* record 5; RVA 0x6acd8 */
    { eng1_native_code_target_01a2b0, eng1_native_code_target_01a390 }, /* record 6; RVA 0x6ace0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x6ace8 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 8; RVA 0x6acf0 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 9; RVA 0x6acf8 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 10; RVA 0x6ad00 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 11; RVA 0x6ad08 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 12; RVA 0x6ad10 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 13; RVA 0x6ad18 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 14; RVA 0x6ad20 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 15; RVA 0x6ad28 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 16; RVA 0x6ad30 */
    { eng1_native_code_target_013990, eng1_native_code_target_0139f0 }, /* record 17; RVA 0x6ad38 */
    { eng1_empty_c720, eng1_native_code_target_013570 }, /* record 18; RVA 0x6ad40 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 19; RVA 0x6ad48 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 20; RVA 0x6ad50 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 21; RVA 0x6ad58 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 22; RVA 0x6ad60 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 23; RVA 0x6ad68 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 24; RVA 0x6ad70 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 25; RVA 0x6ad78 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 26; RVA 0x6ad80 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 27; RVA 0x6ad88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 28; RVA 0x6ad90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 29; RVA 0x6ad98 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x6ada0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x6ada8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x6adb0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x6adb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 34; RVA 0x6adc0 */
    { eng1_native_code_target_00fd00, eng1_native_code_target_00fe60 }, /* record 35; RVA 0x6adc8 */
    { eng1_empty_c720, eng1_native_code_target_0102c0 }, /* record 36; RVA 0x6add0 */
    { eng1_native_code_target_00d380, eng1_native_code_target_00ee70 }, /* record 37; RVA 0x6add8 */
    { eng1_native_code_target_040c90, eng1_native_code_target_040de0 }, /* record 38; RVA 0x6ade0 */
    { init_fields_1d50, eng1_native_code_target_001d80 }, /* record 39; RVA 0x6ade8 */
};
/* callback8_field_024: 40 complete records; native span 0x6acb0..0x6adf0; 0 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_067f80[52] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x67f80 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x67f88 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x67f90 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x67f98 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x67fa0 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x67fa8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x67fb0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x67fb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x67fc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x67fc8 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x67fd0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x67fd8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x67fe0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x67fe8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x67ff0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x67ff8 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x68000 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x68008 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x68010 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x68018 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x68020 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x68028 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x68030 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x68038 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x68040 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x68048 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x68050 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x68058 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x68060 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x68068 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x68070 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x68078 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x68080 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x68088 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x68090 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x68098 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x680a0 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x680a8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x680b0 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x680b8 */
    { eng1_empty_c720, copy_link_state_41fa0 }, /* record 40; RVA 0x680c0 */
    { eng1_native_code_target_02d9b0, eng1_native_code_target_02d9e0 }, /* record 41; RVA 0x680c8 */
    { set_state_flags_41300, eng1_native_code_target_041320 }, /* record 42; RVA 0x680d0 */
    { eng1_native_code_target_02da10, eng1_native_code_target_02da40 }, /* record 43; RVA 0x680d8 */
    { eng1_native_code_target_042220, eng1_native_code_target_042280 }, /* record 44; RVA 0x680e0 */
    { eng1_native_code_target_0417e0, eng1_native_code_target_041810 }, /* record 45; RVA 0x680e8 */
    { eng1_native_code_target_0417e0, eng1_native_code_target_041810 }, /* record 46; RVA 0x680f0 */
    { adjust_bounds_3f070, eng1_native_code_target_03f0b0 }, /* record 47; RVA 0x680f8 */
    { set_kind_422d0, eng1_native_code_target_0422f0 }, /* record 48; RVA 0x68100 */
    { eng1_native_code_target_00d380, eng1_native_code_target_041810 }, /* record 49; RVA 0x68108 */
    { eng1_native_code_target_040950, eng1_native_code_target_040980 }, /* record 50; RVA 0x68110 */
    { eng1_native_code_target_00eec0, eng1_native_code_target_00eee0 }, /* record 51; RVA 0x68118 */
};
/* callback8_field_02c: 52 complete records; native span 0x67f80..0x68120; 0 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_04ec50[40] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x4ec50 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x4ec58 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x4ec60 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x4ec68 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x4ec70 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x4ec78 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x4ec80 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x4ec88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x4ec90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x4ec98 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x4eca0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x4eca8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x4ecb0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x4ecb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x4ecc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x4ecc8 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x4ecd0 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x4ecd8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x4ece0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x4ece8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x4ecf0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x4ecf8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x4ed00 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x4ed08 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x4ed10 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x4ed18 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x4ed20 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x4ed28 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x4ed30 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x4ed38 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x4ed40 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x4ed48 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x4ed50 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x4ed58 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x4ed60 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x4ed68 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x4ed70 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x4ed78 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x4ed80 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x4ed88 */
};
/* callback8_field_02c: 40 complete records; native span 0x4ec50..0x4ed90; 0 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_060f58[47] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x60f58 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x60f60 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x60f68 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x60f70 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x60f78 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x60f80 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x60f88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x60f90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x60f98 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x60fa0 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x60fa8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x60fb0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x60fb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x60fc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x60fc8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x60fd0 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x60fd8 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x60fe0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x60fe8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x60ff0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x60ff8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x61000 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x61008 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x61010 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x61018 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x61020 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x61028 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x61030 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x61038 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x61040 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x61048 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x61050 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x61058 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x61060 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x61068 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x61070 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x61078 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x61080 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x61088 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x61090 */
    { set_mode_flags_36120, eng1_empty_c720 }, /* record 40; RVA 0x61098 */
    { init_defaults_117d0, eng1_native_code_target_011810 }, /* record 41; RVA 0x610a0 */
    { eng1_native_code_target_033d70, eng1_native_code_target_033db0 }, /* record 42; RVA 0x610a8 */
    { eng1_native_code_target_0360a0, eng1_native_code_target_0360f0 }, /* record 43; RVA 0x610b0 */
    { reset_render_parameter, eng1_native_code_target_0253d0 }, /* record 44; RVA 0x610b8 */
    { reset_extent_36b60, eng1_native_code_target_036b90 }, /* record 45; RVA 0x610c0 */
    { eng1_native_code_target_035fb0, eng1_native_code_target_035fe0 }, /* record 46; RVA 0x610c8 */
};
/* callback8_field_02c: 47 complete records; native span 0x60f58..0x610d0; 0 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_0585c0[51] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x585c0 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x585c8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x585d0 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x585d8 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x585e0 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x585e8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x585f0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x585f8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x58600 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x58608 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x58610 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x58618 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x58620 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x58628 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x58630 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x58638 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x58640 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x58648 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x58650 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x58658 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x58660 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x58668 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x58670 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x58678 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x58680 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x58688 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x58690 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x58698 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x586a0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x586a8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x586b0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x586b8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x586c0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x586c8 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x586d0 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x586d8 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x586e0 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x586e8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x586f0 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x586f8 */
    { eng1_native_code_target_01bca0, eng1_empty_c720 }, /* record 40; RVA 0x58700 */
    { eng1_native_code_target_01fea0, eng1_native_code_target_01ff50 }, /* record 41; RVA 0x58708 */
    { initialize_low_backdrop, eng1_empty_22180 }, /* record 42; RVA 0x58710 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 43; RVA 0x58718 */
    { eng1_native_code_target_01eba0, eng1_native_code_target_01ebf0 }, /* record 44; RVA 0x58720 */
    { eng1_native_code_target_01e950, eng1_native_code_target_01e9d0 }, /* record 45; RVA 0x58728 */
    { initialize_high_backdrop, eng1_native_code_target_01ea90 }, /* record 46; RVA 0x58730 */
    { eng1_native_code_target_01dfc0, eng1_native_code_target_01e020 }, /* record 47; RVA 0x58738 */
    { initialize_flagged_effect, follow_attachment_scale }, /* record 48; RVA 0x58740 */
    { eng1_native_code_target_01e870, eng1_native_code_target_01e8b0 }, /* record 49; RVA 0x58748 */
    { clear_effect_animation, eng1_native_code_target_01e910 }, /* record 50; RVA 0x58750 */
};
/* callback8_field_02c: 51 complete records; native span 0x585c0..0x58758; 0 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_059de8[52] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x59de8 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x59df0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x59df8 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x59e00 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x59e08 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x59e10 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x59e18 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x59e20 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x59e28 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x59e30 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x59e38 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x59e40 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x59e48 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x59e50 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x59e58 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x59e60 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x59e68 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x59e70 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x59e78 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x59e80 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x59e88 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x59e90 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x59e98 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x59ea0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x59ea8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x59eb0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x59eb8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x59ec0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x59ec8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x59ed0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x59ed8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x59ee0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x59ee8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x59ef0 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x59ef8 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x59f00 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x59f08 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x59f10 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x59f18 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x59f20 */
    { initialize_particle, eng1_native_code_target_021710 }, /* record 40; RVA 0x59f28 */
    { eng1_native_code_target_021740, eng1_native_code_target_021780 }, /* record 41; RVA 0x59f30 */
    { eng1_native_code_target_022b90, eng1_native_code_target_022c20 }, /* record 42; RVA 0x59f38 */
    { eng1_native_code_target_022d20, eng1_native_code_target_022d60 }, /* record 43; RVA 0x59f40 */
    { show_sprite_7a, eng1_native_code_target_023660 }, /* record 44; RVA 0x59f48 */
    { initialize_backdrop, eng1_empty_22180 }, /* record 45; RVA 0x59f50 */
    { 0, 0 }, /* record 46; RVA 0x59f58 */
    { initialize_linked_particle, follow_owner_scale }, /* record 47; RVA 0x59f60 */
    { eng1_native_code_target_023680, eng1_native_code_target_0236e0 }, /* record 48; RVA 0x59f68 */
    { 0, 0 }, /* record 49; RVA 0x59f70 */
    { eng1_empty_22180, eng1_empty_22180 }, /* record 50; RVA 0x59f78 */
    { eng1_native_code_target_021980, eng1_native_code_target_0219e0 }, /* record 51; RVA 0x59f80 */
};
/* callback8_field_02c: 52 complete records; native span 0x59de8..0x59f88; 4 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_062a60[40] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x62a60 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x62a68 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x62a70 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x62a78 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x62a80 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x62a88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x62a90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x62a98 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x62aa0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x62aa8 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x62ab0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x62ab8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x62ac0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x62ac8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x62ad0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x62ad8 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x62ae0 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x62ae8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x62af0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x62af8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x62b00 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x62b08 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x62b10 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x62b18 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x62b20 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x62b28 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x62b30 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x62b38 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x62b40 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x62b48 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x62b50 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x62b58 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x62b60 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x62b68 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x62b70 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x62b78 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x62b80 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x62b88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x62b90 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x62b98 */
};
/* callback8_field_02c: 40 complete records; native span 0x62a60..0x62ba0; 0 null pointer cells. */

Eng1FrameFactoryPair8 g_eng1_callback_record_prefix_06af60[46] = {
    { eng1_empty_c720, eng1_empty_c720 }, /* record 0; RVA 0x6af60 */
    { eng1_native_code_target_015030, eng1_native_code_target_015060 }, /* record 1; RVA 0x6af68 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 2; RVA 0x6af70 */
    { eng1_native_code_target_01a1d0, eng1_native_code_target_041810 }, /* record 3; RVA 0x6af78 */
    { eng1_empty_c720, eng1_native_code_target_01a950 }, /* record 4; RVA 0x6af80 */
    { set_mode_13c90, eng1_empty_c720 }, /* record 5; RVA 0x6af88 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 6; RVA 0x6af90 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 7; RVA 0x6af98 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 8; RVA 0x6afa0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 9; RVA 0x6afa8 */
    { eng1_native_code_target_013840, eng1_native_code_target_013880 }, /* record 10; RVA 0x6afb0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 11; RVA 0x6afb8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 12; RVA 0x6afc0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 13; RVA 0x6afc8 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 14; RVA 0x6afd0 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 15; RVA 0x6afd8 */
    { eng1_native_code_target_012a10, eng1_native_code_target_012b00 }, /* record 16; RVA 0x6afe0 */
    { eng1_native_code_target_012ed0, eng1_native_code_target_041810 }, /* record 17; RVA 0x6afe8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 18; RVA 0x6aff0 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 19; RVA 0x6aff8 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 20; RVA 0x6b000 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 21; RVA 0x6b008 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 22; RVA 0x6b010 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 23; RVA 0x6b018 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 24; RVA 0x6b020 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 25; RVA 0x6b028 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 26; RVA 0x6b030 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 27; RVA 0x6b038 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 28; RVA 0x6b040 */
    { eng1_native_code_target_012c00, eng1_native_code_target_012d80 }, /* record 29; RVA 0x6b048 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 30; RVA 0x6b050 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 31; RVA 0x6b058 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 32; RVA 0x6b060 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 33; RVA 0x6b068 */
    { eng1_native_code_target_00efa0, eng1_native_code_target_00efe0 }, /* record 34; RVA 0x6b070 */
    { init_flags_f070, eng1_native_code_target_00f0a0 }, /* record 35; RVA 0x6b078 */
    { init_flags_f070, eng1_native_code_target_00f180 }, /* record 36; RVA 0x6b080 */
    { eng1_native_code_target_00f380, eng1_native_code_target_00f3b0 }, /* record 37; RVA 0x6b088 */
    { eng1_empty_c720, eng1_empty_c720 }, /* record 38; RVA 0x6b090 */
    { eng1_native_code_target_001e00, eng1_native_code_target_001e30 }, /* record 39; RVA 0x6b098 */
    { 0, 0 }, /* record 40; RVA 0x6b0a0 */
    { eng1_native_code_target_045930, eng1_native_code_target_045960 }, /* record 41; RVA 0x6b0a8 */
    { eng1_native_code_target_045cc0, eng1_native_code_target_045d50 }, /* record 42; RVA 0x6b0b0 */
    { eng1_native_code_target_046230, eng1_native_code_target_046320 }, /* record 43; RVA 0x6b0b8 */
    { set_flags_461d0, eng1_native_code_target_0461f0 }, /* record 44; RVA 0x6b0c0 */
    { set_flags_459e0, eng1_native_code_target_045a00 }, /* record 45; RVA 0x6b0c8 */
};
/* callback8_field_02c: 46 complete records; native span 0x6af60..0x6b0d0; 2 null pointer cells. */

/* Distinct same-TU views to all callback family roots. */
#define eng1_frame_target_067608 ((unsigned char *)&g_eng1_callback_record_block_067608.records[0])
#define eng1_frame_target_04e268 ((unsigned char *)&g_eng1_callback_record_block_04e268.records[0])
#define eng1_frame_target_060778 ((unsigned char *)&g_eng1_callback_record_block_060778.records[0])
#define eng1_frame_target_057f10 ((unsigned char *)&g_eng1_callback_record_prefix_057f10[0])
#define eng1_frame_target_059720 ((unsigned char *)&g_eng1_callback_record_prefix_059720[0])
#define eng1_frame_target_061f80 ((unsigned char *)&g_eng1_callback_record_block_061f80.records[0])
#define eng1_frame_target_06a968 ((unsigned char *)&g_eng1_callback_record_block_06a968.records[0])
#define eng1_frame_target_067bd0 ((unsigned char *)&g_eng1_callback_record_prefix_067bd0[0])
#define eng1_frame_target_04e8c8 ((unsigned char *)&g_eng1_callback_record_prefix_04e8c8[0])
#define eng1_frame_target_060c70 ((unsigned char *)&g_eng1_callback_record_prefix_060c70[0])
#define eng1_frame_target_0582e0 ((unsigned char *)&g_eng1_callback_record_prefix_0582e0[0])
#define eng1_frame_target_059af8 ((unsigned char *)&g_eng1_callback_record_prefix_059af8[0])
#define eng1_frame_target_062790 ((unsigned char *)&g_eng1_callback_record_prefix_062790[0])
#define eng1_frame_target_06acb0 ((unsigned char *)&g_eng1_callback_record_prefix_06acb0[0])
#define eng1_frame_target_067f80 ((unsigned char *)&g_eng1_callback_record_prefix_067f80[0])
#define eng1_frame_target_04ec50 ((unsigned char *)&g_eng1_callback_record_prefix_04ec50[0])
#define eng1_frame_target_060f58 ((unsigned char *)&g_eng1_callback_record_prefix_060f58[0])
#define eng1_frame_target_0585c0 ((unsigned char *)&g_eng1_callback_record_prefix_0585c0[0])
#define eng1_frame_target_059de8 ((unsigned char *)&g_eng1_callback_record_prefix_059de8[0])
#define eng1_frame_target_062a60 ((unsigned char *)&g_eng1_callback_record_prefix_062a60[0])
#define eng1_frame_target_06af60 ((unsigned char *)&g_eng1_callback_record_prefix_06af60[0])
