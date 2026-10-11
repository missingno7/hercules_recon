/* New 0x54f0 state, private to this TU.  The neighboring statics already
   defined in r04d80.c (g_29dac, g_29db8 and g_29e04) remain shared in place. */
static int g_29da8;
static int g_29db0;
static int g_29db4;
static TitleRec64 g_29dc0;
static int g_29e00;

void title_054f0(unsigned short id, TitleObject *owner)
{
    unsigned long vol;

    title_0c6f0();
    g_29da8 = id & ~0xf800;
    g_29db0 = g_engine_interface.context_004->sound_2f0[g_29da8][0];
    g_29db4 = g_engine_interface.context_004->sound_2f0[g_29da8][1];
    title_016e0("\n LinkedSoundFx %d :: %d %d", g_29da8, g_29db0, g_29db4);
    title_016e0("\n 0x%x 0x%x 0x%x \n", g_engine_interface.context_004->unknown_150[g_29db0],
                g_engine_interface.context_004->unknown_178[g_29db0], g_engine_interface.context_004->unknown_18c[g_29db0]);
    if (g_29db0 == 0xff || g_29db4 == 0xff) {
        goto done;
    }
    if (owner == 0 && (id & 0xf800) != 0) {
        goto done;
    }
    if ((id & 0x4000) != 0 && owner != 0 && owner->unknown_068 == 0) {
        title_0c700();
        return;
    }
    if (title_05080(g_29da8) > 5) {
        goto done;
    }
    if ((id & 0x4000) != 0) {
        g_29dc0.unknown_0e = (short)g_29db4;
        g_29dc0.word_0c = (short)g_29db0;
        g_29dc0.flags_34 = id;
        g_29dc0.unknown_38 = g_engine_interface.context_004->sound_2f0[g_29da8][2];
        g_29dc0.owner_30 = owner;
        title_fx_pan(&g_29dc0);
        if (g_engine_interface.context_004->unknown_1b24 == 0) {
            g_29e00 = title_0cc10((short)g_29db0, (short)g_29db4, 0, 0x3c, 0,
                                  (short)((g_29dc0.unknown_10 + g_29dc0.unknown_14) * g_engine_interface.context_004->unknown_1b18 >> 8),
                                  (short)((g_29dc0.unknown_10 + g_29dc0.unknown_14) * g_engine_interface.context_004->unknown_1b18 >> 8));
        } else {
            g_29e00 = title_0cc10((short)g_29db0, (short)g_29db4, 0, 0x3c, 0,
                                  (short)(g_29dc0.unknown_10 * g_engine_interface.context_004->unknown_1b18 >> 7),
                                  (short)(g_29dc0.unknown_14 * g_engine_interface.context_004->unknown_1b18 >> 7));
        }
        if (g_29e00 >= 0) {
            g_2cca0[g_29e00].unknown_00 = g_29dc0.unknown_00;
            g_2cca0[g_29e00].unknown_04 = g_29dc0.unknown_04;
            g_2cca0[g_29e00].unknown_10 = g_29dc0.unknown_10;
            g_2cca0[g_29e00].unknown_14 = g_29dc0.unknown_14;
            g_2cca0[g_29e00].unknown_18 = g_29dc0.unknown_18;
            g_2cca0[g_29e00].unknown_1c = g_29dc0.unknown_1c;
        }
        id &= ~0x4000;
    } else if (id & 0x800) {
        g_29e00 = title_0cc10((short)g_29db0, (short)g_29db4, 0, 0x3c, 0, 0, 0);
        if (g_29e00 >= 0) {
            g_2cca0[g_29e00].unknown_14 = 0;
            g_2cca0[g_29e00].unknown_10 = 0;
        }
    } else {
        vol = g_engine_interface.context_004->unknown_1b18 * 127 >> 7;
        g_29e00 = title_0cc10((short)g_29db0, (short)g_29db4, 0, 0x3c, 0, (short)vol, (short)vol);
        if (g_29e00 >= 0) {
            g_2cca0[g_29e00].unknown_14 = vol;
            g_2cca0[g_29e00].unknown_10 = vol;
        }
    }
    if (g_29e00 >= 0) {
        g_2cca0[g_29e00].unknown_20 = g_2cca0[g_29e00].unknown_10;
        g_2cca0[g_29e00].unknown_24 = g_2cca0[g_29e00].unknown_14;
        g_2cca0[g_29e00].word_0c = (short)g_29db0;
        g_2cca0[g_29e00].unknown_0e = (short)g_29db4;
        g_2cca0[g_29e00].owner_30 = owner;
        g_2cca0[g_29e00].id_08 = (short)g_29da8;
        g_2cca0[g_29e00].flags_34 = id;
        g_2cca0[g_29e00].unknown_0a = 0;
        g_2cca0[g_29e00].unknown_38 = g_engine_interface.context_004->sound_2f0[g_29da8][2];
        if (owner != 0) {
            owner->unknown_06c = (void *)1;
        }
    }
done:
    title_0c700();
}
