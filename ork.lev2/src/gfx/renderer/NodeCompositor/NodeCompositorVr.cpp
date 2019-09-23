////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include "NodeCompositorVr.h"
#include <ork/application/application.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/vr/vr.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/camera/cameraman.h>

ImplementReflectionX(ork::lev2::VrCompositingNode, "VrCompositingNode");

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 {
///////////////////////////////////////////////////////////////////////////////
void VrCompositingNode::describeX(class_t*c) {
}
///////////////////////////////////////////////////////////////////////////////
struct VRIMPL {
  ///////////////////////////////////////
  VRIMPL(VrCompositingNode*node)
      : _vrnode(node)
      , _camname(AddPooledString("Camera"))
      , _layers(AddPooledString("All")){}
  ///////////////////////////////////////
  ~VRIMPL() {}
  ///////////////////////////////////////
  void gpuInit(lev2::GfxTarget* pTARG) {
    if( _doinit ){
      pTARG->debugPushGroup("VRIMPL::gpuInit");
      _width     = orkidvr::device()._width*2;
      _height     = orkidvr::device()._height;
      _blit2screenmtl.SetUserFx( "orkshader://solid", "texcolor" );
      _blit2screenmtl.Init(pTARG);

      _rtg            = new RtGroup(pTARG, _width, _height, 1);
      auto buf        = new RtBuffer(_rtg, lev2::ETGTTYPE_MRT0, lev2::EBUFFMT_RGBA32, _width, _height );
      buf->_debugName = "WtfVrRt";
      _rtg->SetMrt(0, buf);


      pTARG->debugPopGroup();



      _doinit = false;
    }
  }
  ///////////////////////////////////////
  typedef const std::map<int, orkidvr::ControllerState>& controllermap_t;
  void renderPoses(GfxTarget* targ, CameraData* camdat, controllermap_t controllers) {
    fmtx4 rx;
    fmtx4 ry;
    fmtx4 rz;
    rx.SetRotateX(-PI * 0.5);
    ry.SetRotateY(PI * 0.5);
    rz.SetRotateZ(PI * 0.5);

    for (auto item : controllers) {

      auto c = item.second;
      fmtx4 ivomatrix;
      ivomatrix.inverseOf(_viewOffsetMatrix);

      fmtx4 scalemtx;
      scalemtx.SetScale(c._button1down ? 0.05 : 0.025);

      fmtx4 controller_worldspace = (c._matrix * ivomatrix);

      fmtx4 mmtx = (scalemtx * rx * ry * rz * controller_worldspace);

      targ->MTXI()->PushMMatrix(mmtx);
      targ->MTXI()->PushVMatrix(camdat->GetVMatrix());
      targ->MTXI()->PushPMatrix(camdat->GetPMatrix());
      targ->PushModColor(fvec4::White());
      {
        if (c._button2down)
          ork::lev2::GfxPrimitives::GetRef().RenderBox(targ);
        else
          ork::lev2::GfxPrimitives::GetRef().RenderAxis(targ);
      }
      targ->PopModColor();
      targ->MTXI()->PopPMatrix();
      targ->MTXI()->PopVMatrix();
      targ->MTXI()->PopMMatrix();
    }
  }
  ///////////////////////////////////////
  void beginAssemble(CompositorDrawData& drawdata){
    auto& ddprops                = drawdata._properties;
    FrameRenderer& framerenderer       = drawdata.mFrameRenderer;
    RenderContextFrameData& RCFD = framerenderer.framedata();
    auto DB         = RCFD.GetDB();
    GfxTarget* targ                  = drawdata.target();

    bool simrunning = drawdata._properties["simrunning"_crcu].Get<bool>();
    bool use_vr = (orkidvr::device()._active and simrunning);
    /////////////////////////////////////////////////////////////////////////////
    // default camera selection (todo: hoist to cimpl so it can be shared across output nodes)
    /////////////////////////////////////////////////////////////////////////////
    int primarycamindex = ddprops["primarycamindex"_crcu].Get<int>();
    int cullcamindex    = ddprops["cullcamindex"_crcu].Get<int>();
   // auto CAMDAT     = _CPD.getCamera(RCFD, primarycamindex, cullcamindex);
    auto& CAMCCTX   = RCFD.GetCameraCalcCtx();
    ///////////////////////////////////////////////////////////////////////////
    //targ->debugMarker(FormatString("NodeVr::CAMDAT<%p>", CAMDAT));
    //if (CAMDAT and DB) {
      //_tempcamdat = *CAMDAT;
      ///////////////////////////////////////////////////////////////////////////
    //}
    /////////////////////////////////////////////////////////////////////////////
    // get VR camera
    /////////////////////////////////////////////////////////////////////////////

    if( use_vr ){
      auto vrcamprop = RCFD.getUserProperty("vrcam"_crc);
      fmtx4 rootmatrix;
      if( auto as_cam = vrcamprop.TryAs<const CameraData*>() ){
        targ->debugMarker("Vr::gotcamera");
        auto vrcam = as_cam.value();
        auto eye = vrcam->GetEye();
        auto tgt = vrcam->GetTarget();
        auto up  = vrcam->GetUp();
        rootmatrix.LookAt(eye, tgt, up);
      }
      else{
        targ->debugMarker("Vr::nocamera");
      }

      _viewOffsetMatrix = orkidvr::device()._outputViewOffsetMatrix;
    }

    /////////////////////////////////////////////////////////////////////////////

    RCFD.setLayerName("All");

    if( use_vr ){
      auto vrroot = RCFD.getUserProperty("vrroot"_crc);
      if( auto as_mtx = vrroot.TryAs<fmtx4>() ){
        orkidvr::gpuUpdate(as_mtx.value());
      }
      else{
        printf("vrroottype<%s>\n", vrroot.GetTypeName() );
      }
    }
    ///////////////////////////////////
    ///////////////////////////////////
    //float w = _rtg->GetW(); float h = _rtg->GetH();
    ///////////////////////////////////

    SRect tgt_rect(0, 0, _width, _height);

    _CPD.mbDrawSource = true;
    _CPD.mpFrameTek   = nullptr;
    _CPD.mpCameraName = nullptr;
    _CPD.mpLayerName  = nullptr; // default == "All"
    _CPD._clearColor  = fvec4(0.61,0,0, 1);

    //////////////////////////////////////////////////////
    // is stereo active
    //////////////////////////////////////////////////////

    if (use_vr) {
      auto& LCAM = orkidvr::device()._leftcamera;
      auto& RCAM = orkidvr::device()._rightcamera;
      auto& CONT = orkidvr::device()._controllers;
      LCAM.BindGfxTarget(targ);
      RCAM.BindGfxTarget(targ);
      LCAM.CalcCameraMatrices(CAMCCTX);
      RCFD.SetCameraData(&LCAM);
      RCFD.setStereoOnePass(true);
      RCFD._stereoCamera._left = &LCAM;
      RCFD._stereoCamera._right = &RCAM;
      RCFD._stereoCamera._mono = &LCAM; // todo - blend l&r
      RCFD.SetCameraData(RCFD._stereoCamera._mono);
      _CPD._impl.Set<const CameraData*>(&LCAM);
    } else {
      auto spncam =(CameraData*) DB->GetCameraData("spawncam"_pool);
      ddprops["selcamdat"_crcu].Set<const CameraData*>(spncam);
      auto l2cam = spncam->getEditorCamera();
      if (l2cam){
        spncam->BindGfxTarget(targ);
        l2cam->RenderUpdate();
        CAMCCTX.mfAspectRatio = float(targ->GetW())/float(targ->GetH());
        //CAMCCTX.mfAspectRatio = float(_width)/float(_height);
        spncam->CalcCameraMatrices(CAMCCTX);
        _tempcamdat = l2cam->mCameraData;
      }
      float w = targ->GetW(); float h = targ->GetH();
      RCFD.setStereoOnePass(false);
      RCFD.SetCameraData(&_tempcamdat);
      RCFD._stereoCamera._left = &_tempcamdat;
      RCFD._stereoCamera._right = &_tempcamdat;
      RCFD._stereoCamera._mono = &_tempcamdat; // todo - blend l&r
      _CPD._impl.Set<const CameraData*>(&_tempcamdat);
    }

    //////////////////////////////////////////////////////

    drawdata.mCompositingGroupStack.push(_CPD);
    targ->SetRenderContextFrameData(&RCFD);
    RCFD.SetDstRect(tgt_rect);
    RCFD.SetRenderingMode(RenderContextFrameData::ERENDMODE_STANDARD);
    drawdata._properties["OutputWidth"_crcu].Set<int>(_width);
    drawdata._properties["OutputHeight"_crcu].Set<int>(_height);
  }
  ///////////////////////////////////////
  void endAssemble( CompositorDrawData& drawdata){
    FrameRenderer& framerenderer       = drawdata.mFrameRenderer;
    RenderContextFrameData& RCFD = framerenderer.framedata();
    drawdata.target()->SetRenderContextFrameData(nullptr);
    drawdata.mCompositingGroupStack.pop();
    RCFD.setStereoOnePass(false);
  }
  ///////////////////////////////////////
  PoolString _camname, _layers;
  VrCompositingNode* _vrnode = nullptr;
  CompositingPassData _CPD;
  fmtx4 _viewOffsetMatrix;
  RtGroup* _rtg = nullptr;
  int _width = 0;
  int _height = 0;
  bool _doinit = true;
  CameraData _tempcamdat;
  ork::lev2::GfxMaterial3DSolid	_blit2screenmtl;

};
///////////////////////////////////////////////////////////////////////////////
VrCompositingNode::VrCompositingNode() { _impl = std::make_shared<VRIMPL>(this); }
///////////////////////////////////////////////////////////////////////////////
VrCompositingNode::~VrCompositingNode() {}
///////////////////////////////////////////////////////////////////////////////
void VrCompositingNode::gpuInit(lev2::GfxTarget* pTARG, int iW, int iH)
{ _impl.Get<std::shared_ptr<VRIMPL>>()->gpuInit(pTARG);
}
///////////////////////////////////////////////////////////////////////////////
void VrCompositingNode::beginAssemble(CompositorDrawData& drawdata){
  drawdata.target()->debugPushGroup("VrCompositingNode::beginAssemble");
  _impl.Get<std::shared_ptr<VRIMPL>>()->beginAssemble(drawdata);
  drawdata.target()->debugPopGroup();
}
void VrCompositingNode::endAssemble(CompositorDrawData& drawdata) {
  drawdata.target()->debugPushGroup("VrCompositingNode::endAssemble");
  _impl.Get<std::shared_ptr<VRIMPL>>()->endAssemble(drawdata);
  drawdata.target()->debugPopGroup();
}

void VrCompositingNode::composite(CompositorDrawData& drawdata){
  drawdata.target()->debugPushGroup("VrCompositingNode::composite");
  auto impl = _impl.Get<std::shared_ptr<VRIMPL>>();
  /////////////////////////////////////////////////////////////////////////////
  // VR compositor
  /////////////////////////////////////////////////////////////////////////////
  FrameRenderer& framerenderer       = drawdata.mFrameRenderer;
  RenderContextFrameData& framedata  = framerenderer.framedata();
  GfxTarget* targ                    = framedata.GetTarget();
  if( auto try_final = drawdata._properties["final_out"_crcu].TryAs<RtGroup*>() ){
    auto buffer = try_final.value()->GetMrt(0);
    if( buffer ){
      assert(buffer != nullptr);
      auto tex = buffer->GetTexture();
      if (tex) {
        drawdata.target()->debugPushGroup("VrCompositingNode::to_hmd");
        targ->FBI()->PushRtGroup(impl->_rtg);
        orkidvr::composite(targ, tex);
        targ->FBI()->PopRtGroup();
        drawdata.target()->debugPopGroup();
        /////////////////////////////////////////////////////////////////////////////
        // be nice and composite to main screen as well...
        /////////////////////////////////////////////////////////////////////////////
        drawdata.target()->debugPushGroup("VrCompositingNode::to_screen");
        auto this_buf = targ->FBI()->GetThisBuffer();
        auto& mtl = impl->_blit2screenmtl;
        int iw = targ->GetW();
        int ih = targ->GetH();
        SRect vprect(0,0,iw,ih);
        SRect quadrect(0,ih,iw,0);
        fvec4 color( 1.0f, 1.0f, 1.0f, 1.0f );
        mtl.SetAuxMatrix( fmtx4::Identity );
        mtl.SetTexture( tex );
        mtl.SetTexture2( nullptr );
        mtl.SetColorMode( GfxMaterial3DSolid::EMODE_USER );
        mtl.mRasterState.SetBlending( EBLENDING_OFF );
        this_buf->RenderMatOrthoQuad( vprect,
                                      quadrect,
                                      & mtl,
                                      0.0f, 0.0f, // u0 v0
                                      1.0f, 1.0f, // u1 v1
                                      nullptr, color );
        drawdata.target()->debugPopGroup();
      }
    }
  }
  drawdata.target()->debugPopGroup();
}
///////////////////////////////////////////////////////////////////////////////
}} // namespace ork::lev2
