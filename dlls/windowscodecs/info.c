/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    const IWICBitmapDecoderInfoVtbl *lpIWICBitmapDecoderInfoVtbl;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} BitmapDecoderInfo;

static HRESULT WINAPI BitmapDecoderInfo_QueryInterface(IWICBitmapDecoderInfo *iface, REFIID iid,
    void **ppv)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICComponentInfo, iid) ||
        IsEqualIID(&IID_IWICBitmapCodecInfo, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoderInfo ,iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapDecoderInfo_AddRef(IWICBitmapDecoderInfo *iface)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapDecoderInfo_Release(IWICBitmapDecoderInfo *iface)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        RegCloseKey(This->classkey);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapDecoderInfo_GetComponentType(IWICBitmapDecoderInfo *iface,
    WICComponentType *pType)
{
    TRACE("(%p,%p)\n", iface, pType);
    *pType = WICDecoder;
    return S_OK;
}

static HRESULT WINAPI BitmapDecoderInfo_GetCLSID(IWICBitmapDecoderInfo *iface, CLSID *pclsid)
{
    FIXME("(%p,%p): stub\n", iface, pclsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetSigningStatus(IWICBitmapDecoderInfo *iface, DWORD *pStatus)
{
    FIXME("(%p,%p): stub\n", iface, pStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetAuthor(IWICBitmapDecoderInfo *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchAuthor, wzAuthor, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetVendorGUID(IWICBitmapDecoderInfo *iface, GUID *pguidVendor)
{
    FIXME("(%p,%p): stub\n", iface, pguidVendor);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetVersion(IWICBitmapDecoderInfo *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchVersion, wzVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetSpecVersion(IWICBitmapDecoderInfo *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetFriendlyName(IWICBitmapDecoderInfo *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetContainerFormat(IWICBitmapDecoderInfo *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%p): stub\n", iface, pguidContainerFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetPixelFormats(IWICBitmapDecoderInfo *iface,
    UINT cFormats, GUID *pguidPixelFormats, UINT *pcActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cFormats, pguidPixelFormats, pcActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetColorManagementVersion(IWICBitmapDecoderInfo *iface,
    UINT cchColorManagementVersion, WCHAR *wzColorManagementVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchColorManagementVersion, wzColorManagementVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetDeviceManufacturer(IWICBitmapDecoderInfo *iface,
    UINT cchDeviceManufacturer, WCHAR *wzDeviceManufacturer, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchDeviceManufacturer, wzDeviceManufacturer, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetDeviceModels(IWICBitmapDecoderInfo *iface,
    UINT cchDeviceModels, WCHAR *wzDeviceModels, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchDeviceModels, wzDeviceModels, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetMimeTypes(IWICBitmapDecoderInfo *iface,
    UINT cchMimeTypes, WCHAR *wzMimeTypes, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchMimeTypes, wzMimeTypes, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetFileExtensions(IWICBitmapDecoderInfo *iface,
    UINT cchFileExtensions, WCHAR *wzFileExtensions, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchFileExtensions, wzFileExtensions, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportAnimation(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportAnimation)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportAnimation);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportChromaKey(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportChromaKey)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportChromaKey);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportLossless(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportLossless)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportLossless);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportMultiframe(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportMultiframe)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportMultiframe);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_MatchesMimeType(IWICBitmapDecoderInfo *iface,
    LPCWSTR wzMimeType, BOOL *pfMatches)
{
    FIXME("(%p,%s,%p): stub\n", iface, debugstr_w(wzMimeType), pfMatches);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetPatterns(IWICBitmapDecoderInfo *iface,
    UINT cbSizePatterns, WICBitmapPattern *pPatterns, UINT *pcPatterns, UINT *pcbPatternsActual)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    UINT pattern_count=0, patterns_size=0;
    WCHAR subkeyname[11];
    LONG res;
    HKEY patternskey, patternkey;
    static const WCHAR uintformatW[] = {'%','u',0};
    static const WCHAR patternsW[] = {'P','a','t','t','e','r','n','s',0};
    static const WCHAR positionW[] = {'P','o','s','i','t','i','o','n',0};
    static const WCHAR lengthW[] = {'L','e','n','g','t','h',0};
    static const WCHAR patternW[] = {'P','a','t','t','e','r','n',0};
    static const WCHAR maskW[] = {'M','a','s','k',0};
    static const WCHAR endofstreamW[] = {'E','n','d','O','f','S','t','r','e','a','m',0};
    HRESULT hr=S_OK;
    UINT i;
    BYTE *bPatterns=(BYTE*)pPatterns;
    DWORD length, valuesize;

    TRACE("(%p,%i,%p,%p,%p)\n", iface, cbSizePatterns, pPatterns, pcPatterns, pcbPatternsActual);

    res = RegOpenKeyExW(This->classkey, patternsW, 0, KEY_READ, &patternskey);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    res = RegQueryInfoKeyW(patternskey, NULL, NULL, NULL, &pattern_count, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (res == ERROR_SUCCESS)
    {
        patterns_size = pattern_count * sizeof(WICBitmapPattern);

        for (i=0; i<pattern_count; i++)
        {
            snprintfW(subkeyname, 11, uintformatW, i);
            res = RegOpenKeyExW(patternskey, subkeyname, 0, KEY_READ, &patternkey);
            if (res == ERROR_SUCCESS)
            {
                valuesize = sizeof(ULONG);
                res = RegGetValueW(patternkey, NULL, lengthW, RRF_RT_DWORD, NULL,
                    &length, &valuesize);
                patterns_size += length*2;

                if ((cbSizePatterns >= patterns_size) && (res == ERROR_SUCCESS))
                {
                    pPatterns[i].Length = length;

                    pPatterns[i].EndOfStream = 0;
                    valuesize = sizeof(BOOL);
                    RegGetValueW(patternkey, NULL, endofstreamW, RRF_RT_DWORD, NULL,
                        &pPatterns[i].EndOfStream, &valuesize);

                    pPatterns[i].Position.QuadPart = 0;
                    valuesize = sizeof(ULARGE_INTEGER);
                    res = RegGetValueW(patternkey, NULL, positionW, RRF_RT_DWORD|RRF_RT_QWORD, NULL,
                        &pPatterns[i].Position, &valuesize);

                    if (res == ERROR_SUCCESS)
                    {
                        pPatterns[i].Pattern = bPatterns+patterns_size-length*2;
                        valuesize = length;
                        res = RegGetValueW(patternkey, NULL, patternW, RRF_RT_REG_BINARY, NULL,
                            pPatterns[i].Pattern, &valuesize);
                    }

                    if (res == ERROR_SUCCESS)
                    {
                        pPatterns[i].Mask = bPatterns+patterns_size-length;
                        valuesize = length;
                        res = RegGetValueW(patternkey, NULL, maskW, RRF_RT_REG_BINARY, NULL,
                            pPatterns[i].Mask, &valuesize);
                    }
                }

                RegCloseKey(patternkey);
            }
            if (res != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(res);
                break;
            }
        }
    }
    else hr = HRESULT_FROM_WIN32(res);

    RegCloseKey(patternskey);

    if (hr == S_OK)
    {
        *pcPatterns = pattern_count;
        *pcbPatternsActual = patterns_size;
        if (pPatterns && cbSizePatterns < patterns_size)
            hr = WINCODEC_ERR_INSUFFICIENTBUFFER;
    }

    return hr;
}

static HRESULT WINAPI BitmapDecoderInfo_MatchesPattern(IWICBitmapDecoderInfo *iface,
    IStream *pIStream, BOOL *pfMatches)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pfMatches);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_CreateInstance(IWICBitmapDecoderInfo *iface,
    IWICBitmapDecoder **ppIBitmapDecoder)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;

    TRACE("(%p,%p)\n", iface, ppIBitmapDecoder);

    return CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)ppIBitmapDecoder);
}

static const IWICBitmapDecoderInfoVtbl BitmapDecoderInfo_Vtbl = {
    BitmapDecoderInfo_QueryInterface,
    BitmapDecoderInfo_AddRef,
    BitmapDecoderInfo_Release,
    BitmapDecoderInfo_GetComponentType,
    BitmapDecoderInfo_GetCLSID,
    BitmapDecoderInfo_GetSigningStatus,
    BitmapDecoderInfo_GetAuthor,
    BitmapDecoderInfo_GetVendorGUID,
    BitmapDecoderInfo_GetVersion,
    BitmapDecoderInfo_GetSpecVersion,
    BitmapDecoderInfo_GetFriendlyName,
    BitmapDecoderInfo_GetContainerFormat,
    BitmapDecoderInfo_GetPixelFormats,
    BitmapDecoderInfo_GetColorManagementVersion,
    BitmapDecoderInfo_GetDeviceManufacturer,
    BitmapDecoderInfo_GetDeviceModels,
    BitmapDecoderInfo_GetMimeTypes,
    BitmapDecoderInfo_GetFileExtensions,
    BitmapDecoderInfo_DoesSupportAnimation,
    BitmapDecoderInfo_DoesSupportChromaKey,
    BitmapDecoderInfo_DoesSupportLossless,
    BitmapDecoderInfo_DoesSupportMultiframe,
    BitmapDecoderInfo_MatchesMimeType,
    BitmapDecoderInfo_GetPatterns,
    BitmapDecoderInfo_MatchesPattern,
    BitmapDecoderInfo_CreateInstance
};

static HRESULT BitmapDecoderInfo_Constructor(HKEY classkey, REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    BitmapDecoderInfo *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapDecoderInfo));
    if (!This)
    {
        RegCloseKey(classkey);
        return E_OUTOFMEMORY;
    }

    This->lpIWICBitmapDecoderInfoVtbl = &BitmapDecoderInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

static WCHAR const clsid_keyname[] = {'C','L','S','I','D',0};
static WCHAR const instance_keyname[] = {'I','n','s','t','a','n','c','e',0};

struct category {
    WICComponentType type;
    const GUID *catid;
    HRESULT (*constructor)(HKEY,REFCLSID,IWICComponentInfo**);
};

static const struct category categories[] = {
    {WICDecoder, &CATID_WICBitmapDecoders, BitmapDecoderInfo_Constructor},
    {0}
};

HRESULT CreateComponentInfo(REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    HKEY clsidkey;
    HKEY classkey;
    HKEY catidkey;
    HKEY instancekey;
    WCHAR guidstring[39];
    LONG res;
    const struct category *category;
    int found=0;
    HRESULT hr;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, KEY_READ, &clsidkey);
    if (res != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(res);

    for (category=categories; category->type; category++)
    {
        StringFromGUID2(category->catid, guidstring, 39);
        res = RegOpenKeyExW(clsidkey, guidstring, 0, KEY_READ, &catidkey);
        if (res == ERROR_SUCCESS)
        {
            res = RegOpenKeyExW(catidkey, instance_keyname, 0, KEY_READ, &instancekey);
            if (res == ERROR_SUCCESS)
            {
                StringFromGUID2(clsid, guidstring, 39);
                res = RegOpenKeyExW(instancekey, guidstring, 0, KEY_READ, &classkey);
                if (res == ERROR_SUCCESS)
                {
                    RegCloseKey(classkey);
                    found = 1;
                }
                RegCloseKey(instancekey);
            }
            RegCloseKey(catidkey);
        }
        if (found) break;
    }

    if (found)
    {
        res = RegOpenKeyExW(clsidkey, guidstring, 0, KEY_READ, &classkey);
        if (res == ERROR_SUCCESS)
            hr = category->constructor(classkey, clsid, ppIInfo);
        else
            hr = HRESULT_FROM_WIN32(res);
    }
    else
        hr = E_FAIL;

    RegCloseKey(clsidkey);

    return hr;
}
