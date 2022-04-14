////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "synthdata.h"

namespace ork::audio::singularity {

///////////////////////////////////////////////////////////////////////////////
struct KrzAlgData {
  int _algindex = -1;
  algdata_ptr_t _algdata;
};
///////////////////////////////////////////////////////////////////////////////

struct EnvCtrlData {
  bool _useNatEnv  = false; // kurzeril per-sample envelope
  float _atkAdjust = 1.0f;
  float _decAdjust = 1.0f;
  float _relAdjust = 1.0f;

  float _atkKeyTrack = 1.0f;
  float _atkVelTrack = 1.0f;
  float _decKeyTrack = 1.0f;
  float _decVelTrack = 1.0f;
  float _relKeyTrack = 1.0f;
  float _relVelTrack = 1.0f;
};

} // namespace ork::audio::singularity
