////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/renderable.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/fx_pipeline.h>

#include <ork/kernel/orklut.hpp>
#include <ork/reflect/properties/DirectTypedMap.hpp>
#include <ork/reflect/properties/registerX.inl>

ImplementReflectionX(ork::lev2::InstancedModelDrawableData, "InstancedModelDrawableData");

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

void InstancedModelDrawableData::describeX(object::ObjectClass* clazz){
  clazz->directProperty("assetpath", &InstancedModelDrawableData::_assetpath);
  clazz->directMapProperty("assetvars", &InstancedModelDrawableData::_assetvars);
}

InstancedModelDrawableData::InstancedModelDrawableData(AssetPath path) : _assetpath(path) {
}
///////////////////////////////////////////////////////////////////////////////
drawable_ptr_t InstancedModelDrawableData::createDrawable() const {
  auto drw = std::make_shared<InstancedModelDrawable>();
  drw->_data = this;
  drw->bindModelAsset(_assetpath);
  drw->_modcolor = _modcolor;
  return drw;
}
///////////////////////////////////////////////////////////////////////////////
InstancedModelDrawable::InstancedModelDrawable()
    : InstancedDrawable() {
}
/////////////////////////////////////////////////////////////////////
InstancedModelDrawable::~InstancedModelDrawable() {
}
///////////////////////////////////////////////////////////////////////////////
struct IMDIMPL_SUBMESH {
  xgmsubmesh_constptr_t _xgmsubmesh = nullptr;
  fxpipelinecache_constptr_t _fxcache;
};
struct IMDIMPL_MODEL {
  std::vector<IMDIMPL_SUBMESH> _submeshes;
};
using imdimpl_xgmmodel_ptr_t = std::shared_ptr<IMDIMPL_MODEL>;

///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::bindModelAsset(AssetPath assetpath) {
  auto load_req = std::make_shared<asset::LoadRequest>(assetpath);
  _asset = asset::AssetManager<XgmModelAsset>::load(load_req);
  bindModel(_asset->_model.atomicCopy());
}
///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::bindModel(xgmmodel_ptr_t model) {
  _model = model;
  // generate material instance data
  auto impl = std::make_shared<IMDIMPL_MODEL>();
  _impl.set<imdimpl_xgmmodel_ptr_t>(impl);
  int inummeshes = _model->numMeshes();
  for (int imesh = 0; imesh < inummeshes; imesh++) {
    auto mesh       = _model->mesh(imesh);
    int inumclusset = mesh->numSubMeshes();
    for (int ics = 0; ics < inumclusset; ics++) {
      auto xgmsub = mesh->subMesh(ics);
      IMDIMPL_SUBMESH submesh_impl;
      submesh_impl._fxcache = xgmsub->_material->pipelineCache();
      submesh_impl._xgmsubmesh = xgmsub;
      impl->_submeshes.push_back(submesh_impl);
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::gpuInit(Context* ctx) const {
  _instanceMatrixTex = Texture::createBlank(1024, 256, EBufferFormat::RGBA32F);
  _instanceColorTex  = Texture::createBlank(1024, 256, EBufferFormat::RGBA32F);
  _instanceIdTex     = Texture::createBlank(1024, 128, EBufferFormat::RGBA16UI);

  _instanceMatrixTex->_debugName = "_instanceMatrixTex";
  _instanceColorTex->_debugName  = "_instanceColorTex";
  _instanceIdTex->_debugName     = "_instanceIdTex";
}
///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::enqueueToRenderQueue(
    drawablebufitem_constptr_t dbufitem, //
    lev2::IRenderer* renderer) const {
  ork::opq::assertOnQueue2(opq::mainSerialQueue());
  ////////////////////////////////////////////////////////////////////
  if (not _model)
    return;
  if (not _impl.isA<imdimpl_xgmmodel_ptr_t>())
    return;
  ////////////////////////////////////////////////////////////////////
  auto context                         = renderer->GetTarget();
  auto RCFD                            = context->topRenderContextFrameData();
  const auto& topCPD                   = RCFD->topCPD();
  const auto& monofrustum              = topCPD.monoCamFrustum();
  lev2::CallbackRenderable& renderable = renderer->enqueueCallback();
  ////////////////////////////////////////////////////////////////////
  bool isPick    = context->FBI()->isPickState();
  bool isSkinned = _model->isSkinned();
  if (not _instanceMatrixTex) {
    gpuInit(context); // todo figure out better do-only-once method...
  }
  ////////////////////////////////////////////////////////////////////
  renderable._pickID = _pickID;
  renderable.SetSortKey(0x00000001);
  renderable.SetDrawableDataA(GetUserDataA());
  renderable.SetDrawableDataB(GetUserDataB());
  renderable._instanced = true;
  //printf( "dbufitem _serialno<%d>\n", dbufitem->_serialno );
    auto it = dbufitem->_usermap.find("rtthread_instance_data"_crcu);
    OrkAssert(it!=dbufitem->_usermap.end());

  ////////////////////////////////////////////////////////////////////
  renderable.SetRenderCallback([this,dbufitem](lev2::RenderContextInstData& RCID) { //
    auto context     = RCID.context();
    auto GBI         = context->GBI();
    auto TXI         = context->TXI();
    auto FXI         = context->FXI();
    auto FBI         = context->FBI();
    auto impl        = _impl.getShared<IMDIMPL_MODEL>();
    bool isPick      = FBI->isPickState();
    bool isStereo    = RCID.rcfd()->isStereo();
    int pipeline_index = isStereo ? 1 : (isPick ? 2 : 0);
    ////////////////////////////////////////////////////////
    bool updatetex = true; //( (_drawcount++) < 5000);
    ////////////////////////////////////////////////////////
    // upload instance matrices to GPU
    ////////////////////////////////////////////////////////
    TextureInitData texdata;
    texdata._w           = k_texture_dimension_x; // 64 bytes per instance
    texdata._h           = ((_count+1023)>>10)&0x3ff;
    texdata._src_format  = EBufferFormat::RGBA32F;
    texdata._dst_format  = EBufferFormat::RGBA32F;
    texdata._autogenmips = false;
    //printf( "dbufitem->_usermap size<%zu>\n", dbufitem->_usermap.size() );
    auto it = dbufitem->_usermap.find("rtthread_instance_data"_crcu);
    OrkAssert(it!=dbufitem->_usermap.end());
    auto instances_copy = it->second.get<instanceddrawinstancedata_ptr_t>();
    texdata._data        = (const void*) instances_copy->_worldmatrices.data();
    texdata._truncation_length = _count*64;
    OrkAssert(_count <= k_max_instances);
    if(updatetex)
      TXI->initTextureFromData(_instanceMatrixTex.get(), texdata);
    ////////////////////////////////////////////////////////
    texdata._w    = k_texture_dimension_x; // 16 bytes per instance
    texdata._h    = ((_count+4095)>>12)&0xfff;
    texdata._data = (const void*)instances_copy->_modcolors.data();
    texdata._truncation_length = _count*16;
    if(updatetex)
      TXI->initTextureFromData(_instanceColorTex.get(), texdata);
    ////////////////////////////////////////////////////////
    texdata._w           = k_texture_dimension_x; // 8 bytes per instance
    texdata._h           = ((_count+4095)>>12)&0xfff;
    texdata._src_format  = EBufferFormat::RGBA16UI;
    texdata._dst_format  = EBufferFormat::RGBA16UI;
    texdata._autogenmips = false;
    texdata._data        = (const void*)instances_copy->_pickids.data();
    texdata._truncation_length = _count*8;
    if(updatetex)
      TXI->initTextureFromData(_instanceIdTex.get(), texdata);

    _instanceIdTex->TexSamplingMode().PresetPointAndClamp();
    TXI->ApplySamplingMode(_instanceIdTex.get());
    ////////////////////////////////////////////////////////
    // instanced render
    ////////////////////////////////////////////////////////
    RCID._isInstanced = true;
    for (auto& sub : impl->_submeshes) {
      auto xgmsub = sub._xgmsubmesh;
      auto fxlut = sub._fxcache;
      OrkAssert(fxlut);
      auto pipeline = fxlut->findPipeline(RCID);
      OrkAssert(pipeline);
      pipeline->wrappedDrawCall(RCID, [&]() {
        auto idata = instances_copy;
        ////////////////////////////////////
        // bind instancetex to sampler
        ////////////////////////////////////
        FXI->BindParamCTex(pipeline->_parInstanceMatrixMap, _instanceMatrixTex.get());
        FXI->BindParamCTex(pipeline->_parInstanceIdMap, _instanceIdTex.get());
        FXI->BindParamCTex(pipeline->_parInstanceColorMap, _instanceColorTex.get());
        ////////////////////////////////////
        int inumclus = xgmsub->_clusters.size();
        for (int ic = 0; ic < inumclus; ic++) {
          auto cluster    = xgmsub->cluster(ic);
          auto vtxbuf     = cluster->_vertexBuffer;
          size_t numprims = cluster->numPrimGroups();
          for (size_t ipg = 0; ipg < numprims; ipg++) {
            auto primgroup = cluster->primgroup(ipg);
            auto idxbuf    = primgroup->mpIndices;
            auto primtype  = primgroup->mePrimType;
            int numindices = primgroup->miNumIndices;
            GBI->DrawInstancedIndexedPrimitiveEML(*vtxbuf, *idxbuf, primtype, _count);
          }
        }
      }); // mtlinst->wrappedDrawCall(RCID, [&]() {
    }     // for (auto& sub : impl._submeshes) {
    RCID._isInstanced = false;
  });     // renderable.SetRenderCallback
  ////////////////////////////////////////////////////////////////////
} // InstancedModelDrawable::enqueueToRenderQueue(
/////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
