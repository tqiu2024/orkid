////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>
#include <ork/asset/FileAssetNamer.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace asset {
///////////////////////////////////////////////////////////////////////////////

FileAssetNamer::FileAssetNamer(ConstString prefix)
	: mPrefix(prefix)
{
}

///////////////////////////////////////////////////////////////////////////////

bool FileAssetNamer::Canonicalize(MutableString result, PieceString input)
{
	// TODO: Path canonicalization here.

	result = "";

	if(input.find("://") == PieceString::npos)
	{
		result = "data://";
		result += PieceString(mPrefix);
	}

	result += input;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

} }
