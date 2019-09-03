////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

namespace ork { namespace tool {
///////////////////////////////////////////////////////////////////////////

class ModelChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	ModelChoices();
};

///////////////////////////////////////////////////////////////////////////

class CAnimChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	CAnimChoices();
};

///////////////////////////////////////////////////////////////////////////

class AudioStreamChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	AudioStreamChoices();
};

///////////////////////////////////////////////////////////////////////////

class AudioBankChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	AudioBankChoices();
};

///////////////////////////////////////////////////////////////////////////

class TextureChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	TextureChoices();
};

///////////////////////////////////////////////////////////////////////////

class FxShaderChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	FxShaderChoices();
};

///////////////////////////////////////////////////////////////////////////

class ScriptChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	ScriptChoices();
};

///////////////////////////////////////////////////////////////////////////

class ChsmChoices : public ork::tool::ChoiceList
{
public:

	virtual void EnumerateChoices( bool bforcenocache=false );
	ChsmChoices();
};

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////
