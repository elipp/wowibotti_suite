#pragma once


#include <Windows.h>

#define MYMENU_EXIT         (WM_APP + 101)
#define MYMENU_DUMP			(WM_APP + 102) 
#define MYMENU_HOOK			(WM_APP + 103)
#define MYMENU_UNHOOK		(WM_APP + 104)
#define MYMENU_CTM			(WM_APP + 105)
#define MYMENU_CTM_CONSTANT (WM_APP + 106)

extern LRESULT CALLBACK DLLWindowProc(HWND, UINT, WPARAM, LPARAM);

BOOL RegisterDLLWindowClass(char szClassName[], HINSTANCE hModule);
HMENU CreateDLLWindowMenu();
