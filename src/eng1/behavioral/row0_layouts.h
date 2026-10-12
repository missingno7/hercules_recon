#ifndef ENG1_ROW0_LAYOUTS_H
#define ENG1_ROW0_LAYOUTS_H
typedef unsigned char eng1_u8;
typedef unsigned short eng1_u16;
typedef void (__cdecl *Eng1RawCodePointer)();
typedef char eng1_requires_32bit_code_pointer[(sizeof(void *) == 4) ? 1 : -1];
typedef struct Eng1ConfigAction16 { void *pointer_000; void *pointer_004; void *pointer_008; eng1_u8 bytes_00c[4]; } Eng1ConfigAction16;
typedef char Eng1ConfigAction16_size[(sizeof(Eng1ConfigAction16) == 16) ? 1 : -1];
typedef struct Eng1HandlerTriple12 { Eng1RawCodePointer code_000; Eng1RawCodePointer code_004; Eng1RawCodePointer code_008; } Eng1HandlerTriple12;
typedef char Eng1HandlerTriple12_size[(sizeof(Eng1HandlerTriple12) == 12) ? 1 : -1];
typedef struct Eng1ActionFlags8 { void *data_000; eng1_u8 bytes_004[4]; } Eng1ActionFlags8;
typedef char Eng1ActionFlags8_size[(sizeof(Eng1ActionFlags8) == 8) ? 1 : -1];
typedef struct Eng1CallbackPair8 { Eng1RawCodePointer code_000; Eng1RawCodePointer code_004; } Eng1CallbackPair8;
typedef char Eng1CallbackPair8_size[(sizeof(Eng1CallbackPair8) == 8) ? 1 : -1];
typedef struct Eng1InitializerFlags8 { void *data_000; eng1_u8 bytes_004[4]; } Eng1InitializerFlags8;
typedef char Eng1InitializerFlags8_size[(sizeof(Eng1InitializerFlags8) == 8) ? 1 : -1];
typedef struct Eng1FactoryPair8 { Eng1RawCodePointer code_000; Eng1RawCodePointer code_004; } Eng1FactoryPair8;
typedef char Eng1FactoryPair8_size[(sizeof(Eng1FactoryPair8) == 8) ? 1 : -1];

extern Eng1ConfigAction16 g_eng1_row0_config_action_5c408[40];
extern Eng1HandlerTriple12 g_eng1_row0_handler_triples_5c688[40];
extern Eng1ActionFlags8 g_eng1_row0_action_flags_5c868[40];
extern Eng1CallbackPair8 g_eng1_row0_callback_pairs_5c9a8[40];
extern Eng1InitializerFlags8 g_eng1_row0_initializer_flags_5cae8[40];
extern Eng1FactoryPair8 g_eng1_row0_factory_pairs_5cc28[40];
extern Eng1RawCodePointer g_eng1_row0_callback_words_5cd68[6];
extern Eng1RawCodePointer g_eng1_row0_shared_callback_words_5cd80[12];
extern eng1_u16 g_eng1_row0_word_map_prefix_5cdb0[30];
#endif
