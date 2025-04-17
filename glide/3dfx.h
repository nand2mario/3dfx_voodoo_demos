/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
*/
#ifndef __3DFX_H__
#define __3DFX_H__

/*
** basic data types
*/
typedef unsigned char   FxU8;
typedef signed   char   FxI8;
typedef unsigned short  FxU16;
typedef signed   short  FxI16;
#if defined(__DOS__) || defined(__MSDOS__) || defined(_WIN32) || defined(macintosh)
typedef signed   long   FxI32;
typedef unsigned long   FxU32;
#else
typedef signed   int    FxI32;
typedef unsigned int    FxU32;
#endif

typedef long long       FxI64;
typedef unsigned long long FxU64;

typedef unsigned long   AnyPtr;
typedef int             FxBool;
typedef float           FxFloat;
typedef double          FxDouble;

/*
** color types
*/
typedef unsigned long                FxColor_t;
typedef struct { float r, g, b, a; } FxColor4;

/*
** fundamental types
*/
#define FXTRUE    1
#define FXFALSE   0

/*
** helper macros
*/
#define FXUNUSED( a ) ((void)(a))
#define FXBIT( i )    ( 1 << (i) )

/*
** export macros
*/

#if defined(__MSC__) || defined(_MSC_VER)
#  if defined (MSVC16)
#    define FX_ENTRY 
#    define FX_CALL
#  else
#    define FX_ENTRY extern
#    define FX_CALL  __stdcall
#  endif
#elif defined(__MINGW32__)
#  define FX_ENTRY extern
#  define FX_CALL  __stdcall
#elif defined(__unix__)
#  define FX_ENTRY extern
#  define FX_CALL
#elif defined(__MWERKS__)
#  if macintosh
#    define FX_ENTRY extern
#    define FX_CALL
#  else /* !macintosh */
#    error "Unknown MetroWerks target platform"
#  endif /* !macintosh */
#else
// #  warning define FX_ENTRY & FX_CALL for your compiler
#  define FX_ENTRY extern
#  define FX_CALL
#endif

#endif /* !__3DFX_H__ */
