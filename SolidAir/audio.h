#pragma once

#include <iostream>
#include <objidl.h> 
#include <windows.h>
#include <gdiplus.h>
#include <tchar.h>
#include "resource.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

MIX_Audio* LoadBackgroundMusic(MIX_Mixer* mixer, int resourceId, void** ppData);
BOOL LoadResourceChunk(int resourceId, void** ppData, DWORD* pwDataBytes);