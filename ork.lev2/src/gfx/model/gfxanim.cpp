////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/application/application.h>
#include <ork/kernel/orklut.hpp>
#include <ork/file/chunkfile.h>
#include <ork/file/chunkfile.inl>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/kernel/string/deco.inl>

using namespace std::string_literals;

ImplementReflectionX(ork::lev2::XgmAnimChannel, "XgmAnimChannel");
ImplementReflectionX(ork::lev2::XgmFloatAnimChannel, "XgmFloatAnimChannel");
ImplementReflectionX(ork::lev2::XgmVect3AnimChannel, "XgmVect3AnimChannel");
ImplementReflectionX(ork::lev2::XgmVect4AnimChannel, "XgmVect4AnimChannel");
ImplementReflectionX(ork::lev2::XgmMatrixAnimChannel, "XgmMatrixAnimChannel");
ImplementReflectionX(ork::lev2::XgmDecompAnimChannel, "XgmDecompAnimChannel");

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

void XgmAnimChannel::describeX(class_t* clazz) {
}
void XgmFloatAnimChannel::describeX(class_t* clazz) {
}
void XgmVect3AnimChannel::describeX(class_t* clazz) {
}
void XgmVect4AnimChannel::describeX(class_t* clazz) {
}
void XgmMatrixAnimChannel::describeX(class_t* clazz) {
}
void XgmDecompAnimChannel::describeX(class_t* clazz) {
}

///////////////////////////////////////////////////////////////////////////////

XgmAnimMask::XgmAnimMask() {
  EnableAll();
}

XgmAnimMask::XgmAnimMask(const XgmAnimMask& mask) {
  for (int i = 0; i < knummaskbytes; i++)
    mMaskBits[i] = mask.mMaskBits[i];
}

XgmAnimMask& XgmAnimMask::operator=(const XgmAnimMask& mask) {
  for (int i = 0; i < knummaskbytes; i++)
    mMaskBits[i] = mask.mMaskBits[i];
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

void XgmAnimMask::EnableAll() {
  for (int i = 0; i < knummaskbytes; i++)
    mMaskBits[i] = 0xff;
}

///////////////////////////////////////////////////////////////////////////////

// because there are 2 bones per char (4 bits each)
#define BONE_TO_CHAR(iboneindex) (iboneindex >> 1)
#define BONE_TO_BIT(iboneindex) ((iboneindex & 0x1) << 2)

#define CHECK_BONE(iboneindex) (iboneindex >= 0 && iboneindex < ((XgmAnimMask::knummaskbytes << 3) >> 2))
#define ASSERT_BONE(iboneindex) OrkAssert(CHECK_BONE(iboneindex))

#define SET_BITS(c, ibitindex, bits) (c |= (bits << ibitindex))
#define CLEAR_BITS(c, ibitindex, bits) (c &= ~(bits << ibitindex))
#define CHECK_BITS(c, ibitindex, bits) ((c >> ibitindex) & bits)

void XgmAnimMask::Enable(const XgmSkeleton& Skel, const PoolString& BoneName, EXFORM_COMPONENT components) {
  if (XFORM_COMPONENT_NONE == components)
    return;

  int iboneindex = Skel.jointIndex(BoneName);

  // this used to be assert, but designers may someday be setting bone names in tool. Be nicer to them.
  if (!CHECK_BONE(iboneindex)) {
    orkprintf("Bone does not exist: %s!\n", BoneName.c_str());
    return;
  }

  Enable(iboneindex, components);
}

void XgmAnimMask::Enable(int iboneindex, EXFORM_COMPONENT components) {
  // this used to be assert, but designers may someday be setting bone names in tool. Be nicer to them.
  if (!CHECK_BONE(iboneindex)) {
    orkprintf("Bone index does not exist: %d!\n", iboneindex);
    return;
  }

  if (XFORM_COMPONENT_NONE == components)
    return;

  int icharindex = BONE_TO_CHAR(iboneindex);
  int ibitindex  = BONE_TO_BIT(iboneindex);
  SET_BITS(mMaskBits[icharindex], ibitindex, components);
}

///////////////////////////////////////////////////////////////////////////////

void XgmAnimMask::DisableAll() {
  for (int i = 0; i < knummaskbytes; i++)
    mMaskBits[i] = 0x0;
}

///////////////////////////////////////////////////////////////////////////////

void XgmAnimMask::Disable(const XgmSkeleton& Skel, const PoolString& BoneName, EXFORM_COMPONENT components) {
  if (XFORM_COMPONENT_NONE == components)
    return;

  int iboneindex = Skel.jointIndex(BoneName);

  // this used to be assert, but designers may someday be setting bone names in tool. Be nicer to them.
  if (!CHECK_BONE(iboneindex)) {
    orkprintf("Bone does not exist: %s!\n", BoneName.c_str());
    return;
  }

  Disable(iboneindex, components);
}

void XgmAnimMask::Disable(int iboneindex, EXFORM_COMPONENT components) {
  ASSERT_BONE(iboneindex);

  if (XFORM_COMPONENT_NONE == components)
    return;

  int icharindex = BONE_TO_CHAR(iboneindex);
  int ibitindex  = BONE_TO_BIT(iboneindex);
  CLEAR_BITS(mMaskBits[icharindex], ibitindex, components);
}

///////////////////////////////////////////////////////////////////////////////

bool XgmAnimMask::Check(const XgmSkeleton& Skel, const PoolString& BoneName, EXFORM_COMPONENT components) const {
  if (XFORM_COMPONENT_NONE == components)
    return false;

  int iboneindex = Skel.jointIndex(BoneName);

  // this used to be assert, but designers may someday be setting bone names in tool. Be nicer to them.
  if (!CHECK_BONE(iboneindex)) {
    orkprintf("Bone does not exist: %s!\n", BoneName.c_str());
    return false;
  }

  return Check(iboneindex, components);
}

bool XgmAnimMask::Check(int iboneindex, EXFORM_COMPONENT components) const {
  ASSERT_BONE(iboneindex);

  if (XFORM_COMPONENT_NONE == components)
    return false;

  int icharindex = BONE_TO_CHAR(iboneindex);
  int ibitindex  = BONE_TO_BIT(iboneindex);
  return CHECK_BITS(mMaskBits[icharindex], ibitindex, components);
}

EXFORM_COMPONENT XgmAnimMask::GetComponents(const XgmSkeleton& Skel, const PoolString& BoneName) const {
  int iboneindex = Skel.jointIndex(BoneName);

  // this used to be assert, but designers may someday be setting bone names in tool. Be nicer to them.
  if (!CHECK_BONE(iboneindex)) {
    orkprintf("Bone does not exist: %s!\n", BoneName.c_str());
    return XFORM_COMPONENT_NONE;
  }

  return GetComponents(iboneindex);
}

EXFORM_COMPONENT XgmAnimMask::GetComponents(int iboneindex) const {
  // this used to be assert, but designers may someday be setting bone names in tool. Be nicer to them.
  if (!CHECK_BONE(iboneindex)) {
    orkprintf("Bone does not exist: %d!\n", iboneindex);
    return XFORM_COMPONENT_NONE;
  }

  int icharindex = BONE_TO_CHAR(iboneindex);
  int ibitindex  = BONE_TO_BIT(iboneindex);
  return EXFORM_COMPONENT(CHECK_BITS(mMaskBits[icharindex], ibitindex, XFORM_COMPONENT_ALL));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

XgmMaterialStateInst::XgmMaterialStateInst(const XgmModelInst& minst)
    : mModelInst(minst)
    , mModel(minst.xgmModel())
    , mVarMap(EKEYPOLICY_MULTILUT) {
}

///////////////////////////////////////////////////////////////////////////////

class MaterialInstItem_UvXf : public MaterialInstItemMatrix {

  const XgmAnimInst& mAnimInst;
  const XgmMatrixAnimChannel* mChannel;

public:
  MaterialInstItem_UvXf(const XgmAnimInst& ai, const XgmMatrixAnimChannel* chan)
      : mAnimInst(ai)
      , mChannel(chan) {
  }

  void set() final {
    const XgmAnim& anim = *mAnimInst.GetAnim();

    /*float fx = 0.0f;
    if( 0 != strstr( anim.GetName().c_str(), "electric" ) )
    {
        fx = 0.0f;
    }

    if( fx == 1.0f )
    {
        orkprintf( "yo\n" );
    }*/

    float fw         = mAnimInst.GetWeight();
    float fr         = mAnimInst.GetCurrentFrame();
    const fmtx4& mtx = mChannel->GetFrame(int(fr));
    SetMatrix(mtx);
  }
};

/// ////////////////////////////////////////////////////////////////////////////
/// bind the animinst to the materialpose
/// this will figure out which anim channels match (bind) to slots in the materials
/// so that it does not have to be done every frame
/// ////////////////////////////////////////////////////////////////////////////

void XgmMaterialStateInst::BindAnimInst(const XgmAnimInst& AnimInst) {
  if (AnimInst.GetAnim()) {
    const XgmAnim& anim = *AnimInst.GetAnim();

    size_t inummaterialchannels = anim.GetNumMaterialChannels();
    int nummaterials            = mModel->GetNumMaterials();

    const XgmAnim::MaterialChannelsMap& map = anim.RefMaterialChannels();

    for (XgmAnim::MaterialChannelsMap::const_iterator it = map.begin(); it != map.end(); it++) {
      const PoolString& channelname = it->first;
      auto channel                  = it->second.get();
      const PoolString& objectname  = channel->GetObjectName();

      const XgmFloatAnimChannel* __restrict fchan  = rtti::autocast(channel);
      const XgmVect3AnimChannel* __restrict v3chan = rtti::autocast(channel);
      const XgmMatrixAnimChannel* __restrict mchan = rtti::autocast(channel);

      if (mchan) {
        for (int imat = 0; imat < nummaterials; imat++) {
          auto material             = mModel->GetMaterial(imat);
          const PoolString& matname = material->GetName();

          if (matname == objectname) {
            /*
            const GfxMaterialWiiBasic* __restrict material_basic = rtti::autocast(material);
            const GfxMaterialFx* __restrict material_fx          = rtti::autocast(material);

            if (material_basic) {
              const TextureContext& ctx = material_basic->GetTexture(ETEXDEST_DIFFUSE);
              const Texture* ptex       = ctx.mpTexture;

              MaterialInstItem_UvXf* pxmsis = new MaterialInstItem_UvXf(AnimInst, mchan);
              pxmsis->mObjectName           = objectname;
              pxmsis->mChannelName          = channelname;
              pxmsis->mMaterial             = material_basic;
              material_basic->BindMaterialInstItem(pxmsis);
              mVarMap.AddSorted(&AnimInst, pxmsis);
            } else if (material_fx) {
            }*/
          }
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void XgmMaterialStateInst::UnBindAnimInst(const XgmAnimInst& AnimInst) {
  auto it = mVarMap.find(&AnimInst);

  while (it != mVarMap.end()) {
    MaterialInstItem* item = it->second;

    const GfxMaterial* pmaterial = item->mMaterial;

    if (pmaterial) {
      pmaterial->UnBindMaterialInstItem(item);
    }

    delete item;
    mVarMap.RemoveItem(it);
    it = mVarMap.find(&AnimInst);
  }
}

///////////////////////////////////////////////////////////////////////////////

void XgmMaterialStateInst::ApplyAnimInst(const XgmAnimInst& AnimInst) {
  auto lb = mVarMap.LowerBound(&AnimInst);
  auto ub = mVarMap.UpperBound(&AnimInst);

  for (auto it = lb; it != ub; it++) {
    MaterialInstItem* psetter = it->second;

    if (psetter) {
      psetter->set();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

XgmAnim::XgmAnim()
    : miNumFrames(0) {
}

///////////////////////////////////////////////////////////////////////////////

void XgmAnim::AddChannel(const PoolString& Name, animchannel_ptr_t pchan) {
  const PoolString& usage = pchan->GetUsageSemantic();

  if (usage == FindPooledString("Joint")) {
    auto MtxChan = std::dynamic_pointer_cast<XgmDecompAnimChannel>(pchan).get();
    OrkAssert(MtxChan);
    mJointAnimationChannels.AddSorted(Name, MtxChan);
  } else if (usage == FindPooledString("UvTransform")) {
    auto MtxChan = std::dynamic_pointer_cast<XgmMatrixAnimChannel>(pchan).get();
    OrkAssert(MtxChan);
    mMaterialAnimationChannels.AddSorted(Name, pchan);
  } else if (usage == FindPooledString("FxParam")) {
    OrkAssert(pchan);
    mMaterialAnimationChannels.AddSorted(Name, pchan);
  }
}

///////////////////////////////////////////////////////////////////////////////

const XgmAnimInst::Binding XgmAnimInst::gBadBinding;

XgmAnimInst::XgmAnimInst()
    : mAnim(NULL)
    , mFrame(0.0f)
    , mWeight(1.0f) {
}

///////////////////////////////////////////////////////////////////////////////

void XgmAnimInst::BindAnim(const XgmAnim* anim) {
  mAnim  = anim;
  mFrame = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////

XgmAnimChannel::XgmAnimChannel(
    const PoolString& ObjName,
    const PoolString& ChanName,
    const PoolString& UsageSemantic,
    EChannelType etype)
    : mChannelName(ChanName)
    , mObjectName(ObjName)
    , mUsageSemantic(UsageSemantic) 
    , miNumFrames(0)
    , meChannelType(etype) {
}

///////////////////////////////////////////////////////////////////////////////

XgmAnimChannel::XgmAnimChannel(EChannelType etype)
    : mChannelName()
    , mUsageSemantic() 
    , miNumFrames(0)
    , meChannelType(etype) {
}

///////////////////////////////////////////////////////////////////////////////

XgmFloatAnimChannel::XgmFloatAnimChannel()
    : XgmAnimChannel(EXGMAC_FLOAT) {
}

XgmFloatAnimChannel::XgmFloatAnimChannel(const PoolString& ObjName, const PoolString& ChanName, const PoolString& Usage)
    : XgmAnimChannel(ObjName, ChanName, Usage, EXGMAC_FLOAT) {
}

///////////////////////////////////////////////////////////////////////////////

XgmVect3AnimChannel::XgmVect3AnimChannel()
    : XgmAnimChannel(EXGMAC_VECT3) {
}

XgmVect3AnimChannel::XgmVect3AnimChannel(const PoolString& ObjName, const PoolString& ChanName, const PoolString& Usage)
    : XgmAnimChannel(ObjName, ChanName, Usage, EXGMAC_VECT3) {
}

///////////////////////////////////////////////////////////////////////////////

XgmVect4AnimChannel::XgmVect4AnimChannel()
    : XgmAnimChannel(EXGMAC_VECT4) {
}

XgmVect4AnimChannel::XgmVect4AnimChannel(const PoolString& ObjName, const PoolString& ChanName, const PoolString& Usage)
    : XgmAnimChannel(ObjName, ChanName, Usage, EXGMAC_VECT4) {
}

///////////////////////////////////////////////////////////////////////////////

XgmMatrixAnimChannel::XgmMatrixAnimChannel()
    : XgmAnimChannel(EXGMAC_MTX44) {
}

XgmMatrixAnimChannel::XgmMatrixAnimChannel(const PoolString& ObjName, const PoolString& ChanName, const PoolString& Usage)
    : XgmAnimChannel(ObjName, ChanName, Usage, EXGMAC_MTX44) {
}

///////////////////////////////////////////////////////////////////////////////

void XgmMatrixAnimChannel::setFrame(size_t i, const fmtx4& v) {
  _sampledFrames[i] = v;
}

///////////////////////////////////////////////////////////////////////////////

fmtx4 XgmMatrixAnimChannel::GetFrame(int index) const {
  if(index>=_sampledFrames.size()){
    return fmtx4();
  }
  return _sampledFrames[index];
}

///////////////////////////////////////////////////////////////////////////////

size_t XgmMatrixAnimChannel::numFrames() const {
  return _sampledFrames.size();
}

///////////////////////////////////////////////////////////////////////////////

void XgmMatrixAnimChannel::reserveFrames(size_t icount) {
  _sampledFrames.resize(icount);
}

///////////////////////////////////////////////////////////////////////////////

XgmDecompAnimChannel::XgmDecompAnimChannel()
    : XgmAnimChannel(EXGMAC_DCMTX) {
}

XgmDecompAnimChannel::XgmDecompAnimChannel(const PoolString& ObjName, const PoolString& ChanName, const PoolString& Usage)
    : XgmAnimChannel(ObjName, ChanName, Usage, EXGMAC_DCMTX){

}
 

///////////////////////////////////////////////////////////////////////////////

void XgmDecompAnimChannel::setFrame(size_t i,const DecompMtx44& v) {
  _sampledFrames[i]=v;
}

///////////////////////////////////////////////////////////////////////////////

DecompMtx44 XgmDecompAnimChannel::GetFrame(int index) const {
  if(index>=_sampledFrames.size()){
    return DecompMtx44();
  }
  return _sampledFrames[index];
}

///////////////////////////////////////////////////////////////////////////////

size_t XgmDecompAnimChannel::numFrames() const {
  return _sampledFrames.size();
}

///////////////////////////////////////////////////////////////////////////////

void XgmDecompAnimChannel::reserveFrames(size_t icount) {
  _sampledFrames.resize(icount);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::lev2

template void ork::chunkfile::OutputStream::AddItem<ork::lev2::DecompMtx44>(const ork::lev2::DecompMtx44& item);
template class ork::orklut<ork::PoolString, ork::lev2::DecompMtx44>;
