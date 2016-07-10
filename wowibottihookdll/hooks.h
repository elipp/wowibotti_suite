#pragma once

#include <Windows.h>
#include <D3D9.h>
#include <string>


int hook_all();
int unhook_all();

int hook_EndScene();

int patch_DelIgnore();