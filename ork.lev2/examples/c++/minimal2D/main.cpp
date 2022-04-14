////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/kernel/string/deco.inl>
#include <ork/lev2/ezapp.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/material_freestyle.h>

using namespace std::string_literals;
using namespace ork;
using namespace ork::lev2;

struct Resources {

  Resources(Context* ctx){
    _material = std::make_shared<FreestyleMaterial>();
    _material->gpuInit(ctx, "orkshader://solid");
    _fxtechnique     = _material->technique("mmodcolor");
    _fxparameterMVP  = _material->param("MatMVP");
    _fxparameterMODC = _material->param("modcolor");
    deco::printf(fvec3::White(), "gpuINIT - context<%p>\n", ctx, _fxtechnique);
    deco::printf(fvec3::Yellow(), "  fxtechnique<%p>\n", _fxtechnique);
    deco::printf(fvec3::Yellow(), "  fxparameterMVP<%p>\n", _fxparameterMVP);
    deco::printf(fvec3::Yellow(), "  fxparameterMODC<%p>\n", _fxparameterMODC);
  }

  freestyle_mtl_ptr_t _material;
  const FxShaderTechnique* _fxtechnique = nullptr;
  const FxShaderParam* _fxparameterMVP  = nullptr;
  const FxShaderParam* _fxparameterMODC = nullptr;


};

using resources_ptr_t = std::shared_ptr<Resources>;

int main(int argc, char** argv,char** envp) {
  auto init_data = std::make_shared<ork::AppInitData>(argc,argv,envp);
  auto qtapp  = OrkEzApp::create(init_data);
  auto qtwin  = qtapp->_mainWindow;
  auto gfxwin = qtwin->_gfxwin;
  //////////////////////////////////////////////////////////
  resources_ptr_t resources;
  //////////////////////////////////////////////////////////
  qtapp->onGpuInit([&](Context* ctx) {
    resources = std::make_shared<Resources>(ctx);
  });
  //////////////////////////////////////////////////////////
  qtapp->onDraw([&](ui::drawevent_constptr_t drwev) {
    auto context        = drwev->GetTarget();
    auto fbi            = context->FBI(); // FrameBufferInterface
    auto fxi            = context->FXI(); // FX Interface
    float r             = float(rand() % 256) / 255.0f;
    float g             = float(rand() % 256) / 255.0f;
    float b             = float(rand() % 256) / 255.0f;
    int TARGW           = context->mainSurfaceWidth();
    int TARGH           = context->mainSurfaceHeight();
    const SRect tgtrect = SRect(0, 0, TARGW, TARGH);

    fbi->SetClearColor(fvec4(r, g, b, 1));
    context->beginFrame();
    RenderContextFrameData RCFD(context);
    resources->_material->begin(resources->_fxtechnique, RCFD);
    resources->_material->bindParamMatrix(resources->_fxparameterMVP, fmtx4::Identity());
    resources->_material->bindParamVec4(resources->_fxparameterMODC, fvec4::Red());
    gfxwin->Render2dQuadEML(fvec4(-0.5, -0.5, 1, 1), fvec4(0, 0, 1, 1), fvec4(0, 0, 1, 1));
    resources->_material->end(RCFD);
    context->endFrame();
  });
  //////////////////////////////////////////////////////////
  qtapp->onResize([&](int w, int h) { printf("GOTRESIZE<%d %d>\n", w, h); });
  //////////////////////////////////////////////////////////
  qtapp->onUiEvent([&](ui::event_constptr_t ev) -> ui::HandlerResult {
    switch (ev->_eventcode) {
      case ui::EventCode::DOUBLECLICK:
        OrkAssert(false);
        break;
      default:
        break;
    }
    ui::HandlerResult rval;
    return rval;
  });
  //////////////////////////////////////////////////////////
  qtapp->onGpuExit([&](Context* ctx) {
    resources = nullptr;
  });
  //////////////////////////////////////////////////////////
  return qtapp->mainThreadLoop();
}
