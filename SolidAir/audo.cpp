#include "audio.h"

MIX_Audio* LoadBackgroundMusic(MIX_Mixer* mixer, int resourceId, void** ppData)
{
    MIX_Audio* audio;
    DWORD audioDataSize;

    LoadResourceChunk(resourceId, ppData, &audioDataSize);
    audio = MIX_LoadAudioNoCopy(mixer, *ppData, audioDataSize, false);
    if (!audio)
    {
        SDL_Log("Couldn't load audio resource %i: %s", resourceId, SDL_GetError());
    }

	return audio;
}

BOOL LoadResourceChunk(int resourceId, void** ppData, DWORD* pwDataBytes)
{
    const auto& hModule = GetModuleHandle(NULL);
    
    if (hModule == 0)
    {
        return false;
    }

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), _T("MUSIC"));

    if (hRes == 0)
    {
        return false;
    }

    HGLOBAL hGlobal = LoadResource(hModule, hRes);

    if (hGlobal == 0)
    {
        return false;
    }

    *ppData = LockResource(hGlobal);

    const auto size = SizeofResource(hModule, hRes);

    *pwDataBytes = size;

    return size > 0;
}