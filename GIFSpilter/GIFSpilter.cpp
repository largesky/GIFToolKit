// GIFSpilter.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "GIFSpilter.h"
#include <ObjBase.h>
#include <Shobjidl.h>
#include <Winuser.h> 
#include <Shellapi.h>
#include "GIFFile.h"
#include <ShlObj.h>

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;								// 当前实例
HWND g_HDlgMain=NULL;
INT_PTR CALLBACK DLGMAINPROC(HWND, UINT, WPARAM, LPARAM);

BOOL OpenFile(wchar_t * path);

BOOL OpenDirWithExplore(const wchar_t *dir);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)!=S_OK)
	{
		MessageBox(NULL,L"应用程序无法初始化COM运行环境",L"错误",MB_OK|MB_ICONERROR);
		return -1;
	}
	
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOGMAIN),NULL,DLGMAINPROC);

	CoUninitialize();
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK DLGMAINPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			g_HDlgMain=hDlg;
			CHANGEFILTERSTRUCT filter={sizeof(CHANGEFILTERSTRUCT)};
			ChangeWindowMessageFilterEx(hDlg,WM_DROPFILES,MSGFLT_ADD,&filter);
			ChangeWindowMessageFilterEx(hDlg,WM_COPYDATA,MSGFLT_ADD,&filter);
			ChangeWindowMessageFilterEx(hDlg,0x0049,MSGFLT_ADD,&filter);
			CheckDlgButton(hDlg,IDC_CHKIMGOVERLAY,BST_CHECKED);
			CheckDlgButton(hDlg,IDC_CHKOPEN,BST_CHECKED);
			return (INT_PTR)TRUE;
		}
	case WM_DROPFILES:
		{
			HDROP hDrop=(HDROP)wParam;
			if(DragQueryFile(hDrop,-1,NULL,0) >0)
			{
				wchar_t path[MAX_PATH]={0};
				if(DragQueryFile(hDrop,0,path,MAX_PATH)>0)
				{
					DWORD att=GetFileAttributes(path);
					if(att== INVALID_FILE_ATTRIBUTES)
					{
						MessageBox(hDlg,L"文件或者路径非法",L"错误",MB_OK | MB_ICONERROR);
					}
					else if((att & FILE_ATTRIBUTE_DIRECTORY) ==FILE_ATTRIBUTE_DIRECTORY)
					{
						MessageBox(hDlg,L"不支持文件夹",L"错误",MB_OK | MB_ICONERROR);
					}
					else
					{
						SetDlgItemText(hDlg,IDC_TXTPATH,path);
					}
				}
			}
			DragFinish(hDrop);
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if(LOWORD(wParam) ==IDC_BTNSCAN)
		{
			wchar_t path[MAX_PATH]={0};
			if(OpenFile(path))
			{
				SetDlgItemText(hDlg,IDC_TXTPATH,path);
			}
			return (INT_PTR)TRUE;
		}
		else if(LOWORD(wParam) ==IDC_BTNPARSE)
		{
			wchar_t path[MAX_PATH]={0};
			wchar_t outDir[MAX_PATH]={0};
			GetDlgItemText(hDlg,IDC_TXTPATH,path,MAX_PATH);
			if(wcslen(path) <1)
			{
				MessageBox(hDlg,L"要解析的文件不能为空",L"错误",MB_OK | MB_ICONERROR);
			}
			else
			{
				BOOL imgOverlay=IsDlgButtonChecked(hDlg,IDC_CHKIMGOVERLAY)==BST_CHECKED;
				HRESULT hr= ParseGIFFile(path,imgOverlay,outDir);
				wchar_t message[250]={0};
				if(hr!=S_OK)
				{
					wsprintf(message,L"解析文件失败 错误代码 0x%X",hr);
					MessageBox(hDlg,message,L"提示",MB_OK);
				}
				else
				{
					wsprintf(message,L"解析文件成功 文件位于 %s",outDir);
					if(IsDlgButtonChecked(hDlg,IDC_CHKOPEN)!=BST_CHECKED || OpenDirWithExplore(outDir)==FALSE)
					{
						MessageBox(hDlg,message,L"提示",MB_OK);
					}
				}
			}
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

BOOL OpenFile(wchar_t * path)
{
	IFileOpenDialog *ofd=NULL;
	IShellItem *item=0;
	__try
	{
		if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&ofd))))
		{
			return FALSE;
		}

		DWORD dwFlags=0;
		if(FAILED(ofd->GetOptions(&dwFlags)))
		{
			return FALSE;
		}
		if(FAILED(ofd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM)))
		{
			return FALSE;
		}
		COMDLG_FILTERSPEC filter[]=
		{
			{L"Gif Files",L"*.gif"},
			{L"All Files",L"*.*"}
		};
		if(FAILED(ofd->SetFileTypes(2,filter)))
		{
			return FALSE;
		}

		if(FAILED(ofd->Show(g_HDlgMain)))
		{
			return FALSE;
		}
		if(FAILED(ofd->GetResult(&item)))
		{
			return FALSE;
		}
		wchar_t *pa=0;
		if(FAILED(item->GetDisplayName(SIGDN_FILESYSPATH,&pa)))
		{
			return FALSE;
		}

		wcscpy_s(path,MAX_PATH,pa);
		CoTaskMemFree(pa);
	}
	__finally
	{
		if(ofd != NULL)
		{
			ofd->Release();
			ofd=NULL;
		}
		if(item !=NULL)
		{
			item->Release();
			item=NULL;
		}
	}

	return TRUE;
}

BOOL OpenDirWithExplore(const wchar_t *dir)
{
	STARTUPINFO  startupInfo={sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pInfo={0};
	wchar_t arg[400]={0};
	wchar_t *winDir=0;
	wchar_t exPath[MAX_PATH]={0};

	wsprintf(arg,L"/e \"%s\"",dir);
	
	if(FAILED(SHGetKnownFolderPath(FOLDERID_Windows,KF_FLAG_SIMPLE_IDLIST | KF_FLAG_DEFAULT_PATH ,NULL,&winDir)))
	{
		return FALSE;
	}

	wsprintf(exPath,L"%s\\explorer.exe",winDir);
	BOOL ret=CreateProcess(exPath,arg,NULL,NULL,FALSE,0,NULL,NULL,&startupInfo,&pInfo);
	CloseHandle(pInfo.hProcess);
	CloseHandle(pInfo.hThread);
	CoTaskMemFree(winDir);
	return ret;
}