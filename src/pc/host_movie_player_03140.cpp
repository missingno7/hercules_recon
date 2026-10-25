/* HERCULES.EXE streamed movie player 0x3140..0x37a0 (lane h1 candidate).
 * Same unit as audio_native_035d0 (0x35d0) and graphics_native_03770 (0x3770),
 * whose recorded sources are C++.  Names, header view and TU unproved. */
#include <windows.h>
#include <dsound.h>
#include <string.h>

/* 0x348-byte "ETV\n" movie header read by 0x3140 (storage: g_exe_4a3838[210]). */
typedef struct HostMovieHeader {
    DWORD magic_000;
    DWORD width_004;
    DWORD height_008;
    DWORD frames_00c;
    DWORD frames_per_chunk_010;
    DWORD channels_014;
    DWORD rate_018;
    DWORD bits_01c;
    DWORD unknown_020[2];
    DWORD chunk_bytes_028[200];
} HostMovieHeader;

extern "C" {
extern HostMovieHeader g_exe_4a3838;
extern DWORD g_exe_4a3820[2];
extern DWORD g_exe_4a3828;
extern DWORD g_exe_4a382c;
extern DWORD g_exe_4a3830;
extern BYTE g_exe_4a3b80[2][0xaf000];
extern DWORD g_exe_601b80;
extern BYTE g_exe_601b88[0x14000];
extern DWORD g_exe_615b88;
extern DWORD g_exe_615b8c;
extern IDirectSound *g_exe_615b98;
extern IDirectSoundBuffer *g_exe_615b9c;
extern DWORD g_exe_615ba0;
extern HANDLE g_exe_615ba4;
extern unsigned long g_exe_615ba8;
extern DWORD g_exe_615bac;
extern short g_exe_615bb0;
extern BYTE g_exe_615bb4;
extern DWORD g_exe_615bb8;
extern HANDLE g_exe_615bbc;
extern volatile DWORD g_exe_615bc0;
extern volatile DWORD g_exe_615bc4;
extern BYTE *g_exe_615bc8;
extern DWORD g_exe_615bcc;

void __cdecl movie_native_020c0(DWORD width, DWORD height);
void __cdecl host_02130(const BYTE *mask);
void __cdecl host_021e0(BYTE *screen, BYTE *packed);
void __cdecl audio_native_035d0(BYTE enabled);
DWORD __cdecl host_03680(void);
DWORD WINAPI host_033e0(LPVOID unused);

DWORD __cdecl host_03140(const char *path, IDirectSound *sound, BYTE **screen, DWORD *width, DWORD *height)
{
    DWORD bytes;
    PCMWAVEFORMAT format;
    DSBUFFERDESC desc;
    int waited;

    host_03680();
    timeBeginPeriod(1);
    g_exe_615bb4 = 1;
    g_exe_615bcc = 0;
    g_exe_615ba4 = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (g_exe_615ba4 == NULL)
        goto fail;
    if (!ReadFile(g_exe_615ba4, &g_exe_4a3838, sizeof(g_exe_4a3838), &bytes, NULL))
        goto fail;
    if (g_exe_4a3838.magic_000 != 0x0a565445)
        goto fail;
    if (g_exe_4a3838.frames_per_chunk_010 == 0)
        goto fail;
    memset(g_exe_601b88, 0, sizeof(g_exe_601b88));
    g_exe_615b8c = g_exe_4a3838.channels_014 * g_exe_4a3838.bits_01c / 8 * g_exe_4a3838.rate_018
        / g_exe_4a3838.frames_per_chunk_010;
    g_exe_615b98 = sound;
    memset(&format, 0, sizeof(format));
    format.wf.wFormatTag = WAVE_FORMAT_PCM;
    format.wf.nChannels = (WORD)g_exe_4a3838.channels_014;
    format.wf.nSamplesPerSec = (WORD)g_exe_4a3838.rate_018;
    format.wBitsPerSample = (WORD)g_exe_4a3838.bits_01c;
    format.wf.nBlockAlign = (WORD)g_exe_4a3838.channels_014 * (WORD)g_exe_4a3838.bits_01c / 8;
    format.wf.nAvgBytesPerSec = format.wf.nBlockAlign * (WORD)g_exe_4a3838.rate_018;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
    desc.dwBufferBytes = 0x15888;
    desc.lpwfxFormat = (LPWAVEFORMATEX)&format;
    if (sound != NULL)
        sound->CreateSoundBuffer(&desc, &g_exe_615b9c, NULL);
    if (g_exe_615b9c != NULL) {
        g_exe_615b9c->SetCurrentPosition(0);
        g_exe_615b9c->SetFrequency(0);
        g_exe_615b9c->SetPan(0);
        g_exe_615b9c->SetVolume(-10000);
    }
    g_exe_615bb0 = -10000;
    movie_native_020c0(g_exe_4a3838.width_004, g_exe_4a3838.height_008);
    g_exe_615ba8 = 0;
    g_exe_615ba0 = 1;
    g_exe_615bac = g_exe_4a3838.frames_00c;
    if (screen != NULL)
        *screen = g_exe_601b88;
    if (width != NULL)
        *width = g_exe_4a3838.width_004;
    if (height != NULL)
        *height = g_exe_4a3838.height_008;
    g_exe_615bc0 = 0;
    g_exe_615bc4 = 1;
    g_exe_4a3820[0] = 0;
    g_exe_4a3820[1] = 0;
    g_exe_615bc0 = 0;
    g_exe_615bbc = CreateThread(NULL, 0, host_033e0, NULL, 0, &g_exe_615bb8);
    if (g_exe_615bbc == NULL)
        goto fail;
    audio_native_035d0(1);
    for (waited = 0; !g_exe_615bc0 && waited < 3000; waited += 100)
        Sleep(100);
    if (waited < 3000)
        return g_exe_4a3838.frames_00c;
fail:
    host_03680();
    return 0;
}

DWORD WINAPI host_033e0(LPVOID unused)
{
    DWORD frame = 1;
    DWORD size, bytes;
    void *block;
    DWORD block_bytes;

    for (;;) {
        g_exe_615bc0 = 0;
        if (frame <= g_exe_4a3838.frames_00c) {
            size = g_exe_4a3838.chunk_bytes_028[(frame - 1) / g_exe_4a3838.frames_per_chunk_010];
            if (size < 0xaf000 && ReadFile(g_exe_615ba4, g_exe_4a3b80[g_exe_615bc4 ^ 1], size, &bytes, NULL)) {
                frame += g_exe_4a3838.frames_per_chunk_010;
                if (frame > g_exe_4a3838.frames_00c)
                    frame = g_exe_4a3838.frames_00c + 1;
                g_exe_4a3820[g_exe_615bc4 ^ 1] = frame - 1;
            } else {
                g_exe_615bcc = 0xffffffff;
            }
            if (g_exe_615b9c != NULL) {
                block = NULL;
                block_bytes = 0;
                if (g_exe_615b9c->Lock((g_exe_615bc4 ^ 1) * g_exe_615b8c * g_exe_4a3838.frames_per_chunk_010,
                        g_exe_615b8c * g_exe_4a3838.frames_per_chunk_010, &block, &block_bytes, NULL, NULL, 0)
                        == DSERR_BUFFERLOST) {
                    g_exe_615b9c->Restore();
                    g_exe_615b9c->Stop();
                    g_exe_615b9c->Lock(0, 0x15888, &block, &block_bytes, NULL, NULL, 0);
                    if (block != NULL && block_bytes != 0)
                        memset(block, g_exe_4a3838.bits_01c == 8 ? 0x80 : 0, block_bytes);
                    g_exe_615b9c->Unlock(block, block_bytes, NULL, 0);
                } else {
                    if (block != NULL)
                        memcpy(block, g_exe_4a3b80[g_exe_615bc4 ^ 1], block_bytes);
                    g_exe_615b9c->Unlock(block, block_bytes, NULL, 0);
                }
            }
        }
        g_exe_615bc0 = 1;
        SuspendThread(g_exe_615bbc);
    }
    return 0;
}

DWORD __cdecl host_03680(void)
{
    int waited;

    if (g_exe_615bb4)
        timeEndPeriod(1);
    g_exe_615bb4 = 0;
    if (g_exe_615bbc != NULL) {
        for (waited = 0; !g_exe_615bc0 && waited < 3000; waited += 10)
            Sleep(10);
        Sleep(10);
        TerminateThread(g_exe_615bbc, 0);
        CloseHandle(g_exe_615bbc);
        g_exe_615bbc = NULL;
    }
    if (g_exe_615b9c != NULL) {
        g_exe_615b9c->SetVolume(-10000);
        Sleep(1);
        g_exe_615b9c->Stop();
        g_exe_615b9c->Release();
        g_exe_615b9c = NULL;
    }
    g_exe_615bac = 0;
    memset(&g_exe_4a3838, 0, sizeof(g_exe_4a3838));
    if (g_exe_615ba4 != NULL) {
        CloseHandle(g_exe_615ba4);
        g_exe_615ba4 = NULL;
    }
    audio_native_035d0(0);
    return g_exe_615bcc;
}

long __cdecl host_037a0(BYTE **palette, short volume)
{
    long delay;
    int waited;
    BYTE has_palette;
    BOOL late;

    if (g_exe_615bac < 1)
        return 0;
    if (g_exe_615bcc != 0) {
        host_03680();
        return 0;
    }
    if (g_exe_615ba0) {
        g_exe_4a3828 = 1;
        g_exe_615b88 = 1;
        g_exe_4a3830 = 0;
    }
    audio_native_035d0(1);
    if (g_exe_615ba8) {
        g_exe_615ba8 = 0;
        g_exe_4a382c = timeGetTime() - (long)((float)(g_exe_4a3828 * 1000.0f / g_exe_4a3838.frames_per_chunk_010) + 0.5f);
        g_exe_601b80 = 0;
        g_exe_4a3830 = 0;
    }
    delay = g_exe_601b80 - timeGetTime();
    if (!g_exe_615ba0 && delay > 0) {
        delay >>= 1;
        if (delay > 1)
            Sleep(delay);
        return -(long)g_exe_615bac;
    }
    if (g_exe_4a3828 > g_exe_4a3820[g_exe_615bc4]) {
        for (waited = 0; !g_exe_615bc0 && waited < 3000; waited += 2)
            Sleep(2);
        if (waited >= 3000) {
            host_03680();
            return 0;
        }
        g_exe_615bc4 ^= 1;
        g_exe_615bc0 = 0;
        for (waited = 0; !ResumeThread(g_exe_615bbc) && waited < 3000; waited += 2)
            Sleep(2);
        if (waited >= 3000) {
            host_03680();
            return 0;
        }
        g_exe_615bc8 = g_exe_4a3b80[g_exe_615bc4] + g_exe_615b8c * g_exe_4a3838.frames_per_chunk_010;
    }
    if (g_exe_615ba0) {
        g_exe_4a382c = timeGetTime();
        g_exe_615ba0 = 0;
    }
    g_exe_601b80 = (long)((float)(g_exe_4a3828 * 1000.0f / g_exe_4a3838.frames_per_chunk_010) + 0.5f) + g_exe_4a382c;
    has_palette = g_exe_615bc8[4];
    if (palette != NULL)
        *palette = NULL;
    if (has_palette) {
        if (palette != NULL)
            *palette = g_exe_615bc8 + 5;
        host_02130(g_exe_615bc8 + 0x305);
    }
    host_021e0(g_exe_601b88, g_exe_615bc8 + 5 + (has_palette ? 0x2300 : 0));
    g_exe_615bc8 += *(DWORD *)g_exe_615bc8;
    if (g_exe_615b9c != NULL) {
        DWORD status;
        g_exe_615b9c->GetStatus(&status);
        if (g_exe_615bb0 != volume) {
            g_exe_615b9c->SetVolume(volume);
            g_exe_615bb0 = volume;
        }
        if (!(status & DSBSTATUS_PLAYING)) {
            g_exe_615b9c->SetVolume(-1000);
            g_exe_615b9c->SetCurrentPosition((g_exe_4a3828 - 1) * g_exe_615b8c % 0x15888);
            g_exe_615b9c->Play(0, 0, DSBPLAY_LOOPING);
            Sleep(1);
            g_exe_615b9c->SetVolume(volume);
        }
    }
    late = !g_exe_4a3830 && timeGetTime() >= g_exe_601b80 && !has_palette &&
        g_exe_4a3828 != 1 && g_exe_4a3828 != g_exe_4a3838.frames_00c;
    g_exe_4a3830 = late;
    g_exe_615bac--;
    g_exe_4a3828++;
    if (late)
        return -(long)g_exe_615bac;
    return g_exe_615bac;
}
}
