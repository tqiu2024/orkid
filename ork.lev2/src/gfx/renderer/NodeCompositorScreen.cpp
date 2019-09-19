////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "NodeCompositorScreen.h"

#include <ork/application/application.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/pch.h>
#include <ork/reflect/RegisterProperty.h>

ImplementReflectionX(ork::lev2::ScreenOutputCompositingNode, "ScreenOutputCompositingNode");

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 {
///////////////////////////////////////////////////////////////////////////////
void ScreenOutputCompositingNode::describeX(class_t* c) {
  c->memberProperty("Layer",&ScreenOutputCompositingNode::_layername);
}
///////////////////////////////////////////////////////////////////////////
constexpr int NUMSAMPLES = 1;
struct ScreenOutputTechnique final : public FrameTechniqueBase {
  //////////////////////////////////////////////////////////////////////////////
  ScreenOutputTechnique(int w, int h)
      : FrameTechniqueBase(w, h)
      , _rtg(nullptr) {}
  //////////////////////////////////////////////////////////////////////////////
  void DoInit(GfxTarget* pTARG) final {
    if (nullptr == _rtg) {
      _rtg = new RtGroup(pTARG, miW, miH, NUMSAMPLES);
      auto lbuf = new RtBuffer(_rtg, lev2::ETGTTYPE_MRT0, lev2::EBUFFMT_RGBA32, miW, miH);
      _rtg->SetMrt(0, lbuf);
      _effect.PostInit(pTARG, "orkshader://framefx", "frameeffect_standard");
    }
  }
  //////////////////////////////////////////////////////////////////////////////
  void render(FrameRenderer& renderer,
              CompositorDrawData& drawdata,
              ScreenOutputCompositingNode& node ) {
    RenderContextFrameData& framedata = renderer.framedata();
    GfxTarget* pTARG                  = framedata.GetTarget();

    SRect tgt_rect(0, 0, miW, miH);

    _CPD.mbDrawSource = true;
    _CPD.mpFrameTek   = this;
    _CPD.mpCameraName = nullptr;
    _CPD.mpLayerName  = &node._layername;
    _CPD._clearColor  = fvec4(0.61, 0.61, 0.75, 1);
    //_CPD._impl.Set<const CameraData*>(lcam);

    //////////////////////////////////////////////////////
    pTARG->FBI()->SetAutoClear(false);
    // clear will occur via _CPD
    //////////////////////////////////////////////////////

    RtGroupRenderTarget rt(_rtg);
    drawdata.mCompositingGroupStack.push(_CPD);
    {
      pTARG->SetRenderContextFrameData(&framedata);
      framedata.SetDstRect(tgt_rect);
      framedata.PushRenderTarget(&rt);
      pTARG->FBI()->PushRtGroup(_rtg);
      pTARG->BeginFrame();
      framedata.SetRenderingMode(RenderContextFrameData::ERENDMODE_STANDARD);
      renderer.Render();
      pTARG->EndFrame();
      pTARG->FBI()->PopRtGroup();
      framedata.PopRenderTarget();
      pTARG->SetRenderContextFrameData(nullptr);
      drawdata.mCompositingGroupStack.pop();
    }

    framedata.setStereoOnePass(false);
  }

  RtGroup* _rtg;
  BuiltinFrameEffectMaterial _effect;
  CompositingPassData _CPD;
  fmtx4 _viewOffsetMatrix;
};

///////////////////////////////////////////////////////////////////////////////
struct IMPL {
  ///////////////////////////////////////
  IMPL()
      : _frametek(nullptr)
      , _camname(AddPooledString("Camera"))
      , _layers(AddPooledString("All")) {}
  ///////////////////////////////////////
  ~IMPL() {
    if (_frametek)
      delete _frametek;
  }
  ///////////////////////////////////////
  void init(lev2::GfxTarget* pTARG) {
    _material.Init(pTARG);
    _frametek = new ScreenOutputTechnique(pTARG->GetW(),pTARG->GetH());
    _frametek->Init(pTARG);
  }
  ///////////////////////////////////////
  void _myrender(ScreenOutputCompositingNode* node, FrameRenderer& renderer, CompositorDrawData& drawdata) {
    _frametek->render(renderer, drawdata,*node);
  }
  ///////////////////////////////////////
  PoolString _camname, _layers;
  CompositingMaterial _material;
  ScreenOutputTechnique* _frametek;
};
///////////////////////////////////////////////////////////////////////////////
ScreenOutputCompositingNode::ScreenOutputCompositingNode() { _impl = std::make_shared<IMPL>(); }
///////////////////////////////////////////////////////////////////////////////
ScreenOutputCompositingNode::~ScreenOutputCompositingNode() {}
///////////////////////////////////////////////////////////////////////////////
void ScreenOutputCompositingNode::DoInit(lev2::GfxTarget* pTARG, int iW, int iH) // virtual
{
  auto impl = _impl.Get<std::shared_ptr<IMPL>>();
  if (nullptr == impl->_frametek) {
    impl->init(pTARG);
  }
}
///////////////////////////////////////////////////////////////////////////////
void ScreenOutputCompositingNode::DoRender(CompositorDrawData& drawdata, CompositingImpl* cimpl) // virtual
{
  FrameRenderer& the_renderer       = drawdata.mFrameRenderer;
  RenderContextFrameData& framedata = the_renderer.framedata();
  auto targ                         = framedata.GetTarget();
  auto impl                         = _impl.Get<std::shared_ptr<IMPL>>();
  impl->_layers = _layername;
  if (impl->_frametek) {
    framedata.setLayerName(_layername.c_str());
    impl->_myrender(this,the_renderer, drawdata);
  }
}
///////////////////////////////////////////////////////////////////////////////
}} // namespace ork::lev2
