////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include "dspblocks.h"

namespace ork::audio::singularity {

struct RingModData final : public DspBlockData {
  RingModData(std::string name);
  dspblk_ptr_t createInstance() const override;
};
struct RingModSumAData final : public DspBlockData {
  RingModSumAData(std::string name);
  dspblk_ptr_t createInstance() const override;
};

struct RingMod : public DspBlock {
  using dataclass_t = RingModData;
  RingMod(const DspBlockData* dbd);
  void compute(DspBuffer& dspbuf) final;
};
struct RingModSumA : public DspBlock {
  using dataclass_t = RingModSumAData;
  RingModSumA(const DspBlockData* dbd);
  void compute(DspBuffer& dspbuf) final;
};
} // namespace ork::audio::singularity
