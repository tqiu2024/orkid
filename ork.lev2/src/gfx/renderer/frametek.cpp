////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/lev2/gfx/gfxctxdummy.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/lev2/gfx/gfxmaterial_ui.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/rtgroup.h>

#include <ork/application/application.h>
#include <ork/kernel/any.h>
#include <ork/kernel/orklut.hpp>
#include <ork/lev2/gfx/renderer/rendercontext.h>
#include <ork/lev2/gfx/renderer/renderable.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/shadman.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/ui/viewport.h>

template class ork::orklut<ork::PoolString, anyp>;

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////
IRenderTarget::IRenderTarget() {
}

///////////////////////////////////////////////////////////////////////////////

RtGroupRenderTarget::RtGroupRenderTarget(RtGroup* prtgroup)
    : _rtgroup(prtgroup) {
}
int RtGroupRenderTarget::width() {
  // printf( "RtGroup W<%d> H<%d>\n", _rtgroup->width(), _rtgroup->height() );
  return _rtgroup->width();
}
int RtGroupRenderTarget::height() {
  return _rtgroup->height();
}
void RtGroupRenderTarget::BeginFrame(Context* pTARG) {
}
void RtGroupRenderTarget::EndFrame(Context* pTARG) {
}

///////////////////////////////////////////////////////////////////////////////

UiViewportRenderTarget::UiViewportRenderTarget(ui::Viewport* pVP)
    : mpViewport(pVP) {
}
int UiViewportRenderTarget::width() {
  return mpViewport->width();
}
int UiViewportRenderTarget::height() {
  return mpViewport->height();
}
void UiViewportRenderTarget::BeginFrame(Context* pTARG) {
  mpViewport->BeginFrame(pTARG);
}
void UiViewportRenderTarget::EndFrame(Context* pTARG) {
  mpViewport->EndFrame(pTARG);
}

///////////////////////////////////////////////////////////////////////////////

UiSurfaceRenderTarget::UiSurfaceRenderTarget(ui::Surface* pVP)
    : mSurface(pVP) {
}
int UiSurfaceRenderTarget::width() {
  return mSurface->width();
}
int UiSurfaceRenderTarget::height() {
  return mSurface->height();
}
void UiSurfaceRenderTarget::BeginFrame(Context* pTARG) {
  mSurface->BeginSurface(pTARG);
}
void UiSurfaceRenderTarget::EndFrame(Context* pTARG) {
  mSurface->EndSurface(pTARG);
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
