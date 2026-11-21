#include "config.h"

#include "i_cdmus.h"

int cd_Error = 0;

int I_CDMusInit(void)
{
    return -1;
}

void I_CDMusPrintStartup(void)
{
}

int I_CDMusPlay(int track)
{
    (void)track;
    return -1;
}

int I_CDMusStop(void)
{
    return -1;
}

int I_CDMusResume(void)
{
    return -1;
}

int I_CDMusSetVolume(int volume)
{
    (void)volume;
    return -1;
}

int I_CDMusFirstTrack(void)
{
    return 0;
}

int I_CDMusLastTrack(void)
{
    return 0;
}

int I_CDMusTrackLength(int track)
{
    (void)track;
    return 0;
}
