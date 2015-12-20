#include "window.h"



BOOL RegisterDLLWindowClass(char szClassName[], HINSTANCE hInstance) {

	WNDCLASSEX wc;
	wc.hInstance = hInstance;
	wc.lpszClassName = (LPCSTR)szClassName;
	wc.lpfnWndProc = DLLWindowProc;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

	if (!RegisterClassEx(&wc))
		return 0;
	return 1;

}

HMENU CreateDLLWindowMenu() {

	HMENU hMenu;
	hMenu = CreateMenu();
	HMENU hMenuPopup;
	if (hMenu == NULL)
		return FALSE;
	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_EXIT, TEXT("Exit"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("File"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_DUMP, TEXT("Dump wow objects to out.log"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("Dump"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_HOOK, TEXT("Patch EndScene/DelIgnore/ClosePetStables"));
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_UNHOOK, TEXT("UN-patch EndScene/DelIgnore/ClosePetStables"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("FUNK"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_CTM, TEXT("CTM to target coordinates"));
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_CTM_CONSTANT, TEXT("Toggle walk-to-target"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("CTM"));


	return hMenu;
}
