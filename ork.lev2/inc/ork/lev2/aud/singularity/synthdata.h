#pragma once

#include "krztypes.h"
#include "modulation.h"
#include <ork/kernel/svariant.h>
#include <ork/file/path.h>

namespace ork::audio::singularity {
file::Path basePath();

///////////////////////////////////////////////////////////////////////////////

struct ProgramData {
  lyrdata_ptr_t newLayer();
  lyrdata_ptr_t getLayer(int i) const {
    return _LayerDatas[i];
  }
  std::string _name;
  std::string _role;
  std::vector<lyrdata_ptr_t> _LayerDatas;
};

///////////////////////////////////////////////////////////////////////////////

struct programInst;

struct SynthData {
  SynthData();
  virtual ~SynthData() {
  }

  float seqTime(float dur);
  virtual const ProgramData* getProgram(int progID) const                     = 0;
  virtual const ProgramData* getProgramByName(const std::string& named) const = 0;

  programInst* _prog;
  float _synsr;
  float _seqCursor;
  std::string _staticBankName;
};

} // namespace ork::audio::singularity
