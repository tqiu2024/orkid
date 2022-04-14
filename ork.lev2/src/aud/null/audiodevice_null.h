////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/kernel/orkpool.h>

namespace ork::lev2 {

///////////////////////////////////////////////////////////////////////////////

class AudioDeviceNULL : public AudioDevice {
public:
  AudioDeviceNULL();

protected:
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::lev2
