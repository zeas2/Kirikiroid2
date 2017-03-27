/* ioapi.h -- IO base function header for compress/uncompress .zip
   part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

         Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

         Modifications for Zip64 support
         Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

         For more info read MiniZip_info.txt

*/

#include "ioapi.h"
#include <unistd.h>

#ifdef WIN32
#define _POSIX_
#include <direct.h>
#endif
#include <fcntl.h>
#include <stdint.h>
#include "win32io.h"

//namespace cocos2d {

voidpf call_zopen64 (const zlib_filefunc64_32_def* pfilefunc,const void*filename,int mode)
{
    if (pfilefunc->zfile_func64.zopen64_file != NULL)
        return (*(pfilefunc->zfile_func64.zopen64_file)) (pfilefunc->zfile_func64.opaque,filename,mode);
    else
    {
        return (*(pfilefunc->zopen32_file))(pfilefunc->zfile_func64.opaque,(const char*)filename,mode);
    }
}

long call_zseek64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream, ZPOS64_T offset, int origin)
{
    if (pfilefunc->zfile_func64.zseek64_file != NULL)
        return (*(pfilefunc->zfile_func64.zseek64_file)) (pfilefunc->zfile_func64.opaque,filestream,offset,origin);
    else
    {
        uLong offsetTruncated = (uLong)offset;
        if (offsetTruncated != offset)
            return -1;
        else
            return (*(pfilefunc->zseek32_file))(pfilefunc->zfile_func64.opaque,filestream,offsetTruncated,origin);
    }
}

ZPOS64_T call_ztell64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream)
{
    if (pfilefunc->zfile_func64.zseek64_file != NULL)
        return (*(pfilefunc->zfile_func64.ztell64_file)) (pfilefunc->zfile_func64.opaque,filestream);
    else
    {
        uLong tell_uLong = (*(pfilefunc->ztell32_file))(pfilefunc->zfile_func64.opaque,filestream);
        if ((tell_uLong) == ((uLong)-1))
            return (ZPOS64_T)-1;
        else
            return tell_uLong;
    }
}

void fill_zlib_filefunc64_32_def_from_filefunc32(zlib_filefunc64_32_def* p_filefunc64_32,const zlib_filefunc_def* p_filefunc32)
{
    p_filefunc64_32->zfile_func64.zopen64_file = NULL;
    p_filefunc64_32->zopen32_file = p_filefunc32->zopen_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.zread_file = p_filefunc32->zread_file;
    p_filefunc64_32->zfile_func64.zwrite_file = p_filefunc32->zwrite_file;
    p_filefunc64_32->zfile_func64.ztell64_file = NULL;
    p_filefunc64_32->zfile_func64.zseek64_file = NULL;
    p_filefunc64_32->zfile_func64.zclose_file = p_filefunc32->zclose_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.opaque = p_filefunc32->opaque;
    p_filefunc64_32->zseek32_file = p_filefunc32->zseek_file;
    p_filefunc64_32->ztell32_file = p_filefunc32->ztell_file;
}



static voidpf  ZCALLBACK fopen_file_func OF((voidpf opaque, const char* filename, int mode));
static uLong   ZCALLBACK fread_file_func OF((voidpf opaque, voidpf stream, void* buf, uLong size));
static uLong   ZCALLBACK fwrite_file_func OF((voidpf opaque, voidpf stream, const void* buf,uLong size));
static ZPOS64_T ZCALLBACK ftell64_file_func OF((voidpf opaque, voidpf stream));
static long    ZCALLBACK fseek64_file_func OF((voidpf opaque, voidpf stream, ZPOS64_T offset, int origin));
static int     ZCALLBACK fclose_file_func OF((voidpf opaque, voidpf stream));
static int     ZCALLBACK ferror_file_func OF((voidpf opaque, voidpf stream));

static voidpf ZCALLBACK fopen_file_func (voidpf opaque, const char* filename, int mode)
{
    unsigned int rw = 0;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        rw |= O_RDONLY;
    else if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        rw |= O_RDWR;
    else if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        rw |= O_RDWR | O_CREAT | O_TRUNC;

    int Handle;
    Handle = open(filename, rw);

    return (voidpf)Handle;
}

static voidpf ZCALLBACK fopen64_file_func (voidpf opaque, const void* filename, int mode)
{
    return fopen_file_func(opaque, (const char*)filename, mode);
}


static uLong ZCALLBACK fread_file_func (voidpf opaque, voidpf stream, void* buf, uLong size)
{
    return (uLong)read((int64_t)stream, buf, size);
}

static uLong ZCALLBACK fwrite_file_func (voidpf opaque, voidpf stream, const void* buf, uLong size)
{
    return (uLong)write((int64_t)stream, buf, size);
}

static ZPOS64_T ZCALLBACK ftell64_file_func (voidpf opaque, voidpf stream)
{
    return (ZPOS64_T)lseek64((int64_t)stream, 0, SEEK_CUR);
}

static long ZCALLBACK ftell_file_func (voidpf opaque, voidpf stream)
{
    return (long)ftell64_file_func(opaque, stream);
}

static long ZCALLBACK fseek_file_func (voidpf  opaque, voidpf stream, uLong offset, int origin)
{
    int fseek_origin=0;
    long ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
    ret = 0;
	lseek64((int64_t)stream, offset, fseek_origin);
    return ret;
}

static long ZCALLBACK fseek64_file_func (voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
    int fseek_origin=0;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
	lseek64((int64_t)stream, offset, fseek_origin);
    return 0;
}


static int ZCALLBACK fclose_file_func (voidpf opaque, voidpf stream)
{
    int ret;
    ret = close((int64_t)stream);
    return ret;
}

static int ZCALLBACK ferror_file_func (voidpf opaque, voidpf stream)
{
    return 0;
}

void fill_fopen_filefunc (zlib_filefunc_def* pzlib_filefunc_def)
{
    pzlib_filefunc_def->zopen_file = fopen_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell_file = ftell_file_func;
    pzlib_filefunc_def->zseek_file = fseek_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

zlib_filefunc64_def TVPZlibFileFunc;
void fill_fopen64_filefunc (zlib_filefunc64_def*  pzlib_filefunc_def)
{
    pzlib_filefunc_def->zopen64_file = fopen64_file_func;
    pzlib_filefunc_def->zread_file = fread_file_func;
    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
    pzlib_filefunc_def->ztell64_file = ftell64_file_func;
    pzlib_filefunc_def->zseek64_file = fseek64_file_func;
    pzlib_filefunc_def->zclose_file = fclose_file_func;
    pzlib_filefunc_def->zerror_file = ferror_file_func;
    pzlib_filefunc_def->opaque = NULL;
}

class _TVPInitZlibFileFunc {
public:
	_TVPInitZlibFileFunc() {
		fill_fopen64_filefunc(&TVPZlibFileFunc);
	}
} __TVPInitZlibFileFunc;

//} // end of namespace cocos2d
