/*
 * Chex Quest PS Vita - Sound subsystem
 * Handles SFX and music via SDL2_mixer
 */

#ifdef VITA

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string.h>
#include <stdlib.h>
#include "vita_config.h"

// Doom includes
#include "doomdef.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"

// Maximum concurrent sound channels
#define MAX_CHANNELS    16
#define MAX_SFX_CACHE   128

// Sound channel info
typedef struct {
    Mix_Chunk *chunk;
    int        id;
    int        priority;
    int        handle;
} channel_t;

static channel_t channels[MAX_CHANNELS];
static int sound_initialized = 0;
static int music_initialized = 0;
static int next_handle = 1;

// Music
static Mix_Music *current_music = NULL;

// Cached sound chunks
typedef struct {
    int       lumpnum;
    Mix_Chunk *chunk;
} sfx_cache_t;

static sfx_cache_t sfx_cache[MAX_SFX_CACHE];
static int num_cached = 0;

/*
 * Convert Doom's raw sound format to a WAV-compatible Mix_Chunk
 * Doom sounds: 8-bit unsigned PCM with a simple header
 */
static Mix_Chunk *convert_doom_sound(void *data, int len) {
    if (len < 8) return NULL;

    byte *raw = (byte *)data;

    // Doom sound header:
    // Offset 0-1: format (should be 3)
    // Offset 2-3: sample rate
    // Offset 4-7: number of samples
    unsigned short format = raw[0] | (raw[1] << 8);
    unsigned short samplerate = raw[2] | (raw[3] << 8);
    unsigned int numsamples = raw[4] | (raw[5] << 8) | (raw[6] << 16) | (raw[7] << 24);

    if (format != 3) return NULL;
    if (numsamples > (unsigned int)(len - 8)) numsamples = len - 8;
    if (numsamples == 0) return NULL;

    // Create WAV in memory
    // WAV header is 44 bytes
    int wav_size = 44 + numsamples;
    byte *wav_data = (byte *)malloc(wav_size);
    if (!wav_data) return NULL;

    // RIFF header
    memcpy(wav_data, "RIFF", 4);
    int chunk_size = wav_size - 8;
    wav_data[4] = chunk_size & 0xFF;
    wav_data[5] = (chunk_size >> 8) & 0xFF;
    wav_data[6] = (chunk_size >> 16) & 0xFF;
    wav_data[7] = (chunk_size >> 24) & 0xFF;
    memcpy(wav_data + 8, "WAVE", 4);

    // fmt chunk
    memcpy(wav_data + 12, "fmt ", 4);
    wav_data[16] = 16; wav_data[17] = 0; wav_data[18] = 0; wav_data[19] = 0; // chunk size
    wav_data[20] = 1; wav_data[21] = 0; // PCM format
    wav_data[22] = 1; wav_data[23] = 0; // mono
    wav_data[24] = samplerate & 0xFF;
    wav_data[25] = (samplerate >> 8) & 0xFF;
    wav_data[26] = 0; wav_data[27] = 0; // sample rate high bytes
    wav_data[28] = samplerate & 0xFF;   // byte rate = samplerate * 1 * 1
    wav_data[29] = (samplerate >> 8) & 0xFF;
    wav_data[30] = 0; wav_data[31] = 0;
    wav_data[32] = 1; wav_data[33] = 0; // block align
    wav_data[34] = 8; wav_data[35] = 0; // bits per sample

    // data chunk
    memcpy(wav_data + 36, "data", 4);
    wav_data[40] = numsamples & 0xFF;
    wav_data[41] = (numsamples >> 8) & 0xFF;
    wav_data[42] = (numsamples >> 16) & 0xFF;
    wav_data[43] = (numsamples >> 24) & 0xFF;

    // Copy PCM data (skip 8-byte Doom header, also skip padding bytes at start and end)
    int start = 8;
    if (numsamples > 32) {
        start += 16;  // Skip padding
        numsamples -= 32;
    }
    memcpy(wav_data + 44, raw + start, numsamples);

    // Create SDL_mixer chunk from WAV data
    SDL_RWops *rw = SDL_RWFromMem(wav_data, wav_size);
    Mix_Chunk *chunk = Mix_LoadWAV_RW(rw, 1);

    free(wav_data);
    return chunk;
}

/*
 * Initialize sound subsystem
 */
void I_InitSoundVita(void) {
    if (Mix_OpenAudio(VITA_AUDIO_FREQ, VITA_AUDIO_FORMAT,
                      VITA_AUDIO_CHANNELS, VITA_AUDIO_SAMPLES) < 0) {
        fprintf(stderr, "Could not initialize SDL_mixer: %s\n", Mix_GetError());
        return;
    }

    Mix_AllocateChannels(MAX_CHANNELS);
    Mix_Volume(-1, VITA_SFX_VOLUME);

    memset(channels, 0, sizeof(channels));
    memset(sfx_cache, 0, sizeof(sfx_cache));

    sound_initialized = 1;
    music_initialized = 1;
}

/*
 * Play a sound effect
 * Returns a handle to the playing sound
 */
int I_StartSoundVita(int id, int vol, int sep, int pitch, int priority) {
    if (!sound_initialized) return 0;

    // Find or load the sound
    Mix_Chunk *chunk = NULL;
    int i;

    // Check cache
    for (i = 0; i < num_cached; i++) {
        if (sfx_cache[i].lumpnum == id) {
            chunk = sfx_cache[i].chunk;
            break;
        }
    }

    // If not cached, load it
    if (!chunk) {
        int lumplen;
        void *lumpdata;

        // Get the sound lump from the WAD
        extern int W_CheckNumForName(const char *name);
        extern void *W_CacheLumpNum(int lump, int tag);
        extern int W_LumpLength(int lump);

        char sndname[16];
        snprintf(sndname, sizeof(sndname), "ds%s", S_sfx[id].name);

        int lumpnum = W_CheckNumForName(sndname);
        if (lumpnum < 0) return 0;

        lumplen = W_LumpLength(lumpnum);
        lumpdata = W_CacheLumpNum(lumpnum, 1); // PU_STATIC
        if (!lumpdata) return 0;

        chunk = convert_doom_sound(lumpdata, lumplen);
        if (!chunk) return 0;

        // Cache it
        if (num_cached < MAX_SFX_CACHE) {
            sfx_cache[num_cached].lumpnum = id;
            sfx_cache[num_cached].chunk = chunk;
            num_cached++;
        }
    }

    // Find a free channel (or steal lowest priority)
    int chan = -1;
    int lowest_priority = priority;

    for (i = 0; i < MAX_CHANNELS; i++) {
        if (!Mix_Playing(i)) {
            chan = i;
            break;
        }
        if (channels[i].priority < lowest_priority) {
            lowest_priority = channels[i].priority;
            chan = i;
        }
    }

    if (chan < 0) return 0;

    // Set volume (Doom uses 0-127, SDL_mixer uses 0-128)
    Mix_Volume(chan, vol);

    // Set panning from separation (0=right, 128=center, 255=left)
    int left = sep;
    int right = 255 - sep;
    Mix_SetPanning(chan, left, right);

    // Play the sound
    Mix_PlayChannel(chan, chunk, 0);

    // Store channel info
    channels[chan].chunk = chunk;
    channels[chan].id = id;
    channels[chan].priority = priority;
    channels[chan].handle = next_handle++;

    return channels[chan].handle;
}

/*
 * Stop a sound by handle
 */
void I_StopSoundVita(int handle) {
    int i;
    if (!sound_initialized) return;

    for (i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].handle == handle) {
            Mix_HaltChannel(i);
            channels[i].handle = 0;
            break;
        }
    }
}

/*
 * Check if a sound is still playing
 */
int I_SoundIsPlayingVita(int handle) {
    int i;
    if (!sound_initialized) return 0;

    for (i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].handle == handle) {
            return Mix_Playing(i);
        }
    }
    return 0;
}

/*
 * Update sound parameters (volume, separation)
 */
void I_UpdateSoundParamsVita(int handle, int vol, int sep, int pitch) {
    int i;
    if (!sound_initialized) return;

    for (i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].handle == handle) {
            Mix_Volume(i, vol);
            Mix_SetPanning(i, sep, 255 - sep);
            break;
        }
    }
}

/*
 * Start playing music
 * For Doom, music is in MUS format in the WAD
 */
void I_PlayMusicVita(const char *name, int looping) {
    if (!music_initialized) return;

    // Stop current music
    if (current_music) {
        Mix_FreeMusic(current_music);
        current_music = NULL;
    }

    // Get music lump from WAD
    extern int W_CheckNumForName(const char *name);
    extern void *W_CacheLumpNum(int lump, int tag);
    extern int W_LumpLength(int lump);

    int lumpnum = W_CheckNumForName(name);
    if (lumpnum < 0) return;

    int lumplen = W_LumpLength(lumpnum);
    void *lumpdata = W_CacheLumpNum(lumpnum, 1);
    if (!lumpdata) return;

    // Write to temp file for SDL_mixer
    // (SDL_mixer needs a file for music formats)
    char tmppath[256];
    snprintf(tmppath, sizeof(tmppath), "%smusic.mid", VITA_DATA_PATH);

    FILE *f = fopen(tmppath, "wb");
    if (f) {
        // Check if it's MUS format and needs conversion
        // For simplicity, write raw data - SDL_mixer can handle MIDI
        // MUS format conversion would go here
        fwrite(lumpdata, 1, lumplen, f);
        fclose(f);

        current_music = Mix_LoadMUS(tmppath);
        if (current_music) {
            Mix_VolumeMusic(VITA_MUSIC_VOLUME);
            Mix_PlayMusic(current_music, looping ? -1 : 1);
        }
    }
}

/*
 * Stop music playback
 */
void I_StopMusicVita(void) {
    if (!music_initialized) return;
    Mix_HaltMusic();
}

/*
 * Pause music
 */
void I_PauseMusicVita(void) {
    if (!music_initialized) return;
    Mix_PauseMusic();
}

/*
 * Resume music
 */
void I_ResumeMusicVita(void) {
    if (!music_initialized) return;
    Mix_ResumeMusic();
}

/*
 * Set music volume (0-127)
 */
void I_SetMusicVolumeVita(int volume) {
    if (!music_initialized) return;
    Mix_VolumeMusic(volume);
}

/*
 * Shutdown sound subsystem
 */
void I_ShutdownSoundVita(void) {
    int i;

    // Free cached sounds
    for (i = 0; i < num_cached; i++) {
        if (sfx_cache[i].chunk) {
            Mix_FreeChunk(sfx_cache[i].chunk);
        }
    }
    num_cached = 0;

    // Free music
    if (current_music) {
        Mix_FreeMusic(current_music);
        current_music = NULL;
    }

    Mix_CloseAudio();
    sound_initialized = 0;
    music_initialized = 0;
}

#endif /* VITA */
