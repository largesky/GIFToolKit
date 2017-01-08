#include "GIFFile.h"

#pragma comment(lib,"d2d1.lib")
HRESULT g_hr;

#define FAILEDEX(hr) g_hr=hr; \
	if (FAILED(g_hr))\
	{\
		return g_hr;\
	}

#define SAFERELEASE(obj) \
	if(obj)\
		{\
			obj->Release();\
			obj=NULL;\
		}

BOOL ParseFileNameAndCreateOutdir(const wchar_t *path,wchar_t *nameBuf,wchar_t *outDirBuf)
{
	int len=(int)wcslen(path);

	int dotIndex=len-1;

	while(dotIndex >=0&& path[dotIndex] && path[dotIndex] !=L'.')
		dotIndex--;

	int slashIndex=dotIndex-1;

	while(slashIndex >=0 && path[slashIndex] && (path[slashIndex] !=L'\\' && path[slashIndex] !=L'/'))
		slashIndex--;

	if(dotIndex<0 || slashIndex < 0 || slashIndex >=dotIndex)
	{
		return FALSE;
	}
	wcsncpy_s(nameBuf,MAX_PATH,path+slashIndex+1,dotIndex-slashIndex-1);
	wcsncpy_s(outDirBuf,MAX_PATH,path,slashIndex);

	wsprintf(outDirBuf,L"%s\\%s",outDirBuf,nameBuf);
	//create outDir
	if(CreateDirectory(outDirBuf,NULL)==FALSE &&GetLastError() !=ERROR_ALREADY_EXISTS)
	{
		return E_FAIL;
	}
	return TRUE;
}

HRESULT ParseFrame(const wchar_t *path,IWICImagingFactory *pIWICFactory,IWICBitmapFrameDecode *iBitmapSource)
{
	IWICBitmapEncoder *pJpegEncoder=NULL;
	IWICBitmapFrameEncode *iBitmapSourceFrameEncode=NULL;
	IWICStream *iStream=NULL;
	HRESULT	hr=E_FAIL;

	__try
	{
		FAILEDEX(pIWICFactory->CreateStream(&iStream));
		FAILEDEX(iStream->InitializeFromFilename(path,GENERIC_WRITE));

		FAILEDEX(pIWICFactory->CreateEncoder(GUID_ContainerFormatJpeg,NULL,&pJpegEncoder));
		FAILEDEX(pJpegEncoder->Initialize(iStream,WICBitmapEncoderNoCache));

		FAILEDEX(pJpegEncoder->CreateNewFrame(&iBitmapSourceFrameEncode,NULL));
		FAILEDEX(iBitmapSourceFrameEncode->Initialize(NULL));
		FAILEDEX(iBitmapSourceFrameEncode->WriteSource(iBitmapSource,NULL));

		FAILEDEX(iBitmapSourceFrameEncode->Commit());
		FAILEDEX(pJpegEncoder->Commit());
	}
	__finally
	{
		if(pJpegEncoder !=NULL)
		{
			pJpegEncoder->Release();
		}
		if(iBitmapSourceFrameEncode !=NULL)
		{
			iBitmapSourceFrameEncode->Release();
		}
		if(iStream !=NULL)
		{
			iStream->Release();
		}
	}
	return S_OK;
}

HRESULT ParseGIFWithSperate(const wchar_t *filePath,const wchar_t *fileName, const wchar_t *outDir)
{
	HRESULT hr=E_FAIL;
	IWICImagingFactory *pIWICFactory=NULL;
	IWICBitmapDecoder *pIWICGifDecoder=NULL;
	IWICStream *iStream=NULL;
	wchar_t outputPath[MAX_PATH]={0};
	__try
	{
		// Create WIC factory
		FAILEDEX(CoCreateInstance(CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pIWICFactory)));
		FAILEDEX(pIWICFactory->CreateDecoder(GUID_ContainerFormatGif,NULL,&pIWICGifDecoder));
		FAILEDEX(pIWICFactory->CreateStream(&iStream));
		FAILEDEX(iStream->InitializeFromFilename(filePath,GENERIC_READ));
		FAILEDEX(pIWICGifDecoder->Initialize(iStream,WICDecodeMetadataCacheOnLoad));

		UINT frameCount=0;
		FAILEDEX(pIWICGifDecoder->GetFrameCount(&frameCount));

		for(size_t i=0;i<frameCount;i++)
		{
			wsprintf(outputPath,L"%s\\%s_%d.jpg",outDir,fileName,(int)(i+1));
			IWICBitmapFrameDecode *iBitmapSource=NULL;
			pIWICGifDecoder->GetFrame((UINT)i,&iBitmapSource);
			hr=ParseFrame(outputPath,pIWICFactory,iBitmapSource);
			iBitmapSource->Release();
			FAILEDEX(hr);
		}
	}
	__finally
	{
		if(pIWICFactory != NULL)
		{
			pIWICFactory->Release();
		}

		if(pIWICGifDecoder !=NULL)
		{
			pIWICGifDecoder->Release();
		}
		if(iStream != NULL)
		{
			iStream->Release();
		}
	}
	return S_OK;
}

HRESULT GetGlobalFrameSize(IWICBitmapDecoder *pIWICGifDecoder,UINT &width,UINT &height)
{
	 IWICMetadataQueryReader *pMetadataQueryReader = NULL;
	 FAILEDEX(pIWICGifDecoder->GetMetadataQueryReader(&pMetadataQueryReader));
	 PROPVARIANT propValue;

	 PropVariantInit(&propValue);
	 width=0;
	 height=0;
	 __try
	 {
		 FAILEDEX(pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/Width",&propValue));
		 FAILEDEX(propValue.vt==VT_UI2);
		 width=propValue.uiVal;
		 PropVariantClear(&propValue);

		 FAILEDEX(pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/Height",&propValue));
		 FAILEDEX(propValue.vt==VT_UI2);
		 height=propValue.uiVal;
		 PropVariantClear(&propValue);

		 UINT uPixelAspRatio=0;
		 FAILEDEX(pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/PixelAspectRatio",&propValue));
		 FAILEDEX(propValue.vt==VT_UI1);
		 uPixelAspRatio=propValue.bVal;
		 if(uPixelAspRatio !=0)
		 {
			  FLOAT pixelAspRatio = (uPixelAspRatio + 15.f) / 64.f;
			  if (pixelAspRatio > 1.f)
			  {
				  height = static_cast<UINT>(height / pixelAspRatio);
			  }
			  else
			  {
				  width = static_cast<UINT>(width * pixelAspRatio);
			  }
		 }
		 return S_OK;
	 }
	 __finally
	 {
		 pMetadataQueryReader->Release();
	 }
}

HRESULT GetFrameRect(IWICBitmapFrameDecode *iBitmapSource,D2D1_RECT_F *rect)
{
	D2D1_RECT_F framePosition={0.f};
	IWICMetadataQueryReader *pFrameMetadataQueryReader = NULL;
    HRESULT hr=E_FAIL;
    PROPVARIANT propValue;
    PropVariantInit(&propValue);
	
	__try
	{
		rect->left=rect->right=rect->bottom=rect->top=0.f;

		FAILEDEX(iBitmapSource->GetMetadataQueryReader(&pFrameMetadataQueryReader));
		
		FAILEDEX(pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Left", &propValue));
        FAILEDEX(propValue.vt == VT_UI2 ? S_OK : E_FAIL);
        rect->left = static_cast<FLOAT>(propValue.uiVal);
        PropVariantClear(&propValue);

        FAILEDEX(pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Top", &propValue));
		FAILEDEX((propValue.vt == VT_UI2 ? S_OK : E_FAIL));
		rect->top = static_cast<FLOAT>(propValue.uiVal);
		PropVariantClear(&propValue);
        
        FAILEDEX(pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Width", &propValue));
		FAILEDEX(propValue.vt == VT_UI2 ? S_OK : E_FAIL); 
		rect->right = static_cast<FLOAT>(propValue.uiVal) + rect->left;
		PropVariantClear(&propValue);

        FAILEDEX(pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Height", &propValue));
        FAILEDEX(propValue.vt == VT_UI2 ? S_OK : E_FAIL);
		rect->bottom = static_cast<FLOAT>(propValue.uiVal)+rect->top;
		PropVariantClear(&propValue);
	}
	__finally
	{
		pFrameMetadataQueryReader->Release();
	}
	return S_OK;
}

HRESULT GetBackgroundColor(IWICImagingFactory *pIWICFactory,IWICBitmapDecoder *pIWICGifDecoder,D2D1_COLOR_F *backgroundColor)
{
    DWORD dwBGColor;
    BYTE backgroundIndex = 0;
    WICColor rgColors[256];
    UINT cColorsCopied = 0;
    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);
    IWICPalette *pWicPalette = NULL;
	IWICMetadataQueryReader *pMetadataQueryReader=NULL;

	__try
	{
		FAILEDEX(pIWICGifDecoder->GetMetadataQueryReader(&pMetadataQueryReader));
		FAILEDEX(pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag",&propVariant));
		FAILEDEX(propVariant.vt != VT_BOOL || !propVariant.boolVal);
		PropVariantClear(&propVariant);
		FAILEDEX(pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &propVariant));
		FAILEDEX(propVariant.vt != VT_UI1);
		backgroundIndex = propVariant.bVal;
		PropVariantClear(&propVariant);
		// Get the color from the palette
		FAILEDEX(pIWICFactory->CreatePalette(&pWicPalette));
		// Get the global palette
		FAILEDEX(pIWICGifDecoder->CopyPalette(pWicPalette));
		FAILEDEX( pWicPalette->GetColors(ARRAYSIZE(rgColors),rgColors,&cColorsCopied));
		// Check whether background color is outside range 
		FAILEDEX(backgroundIndex >= cColorsCopied);
		// Get the color in ARGB format
		dwBGColor = rgColors[backgroundIndex];
		// The background color is in ARGB format, and we want to 
		// extract the alpha value and convert it in FLOAT
		FLOAT alpha = (dwBGColor >> 24) / 255.f;
		*backgroundColor = D2D1::ColorF(dwBGColor, alpha);
	}
	__finally
	{
		SAFERELEASE(pWicPalette);
		SAFERELEASE(pMetadataQueryReader);
	}
    return S_OK;
}

HRESULT ParseGIFWithOverlay(const wchar_t *filePath,const wchar_t *fileName, const wchar_t *outDir)
{
	HRESULT hr=E_FAIL;
	ID2D1Factory *pD2DFactory = NULL;
	IWICImagingFactory *pIWICFactory=NULL;
	IWICBitmapDecoder *pIWICGifDecoder=NULL;
	IWICStream *iStream=NULL;
	ID2D1RenderTarget *pRT = NULL;
	IWICBitmap *targetBitmap=0;
	UINT frameCount=0;
	UINT width=0;
	UINT height=0;
	D2D1_COLOR_F backgroundColor;
	wchar_t outputPath[MAX_PATH]={0};

	__try
	{
		// Create all instance
		FAILEDEX(CoCreateInstance(CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pIWICFactory)));
		FAILEDEX(pIWICFactory->CreateDecoder(GUID_ContainerFormatGif,NULL,&pIWICGifDecoder));
		FAILEDEX(pIWICFactory->CreateStream(&iStream));
		FAILEDEX(iStream->InitializeFromFilename(filePath,GENERIC_READ));
		FAILEDEX(pIWICGifDecoder->Initialize(iStream,WICDecodeMetadataCacheOnLoad));
		FAILEDEX(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory));
		FAILEDEX(pIWICGifDecoder->GetFrameCount(&frameCount));
		FAILEDEX(GetGlobalFrameSize(pIWICGifDecoder,width,height));
		FAILEDEX(pIWICFactory->CreateBitmap(width,height,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnDemand,&targetBitmap));
		FAILEDEX(pD2DFactory->CreateWicBitmapRenderTarget(targetBitmap,D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN ,D2D1_ALPHA_MODE_UNKNOWN)),&pRT));

		if(SUCCEEDED(GetBackgroundColor(pIWICFactory,pIWICGifDecoder, &backgroundColor)))
		{
			pRT->BeginDraw();
			pRT->Clear(&backgroundColor);
			FAILEDEX(pRT->EndDraw());
		}

		for(size_t i=0;i<frameCount;i++)
		{
			wsprintf(outputPath,L"%s\\%s_%d.jpg",outDir,fileName,(int)(i+1));
			IWICStream *iFileStream=NULL;
			FAILEDEX(pIWICFactory->CreateStream(&iFileStream));
			FAILEDEX(iFileStream->InitializeFromFilename(outputPath,GENERIC_WRITE));
			IWICBitmapFrameDecode *iBitmapSource=NULL;
			IWICBitmapEncoder *pEncoder = NULL;
			IWICBitmapFrameEncode *pFrameEncode = NULL;
			IWICFormatConverter *pConverter = NULL;
			ID2D1Bitmap *bitmap=NULL;
			__try
			{
				FAILEDEX(pIWICFactory->CreateFormatConverter(&pConverter));
				FAILEDEX(pIWICGifDecoder->GetFrame((UINT)i,&iBitmapSource));
				FAILEDEX(pConverter->Initialize(iBitmapSource,GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,NULL,0.0f,WICBitmapPaletteTypeCustom));
				FAILEDEX(pRT->CreateBitmapFromWicBitmap(pConverter,&bitmap));
				D2D1_RECT_F rect;
				FAILEDEX(GetFrameRect(iBitmapSource,&rect));
				pRT->BeginDraw();
				pRT->DrawBitmap(bitmap,rect);
				FAILEDEX(pRT->EndDraw());
				FAILEDEX(pIWICFactory->CreateEncoder(GUID_ContainerFormatJpeg,NULL,&pEncoder));
				FAILEDEX(pEncoder->Initialize(iFileStream,WICBitmapEncoderNoCache));
				FAILEDEX(pEncoder->CreateNewFrame(&pFrameEncode, NULL));
				FAILEDEX(pFrameEncode->Initialize(NULL));
				FAILEDEX(pFrameEncode->SetSize(width,height));
				WICPixelFormatGUID format = GUID_WICPixelFormatDontCare;
				FAILEDEX(pFrameEncode->SetPixelFormat(&format));
				FAILEDEX(pFrameEncode->WriteSource(targetBitmap,NULL));
				FAILEDEX(pFrameEncode->Commit());
				FAILEDEX(pEncoder->Commit());
			}
			__finally
			{
				SAFERELEASE(iBitmapSource);
				SAFERELEASE(iFileStream);
				SAFERELEASE(pEncoder);
				SAFERELEASE(pFrameEncode);
				SAFERELEASE(pConverter);
				SAFERELEASE(bitmap);
			}
		}
	}
	__finally
	{
		SAFERELEASE(pIWICFactory);
		SAFERELEASE(pD2DFactory);
		SAFERELEASE(pIWICGifDecoder);
		SAFERELEASE(iStream);
		SAFERELEASE(pRT);
		SAFERELEASE(targetBitmap);
	}
	return S_OK;
}

HRESULT ParseGIFFile(const wchar_t *path,BOOL imgOverlay,wchar_t *outDir)
{
	wchar_t fileName[MAX_PATH]={0};

	if(ParseFileNameAndCreateOutdir(path,fileName,outDir)==FALSE)
	{
		return E_FAIL;
	}

	HRESULT hr=imgOverlay ?  ParseGIFWithOverlay(path,fileName,outDir):ParseGIFWithSperate(path,fileName,outDir) ;

	return hr;	
}