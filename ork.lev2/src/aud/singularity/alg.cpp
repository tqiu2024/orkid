#include <ork/lev2/aud/singularity/synth.h>
#include <assert.h>
#include <ork/lev2/aud/singularity/filters.h>
#include <ork/lev2/aud/singularity/alg_eq.h>
#include <ork/lev2/aud/singularity/alg_oscil.h>
#include <ork/lev2/aud/singularity/alg_nonlin.h>
#include <ork/lev2/aud/singularity/alg_filters.h>
#include <ork/lev2/aud/singularity/alg_amp.h>
#include <ork/lev2/aud/singularity/cz101.h>
#include <ork/lev2/aud/singularity/konoff.h>

namespace ork::audio::singularity {

dspblk_ptr_t createDspBlock(const DspBlockData& dbd);

extern synth* the_synth;

///////////////////////////////////////////////////////////////////////////////

Alg::Alg(const AlgData& algd)
    : _algConfig(algd._config)
    , _blockBuf(nullptr) {
  for (int i = 0; i < kmaxdspblocksperlayer; i++)
    _block[i] = nullptr;

  _blockBuf = new DspBuffer;
}

Alg::~Alg() {
  delete _blockBuf;
}

///////////////////////////////////////////////////////////////////////////////

dspblk_ptr_t Alg::lastBlock() const {
  return nullptr; //
  /*    DspBlock* r = nullptr;
      for( int i=0; i<kmaxdspblocksperlayer; i++ )
          if( _block[i] )
          {
              bool ena = the_synth->_fblockEnable[i];
              if( ena )
                  r = _block[i];
          }
      return r;*/ // fix _the_synth
}

///////////////////////////////////////////////////////////////////////////////

void Alg::keyOn(DspKeyOnInfo& koi) {
  auto l = koi._layer;
  assert(l != nullptr);

  const auto ld = l->_LayerData;

  for (int i = 0; i < kmaxdspblocksperlayer; i++) {
    const auto data = ld->_dspBlocks[i];
    if (data and data->_dspBlock.length()) {
      _block[i] = createDspBlock(*data);

      if (i == 0) // pitch block ?
      {
        _block[i]->_iomask._outputMask = 3;
      }
    } else
      _block[i] = nullptr;
  }

  doKeyOn(koi);
}

///////////////////////////////////////////////////////////////////////////////

void Alg::intoDspBuf(const outputBuffer& obuf, DspBuffer& dspbuf) {
  int inumframes = obuf._numframes;
  _blockBuf->resize(inumframes);
  float* lefbuf = obuf._leftBuffer;
  float* rhtbuf = obuf._rightBuffer;
  float* uprbuf = _blockBuf->channel(0);
  float* lwrbuf = _blockBuf->channel(1);
  memcpy(uprbuf, lefbuf, inumframes * 4);
  memcpy(lwrbuf, lefbuf, inumframes * 4);
}

///////////////////////////////////////////////////////////////////////////////

void Alg::intoOutBuf(outputBuffer& obuf, const DspBuffer& dspbuf, int inumo) {
  int inumframes = obuf._numframes;
  _blockBuf->resize(inumframes);
  float* lefbuf = obuf._leftBuffer;
  float* rhtbuf = obuf._rightBuffer;
  float* uprbuf = _blockBuf->channel(0);
  float* lwrbuf = _blockBuf->channel(1);
  memcpy(lefbuf, uprbuf, inumframes * 4);
  memcpy(rhtbuf, lwrbuf, inumframes * 4);
}

///////////////////////////////////////////////////////////////////////////////

void Alg::compute(synth& syn, outputBuffer& obuf) {
  intoDspBuf(obuf, *_blockBuf);

  bool touched = false;

  int inumoutputs = 1;

  for (int i = 0; i < kmaxdspblocksperlayer; i++) {
    auto b = _block[i];
    if (b) {
      bool ena = syn._fblockEnable[i];
      if (ena) {
        b->compute(*_blockBuf);
        inumoutputs = b->numOutputs();
        touched     = true;
      }
    }
  }

  intoOutBuf(obuf, *_blockBuf, inumoutputs);
}

///////////////////////////////////////////////////////////////////////////////

void Alg::doKeyOn(DspKeyOnInfo& koi) {
  auto l = koi._layer;
  assert(l != nullptr);

  koi._prv = nullptr;
  _layer   = l;

  auto procblock = [&koi](dspblk_ptr_t thisblock, layer* l) {
    if (thisblock) {
      koi._prv = thisblock;
      thisblock->keyOn(koi);
    }
  };

  const auto& iomasks = _algConfig._ioMasks;
  for (int i = 0; i < kmaxdspblocksperlayer; i++)
    if (_block[i]) {
      _block[i]->_iomask = iomasks[i];
      procblock(_block[i], l);
    }
}

///////////////////////////////////////////////////////////////////////////////

void Alg::keyOff() {
  for (int i = 0; i < kmaxdspblocksperlayer; i++) {
    auto b = _block[i];
    if (b)
      b->doKeyOff();
  }
}

///////////////////////////////////////////////////////////////////////////////

Alg* AlgData::createAlgInst() const {
  auto alg = new Alg(*this);
  return alg;
}

///////////////////////////////////////////////////////////////////////////////
struct NONE : public DspBlock {
  NONE(const DspBlockData& dbd)
      : DspBlock(dbd) {
    _iomask._inputMask  = 0;
    _iomask._outputMask = 0;
    _numParams          = 0;
  }
  void compute(DspBuffer& dspbuf) final {
  }
};

///////////////////////////////////////////////////////////////////////////////

dspblk_ptr_t createDspBlock(const DspBlockData& dbd) {
  dspblk_ptr_t rval;

  if (dbd._dspBlock == "NONE")
    rval = std::make_shared<NONE>(dbd);

  ////////////////////////
  // amp/mix
  ////////////////////////

  if (dbd._dspBlock == "XFADE")
    rval = std::make_shared<XFADE>(dbd);
  if (dbd._dspBlock == "x GAIN")
    rval = std::make_shared<XGAIN>(dbd);
  if (dbd._dspBlock == "GAIN")
    rval = std::make_shared<GAIN>(dbd);
  if (dbd._dspBlock == "AMP")
    rval = std::make_shared<AMP>(dbd);
  if (dbd._dspBlock == "+ AMP")
    rval = std::make_shared<PLUSAMP>(dbd);
  if (dbd._dspBlock == "x AMP")
    rval = std::make_shared<XAMP>(dbd);
  if (dbd._dspBlock == "PANNER")
    rval = std::make_shared<PANNER>(dbd);
  if (dbd._dspBlock == "AMP U   AMP L")
    rval = std::make_shared<AMPU_AMPL>(dbd);
  if (dbd._dspBlock == "! AMP")
    rval = std::make_shared<BANGAMP>(dbd);

  ////////////////////////
  // osc/gen
  ////////////////////////

  if (dbd._dspBlock == "SAMPLEPB")
    rval = std::make_shared<SAMPLEPB>(dbd);
  if (dbd._dspBlock == "SINE")
    rval = std::make_shared<SINE>(dbd);
  if (dbd._dspBlock == "LF SIN")
    rval = std::make_shared<SINE>(dbd);
  if (dbd._dspBlock == "SAW")
    rval = std::make_shared<SAW>(dbd);
  if (dbd._dspBlock == "SQUARE")
    rval = std::make_shared<SQUARE>(dbd);
  if (dbd._dspBlock == "SINE+")
    rval = std::make_shared<SINEPLUS>(dbd);
  if (dbd._dspBlock == "SAW+")
    rval = std::make_shared<SAWPLUS>(dbd);
  if (dbd._dspBlock == "SW+SHP")
    rval = std::make_shared<SWPLUSSHP>(dbd);
  if (dbd._dspBlock == "+ SHAPEMOD OSC")
    rval = std::make_shared<PLUSSHAPEMODOSC>(dbd);
  if (dbd._dspBlock == "SHAPE MOD OSC")
    rval = std::make_shared<SHAPEMODOSC>(dbd);

  if (dbd._dspBlock == "SYNC M")
    rval = std::make_shared<SYNCM>(dbd);
  if (dbd._dspBlock == "SYNC S")
    rval = std::make_shared<SYNCS>(dbd);
  if (dbd._dspBlock == "PWM")
    rval = std::make_shared<PWM>(dbd);

  if (dbd._dspBlock == "FM4")
    rval = std::make_shared<FM4>(dbd);
  if (dbd._dspBlock == "CZX")
    rval = std::make_shared<CZX>(dbd);

  if (dbd._dspBlock == "NOISE")
    rval = std::make_shared<NOISE>(dbd);

  ////////////////////////
  // EQ
  ////////////////////////

  if (dbd._dspBlock == "PARA BASS")
    rval = std::make_shared<PARABASS>(dbd);
  if (dbd._dspBlock == "PARA MID")
    rval = std::make_shared<PARAMID>(dbd);
  if (dbd._dspBlock == "PARA TREBLE")
    rval = std::make_shared<PARATREBLE>(dbd);
  if (dbd._dspBlock == "PARAMETRIC EQ")
    rval = std::make_shared<PARAMETRIC_EQ>(dbd);

  ////////////////////////
  // filter
  ////////////////////////

  if (dbd._dspBlock == "2POLE ALLPASS")
    rval = std::make_shared<TWOPOLE_ALLPASS>(dbd);
  if (dbd._dspBlock == "2POLE LOWPASS")
    rval = std::make_shared<TWOPOLE_LOWPASS>(dbd);

  if (dbd._dspBlock == "STEEP RESONANT BASS")
    rval = std::make_shared<STEEP_RESONANT_BASS>(dbd);
  if (dbd._dspBlock == "4POLE LOPASS W/SEP")
    rval = std::make_shared<FOURPOLE_LOPASS_W_SEP>(dbd);
  if (dbd._dspBlock == "4POLE HIPASS W/SEP")
    rval = std::make_shared<FOURPOLE_HIPASS_W_SEP>(dbd);
  if (dbd._dspBlock == "NOTCH FILTER")
    rval = std::make_shared<NOTCH_FILT>(dbd);
  if (dbd._dspBlock == "NOTCH2")
    rval = std::make_shared<NOTCH2>(dbd);
  if (dbd._dspBlock == "DOUBLE NOTCH W/SEP")
    rval = std::make_shared<DOUBLE_NOTCH_W_SEP>(dbd);
  if (dbd._dspBlock == "BANDPASS FILT")
    rval = std::make_shared<BANDPASS_FILT>(dbd);
  if (dbd._dspBlock == "BAND2")
    rval = std::make_shared<BAND2>(dbd);

  if (dbd._dspBlock == "LOPAS2")
    rval = std::make_shared<LOPAS2>(dbd);
  if (dbd._dspBlock == "LP2RES")
    rval = std::make_shared<LP2RES>(dbd);
  if (dbd._dspBlock == "LOPASS")
    rval = std::make_shared<LOPASS>(dbd);
  if (dbd._dspBlock == "LPCLIP")
    rval = std::make_shared<LPCLIP>(dbd);
  if (dbd._dspBlock == "LPGATE")
    rval = std::make_shared<LPGATE>(dbd);

  if (dbd._dspBlock == "HIPASS")
    rval = std::make_shared<HIPASS>(dbd);
  if (dbd._dspBlock == "ALPASS")
    rval = std::make_shared<ALPASS>(dbd);

  if (dbd._dspBlock == "HIFREQ STIMULATOR")
    rval = std::make_shared<HIFREQ_STIMULATOR>(dbd);

  ////////////////////////
  // nonlin
  ////////////////////////

  if (dbd._dspBlock == "SHAPER")
    rval = std::make_shared<SHAPER>(dbd);
  if (dbd._dspBlock == "2PARAM SHAPER")
    rval = std::make_shared<TWOPARAM_SHAPER>(dbd);
  if (dbd._dspBlock == "WRAP")
    rval = std::make_shared<WRAP>(dbd);
  if (dbd._dspBlock == "DIST")
    rval = std::make_shared<DIST>(dbd);

  return rval;
}

} // namespace ork::audio::singularity
