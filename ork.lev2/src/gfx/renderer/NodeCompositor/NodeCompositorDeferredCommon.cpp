////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <algorithm>
#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/kernel/mutex.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/application/application.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/material_freestyle.inl>

#include "NodeCompositorDeferred.h"

namespace ork::lev2::deferrednode {
///////////////////////////////////////////////////////////////////////////////

DeferredContext::DeferredContext(DeferredCompositingNode* node) : _node(node) {

  _layername = "All"_pool;

  #if defined(ENABLE_COMPUTE_SHADERS)
  const int knumlights = KMAXLIGHTS;
#else
  const int knumlights = KMAXLIGHTS;
#endif

  assert(knumlights <= KMAXLIGHTS);

  for (int i = 0; i < knumlights; i++) {

    auto p = new PointLight;
    p->next();
    p->_color.x = float(rand() & 0xff) / 256.0;
    p->_color.y = float(rand() & 0xff) / 256.0;
    p->_color.z = float(rand() & 0xff) / 256.0;
    p->_radius  = 16 + float(rand() & 0xff) / 2.0;
    _pointlights.push_back(p);
  }
}

///////////////////////////////////////////////////////////////////////////////

DeferredContext::~DeferredContext() {}

///////////////////////////////////////////////////////////////////////////////
// deferred layout
// rt0/GL_RGBA8    (32,32)  - albedo,ao (primary color)
// rt1/GL_RGB10_A2 (32,64)  - normal,model
// rt2/GL_RGBA8    (32,96)  - mtl,ruf,aux1,aux2
// rt3/GL_R32F     (32,128) - depth
///////////////////////////////////////

void DeferredContext::gpuInit(GfxTarget* target) {
  target->debugPushGroup("Deferred::rendeinitr");
  auto FXI = target->FXI();
  if (nullptr == _rtgGbuffer) {
    for (int i = 0; i < KMAXTILECOUNT; i++) {
      _lighttiles.push_back(pllist_t());
    }
    //////////////////////////////////////////////////////////////
    _lightingmtl.gpuInit(target, "orkshader://deferred");
    _tekBaseLighting          = _lightingmtl.technique("baselight");
    _tekPointLighting         = _lightingmtl.technique("pointlight");
    _tekBaseLightingStereo    = _lightingmtl.technique("baselight_stereo");
    _tekPointLightingStereo   = _lightingmtl.technique("pointlight_stereo");
    _tekDownsampleDepthMinMax = _lightingmtl.technique("downsampledepthminmax");
    //////////////////////////////////////////////////////////////
    // init lightblock
    //////////////////////////////////////////////////////////////
    _lightbuffer = target->FXI()->createParamBuffer(65536);
    _lightblock  = _lightingmtl.paramBlock("ub_light");
    auto mapped  = FXI->mapParamBuffer(_lightbuffer);
    size_t base  = 0;
    for (int i = 0; i < KMAXLIGHTSPERCHUNK; i++)
      mapped->ref<fvec3>(base + i * sizeof(fvec4)) = fvec3(0, 0, 0);
    base += KMAXLIGHTSPERCHUNK * sizeof(fvec4);
    for (int i = 0; i < KMAXLIGHTSPERCHUNK; i++)
      mapped->ref<fvec4>(base + i * sizeof(fvec4)) = fvec4();
    mapped->unmap();
    //////////////////////////////////////////////////////////////
    _parMatIVPArray    = _lightingmtl.param("IVPArray");
    _parMatVArray      = _lightingmtl.param("VArray");
    _parMatPArray      = _lightingmtl.param("PArray");
    _parMapGBufAlbAo   = _lightingmtl.param("MapAlbedoAo");
    _parMapGBufNrmL    = _lightingmtl.param("MapNormalL");
    _parMapDepth       = _lightingmtl.param("MapDepth");
    _parMapDepthMinMax = _lightingmtl.param("MapDepthMinMax");
    _parInvViewSize    = _lightingmtl.param("InvViewportSize");
    _parTime           = _lightingmtl.param("Time");
    _parNumLights      = _lightingmtl.param("NumLights");
    _parTileDim        = _lightingmtl.param("TileDim");
    _parNearFar        = _lightingmtl.param("NearFar");
    _parZndc2eye       = _lightingmtl.param("Zndc2eye");
    //////////////////////////////////////////////////////////////
    _rtgGbuffer      = new RtGroup(target, 8, 8, 1);
    auto buf0        = new RtBuffer(_rtgGbuffer, lev2::ETGTTYPE_MRT0, lev2::EBUFFMT_RGBA8, 8, 8);
    auto buf1        = new RtBuffer(_rtgGbuffer, lev2::ETGTTYPE_MRT1, lev2::EBUFFMT_RGB10A2, 8, 8);
    buf0->_debugName = "DeferredRtAlbAo";
    buf1->_debugName = "DeferredRtNormalDist";
    _rtgGbuffer->SetMrt(0, buf0);
    _rtgGbuffer->SetMrt(1, buf1);
    _gbuffRT = new RtGroupRenderTarget(_rtgGbuffer);
    //////////////////////////////////////////////////////////////
    _rtgMinMaxD      = new RtGroup(target, 8, 8, 1);
    auto bufD        = new RtBuffer(_rtgMinMaxD, lev2::ETGTTYPE_MRT0, lev2::EBUFFMT_R32UI, 8, 8);
    bufD->_debugName = "DeferredMinMaxD";
    _rtgMinMaxD->SetMrt(0, bufD);
    _minmaxRT = new RtGroupRenderTarget(_rtgMinMaxD);
    //////////////////////////////////////////////////////////////
    _rtgLaccum        = new RtGroup(target, 8, 8, 1);
    auto bufLA        = new RtBuffer(_rtgLaccum, lev2::ETGTTYPE_MRT0, lev2::EBUFFMT_RGBA16F, 8, 8);
    bufLA->_debugName = "DeferredLightAccum";
    _rtgLaccum->SetMrt(0, bufLA);
    _accumRT = new RtGroupRenderTarget(_rtgLaccum);
    //////////////////////////////////////////////////////////////
    
  }
  target->debugPopGroup();
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::renderGbuffer(CompositorDrawData& drawdata, const ViewData& VD) {
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = RCFD.GetTarget();
  auto FBI                     = targ->FBI();
  auto& ddprops                = drawdata._properties;
  auto irenderer = ddprops["irenderer"_crcu].Get<lev2::IRenderer*>();
  SRect tgt_rect(0, 0, targ->GetW(), targ->GetH());
  ///////////////////////////////////////////////////////////////////////////
  FBI->PushRtGroup(_rtgGbuffer);
  FBI->SetAutoClear(false); // explicit clear
  targ->BeginFrame();
  ///////////////////////////////////////////////////////////////////////////
  const auto TOPCPD = CIMPL->topCPD();
  auto CPD = TOPCPD;
  CPD._clearColor      = _node->_clearColor;
  CPD.mpLayerName      = &_layername;
  CPD._irendertarget   = _gbuffRT;
  CPD.SetDstRect(tgt_rect);
  CPD._passID       = "defgbuffer1"_crcu;
  ///////////////////////////////////////////////////////////////////////////
  auto DB              = RCFD.GetDB();
  if (DB) {
    ///////////////////////////////////////////////////////////////////////////
    // DrawableBuffer -> RenderQueue enqueue
    ///////////////////////////////////////////////////////////////////////////
    for (const PoolString& layer_name : CPD.getLayerNames()) {
      targ->debugMarker(FormatString("Deferred::renderEnqueuedScene::layer<%s>", layer_name.c_str()));
      DB->enqueueLayerToRenderQueue(layer_name, irenderer);
    }
    /////////////////////////////////////////////////
    auto MTXI = targ->MTXI();
    CIMPL->pushCPD(CPD); // drawenq
    targ->debugPushGroup("toolvp::DrawEnqRenderables");
    FBI->Clear(_node->_clearColor, 1.0f);
    irenderer->drawEnqueuedRenderables();
    framerenderer.renderMisc();
    targ->debugPopGroup(); // drawenq
    CIMPL->popCPD();
  }
  /////////////////////////////////////////////////////////////////////////////////////////
  targ->EndFrame();
  FBI->PopRtGroup();

}

///////////////////////////////////////////////////////////////////////////////

const uint32_t* DeferredContext::captureMinMax(CompositorDrawData& drawdata,const ViewData& VD) {
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = RCFD.GetTarget();
  auto FBI                     = targ->FBI();
    auto this_buf                = FBI->GetThisBuffer();
  auto vprect   = SRect(0, 0, _minmaxW, _minmaxH);
  auto quadrect = SRect(0, 0, _minmaxW, _minmaxH);
  auto tgt_rect = SRect(0, 0, targ->GetW(), targ->GetH());

  const auto TOPCPD = CIMPL->topCPD();
  auto CPD = TOPCPD;
  CPD._clearColor      = _node->_clearColor;
  CPD.mpLayerName      = &_layername;
  CPD.SetDstRect(tgt_rect);
  CPD._passID       = "defgbuffer1"_crcu;
  CPD.SetDstRect(vprect);
  CPD._irendertarget        = _minmaxRT;
  CPD._cameraMatrices       = nullptr;
  CPD._stereoCameraMatrices = nullptr;
  CPD._stereo1pass          = false;
  CIMPL->pushCPD(CPD); // MinMaxD
  targ->debugPushGroup("Deferred::minmaxd");
  {
    FBI->SetAutoClear(true);
    FBI->PushRtGroup(_rtgMinMaxD);
    targ->BeginFrame();
    _lightingmtl.bindTechnique(_tekDownsampleDepthMinMax);
    _lightingmtl.begin(RCFD);
    _lightingmtl.bindParamInt(_parTileDim, KTILEDIMXY);
    _lightingmtl.bindParamCTex(_parMapDepth, _rtgGbuffer->_depthTexture);
    _lightingmtl.bindParamVec2(_parNearFar, fvec2(KNEAR, KFAR));
    _lightingmtl.bindParamVec2(_parZndc2eye, VD._zndc2eye);
    _lightingmtl.bindParamVec2(_parInvViewSize, fvec2(1.0 / float(_width), 1.0f / float(_height)));
    _lightingmtl.mRasterState.SetBlending(EBLENDING_OFF);
    _lightingmtl.mRasterState.SetDepthTest(EDEPTHTEST_OFF);
    _lightingmtl.mRasterState.SetCullTest(ECULLTEST_PASS_BACK);
    _lightingmtl.commit();
    this_buf->Render2dQuadEML(fvec4(-1, -1, 2, 2), fvec4(0, 0, 1, 1), fvec4(0, 0, 0, 0));
    _lightingmtl.end(RCFD);
    targ->EndFrame();
    FBI->PopRtGroup();
  }
  targ->debugPopGroup(); // MinMaxD
  CIMPL->popCPD();       // MinMaxD

  bool captureok = FBI->capture(*_rtgMinMaxD, 0, &_depthcapture);
  assert(captureok);
  auto minmaxdepthbase = (const uint32_t*)_depthcapture._data;
  return (const uint32_t*)_depthcapture._data;
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::renderUpdate(CompositorDrawData& drawdata) {
  auto& ddprops                = drawdata._properties;
  //////////////////////////////////////////////////////
  // Resize RenderTargets
  //////////////////////////////////////////////////////
  int newwidth  = ddprops["OutputWidth"_crcu].Get<int>();
  int newheight = ddprops["OutputHeight"_crcu].Get<int>();
  if (_rtgGbuffer->GetW() != newwidth or _rtgGbuffer->GetH() != newheight) {
    _width   = newwidth;
    _height  = newheight;
    _minmaxW = (newwidth + KTILEDIMXY - 1) / KTILEDIMXY;
    _minmaxH = (newheight + KTILEDIMXY - 1) / KTILEDIMXY;
    _rtgGbuffer->Resize(newwidth, newheight);
    _rtgLaccum->Resize(newwidth, newheight);
    _rtgMinMaxD->Resize(_minmaxW, _minmaxH);
  }

}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::update() {
      //////////////////////////////////////////////////////////////////
      // clear lighttiles
      //////////////////////////////////////////////////////////////////
      int numtiles = _minmaxW * _minmaxH;
      for (int i = 0; i < numtiles; i++)
        _lighttiles[i].clear();
      _activetiles.clear();
      //////////////////////////////////////////////////////////////////
      // update pointlights
      //////////////////////////////////////////////////////////////////
      for (auto pl : _pointlights) {
        if (pl->_counter < 1) {
          pl->next();
        } else {
          fvec3 delta = pl->_dst - pl->_pos;
          pl->_pos += delta.Normal() * 2.0f;
          pl->_counter--;
        }
      }

}

///////////////////////////////////////////////////////////////////////////////

ViewData DeferredContext::computeViewData(CompositorDrawData& drawdata) {
  auto CIMPL                   = drawdata._cimpl;
  const auto TOPCPD = CIMPL->topCPD();
  ViewData VD;
  VD._isStereo = TOPCPD.isStereoOnePass();
  VD._camposmono = TOPCPD.monoCamPos(fmtx4());
  if (VD._isStereo) {
    auto L = TOPCPD._stereoCameraMatrices->_left;
    auto R = TOPCPD._stereoCameraMatrices->_right;
    auto M = TOPCPD._stereoCameraMatrices->_mono;
    VD.VL     = L->_vmatrix;
    VD.VR     = R->_vmatrix;
    VD.VM     = M->_vmatrix;
    VD.PL     = L->_pmatrix;
    VD.PR     = R->_pmatrix;
    VD.PM     = M->_pmatrix;
    VD.VPL    = VD.VL * VD.PL;
    VD.VPR    = VD.VR * VD.PR;
    VD.VPM    = VD.VM * VD.PM;
  } else {
    auto M = TOPCPD._cameraMatrices;
    VD.VM     = M->_vmatrix;
    VD.PM     = M->_pmatrix;
    VD.VL     = VD.VM;
    VD.VR     = VD.VM;
    VD.PL     = VD.PM;
    VD.PR     = VD.PM;
    VD.VPM    = VD.VM * VD.PM;
    VD.VPL    = VD.VPM;
    VD.VPR    = VD.VPM;
  }
  VD.IVPM.inverseOf(VD.VPM);
  VD.IVPL.inverseOf(VD.VPL);
  VD.IVPR.inverseOf(VD.VPR);
  VD._v[0]   = VD.VL;
  VD._v[1]   = VD.VR;
  VD._p[0]   = VD.PL; //_p[0].Transpose();
  VD._p[1]   = VD.PR; //_p[1].Transpose();
  VD._ivp[0] = VD.IVPL;
  VD._ivp[1] = VD.IVPR;
  fmtx4 IVL;
  IVL.inverseOf(VD.VL);
  VD._camposmono = IVL.GetColumn(3).xyz();

  VD._zndc2eye = fvec2(VD._p[0].GetElemXY(3, 2), VD._p[0].GetElemXY(2, 2));
  
  return VD;
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::renderBaseLighting( CompositorDrawData& drawdata,
                                          const ViewData& VD ) {
    /////////////////////////////////////////////////////////////////
    FrameRenderer& framerenderer = drawdata.mFrameRenderer;
    RenderContextFrameData& RCFD = framerenderer.framedata();
    auto CIMPL                   = drawdata._cimpl;
    auto targ                    = RCFD.GetTarget();
    auto FBI                     = targ->FBI();
    auto this_buf                = FBI->GetThisBuffer();
    auto RSI                     = targ->RSI();
    const auto TOPCPD = CIMPL->topCPD();
    _accumCPD = TOPCPD;
    /////////////////////////////////////////////////////////////////
    auto vprect   = SRect(0, 0, _width, _height);
    auto quadrect = SRect(0, 0, _width, _height);
    _accumCPD.SetDstRect(vprect);
    _accumCPD._irendertarget        = _accumRT;
    _accumCPD._cameraMatrices       = nullptr;
    _accumCPD._stereoCameraMatrices = nullptr;
    _accumCPD._stereo1pass          = false;
    CIMPL->pushCPD(_accumCPD); // base lighting
      FBI->SetAutoClear(false);
      FBI->PushRtGroup(_rtgLaccum);
      FBI->SetAutoClear(true);
      targ->BeginFrame();
      FBI->Clear(fvec4(0.1, 0.2, 0.3, 1), 1.0f);
      //////////////////////////////////////////////////////////////////
      // base lighting
      //////////////////////////////////////////////////////////////////
    targ->debugPushGroup("Deferred::BaseLighting");
      _lightingmtl.bindTechnique(VD._isStereo ? _tekBaseLightingStereo : _tekBaseLighting);
      _lightingmtl.mRasterState.SetBlending(EBLENDING_OFF);
      _lightingmtl.mRasterState.SetDepthTest(EDEPTHTEST_OFF);
      _lightingmtl.mRasterState.SetCullTest(ECULLTEST_PASS_BACK);
      _lightingmtl.begin(RCFD);
      //////////////////////////////////////////////////////
      _lightingmtl.bindParamMatrixArray(_parMatIVPArray, VD._ivp, 2);
      _lightingmtl.bindParamMatrixArray(_parMatVArray, VD._v, 2);
      _lightingmtl.bindParamMatrixArray(_parMatPArray, VD._p, 2);
      _lightingmtl.bindParamCTex(_parMapGBufAlbAo, _rtgGbuffer->GetMrt(0)->GetTexture());
      _lightingmtl.bindParamCTex(_parMapGBufNrmL, _rtgGbuffer->GetMrt(1)->GetTexture());
      _lightingmtl.bindParamCTex(_parMapDepth, _rtgGbuffer->_depthTexture);
      _lightingmtl.bindParamVec2(_parNearFar, fvec2(KNEAR, KFAR));
      _lightingmtl.bindParamVec2(_parInvViewSize, fvec2(1.0 / float(_width), 1.0f / float(_height)));
      _lightingmtl.commit();
      RSI->BindRasterState(_lightingmtl.mRasterState);
      this_buf->Render2dQuadEML(fvec4(-1, -1, 2, 2), fvec4(0, 0, 1, 1), fvec4(0, 0, 0, 0));
      _lightingmtl.end(RCFD);
    CIMPL->popCPD();       // base lighting
    targ->debugPopGroup(); // BaseLighting

}

///////////////////////////////////////////////////////////////////////////////
} //namespace ork { namespace lev2 {
