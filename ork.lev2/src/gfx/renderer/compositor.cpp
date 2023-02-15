////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/application/application.h>
#include <ork/reflect/properties/register.h>
#include <ork/reflect/properties/DirectTypedMap.hpp>
#include <ork/reflect/properties/DirectTyped.hpp>
#include <ork/reflect/enum_serializer.inl>
#include <ork/rtti/downcast.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/lev2/ui/event.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/lev2/imgui/imgui.h>
#include <ork/lev2/imgui/imgui_impl_glfw.h>
#include <ork/lev2/imgui/imgui_impl_opengl3.h>
#include <ork/lev2/imgui/ork_imgui_dockspace.inl>
#include <ork/lev2/imgui/ImGuizmo.h>
///////////////////////////////////////////////////////////////////////////////
ImplementReflectionX(ork::lev2::CompositingScene, "CompositingScene");
ImplementReflectionX(ork::lev2::CompositingSceneItem, "CompositingSceneItem");
INSTANTIATE_TRANSPARENT_RTTI(ork::lev2::CompositingTechnique, "CompositingTechnique");
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 {
///////////////////////////////////////////////////////////////////////////////

void CompositingTechnique::Describe() {
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CompositingScene::describeX(class_t* c) {
  ork::reflect::RegisterMapProperty("Items", &CompositingScene::_items);
  ork::reflect::annotatePropertyForEditor<CompositingScene>("Items", "editor.factorylistbase", "CompositingSceneItem");
}

///////////////////////////////////////////////////////////////////////////////

CompositingScene::CompositingScene() {
}

///////////////////////////////////////////////////////////////////////////////

void CompositingSceneItem::describeX(class_t* c) {
}

///////////////////////////////////////////////////////////////////////////////

CompositingSceneItem::CompositingSceneItem()
    : _technique(nullptr) {
}

///////////////////////////////////////////////////////////////////////////////

void CompositingMorphable::WriteMorphTarget(dataflow::MorphKey name, float flerpval) {
}

///////////////////////////////////////////////////////////////////////////////

void CompositingMorphable::RecallMorphTarget(dataflow::MorphKey name) {
}

///////////////////////////////////////////////////////////////////////////////

void CompositingMorphable::Morph1D(const dataflow::morph_event* pme) {
}

///////////////////////////////////////////////////////////////////////////////

void PickingCompositorTechnique::gpuInit(lev2::Context* pTARG, int w, int h) {
}
bool PickingCompositorTechnique::assemble(CompositorDrawData& drawdata) {
  OrkAssert(false);
  return true;
}
void PickingCompositorTechnique::composite(CompositorDrawData& drawdata) {
  OrkAssert(false);
}

///////////////////////////////////////////////////////////////////////////////
AcquiredRenderDrawBuffer::AcquiredRenderDrawBuffer(rcfd_ptr_t rcfd) {
  _RCFD = rcfd;
}
///////////////////////////////////////////////////////////////////////////////
StandardCompositorFrame::StandardCompositorFrame(uidrawevent_constptr_t drawEvent)
    : _drawEvent(drawEvent) {
  _dbufcontextSFRAME = std::make_shared<DrawBufContext>();
  _dbufcontextSFRAME->_name = "DBC.StandardCompositorFrame";
}
///////////////////////////////////////////////////////////////////////////////
void StandardCompositorFrame::pushEmptyUpdateDrawBuf(){
  auto DB = _dbufcontextSFRAME->acquireForWriteLocked();
  OrkAssert(DB);
  _dbufcontextSFRAME->releaseFromWriteLocked(DB);
}
///////////////////////////////////////////////////////////////////////////////
void StandardCompositorFrame::withAcquiredUpdateDrawBuffer(
    int debugcode,   //
    bool rendersync, //
    acqupdatebuffer_lambda_t l) {

  DrawableBuffer* DB = nullptr;

  while (nullptr == DB) {
    DB = _dbufcontextSFRAME->acquireForWriteLocked();

    if (DB) {
      DB->Reset();
      AcquiredUpdateDrawBuffer udb;
      udb._DB = DB;
      l(udb);
      if (rendersync)
        _updateEnqueueLockedAndReleaseFrame(rendersync, DB);
      else
        _updateEnqueueUnlockedAndReleaseFrame(rendersync, DB);
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void StandardCompositorFrame::_updateEnqueueLockedAndReleaseFrame(bool rendersync, DrawableBuffer* dbuf) {
  _dbufcontextSFRAME->releaseFromWriteLocked(dbuf);
}
///////////////////////////////////////////////////////////////////////////////
void StandardCompositorFrame::_updateEnqueueUnlockedAndReleaseFrame(bool rendersync, DrawableBuffer* dbuf) {
  _dbufcontextSFRAME->releaseFromWriteLocked(dbuf);
}
///////////////////////////////////////////////////////////////////////////////
const DrawableBuffer* StandardCompositorFrame::_tryAcquireDrawBuffer() {
  return _dbufcontextSFRAME->acquireForReadLocked();
}
///////////////////////////////////////////////////////////////////////////////
void StandardCompositorFrame::render() {
  const DrawableBuffer* DB = nullptr;

  while (nullptr == DB) {
    DB = _dbufcontextSFRAME->acquireForReadLocked();
    if (nullptr == DB) {
      ork::usleep(0);
    }
  }
  // printf( "sframe renderdb<%p>\n", DB );
  if (DB) {
    auto context = _drawEvent->GetTarget();
    if (nullptr == _RCFD) {
      _RCFD = std::make_shared<RenderContextFrameData>(context);
    }
    AcquiredRenderDrawBuffer acq(_RCFD);
    acq._DB = DB;

    /////////////////////////////////////////////

    acq._RCFD->_cimpl = this->compositor;
    acq._RCFD->setUserProperty("DB"_crc, lev2::rendervar_t(acq._DB));

    /////////////////////////////////////////////
    // copy in frame global user properties
    /////////////////////////////////////////////

    for (auto item : _userprops) {
      const auto& K = item.first;
      const auto& V = item.second;
      acq._RCFD->setUserProperty(K, V);
    }

    /////////////////////////////////////////////

    context->pushRenderContextFrameData(acq._RCFD.get());

    ///////////////////////////////////////
    // compositor setup
    ///////////////////////////////////////

    float TARGW  = context->mainSurfaceWidth();
    float TARGH  = context->mainSurfaceHeight();
    auto tgtrect = ViewportRect(0, 0, TARGW, TARGH);
    auto fbi     = context->FBI();
    fbi->SetClearColor(fvec4(0.5, 0.5, 0.5, 1));
    fbi->setViewport(tgtrect);
    fbi->setScissor(tgtrect);

    if (this->passdata) {
      this->passdata->_irendertarget = _rendertarget.get();
      this->passdata->SetDstRect(tgtrect);
      this->compositor->pushCPD(*this->passdata);

      /////////////////////////////////////////////

      if (this->onPreCompositorRender)
        this->onPreCompositorRender(acq);

      /////////////////////////////////////////////

      context->pushRenderContextFrameData(acq._RCFD.get());

      FrameRenderer framerenderer(*acq._RCFD, [&]() {});
      CompositorDrawData drawdata(framerenderer);
      drawdata._properties["primarycamindex"_crcu].set<int>(0);
      drawdata._properties["cullcamindex"_crcu].set<int>(0);
      drawdata._properties["irenderer"_crcu].set<lev2::IRenderer*>(this->renderer.get());
      drawdata._properties["simrunning"_crcu].set<bool>(true);
      drawdata._properties["DB"_crcu].set<const DrawableBuffer*>(acq._DB);
      drawdata._cimpl = this->compositor;
      this->compositor->assemble(drawdata);
      this->compositor->composite(drawdata);
      this->compositor->popCPD();
      context->popRenderContextFrameData();

      /////////////////////////////////////////////

      if (this->onPostCompositorRender)
        this->onPostCompositorRender(acq);

      /////////////////////////////////////////////

      if (this->onImguiRender) {

        /////////////////////////////////////

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        if (_use_imgui_docking) {
          static bool docking_enable = true;
          OrkidDockSpace(&docking_enable);
        }

        this->onImguiRender(acq);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      }
    }
    /////////////////////////////////////////////

    _dbufcontextSFRAME->releaseFromReadLocked(DB);
  }
}

void StandardCompositorFrame::attachDrawBufContext(dbufcontext_ptr_t dbc){
  _dbufcontextSFRAME = dbc;
}


///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2
