#include "config.h"

#include "doomtype.h"
#include "i_sound.h"

#include <string.h>

int snd_sfxdevice = SNDDEVICE_NONE;
int snd_musicdevice = SNDDEVICE_NONE;
int snd_samplerate = 44100;
int snd_cachesize = 0;
int snd_maxslicetime_ms = 28;
char *snd_musiccmd = "";
int snd_pitchshift = 0;
char *snd_dmxoption = "";
int use_libsamplerate = 0;
float libsamplerate_scale = 1.0f;
int opl_io_port = 0;
char *music_pack_path = "";
char *timidity_cfg_path = "";

void I_InitSound(GameMission_t mission)
{
    (void)mission;
}

void I_ShutdownSound(void)
{
}

int I_GetSfxLumpNum(sfxinfo_t *sfxinfo)
{
    if ( sfxinfo == NULL )
    {
        return -1;
    }
    return sfxinfo->lumpnum;
}

void I_UpdateSound(void)
{
}

void I_UpdateSoundParams(int channel, int vol, int sep)
{
    (void)channel;
    (void)vol;
    (void)sep;
}

int I_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch)
{
    (void)sfxinfo;
    (void)vol;
    (void)sep;
    (void)pitch;
    return channel;
}

void I_StopSound(int channel)
{
    (void)channel;
}

boolean I_SoundIsPlaying(int channel)
{
    (void)channel;
    return false;
}

void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    (void)sounds;
    (void)num_sounds;
}

void I_InitMusic(void)
{
}

void I_ShutdownMusic(void)
{
}

void I_SetMusicVolume(int volume)
{
    (void)volume;
}

void I_PauseSong(void)
{
}

void I_ResumeSong(void)
{
}

void *I_RegisterSong(void *data, int len)
{
    (void)data;
    (void)len;
    return NULL;
}

void I_UnRegisterSong(void *handle)
{
    (void)handle;
}

void I_PlaySong(void *handle, boolean looping)
{
    (void)handle;
    (void)looping;
}

void I_StopSong(void)
{
}

boolean I_MusicIsPlaying(void)
{
    return false;
}

void I_BindSoundVariables(void)
{
}

void I_SetOPLDriverVer(opl_driver_ver_t ver)
{
    (void)ver;
}

void I_OPL_DevMessages(char *result, size_t result_len)
{
    if ( (result != NULL) && (result_len > 0) )
    {
        result[0] = '\0';
    }
}

void I_InitTimidityConfig(void)
{
}
