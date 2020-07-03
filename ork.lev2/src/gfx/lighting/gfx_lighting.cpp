////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/kernel/Array.hpp>
#include <ork/kernel/fixedlut.hpp>
#include <ork/lev2/gfx/camera/cameradata.h>
#include <ork/lev2/gfx/lighting/gfx_lighting.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/math/collision_test.h>
#include <ork/reflect/properties/registerX.inl>

ImplementReflectionX(ork::lev2::LightManagerData, "LightManagerData");
ImplementReflectionX(ork::lev2::LightData, "LightData");
ImplementReflectionX(ork::lev2::PointLightData, "PointLightData");
ImplementReflectionX(ork::lev2::DirectionalLightData, "DirectionalLightData");
ImplementReflectionX(ork::lev2::AmbientLightData, "AmbientLightData");
ImplementReflectionX(ork::lev2::SpotLightData, "SpotLightData");

///////////////////////////////////////////////////////////////////////////////
namespace ork {

template class fixedlut<float, lev2::Light*, lev2::LightContainer::kmaxlights>;
template class fixedlut<float, lev2::Light*, lev2::GlobalLightContainer::kmaxlights>;
template class ork::fixedvector<std::pair<U32, lev2::LightingGroup*>, lev2::LightCollector::kmaxonscreengroups>;
template class ork::fixedvector<lev2::LightingGroup, lev2::LightCollector::kmaxonscreengroups>;

namespace lev2 {
///////////////////////////////////////////////////////////////////////////////

bool Light::isShadowCaster() const {
  return _data->IsShadowCaster();
}
float Light::distance(fvec3 pos) const {
  return (worldPosition() - pos).Mag();
}

///////////////////////////////////////////////////////////////////////////////

void LightData::describeX(class_t* c) {

  c->memberProperty("Color", &LightData::mColor);
  c->memberProperty("ShadowCaster", &LightData::mbShadowCaster);
  c->memberProperty("Decal", &LightData::_decal);

  c->floatProperty("ShadowBias", float_range{0.0, 0.01}, &LightData::mShadowBias)
      ->annotate<ConstString>("editor.range.log", "true");
  c->floatProperty("ShadowBlur", float_range{0.0, 1.0}, &LightData::mShadowBlur);
  c->memberProperty("ShadowMapSize", &LightData::_shadowMapSize)
      ->annotate<ConstString>("editor.range.min", "128")
      ->annotate<ConstString>("editor.range.max", "4096");
  c->memberProperty("ShadowSamples", &LightData::_shadowsamples)
      ->annotate<ConstString>("editor.range.min", "1")
      ->annotate<ConstString>("editor.range.max", "16");

  c->accessorProperty("Cookie", &LightData::_readCookie, &LightData::_writeCookie)
      ->annotate<ConstString>("editor.class", "ged.factory.assetlist")
      ->annotate<ConstString>("editor.assettype", "lev2tex")
      ->annotate<ConstString>("editor.assetclass", "lev2tex");
}

void LightData::_readCookie(ork::rtti::ICastable*& tex) const {
  tex = _cookie;
}
void LightData::_writeCookie(ork::rtti::ICastable* const& tex) {
  _cookie = tex ? ork::rtti::autocast(tex) : nullptr;
}

LightData::LightData()
    : mColor(1.0f, 0.0f, 0.0f)
    , _cookie(nullptr)
    , mbShadowCaster(false)
    , _shadowsamples(1)
    , mShadowBlur(0.0f)
    , mShadowBias(0.002f) {
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PointLightData::describeX(class_t* c) {

  c->floatProperty("Radius", float_range{1, 3000}, &PointLightData::_radius)->annotate<ConstString>("editor.range.log", "true");
  c->floatProperty("Falloff", float_range{1, 10}, &PointLightData::_falloff)->annotate<ConstString>("editor.range.log", "true");
}

///////////////////////////////////////////////////////////////////////////////

PointLight::PointLight(xform_generator_t mtx, const PointLightData* pld)
    : Light(mtx, pld)
    , _pldata(pld) {
}

///////////////////////////////////////////////////////////////////////////////

bool PointLight::IsInFrustum(const Frustum& frustum) {
  const fvec3& wpos = worldPosition();

  float fd = frustum._nearPlane.pointDistance(wpos);

  if (fd > 200.0f)
    return false;

  return CollisionTester::FrustumSphereTest(frustum, Sphere(worldPosition(), radius()));
}

///////////////////////////////////////////////////////////

bool PointLight::AffectsSphere(const fvec3& center, float radius_) {
  float dist          = (worldPosition() - center).Mag();
  float combinedradii = (radius_ + radius());
  return (dist < combinedradii);
}

///////////////////////////////////////////////////////////////////////////////

bool PointLight::AffectsAABox(const AABox& aab) {
  return CollisionTester::SphereAABoxTest(Sphere(worldPosition(), radius()), aab);
}

///////////////////////////////////////////////////////////////////////////////

bool PointLight::AffectsCircleXZ(const Circle& cirXZ) {
  fvec3 center(cirXZ.mCenter.GetX(), worldPosition().GetY(), cirXZ.mCenter.GetY());
  float dist          = (worldPosition() - center).Mag();
  float combinedradii = (cirXZ.mRadius + radius());
  return (dist < combinedradii);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DirectionalLight::DirectionalLight(xform_generator_t mtx, const DirectionalLightData* dld)
    : Light(mtx, dld) {
}

///////////////////////////////////////////////////////////////////////////////

void DirectionalLightData::describeX(class_t* c) {
}

///////////////////////////////////////////////////////////////////////////////

bool DirectionalLight::IsInFrustum(const Frustum& frustum) {
  return true; // directional lights are unbounded, hence always true
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AmbientLight::AmbientLight(xform_generator_t mtx, const AmbientLightData* dld)
    : Light(mtx, dld)
    , mAld(dld) {
}

///////////////////////////////////////////////////////////////////////////////

void AmbientLightData::describeX(class_t* c) {
  c->floatProperty("AmbientShade", float_range{0, 1}, &AmbientLightData::mfAmbientShade);
  c->memberProperty("HeadlightDir", &AmbientLightData::mvHeadlightDir);
}

///////////////////////////////////////////////////////////////////////////////

bool AmbientLight::IsInFrustum(const Frustum& frustum) {
  return true; // ambient lights are unbounded, hence always true
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

void SpotLightData::describeX(class_t* c) {
  c->floatProperty("Fovy", float_range{0, 180}, &SpotLightData::mFovy);
  c->floatProperty("Range", float_range{1, 1000}, &SpotLightData::mRange) //
      ->annotate<bool>("editor.range.log", true);
}

SpotLightData::SpotLightData()
    : mFovy(10.0f)
    , mRange(1.0f) {
}

///////////////////////////////////////////////////////////////////////////////

SpotLight::SpotLight(xform_generator_t mtx, const SpotLightData* sld)
    : Light(mtx, sld)
    , _SLD(sld)
    , _shadowRTG(nullptr)
    , _shadowIRT(nullptr)
    , _shadowmapDim(0) {
}

///////////////////////////////////////////////////////////////////////////////

RtGroupRenderTarget* SpotLight::rendertarget(Context* ctx) {
  if (nullptr == _shadowIRT or (_SLD->shadowMapSize() != _shadowmapDim)) {
    _shadowmapDim = _SLD->shadowMapSize();
    _shadowRTG    = new RtGroup(ctx, _shadowmapDim, _shadowmapDim, _SLD->shadowSamples());
    _shadowIRT    = new RtGroupRenderTarget(_shadowRTG);
  }
  return _shadowIRT;
}

///////////////////////////////////////////////////////////////////////////////

bool SpotLight::IsInFrustum(const Frustum& frustum) {
  const auto& mtx = worldMatrix();
  fvec3 pos       = mtx.GetTranslation();
  fvec3 tgt       = pos + mtx.GetZNormal() * GetRange();
  fvec3 up        = mtx.GetYNormal();
  float fovy      = 15.0f;

  Set(pos, tgt, up, fovy);

  return false; // CollisionTester::FrustumFrustumTest( frustum, mWorldSpaceLightFrustum );
}

///////////////////////////////////////////////////////////

void SpotLight::Set(const fvec3& pos, const fvec3& tgt, const fvec3& up, float fovy) {
  // mFovy = fovy;

  // mWorldSpaceDirection = (tgt-pos);

  // mRange = mWorldSpaceDirection.Mag();

  // mWorldSpaceDirection.Normalize();

  mProjectionMatrix.Perspective(GetFovy(), 1.0, GetRange() / float(1000.0f), GetRange());
  mViewMatrix.LookAt(pos.GetX(), pos.GetY(), pos.GetZ(), tgt.GetX(), tgt.GetY(), tgt.GetZ(), up.GetX(), up.GetY(), up.GetZ());
  // mFovy = fovy;

  mWorldSpaceLightFrustum.Set(mViewMatrix, mProjectionMatrix);

  // SetPosition( pos );
}

///////////////////////////////////////////////////////////

bool SpotLight::AffectsSphere(const fvec3& center, float radius) {
  return CollisionTester::FrustumSphereTest(mWorldSpaceLightFrustum, Sphere(center, radius));
}

///////////////////////////////////////////////////////////////////////////////

bool SpotLight::AffectsAABox(const AABox& aab) {
  return CollisionTester::Frustu_aaBoxTest(mWorldSpaceLightFrustum, aab);
}

///////////////////////////////////////////////////////////////////////////////

bool SpotLight::AffectsCircleXZ(const Circle& cirXZ) {
  return CollisionTester::FrustumCircleXZTest(mWorldSpaceLightFrustum, cirXZ);
}

///////////////////////////////////////////////////////////////////////////////

fmtx4 SpotLight::shadowMatrix() const {
  fmtx4 matW   = worldMatrix();
  float fovy   = GetFovy();
  float range  = GetRange();
  float near   = range / 1000.0f;
  float far    = range;
  float aspect = 1.0;
  fvec3 wnx, wny, wnz, wpos;
  matW.toNormalVectors(wnx, wny, wnz);
  wpos      = matW.GetTranslation();
  fvec3 ctr = wpos + wnz * 0.01;
  fmtx4 matV, matP;
  matV.LookAt(wpos, ctr, wny);
  matP.Perspective(fovy, aspect, near, far);
  return matV * matP;
}

CameraData SpotLight::shadowCamDat() const {
  CameraData rval;
  fmtx4 matW   = worldMatrix();
  float fovy   = GetFovy();
  float range  = GetRange();
  float near   = range / 1000.0f;
  float far    = range;
  float aspect = 1.0;
  fvec3 wnx, wny, wnz, wpos;
  matW.toNormalVectors(wnx, wny, wnz);
  wpos      = matW.GetTranslation();
  fvec3 ctr = wpos + wnz * 0.01;

  rval.mEye       = wpos;
  rval.mTarget    = ctr;
  rval.mUp        = wny;
  rval._xnormal   = wnx;
  rval._ynormal   = wny;
  rval._znormal   = wnz;
  rval.mAper      = fovy;
  rval.mHorizAper = fovy;
  rval.mNear      = near;
  rval.mFar       = far;

  return rval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LightContainer::AddLight(Light* plight) {
  if (mPrioritizedLights.size() < map_type::kimax) {
    mPrioritizedLights.AddSorted(plight->mPriority, plight);
  }
}

void LightContainer::RemoveLight(Light* plight) {
  map_type::iterator it = mPrioritizedLights.find(plight->mPriority);
  if (it != mPrioritizedLights.end()) {
    mPrioritizedLights.erase(it);
  }
}

LightContainer::LightContainer()
    : mPrioritizedLights(EKEYPOLICY_MULTILUT) {
}

void LightContainer::Clear() {
  mPrioritizedLights.clear();
}

///////////////////////////////////////////////////////////////////////////////

void GlobalLightContainer::AddLight(Light* plight) {
  if (mPrioritizedLights.size() < map_type::kimax) {
    mPrioritizedLights.AddSorted(plight->mPriority, plight);
  }
}

void GlobalLightContainer::RemoveLight(Light* plight) {
  map_type::iterator it = mPrioritizedLights.find(plight->mPriority);
  if (it != mPrioritizedLights.end()) {
    mPrioritizedLights.erase(it);
  }
}

GlobalLightContainer::GlobalLightContainer()
    : mPrioritizedLights(EKEYPOLICY_MULTILUT) {
}

void GlobalLightContainer::Clear() {
  mPrioritizedLights.clear();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// const LightingGroup& LightCollector::GetActiveGroup( int idx ) const
//{
//	return mGroups[ idx ];
//}

size_t LightCollector::GetNumGroups() const {
  return mGroups.size();
}

void LightCollector::SetManager(LightManager* mgr) {
  mManager = mgr;
}

void LightCollector::Clear() {
  mGroups.clear();
  /*for( int i=0; i<kmaxonscreengroups; i++ )
  {
      mGroups[i].mLightManager = mManager;
      mGroups[i].mInstances.clear();
  }*/
  mActiveMap.clear();
}

LightCollector::LightCollector()
    : mManager(0) {
  // for( int i=0; i<kmaxonscreengroups; i++ )
  //{
  //	mGroups[i].mLightMask.SetMask(i);
  //}
}

LightCollector::~LightCollector() {
  static size_t imax = 0;

  size_t isize = mActiveMap.size();

  if (isize > imax) {
    imax = isize;
  }

  /// printf( "lc maxgroups<%d>\n", imax );
}

void LightCollector::QueueInstance(const LightMask& lmask, const fmtx4& mtx) {
  U32 uval = lmask.mMask;

  ActiveMapType::const_iterator it = mActiveMap.find(uval);

  if (it != mActiveMap.end()) {
    LightingGroup* pgrp = it->second;

    pgrp->mInstances.push_back(mtx);
  } else {
    size_t index        = mGroups.size();
    LightingGroup* pgrp = mGroups.allocate();

    pgrp->mLightMask.SetMask(uval);
    mActiveMap.AddSorted(uval, pgrp);

    pgrp->mInstances.push_back(mtx);
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LightManagerData::describeX(class_t* c) {
}

///////////////////////////////////////////////////////////////////////////////

void LightManager::enumerateInPass(const CompositingPassData& CPD, EnumeratedLights& out) const {
  out._enumeratedLights.clear();
  ////////////////////////////////////////////////////////////
  for (GlobalLightContainer::map_type::const_iterator it = mGlobalStationaryLights.mPrioritizedLights.begin();
       it != mGlobalStationaryLights.mPrioritizedLights.end();
       it++) {
    Light* plight = it->second;

    if (true) { // plight->IsInFrustum(frustum)) {
      size_t idx = out._enumeratedLights.size();

      plight->miInFrustumID = 1 << idx;
      out._enumeratedLights.push_back(plight);
    } else {
      plight->miInFrustumID = -1;
    }
  }
  ////////////////////////////////////////////////////////////
  for (LightContainer::map_type::const_iterator it = mGlobalMovingLights.mPrioritizedLights.begin();
       it != mGlobalMovingLights.mPrioritizedLights.end();
       it++) {
    Light* plight = it->second;

    if (true) { // plight->IsInFrustum(frustum)) {
      size_t idx = out._enumeratedLights.size();

      plight->miInFrustumID = 1 << idx;
      out._enumeratedLights.push_back(plight);
    } else {
      plight->miInFrustumID = -1;
    }
  }

  ////////////////////////////////////////////////////////////
  // mcollector.SetManager(this);
  // mcollector.Clear();
}

///////////////////////////////////////////////////////////

void LightManager::QueueInstance(const LightMask& lmask, const fmtx4& mtx) {
  mcollector.QueueInstance(lmask, mtx);
}

///////////////////////////////////////////////////////////

size_t LightManager::GetNumLightGroups() const {
  return mcollector.GetNumGroups();
}

///////////////////////////////////////////////////////////

void LightManager::Clear() {
  // mGlobalStationaryLights.Clear();
  mGlobalMovingLights.Clear();

  mcollector.Clear();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LightMask::AddLight(const Light* plight) {
  mMask |= plight->miInFrustumID;
}

///////////////////////////////////////////////////////////

size_t LightingGroup::GetNumLights() const {
  return size_t(countbits(mLightMask.mMask));
}

///////////////////////////////////////////////////////////

size_t LightingGroup::GetNumMatrices() const {
  return mInstances.size();
}

///////////////////////////////////////////////////////////

const fmtx4* LightingGroup::GetMatrices() const {
  return &mInstances[0];
}

///////////////////////////////////////////////////////////////////////////////

int LightingGroup::GetLightId(int idx) const {
  U32 mask = mLightMask.mMask;

  int ilightid = -1;

  U32 umask = 1;
  for (int b = 0; b < 32; b++) {
    if (mask & umask) {
      ilightid = b;
      idx--;
      if (0 == idx)
        return ilightid;
    }
    umask <<= 1;
  }

  return ilightid;
}

///////////////////////////////////////////////////////////////////////////////

LightingGroup::LightingGroup()
    : mLightManager(0)
    , mLightMap(0)
    , mDPEnvMap(0) {
}

///////////////////////////////////////////////////////////////////////////////

HeadLightManager::HeadLightManager(RenderContextFrameData& FrameData)
    : mHeadLight([this]() -> fmtx4 { return mHeadLightMatrix; }, &mHeadLightData)
    , mHeadLightManager(mHeadLightManagerData) {
  auto cdata = FrameData.topCPD().cameraMatrices();
  /*
  auto camvd = cdata->computeMatrices();
    ork::fvec3 vZ = cdata->xNormal();
    ork::fvec3 vY = cdata->yNormal();
    ork::fvec3 vP = cdata->GetFrustum().mNearCorners[0];
    mHeadLightMatrix = cdata->GetIVMatrix();
    mHeadLightData.SetColor( fvec3(1.3f,1.3f,1.5f) );
    mHeadLightData.SetAmbientShade(0.757f);
    mHeadLightData.SetHeadlightDir(fvec3(0.0f,0.5f,1.0f));
    mHeadLight.miInFrustumID = 1;
    mHeadLightGroup.mLightMask.AddLight( & mHeadLight );
    mHeadLightGroup.mLightManager = FrameData.GetLightManager();
    mHeadLightMatrix.SetTranslation( vP );
    mHeadLightManager.mGlobalMovingLights.AddLight( & mHeadLight );
    mHeadLightManager._enumeratedLights.push_back(& mHeadLight);*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
/////////////////////////////////////
// global lighting alg
/////////////////////////////////////
/////////////////////////////////////
// .	enumerate set of ONSCREEN static lights via scenegraph
// .	enumerate set of ONSCREEN dynamic lights via scenegraph
// .	cull light sets based on a policy (example - no more than 64 lights on screen at once)
//////////////
// .	when queing a renderable, identify its statically (precomputed) linked lights
// .    determine which dynamic lights affect the renderable
// .	cull the renderables lights based on a policy (example - use the 3 lights with the highest priority)
// .	map the renderable's culled light set to a specific lighting group, creating the group if necessary
// .        some of these light groups could potentially be precomputed (if no dynamics allowed in the group)
//////////////
// OR
// .   for each renderable in renderqueue
// .       bind lightgroup (with redundant state change checking)
// .	   render the renderable
//////////////

} // namespace lev2
} // namespace ork
