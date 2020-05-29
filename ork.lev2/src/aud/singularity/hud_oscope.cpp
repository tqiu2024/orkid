#include <ork/lev2/aud/singularity/hud.h>
#include <ork/lev2/aud/singularity/dspblocks.h>
#include <ork/lev2/gfx/pickbuffer.h>
#include <ork/util/triple_buffer.h>

using namespace ork;
using namespace ork::lev2;

///////////////////////////////////////////////////////////////////////////////
namespace ork::audio::singularity {
struct ScopeSurf final : public ui::Surface {
  ScopeSurf();
  void DoRePaintSurface(ui::drawevent_constptr_t drwev) override;
  void DoInit(lev2::Context* pt) override;
  ui::HandlerResult DoOnUiEvent(ui::event_constptr_t EV) override;
  ork::lev2::CTXBASE* _ctxbase = nullptr;
  concurrent_triple_buffer<ScopeBuffer> _scopebuffers;
};
///////////////////////////////////////////////////////////////////////////////
signalscope_ptr_t create_oscilloscope(hudvp_ptr_t vp) {
  auto hudpanel        = std::make_shared<HudPanel>();
  auto scopesurf       = std::make_shared<ScopeSurf>();
  hudpanel->_uipanel   = std::make_shared<ui::Panel>("scope", 0, 0, 32, 32);
  hudpanel->_uisurface = scopesurf;
  hudpanel->_uipanel->setChild(hudpanel->_uisurface);
  hudpanel->_uipanel->snap();
  auto instrument              = std::make_shared<SignalScope>();
  instrument->_hudpanel        = hudpanel;
  instrument->_sink            = std::make_shared<ScopeSink>();
  instrument->_sink->_onupdate = [scopesurf](const ScopeSource& src) { //
    auto dest_scopebuf = scopesurf->_scopebuffers.begin_push();
    memcpy(
        dest_scopebuf->_samples, //
        src._scopebuffer._samples,
        koscopelength * sizeof(float));
    scopesurf->_scopebuffers.end_push(dest_scopebuf);
    scopesurf->SetDirty();
  };
  vp->addChild(hudpanel->_uipanel);
  vp->_hudpanels.insert(hudpanel);
  return instrument;
}
///////////////////////////////////////////////////////////////////////////////
ScopeSurf::ScopeSurf() //
    : ui::Surface("Scope", 0, 0, 32, 32, fvec3(0.2, 0.1, 0.2), 1.0) {
}
///////////////////////////////////////////////////////////////////////////////
void ScopeSurf::DoRePaintSurface(ui::drawevent_constptr_t drwev) {
  auto context = drwev->GetTarget();
  auto fbi     = context->FBI();
  auto syn     = synth::instance();
  auto vp      = syn->_hudvp;
  double time  = syn->_timeaccum;

  auto scopebuf = _scopebuffers.begin_pull();
  if (nullptr == scopebuf)
    return;
  const float* _samples = scopebuf->_samples;

  mRtGroup->_clearColor = _clearColor;
  fbi->rtGroupClear(mRtGroup);

  ///////////////////////////////////////////////

  hudlines_t lines;

  int inumframes = syn->_oswidth;

  const float OSC_X1 = 0;
  const float OSC_Y1 = 0;
  const float OSC_W  = miW;
  const float OSC_H  = miH;
  const float OSC_X2 = miW;
  const float OSC_Y2 = miH;
  const float OSC_HH = OSC_H * 0.5;
  const float OSC_CY = OSC_Y1 + OSC_HH;

  // DrawBorder(context, OSC_X1, OSC_Y1, OSC_X2, OSC_Y2);

  int ycursor = OSC_Y1;

  int window_width = syn->_oswidth;

  double width       = double(window_width) / double(48000.0);
  double frq         = 1.0 / width;
  float triggerlevel = syn->_ostriglev;

  drawtext(
      this,
      context, //
      FormatString("-= width: %g msec", width * 1000.0),
      OSC_X1,
      ycursor,
      fontscale,
      1,
      1,
      0);
  ycursor += hud_lineheight();
  drawtext(
      this,
      context, //
      FormatString("   width-frq: %g hz", frq),
      OSC_X1,
      ycursor,
      fontscale,
      1,
      1,
      0);
  ycursor += hud_lineheight();
  drawtext(
      this,
      context, //
      FormatString("[] triglev: %0.4f", triggerlevel),
      OSC_X1,
      ycursor,
      fontscale,
      1,
      1,
      0);
  ycursor += hud_lineheight();
  drawtext(
      this,
      context, //
      FormatString("\\  trigdir: %s", syn->_ostrigdir ? "up" : "down"),
      OSC_X1,
      ycursor,
      fontscale,
      1,
      1,
      0);

  /////////////////////////////////////////////
  // oscilloscope centerline
  /////////////////////////////////////////////

  lines.push_back(HudLine{
      fvec2(OSC_X1, OSC_CY), //
      fvec2(OSC_X2, OSC_CY),
      fvec3(1, 1, 1)});

  /////////////////////////////////////////////
  // oscilloscope trace
  /////////////////////////////////////////////

  float x1 = OSC_X1;
  float y1 = OSC_CY + _samples[0] * -OSC_HH;
  float x2, y2;

  const int koscfr = window_width;

  //////////////////////////////////////////////
  // find set of all level trigger crossings
  //   matching the selected direction
  //////////////////////////////////////////////

  std::set<int> crossings;
  {
    float ly = _samples[0];
    for (int i = 0; i < koscopelength; i++) {
      float y = _samples[i];
      if (syn->_ostrigdir) {
        if (ly > triggerlevel and y <= triggerlevel)
          crossings.insert(i);
      } else {
        if (ly < triggerlevel and y >= triggerlevel) {
          crossings.insert(i);
        }
      }
      ly = y;
    }
  }

  //////////////////////////////////////////////
  // compute crossing to crossing widths
  //////////////////////////////////////////////
  struct TriggerItem {
    int _crossing, _delta;
  };

  std::map<int, int> crosdeltascount;
  std::map<float, TriggerItem> weightedcrossings;

  static int lastdelta = 0;
  int maxcrossingcount = 0;
  int crossing         = 0;

  //////////////////////////////////////////////////
  // get counts
  //////////////////////////////////////////////////

  for (std::set<int>::iterator it = crossings.begin(); it != crossings.end(); it++) {
    auto itnext = it;
    itnext++;
    if (itnext != crossings.end()) {
      int i     = *it;
      int inext = *itnext;
      int delta = inext - i;
      int count = crosdeltascount[delta]++;
    }
  }

  //////////////////////////////////////////////////
  // weighting heuristic
  //  in order to stabilize trigger utilizing
  //   global crossing data
  //////////////////////////////////////////////////

  for (std::set<int>::iterator it = crossings.begin(); it != crossings.end(); it++) {
    auto itnext = it;
    itnext++;
    if (itnext != crossings.end()) {
      int i     = *it;
      int inext = *itnext;
      int delta = inext - i;
      int count = crosdeltascount[delta];
      /////////////////////////////////////////////////////////
      // start weight out with number of repeats
      /////////////////////////////////////////////////////////
      float weight = count;
      /////////////////////////////////////////////////////////
      // mildly prefer earlier candidates
      //  they represent older data, but there is more history
      //  to work with, and the write head is out of the way.
      /////////////////////////////////////////////////////////
      weight *= 1.0 - (0.5 * float(i) / float(koscopelength));
      if (lastdelta != 0) {
        int dist2last = abs(lastdelta - delta);
        /////////////////////////////////////////////////////////
        // prefer candidates most closely matching the last delta
        /////////////////////////////////////////////////////////
        if (dist2last == 0)
          weight *= 2.101f;
        else
          weight *= 1.0f / float(dist2last);
      }
      weightedcrossings[weight] = TriggerItem{i, delta};
    }
  }

  //////////////////////////////////////////////
  // get crossing with largest weight
  //////////////////////////////////////////////
  auto itcrossing = weightedcrossings.rbegin();
  if (itcrossing != weightedcrossings.rend()) {
    float weight      = itcrossing->first;
    TriggerItem& item = itcrossing->second;
    crossing          = item._crossing;
    lastdelta         = item._delta;
  }

  //////////////////////////////////////////////

  for (int i = 0; i < window_width; i++) {
    int j   = (i + crossing) & koscopelengthmask;
    float x = OSC_W * float(i) / float(window_width);
    float y = _samples[j] * -OSC_HH;
    x2      = OSC_X1 + x;
    y2      = OSC_CY + y;
    if (i < koscfr)
      lines.push_back(HudLine{
          fvec2(x1, y1), //
          fvec2(x2, y2),
          fvec3(.3, 1, .3)});
    x1 = x2;
    y1 = y2;
  }

  _scopebuffers.end_pull(scopebuf);

  drawHudLines(this, context, lines);
}
///////////////////////////////////////////////////////////////////////////////
void ScopeSurf::DoInit(lev2::Context* pt) {
  _pickbuffer = new lev2::PickBuffer(this, pt, miW, miH);
  _ctxbase    = pt->GetCtxBase();
}
///////////////////////////////////////////////////////////////////////////////
ui::HandlerResult ScopeSurf::DoOnUiEvent(ui::event_constptr_t EV) {
  ui::HandlerResult ret(this);
  return ret;
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::audio::singularity
///////////////////////////////////////////////////////////////////////////////
