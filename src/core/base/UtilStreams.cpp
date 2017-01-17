//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Stream related utilities / utility streams
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "UtilStreams.h"
#include "MsgIntf.h"
#include "DebugIntf.h"
#include "EventIntf.h"
#include "StorageIntf.h"






//---------------------------------------------------------------------------
// tTVPLocalTempStorageHolder
//---------------------------------------------------------------------------
#define TVP_LOCAL_TEMP_COPY_BLOCK_SIZE 65536*2
tTVPLocalTempStorageHolder::tTVPLocalTempStorageHolder(const ttstr & name)
{
	// name must be normalized !!!

	FileMustBeDeleted = false;
	FolderMustBeDeleted = false;
	LocalName = TVPGetLocallyAccessibleName(name);
	if(LocalName.IsEmpty())
	{
		// file must be copied to local filesystem

		// note that the basename is much more important than the directory
		// which the file is to be in, so we create a temporary folder and
		// store the file in it.

		LocalFolder = TVPGetTemporaryName();
		LocalName = LocalFolder + TJS_W("/") + TVPExtractStorageName(name);
		TVPCreateFolders(LocalFolder); // create temporary folder
		FolderMustBeDeleted = true;
		FileMustBeDeleted = true;

		try
		{
			// copy to local file
			tTVPStreamHolder src(name);
			tTVPStreamHolder dest(LocalName, TJS_BS_WRITE|TJS_BS_DELETE_ON_CLOSE);
			tjs_uint8 * buffer = new tjs_uint8[TVP_LOCAL_TEMP_COPY_BLOCK_SIZE];
			try
			{
				tjs_uint read;
				while(true)
				{
					read = src->Read(buffer, TVP_LOCAL_TEMP_COPY_BLOCK_SIZE);
					if(read == 0) break;
					dest->WriteBuffer(buffer, read);
				}
			}
			catch(...)
			{
				delete [] buffer;
				throw;
			}
			delete [] buffer;
		}
		catch(...)
		{
			if(FileMustBeDeleted) TVPRemoveFile(LocalName);
			if(FolderMustBeDeleted) TVPRemoveFolder(LocalFolder);
			throw;
		}
	}
}
//---------------------------------------------------------------------------
tTVPLocalTempStorageHolder::~tTVPLocalTempStorageHolder()
{
	if(FileMustBeDeleted) TVPRemoveFile(LocalName);
	if(FolderMustBeDeleted) TVPRemoveFolder(LocalFolder);
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPMemoryStream
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream()
{
	Init();
}
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream(const void * block, tjs_uint size)
{
	Init();
	Block = (void*)block;
	if(!Block)
	{
		Block = Alloc(size);
		if(!Block) TVPThrowExceptionMessage(TVPInsufficientMemory);
	}
	else
	{
		Reference = true; // memory block was given
	}
	Size = size;
	AllocSize = size;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
tTVPMemoryStream::~tTVPMemoryStream()
{
	if(Block && !Reference) Free(Block);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPMemoryStream::Seek(tjs_int64 offset, tjs_int whence)
{
	tjs_int64 newpos;
	switch(whence)
	{
	case TJS_BS_SEEK_SET:
		if(offset >= 0)
		{
			if(offset <= Size) CurrentPos = static_cast<tjs_uint>(offset);
		}
		return CurrentPos;

	case TJS_BS_SEEK_CUR:
		if((newpos = offset + (tjs_int64)CurrentPos) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if(np <= Size) CurrentPos = np;
		}
		return CurrentPos;

	case TJS_BS_SEEK_END:
		if((newpos = offset + (tjs_int64)Size) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if(np <= Size) CurrentPos = np;
		}
		return CurrentPos;
	}
	return CurrentPos;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Read(void *buffer, tjs_uint read_size)
{
	if(CurrentPos + read_size >= Size)
	{
		read_size = Size - CurrentPos;
	}

	memcpy(buffer, (tjs_uint8*)Block + CurrentPos, read_size);

	CurrentPos += read_size;

	return read_size;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Write(const void *buffer, tjs_uint write_size)
{
	// writing may increase the internal buffer size.
	if(Reference) TVPThrowExceptionMessage(TVPWriteError);

	tjs_uint newpos = CurrentPos + write_size;
	if(newpos >= AllocSize)
	{
		// exceeds AllocSize
		tjs_uint onesize;
		if(AllocSize < 64*1024) onesize = 4*1024;
		else if(AllocSize < 512*1024) onesize = 16*1024;
		else if(AllocSize < 4096*1024) onesize = 256*1024;
		else onesize = 2024*1024;
		AllocSize += onesize;

		if(CurrentPos + write_size >= AllocSize) // still insufficient ?
		{
			AllocSize = CurrentPos + write_size;
		}

		Block = Realloc(Block, AllocSize);

		if(AllocSize && !Block)
			TVPThrowExceptionMessage(TVPInsufficientMemory);
			// this exception cannot be repaird; a fatal error.
	}

	memcpy((tjs_uint8*)Block + CurrentPos, buffer, write_size);

	CurrentPos = newpos;

	if(CurrentPos > Size) Size = CurrentPos;

	return write_size;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPMemoryStream::SetEndOfStorage()
{
	if(Reference) TVPThrowExceptionMessage(TVPWriteError);

	Size = CurrentPos;
	AllocSize = Size;
	Block = Realloc(Block, Size);
	if(Size && !Block)
		TVPThrowExceptionMessage(TVPInsufficientMemory);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Clear(void)
{
	if(Block && !Reference) Free(Block);
	Init();
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::SetSize(tjs_uint size)
{
	if(Reference) TVPThrowExceptionMessage(TVPWriteError);

	if(Size > size)
	{
		// decrease
		Size = size;
		AllocSize = size;
		Block = Realloc(Block, size);
		if(CurrentPos > Size) CurrentPos = Size;
		if(size && !Block)
			TVPThrowExceptionMessage(TVPInsufficientMemory);
	}
	else
	{
		// increase
		AllocSize = size;
		Size = size;
		Block = Realloc(Block, size);
		if(size && !Block)
			TVPThrowExceptionMessage(TVPInsufficientMemory);

	}
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Init()
{
	Block = NULL;
	Reference = false;
	Size = 0;
	AllocSize = 0;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Alloc(size_t size)
{
	return TJS_malloc(size);
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Realloc(void *orgblock, size_t size)
{
	return TJS_realloc(orgblock, size);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Free(void *block)
{
	TJS_free(block);
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTVPPartialStream
//---------------------------------------------------------------------------
tTVPPartialStream::tTVPPartialStream(tTJSBinaryStream *stream, tjs_uint64 start,
	tjs_uint64 size)
{
	// the stream given as a argument here will be owned by this instance,
	// and freed at the destruction.

	Stream = stream;
	Start = start;
	Size = size;
	CurrentPos = 0;

	try
	{
		Stream->SetPosition(Start);
	}
	catch(...)
	{
		delete Stream;
		Stream = NULL;
		throw;
	}
}
//---------------------------------------------------------------------------
tTVPPartialStream::~tTVPPartialStream()
{
	if(Stream) delete Stream;
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPPartialStream::Seek(tjs_int64 offset, tjs_int whence)
{
	tjs_int64 newpos;
	switch(whence)
	{
	case TJS_BS_SEEK_SET:
		newpos = offset;
		if(offset >= 0 && offset <= static_cast<tjs_int64>(Size) )
		{
			newpos += Start;
			CurrentPos = Stream->Seek(newpos, TJS_BS_SEEK_SET) - Start;
		}
		return CurrentPos;

	case TJS_BS_SEEK_CUR:
		newpos = offset + CurrentPos;
		if(offset >= 0 && offset <= static_cast<tjs_int64>(Size) )
		{
			newpos += Start;
			CurrentPos = Stream->Seek(newpos, TJS_BS_SEEK_SET) - Start;
		}
		return CurrentPos;

	case TJS_BS_SEEK_END:
		newpos = offset + Size;
		if(offset >= 0 && offset <= static_cast<tjs_int64>(Size) )
		{
			newpos += Start;
			CurrentPos = Stream->Seek(newpos, TJS_BS_SEEK_SET) - Start;
		}
		return CurrentPos;
	}
	return CurrentPos;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPPartialStream::Read(void *buffer, tjs_uint read_size)
{
	if(CurrentPos + read_size >= Size)
	{
		read_size = static_cast<tjs_uint>(Size - CurrentPos);
	}

	tjs_uint read = Stream->Read(buffer, read_size);

	CurrentPos += read;

	return read;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPPartialStream::Write(const void *buffer, tjs_uint write_size)
{
	return 0;
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPPartialStream::GetSize()
{
	return Size;
}
//---------------------------------------------------------------------------


