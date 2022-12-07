////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/application/application.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxanim.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/kernel/string/string.h>
#include <ork/kernel/prop.h>

#include <ork/lev2/gfx/gfxctxdummy.h>
#include <ork/file/chunkfile.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork::tool::meshutil {

///////////////////////////////////////////////////////////////////////////////
#if 0
bool DAEXGAFilter_ConvertAsset(const tokenlist& toklist) {
  ork::tool::FilterOptMap options;
  options.SetDefault("-in", "yo");
  options.SetDefault("--out", "yo");
  options.SetOptions(toklist);

  const std::string inf  = options.GetOption("--in")->GetValue();
  const std::string outf = options.GetOption("--out")->GetValue();

  ColladaExportPolicy policy;
  policy.mUnits = UNITS_METER;

  bool brval = false;

  bool bwii   = (0 != strstr(outf.c_str(), "wii"));
  bool bxb360 = (0 != strstr(outf.c_str(), "xb360"));

  EndianContext* pendianctx = 0;

  if (bwii || bxb360) {
    pendianctx          = new EndianContext;
    pendianctx->mendian = ork::EENDIAN_BIG;
  }

  printf("inf<%s>\n", inf.c_str());

  CColladaAnim* colanim = CColladaAnim::Load(inf.c_str());

  printf("colanim<%p>\n", colanim);
  if (colanim) {
    int inumframes = colanim->GetNumFrames();

    if (inumframes == 0)
      orkprintf("WARNING: Anim has zero frames: %s\n", outf.c_str());

    colanim->mXgmAnim.SetNumFrames(inumframes);

    const CColladaAnim::ChannelsMap& Channels = colanim->mAnimationChannels;

    const PoolString JointPS = AddPooledString("Joint");
    const PoolString UvXfPS  = AddPooledString("UvTransform");

    for (CColladaAnim::ChannelsMap::const_iterator it = Channels.begin(); it != Channels.end(); it++) {
      const std::string& ChannelName = it->first;
      PoolString ChannelPooledName   = AddPooledString(ChannelName.c_str());

      const ColladaAnimChannel* ChannelData = it->second;

      const ColladaMatrixAnimChannel* MatrixChannelData = rtti::autocast(ChannelData);
      const ColladaUvAnimChannel* UvChannelData         = rtti::autocast(ChannelData);

      if (MatrixChannelData) {
        PoolString objnameps                     = AddPooledString("");
        ork::lev2::XgmDecompAnimChannel* XgmChan = new ork::lev2::XgmDecompAnimChannel(objnameps, ChannelPooledName, JointPS);
        XgmChan->ReserveFrames(inumframes);
        for (int ifr = 0; ifr < inumframes; ifr++) {
          const fmtx4& Matrix = MatrixChannelData->GetFrame(ifr);
          ork::lev2::DecompMtx44 decomp;
          Matrix.decompose(decomp.mTrans, decomp.mRot, decomp.mScale);
          XgmChan->AddFrame(decomp);
        }
        colanim->mXgmAnim.AddChannel(ChannelPooledName, XgmChan);
      } else if (UvChannelData) {
        const std::string& materialname          = UvChannelData->GetMaterialName();
        PoolString objnameps                     = AddPooledString(materialname.c_str());
        ork::lev2::XgmMatrixAnimChannel* XgmChan = new ork::lev2::XgmMatrixAnimChannel(objnameps, ChannelPooledName, UvXfPS);
        XgmChan->ReserveFrames(inumframes);
        for (int ifr = 0; ifr < inumframes; ifr++) {
          const Place2dData& p2d = UvChannelData->GetFrame(ifr);

          fmtx4 mtxR, mtxS, mtxT, mtx;
          fmtx4 mtxRO, mtxRIO;
          mtxRIO.Translate(-0.5f, -0.5f, 0.0f);
          mtxRO.Translate(0.5f, 0.5f, 0.0f);

          mtxR.rotateOnZ(-p2d.rotateUV);
          mtxT.Translate(p2d.offsetU, p2d.offsetV, 0.0f);
          // mtxS.Scale( p2d.repeatU, p2d.repeatV, 1.0f );
          // mtxS.Scale( p2d.repeatU, p2d.repeatV, 1.0f );

          mtx = mtxT * (mtxRIO * mtxR * mtxRO); //*mtxS;

          XgmChan->AddFrame(mtx);
        }
        colanim->mXgmAnim.AddChannel(ChannelPooledName, XgmChan);
      }
    }

    ////////////////////////
    // create a POSE
    ////////////////////////

    orklut<PoolString, ork::lev2::DecompMtx44>& StaticPose = colanim->mXgmAnim.GetStaticPose();

    for (orkmap<std::string, fmtx4>::const_iterator it = colanim->mPose.begin(); it != colanim->mPose.end(); it++) {
      const std::string& jname     = (*it).first;
      const fmtx4& Mtx             = (*it).second;
      PoolString ChannelPooledName = AddPooledString(jname.c_str());

      ork::lev2::DecompMtx44 decmtx;
      Mtx.decompose(decmtx.mTrans, decmtx.mRot, decmtx.mScale);

      StaticPose.AddSorted(ChannelPooledName, decmtx);
    }

    ////////////////////////

    brval = ork::lev2::XgmAnim::Save(file::Path(outf.c_str()), &colanim->mXgmAnim);
  }

  if (pendianctx) {
    delete pendianctx;
  }
  return brval;
}
#endif
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::tool::meshutil
///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

datablock_ptr_t XgmAnim::Save(const XgmAnim* anm) {
  chunkfile::Writer chunkwriter("xga");
  ///////////////////////////////////
  chunkfile::OutputStream* HeaderStream   = chunkwriter.AddStream("header");
  chunkfile::OutputStream* AnimDataStream = chunkwriter.AddStream("animdata");
  ///////////////////////////////////

  int inumjointchannels    = (int)anm->GetNumJointChannels();
  int inummaterialchannels = (int)anm->GetNumMaterialChannels();

  int inumchannels = inumjointchannels + inummaterialchannels;

  int inumframes = anm->GetNumFrames();

  printf("XGAOUT inumjointchannels<%d> inumframes<%d>\n", inumchannels, inumframes);

  HeaderStream->AddItem(inumframes);
  HeaderStream->AddItem(inumchannels);

  ///////////////////////////////////////////////////////////////////////////////////////////////

  const XgmAnim::JointChannelsMap& JointChannels = anm->RefJointChannels();

  HeaderStream->AddItem(int(JointChannels.size()));
  for (XgmAnim::JointChannelsMap::const_iterator it = JointChannels.begin(); it != JointChannels.end(); it++) {
    const PoolString& ChannelName          = it->first;
    const PoolString& ChannelUsage         = it->second->GetUsageSemantic();
    const XgmMatrixAnimChannel* MtxChannel = rtti::autocast(it->second);
    const PoolString& ObjectName           = MtxChannel->GetObjectName();

    int idataoffset = AnimDataStream->GetSize();

    if (MtxChannel) {
      for (int ifr = 0; ifr < inumframes; ifr++) {
        const fmtx4& Matrix = MtxChannel->GetFrame(ifr);
        AnimDataStream->AddItem(Matrix);
      }
    }

    int ichnclas = chunkwriter.stringIndex(it->second->GetClass()->Name().c_str());
    int iobjname = chunkwriter.stringIndex(ObjectName.c_str());
    int ichnname = chunkwriter.stringIndex(ChannelName.c_str());
    int iusgname = chunkwriter.stringIndex(ChannelUsage.c_str());

    printf("XGAOUT channelname<%s>\n", ChannelName.c_str());
    HeaderStream->AddItem(ichnclas);
    HeaderStream->AddItem(iobjname);
    HeaderStream->AddItem(ichnname);
    HeaderStream->AddItem(iusgname);
    HeaderStream->AddItem(idataoffset);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////

  const XgmAnim::MaterialChannelsMap& MaterialChannels = anm->RefMaterialChannels();

  HeaderStream->AddItem(int(MaterialChannels.size()));
  for (XgmAnim::MaterialChannelsMap::const_iterator it = MaterialChannels.begin(); it != MaterialChannels.end(); it++) {
    const PoolString& ChannelName  = it->first;
    const PoolString& ChannelUsage = it->second->GetUsageSemantic();
    const PoolString& ObjectName   = it->second->GetObjectName();

    auto MtxChannel  = std::dynamic_pointer_cast<XgmMatrixAnimChannel>(it->second);
    auto F32Channel  = std::dynamic_pointer_cast<XgmFloatAnimChannel>(it->second);
    auto Vec3Channel = std::dynamic_pointer_cast<XgmVect3AnimChannel>(it->second);

    int idataoffset = AnimDataStream->GetSize();

    if (MtxChannel) {
      for (int ifr = 0; ifr < inumframes; ifr++) {
        const fmtx4& Matrix = MtxChannel->GetFrame(ifr);
        AnimDataStream->AddItem(Matrix);
      }
    } else if (F32Channel) {
      for (int ifr = 0; ifr < inumframes; ifr++) {
        const float& value = F32Channel->GetFrame(ifr);
        AnimDataStream->AddItem(value);
      }
    } else if (Vec3Channel) {
      for (int ifr = 0; ifr < inumframes; ifr++) {
        const fvec3& value = Vec3Channel->GetFrame(ifr);
        AnimDataStream->AddItem(value);
      }
    }

    int ichnclas = chunkwriter.stringIndex(it->second->GetClass()->Name().c_str());
    int iobjname = chunkwriter.stringIndex(ObjectName.c_str());
    int ichnname = chunkwriter.stringIndex(ChannelName.c_str());
    int iusgname = chunkwriter.stringIndex(ChannelUsage.c_str());
    HeaderStream->AddItem(ichnclas);
    HeaderStream->AddItem(iobjname);
    HeaderStream->AddItem(ichnname);
    HeaderStream->AddItem(iusgname);
    HeaderStream->AddItem(idataoffset);
  }

  ///////////////////////////////////
  // write out pose information

  int inumposebones = (int)anm->_pose.size();

  HeaderStream->AddItem(inumposebones);

  for (auto it : anm->_pose ) {
    const PoolString& name = it.first;
    const fmtx4& mtx = it.second;

    // int idataoffset = AnimDataStream->GetSize();
    int ichannelname = chunkwriter.stringIndex(name.c_str());

    HeaderStream->AddItem(ichannelname);
    // HeaderStream->AddItem( idataoffset );
    HeaderStream->AddItem(mtx);
  }

  ////////////////////////////////////////////////////////////////////////////////////

  datablock_ptr_t out_datablock = std::make_shared<DataBlock>();
  chunkwriter.writeToDataBlock(out_datablock);

  ////////////////////////////////////////////////////////////////////////////////////

  return out_datablock;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::lev2
