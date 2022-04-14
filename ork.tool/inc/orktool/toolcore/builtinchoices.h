////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

namespace ork { namespace tool {
///////////////////////////////////////////////////////////////////////////

class ModelChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  ModelChoices();
};

///////////////////////////////////////////////////////////////////////////

class AnimChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  AnimChoices();
};

///////////////////////////////////////////////////////////////////////////

class AudioStreamChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  AudioStreamChoices();
};

///////////////////////////////////////////////////////////////////////////

class AudioBankChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  AudioBankChoices();
};

///////////////////////////////////////////////////////////////////////////

class TextureChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  TextureChoices();
};

///////////////////////////////////////////////////////////////////////////

class FxShaderChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  FxShaderChoices();
};

///////////////////////////////////////////////////////////////////////////

class ScriptChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  ScriptChoices();
};

///////////////////////////////////////////////////////////////////////////

class ChsmChoices : public ork::util::ChoiceList {
public:
  virtual void EnumerateChoices(bool bforcenocache = false);
  ChsmChoices();
};

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
}} // namespace ork::tool
///////////////////////////////////////////////////////////////////////////
