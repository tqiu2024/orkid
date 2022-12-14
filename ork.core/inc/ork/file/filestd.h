////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 

#pragma once

///////////////////////////////////////////////////////////////////////////////

#include <ork/file/file.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork {
///////////////////////////////////////////////////////////////////////////////

class FileDevStd : public ork::FileDev
{

public:

	FileDevStd( );

private:
	virtual ork::EFileErrCode _doOpenFile( ork::File &rFile );
	virtual ork::EFileErrCode _doCloseFile( ork::File &rFile );
	virtual ork::EFileErrCode _doRead( ork::File &rFile, void *pTo, size_t iSize, size_t& actualread );
	virtual ork::EFileErrCode _doSeekFromStart( ork::File &rFile, size_t iTo );
	virtual ork::EFileErrCode _doSeekFromCurrent( ork::File &rFile, size_t iOffset );
	virtual ork::EFileErrCode _doGetLength( ork::File &rFile, size_t &riLength );

	virtual ork::EFileErrCode write( ork::File &rFile, const void *pFrom, size_t iSize );
	virtual ork::EFileErrCode getCurrentDirectory( file::Path::NameType& directory );
	virtual ork::EFileErrCode setCurrentDirectory( const file::Path::NameType& directory );

	virtual bool doesFileExist( const file::Path& filespec );
	virtual bool doesDirectoryExist( const file::Path& filespec );
	virtual bool isFileWritable( const file::Path& filespec );

  orkmap< ork::File *, ork::FileH > _fileHandleMap;
};

///////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////


