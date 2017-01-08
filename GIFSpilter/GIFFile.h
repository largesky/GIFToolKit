#ifndef GIFFILE_H
#define GIFFILE_H

#include <windows.h>
#include <wincodec.h>
#include <commdlg.h>
#include <d2d1.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

HRESULT ParseGIFFile(const wchar_t *path, BOOL imgOverlay, wchar_t *outDir);

HRESULT ParseGIFFileWithOverlay(const wchar_t *path,wchar_t *outDir);

#endif