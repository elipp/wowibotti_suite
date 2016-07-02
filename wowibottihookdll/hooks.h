#pragma once

#include <Windows.h>
#include <D3D9.h>
#include <string>

int install_hook(const std::string &funcname, LPVOID hook_func_addr);
int uninstall_hook(const std::string &funcname);

int hook_all();

int patch_DelIgnore();