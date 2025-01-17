/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2005, 2006, 2008 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"
#include "wine/debug.h"

#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

static DWORD from_bmformat( BMFORMAT format )
{
    static BOOL quietfixme = FALSE;
    DWORD ret;

    switch (format)
    {
    case BM_RGBTRIPLETS: ret = TYPE_BGR_8; break;
    case BM_BGRTRIPLETS: ret = TYPE_RGB_8; break;
    case BM_GRAY:        ret = TYPE_GRAY_8; break;
    case BM_xRGBQUADS:   ret = TYPE_BGRA_8; break;
    case BM_xBGRQUADS:   ret = TYPE_RGBA_8; break;
    case BM_KYMCQUADS:   ret = TYPE_CMYK_8; break;
    default:
        if (!quietfixme)
        {
            FIXME( "unhandled bitmap format %#x\n", format );
            quietfixme = TRUE;
        }
        ret = TYPE_RGB_8;
        break;
    }
    TRACE( "color space: %#x -> %#lx\n", format, ret );
    return ret;
}

static DWORD from_type( COLORTYPE type )
{
    DWORD ret;

    switch (type)
    {
    case COLOR_GRAY: ret = TYPE_GRAY_16; break;
    case COLOR_RGB:  ret = TYPE_RGB_16; break;
    case COLOR_XYZ:  ret = TYPE_XYZ_16; break;
    case COLOR_Yxy:  ret = TYPE_Yxy_16; break;
    case COLOR_Lab:  ret = TYPE_Lab_16; break;
    case COLOR_CMYK: ret = TYPE_CMYK_16; break;
    default:
        FIXME( "unhandled color type %08x\n", type );
        ret = TYPE_RGB_16;
        break;
    }

    TRACE( "color type: %#x -> %#lx\n", type, ret );
    return ret;
}

/******************************************************************************
 * CreateColorTransformA            [MSCMS.@]
 *
 * See CreateColorTransformW.
 */
HTRANSFORM WINAPI CreateColorTransformA( LPLOGCOLORSPACEA space, HPROFILE dest, HPROFILE target, DWORD flags )
{
    LOGCOLORSPACEW spaceW;
    DWORD len;

    TRACE( "( %p, %p, %p, %#lx )\n", space, dest, target, flags );

    if (!space || !dest) return FALSE;

    memcpy( &spaceW, space, FIELD_OFFSET(LOGCOLORSPACEA, lcsFilename) );
    spaceW.lcsSize = sizeof(LOGCOLORSPACEW);

    len = MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, NULL, 0 );
    MultiByteToWideChar( CP_ACP, 0, space->lcsFilename, -1, spaceW.lcsFilename, len );

    return CreateColorTransformW( &spaceW, dest, target, flags );
}

/******************************************************************************
 * CreateColorTransformW            [MSCMS.@]
 *
 * Create a color transform.
 *
 * PARAMS
 *  space  [I] Input color space.
 *  dest   [I] Color profile of destination device.
 *  target [I] Color profile of target device.
 *  flags  [I] Flags.
 *
 * RETURNS
 *  Success: Handle to a transform.
 *  Failure: NULL
 */
HTRANSFORM WINAPI CreateColorTransformW( LPLOGCOLORSPACEW space, HPROFILE dest, HPROFILE target, DWORD flags )
{
    HTRANSFORM ret = NULL;
    cmsHTRANSFORM transform;
    struct profile *dst, *tgt = NULL;
    DWORD proofing = 0;
    cmsHPROFILE input;
    int intent;

    TRACE( "( %p, %p, %p, %#lx )\n", space, dest, target, flags );

    if (!space || !(dst = grab_profile( dest ))) return FALSE;

    if (target && !(tgt = grab_profile( target )))
    {
        release_profile( dst );
        return FALSE;
    }
    intent = space->lcsIntent > 3 ? INTENT_PERCEPTUAL : space->lcsIntent;

    TRACE( "lcsIntent:   %#lx\n", space->lcsIntent );
    TRACE( "lcsCSType:   %s\n", dbgstr_tag( space->lcsCSType ) );
    TRACE( "lcsFilename: %s\n", debugstr_w( space->lcsFilename ) );

    input = cmsCreate_sRGBProfile(); /* FIXME: create from supplied color space */
    if (target) proofing = cmsFLAGS_SOFTPROOFING;
    transform = cmsCreateProofingTransform( input, 0, dst->cmsprofile, 0, tgt ? tgt->cmsprofile : NULL,
                                            intent, INTENT_ABSOLUTE_COLORIMETRIC, proofing );
    if (!transform)
    {
        if (tgt) release_profile( tgt );
        release_profile( dst );
        return FALSE;
    }

    ret = create_transform( transform );

    if (tgt) release_profile( tgt );
    release_profile( dst );
    return ret;
}

/******************************************************************************
 * CreateMultiProfileTransform      [MSCMS.@]
 *
 * Create a color transform from an array of color profiles.
 *
 * PARAMS
 *  profiles  [I] Array of color profiles.
 *  nprofiles [I] Number of color profiles.
 *  intents   [I] Array of rendering intents.
 *  flags     [I] Flags.
 *  cmm       [I] Profile to take the CMM from.
 *
 * RETURNS
 *  Success: Handle to a transform.
 *  Failure: NULL
 */ 
HTRANSFORM WINAPI CreateMultiProfileTransform( PHPROFILE profiles, DWORD nprofiles,
    PDWORD intents, DWORD nintents, DWORD flags, DWORD cmm )
{
    HTRANSFORM ret = NULL;
    cmsHPROFILE cmsprofiles[2];
    cmsHTRANSFORM transform;
    struct profile *profile0, *profile1;

    TRACE( "( %p, %#lx, %p, %lu, %#lx, %#lx )\n", profiles, nprofiles, intents, nintents, flags, cmm );

    if (!profiles || !nprofiles || !intents) return NULL;

    if (nprofiles > 2)
    {
        FIXME("more than 2 profiles not supported\n");
        return NULL;
    }

    profile0 = grab_profile( profiles[0] );
    if (!profile0) return NULL;
    profile1 = grab_profile( profiles[1] );
    if (!profile1)
    {
        release_profile( profile0 );
        return NULL;
    }

    cmsprofiles[0] = profile0->cmsprofile;
    cmsprofiles[1] = profile1->cmsprofile;

    transform = cmsCreateMultiprofileTransform( cmsprofiles, nprofiles, 0, 0, *intents, 0 );
    if (transform) ret = create_transform( transform );

    release_profile( profile0 );
    release_profile( profile1 );
    return ret;
}

/******************************************************************************
 * DeleteColorTransform             [MSCMS.@]
 *
 * Delete a color transform.
 *
 * PARAMS
 *  transform [I] Handle to a color transform.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */ 
BOOL WINAPI DeleteColorTransform( HTRANSFORM handle )
{
    TRACE( "( %p )\n", handle );

    return close_transform( handle );
}

/******************************************************************************
 * TranslateBitmapBits              [MSCMS.@]
 *
 * Perform color translation.
 *
 * PARAMS
 *  transform    [I] Handle to a color transform.
 *  srcbits      [I] Source bitmap.
 *  input        [I] Format of the source bitmap.
 *  width        [I] Width of the source bitmap.
 *  height       [I] Height of the source bitmap.
 *  inputstride  [I] Number of bytes in one scanline.
 *  destbits     [I] Destination bitmap.
 *  output       [I] Format of the destination bitmap.
 *  outputstride [I] Number of bytes in one scanline. 
 *  callback     [I] Callback function.
 *  data         [I] Callback data. 
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI TranslateBitmapBits( HTRANSFORM handle, PVOID srcbits, BMFORMAT input,
    DWORD width, DWORD height, DWORD inputstride, PVOID destbits, BMFORMAT output,
    DWORD outputstride, PBMCALLBACKFN callback, ULONG data )
{
    BOOL ret;
    cmsHTRANSFORM transform = grab_transform( handle );

    TRACE( "( %p, %p, %#x, %lu, %lu, %lu, %p, %#x, %lu, %p, %#lx )\n",
           handle, srcbits, input, width, height, inputstride, destbits, output,
           outputstride, callback, data );

    if (!transform) return FALSE;
    ret = cmsChangeBuffersFormat( transform, from_bmformat(input), from_bmformat(output) );
    if (ret) cmsDoTransform( transform, srcbits, destbits, width * height );
    release_transform( transform );
    return ret;
}

/******************************************************************************
 * TranslateColors              [MSCMS.@]
 *
 * Perform color translation.
 *
 * PARAMS
 *  transform    [I] Handle to a color transform.
 *  input        [I] Array of input colors.
 *  number       [I] Number of colors to translate.
 *  input_type   [I] Input color format.
 *  output       [O] Array of output colors.
 *  output_type  [I] Output color format.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI TranslateColors( HTRANSFORM handle, PCOLOR in, DWORD count,
                             COLORTYPE input_type, PCOLOR out, COLORTYPE output_type )
{
    BOOL ret;
    unsigned int i;
    cmsHTRANSFORM transform = grab_transform( handle );

    TRACE( "( %p, %p, %lu, %d, %p, %d )\n", handle, in, count, input_type, out, output_type );

    if (!transform) return FALSE;

    ret = cmsChangeBuffersFormat( transform, from_type(input_type), from_type(output_type) );
    if (ret)
        for (i = 0; i < count; i++) cmsDoTransform( transform, &in[i], &out[i], 1 );

    release_transform( transform );
    return ret;
}
