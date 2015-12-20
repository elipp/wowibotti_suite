#pragma once

#include <Windows.h>
#include <D3D9.h>
#include <string>

int prepare_all_patches();

int install_hook(const std::string &funcname, LPVOID hook_func_addr);
int uninstall_hook(const std::string &funcname);