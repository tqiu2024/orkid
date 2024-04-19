////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/application/application.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/pch.h>
#include <ork/reflect/properties/registerX.inl>

#include <ork/lev2/gfx/renderer/NodeCompositor/unlit_node.h>

ImplementReflectionX(ork::lev2::compositor::UnlitNode, "UnlitNode");

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::compositor {
///////////////////////////////////////////////////////////////////////////////
void UnlitNode::describeX(class_t* c) {
  c->directProperty("ClearColor", &UnlitNode::_clearColor);
}
///////////////////////////////////////////////////////////////////////////////
namespace _unlitnode {
struct IMPL {
  ///////////////////////////////////////
  IMPL()
      : _camname("Camera") {
    _layername = "All";
  }
  ///////////////////////////////////////
  ~IMPL() {
  }
  ///////////////////////////////////////
  void init(lev2::Context* pTARG) {
    pTARG->debugPushGroup("Forward::rendeinitr");
    if (nullptr == _rtg) {
      _material.gpuInit(pTARG);
      _rtg             = new RtGroup(pTARG, 8, 8);
      auto buf1        = _rtg->createRenderTarget(lev2::EBufferFormat::RGBA32F);
      auto buf2        = _rtg->createRenderTarget(lev2::EBufferFormat::RGBA32F);
      buf1->_debugName = "ForwardRt0";
      buf2->_debugName = "ForwardRt1";
      _profile_timer.Start();
    }
    pTARG->debugPopGroup();
  }
  ///////////////////////////////////////
  void _render(UnlitNode* node, CompositorDrawData& drawdata) {
    // float t1 = _profile_timer.SecsSinceStart();
    auto framerenderer           = drawdata._frameRenderer;
    RenderContextFrameData& RCFD = framerenderer->framedata();
    auto targ                    = RCFD.GetTarget();
    auto CIMPL                   = drawdata._cimpl;
    auto FBI                     = targ->FBI();
    auto this_buf                = FBI->GetThisBuffer();
    auto RSI                     = targ->RSI();
    auto tgt_rect                = targ->mainSurfaceRectAtOrigin();
    auto& ddprops                = drawdata._properties;
    //////////////////////////////////////////////////////
    // Resize RenderTargets
    //////////////////////////////////////////////////////
    int newwidth  = ddprops["OutputWidth"_crcu].get<int>();
    int newheight = ddprops["OutputHeight"_crcu].get<int>();
    if (_rtg->width() != newwidth or _rtg->height() != newheight) {
      _rtg->Resize(newwidth, newheight);
    }
    // float t2 = _profile_timer.SecsSinceStart();
    //////////////////////////////////////////////////////
    auto irenderer = ddprops["irenderer"_crcu].get<lev2::IRenderer*>();
    //////////////////////////////////////////////////////
    targ->debugPushGroup("Forward::render");
    RtGroupRenderTarget rt(_rtg);
    {
      targ->FBI()->PushRtGroup(_rtg);
      targ->FBI()->SetAutoClear(true); // explicit clear
      targ->beginFrame();
      /////////////////////////////////////////////////////////////////////////////////////////
      auto DB  = RCFD.GetDB();
      auto CPD = CIMPL->topCPD();
      CPD.assignLayers(_layername);
      CPD._clearColor     = node->_clearColor;
      CPD._irendertarget  = &rt;
      CPD._cameraMatrices = ddprops["defcammtx"_crcu].get<const CameraMatrices*>();
      CPD.SetDstRect(tgt_rect);
      ///////////////////////////////////////////////////////////////////////////
      // float t3 = _profile_timer.SecsSinceStart();
      if (DB) {
        ///////////////////////////////////////////////////////////////////////////
        // DrawableBuffer -> RenderQueue enqueue
        ///////////////////////////////////////////////////////////////////////////
        for (const auto& layer_name : CPD.getLayerNames()) {
          targ->debugMarker(FormatString("Forward::renderEnqueuedScene::layer<%s>", layer_name.c_str()));
          DB->enqueueLayerToRenderQueue(layer_name, irenderer);
        }
        /////////////////////////////////////////////////
        RCFD._renderingmodel = node->_renderingmodel;
        auto MTXI            = targ->MTXI();
        CIMPL->pushCPD(CPD);
        targ->debugPushGroup("toolvp::DrawEnqRenderables");
        targ->FBI()->Clear(node->_clearColor, 1.0f);
        irenderer->drawEnqueuedRenderables();
        framerenderer->renderMisc();
        targ->debugPopGroup();
        CIMPL->popCPD();
      }
      irenderer->resetQueue();
      // float t4 = _profile_timer.SecsSinceStart();
      // printf( "unlitnode t4-t3 %g(mSec)\n", (t2-t1)*1000.0f);
      /////////////////////////////////////////////////////////////////////////////////////////
      targ->endFrame();
      targ->FBI()->PopRtGroup();
    }
    targ->debugPopGroup();
    // float t5 = _profile_timer.SecsSinceStart();

    // printf( "unlitnode t5-t1 %g(mSec)\n", (t5-t1)*1000.0f);
  }
  ///////////////////////////////////////
  std::string _camname, _layername;
  CompositingMaterial _material;
  RtGroup* _rtg = nullptr;
  fmtx4 _viewOffsetMatrix;
  Timer _profile_timer;
};
} // namespace _unlitnode

///////////////////////////////////////////////////////////////////////////////
UnlitNode::UnlitNode() {
  _impl           = std::make_shared<_unlitnode::IMPL>();
  _renderingmodel = RenderingModel("FORWARD_UNLIT"_crcu);
  _clearColor     = fvec4(0, 0, 0, 1);
}
///////////////////////////////////////////////////////////////////////////////
UnlitNode::~UnlitNode() {
}
///////////////////////////////////////////////////////////////////////////////
void UnlitNode::doGpuInit(lev2::Context* pTARG, int iW, int iH) {
  _impl.get<std::shared_ptr<_unlitnode::IMPL>>()->init(pTARG);
}
///////////////////////////////////////////////////////////////////////////////
void UnlitNode::DoRender(CompositorDrawData& drawdata) {
  auto impl = _impl.get<std::shared_ptr<_unlitnode::IMPL>>();
  impl->_render(this, drawdata);
}
///////////////////////////////////////////////////////////////////////////////
rtbuffer_ptr_t UnlitNode::GetOutput() const {
  return _impl.get<std::shared_ptr<_unlitnode::IMPL>>()->_rtg->GetMrt(0);
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::compositor
