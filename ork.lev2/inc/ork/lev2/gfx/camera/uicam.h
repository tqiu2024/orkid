////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////////
#include <ork/lev2/gfx/camera/cameradata.h>
#include <ork/kernel/core/singleton.h>
#include <ork/lev2/ui/ui.h>
#include <ork/util/hotkey.h>

namespace ork { namespace lev2 {

#define DEF_EYEZ -500.0f
#define DEF_DEPTH 1000.0f

///////////////////////////////////////////////////////////////////////////////

class ManipHandler {
public: //
  fmtx4 IMVPMat;
  fquat Quat;
  fvec3 Origin;
  fvec3 RayNear;
  fvec3 RayFar;
  fvec3 RayNormal;

  fvec3 XNormal;
  fvec3 YNormal;
  fvec3 ZNormal;

  fplane3 XYPlane;
  fplane3 XZPlane;
  fplane3 YZPlane;

  fvec3 XYIntersect;
  fvec3 XZIntersect;
  fvec3 YZIntersect;
  fvec3 XYIntersectBase;
  fvec3 XZIntersectBase;
  fvec3 YZIntersectBase;

  float XYDistance;
  float XZDistance;
  float YZDistance;

  bool DoesIntersectXY;
  bool DoesIntersectXZ;
  bool DoesIntersectYZ;

  float XZAngle;
  float YZAngle;
  float XYAngle;

  float XZAngleBase;
  float YZAngleBase;
  float XYAngleBase;

  ///////////////////////////////////////////////////////////////////////////////
  //

  fvec3 CamXNormal;
  fvec3 CamYNormal;
  fvec3 CamZNormal;
  Frustum mFrustum;
  fvec3 camrayN, camrayF;

  ///////////////////////////////////////////////////////////////////////////////

  ManipHandler();

  void Init(const ork::fvec2& posubp, const fmtx4& RCurIMVPMat, const fquat& RCurQuat);
  bool IntersectXZ(const ork::fvec2& posubp, fvec3& Intersection, float& Angle);
  bool IntersectYZ(const ork::fvec2& posubp, fvec3& Intersection, float& Angle);
  bool IntersectXY(const ork::fvec2& posubp, fvec3& Intersection, float& Angle);
  void Intersect(const ork::fvec2& posubp);
  void GenerateIntersectionRays(const ork::fvec2& posubp, fvec3& RayZNormal, fvec3& RayNear);

  ///////////////////////////////////////////////////////////////////////////////
};

class UiCamera : public ork::Object {
  RttiDeclareAbstract(UiCamera, ork::Object);

protected:
  std::string type_name;
  std::string instance_name;
  std::string other_info;

public:
  /////////////////////////////////////////////////////////////////////
  UiCamera();
  /////////////////////////////////////////////////////////////////////

  CameraData _camcamdata;
  CameraMatrices _curMatrices;
  fvec2 _vpdim;

  float mfWorldSizeAtLocator;

  fmtx4 mCamRot;

  fquat QuatC, QuatL, QuatCPushed;

  fmtx4 lookatmatrix;
  fmtx4 eyematrixROT;
  fmtx4 clipmatrix;

  fvec3 camrayN, camrayF;
  fvec4 CamSet;
  fvec3 CamLoc;
  fvec3 PrevCamLoc;
  fvec4 MeasuredCameraVelocity;
  fvec4 LastMeasuredCameraVelocity;

  fvec4 vec_billboardUp;
  fvec4 vec_billboardRight;

  float mfLoc;

  fvec3 mvCenter;

  float locscale;

  ManipHandler _manipHandler;

  bool mbInMotion;

  //////////////////////////////////////////////////////////////////////////////

  bool IsXVertical() const;
  bool IsYVertical() const;
  bool IsZVertical() const;

  fquat VerticalRot(float amt) const;
  fquat HorizontalRot(float amt) const;

  //////////////////////////////////////////////////////////////////////////////

  CameraData& cameraMatrices() {
    return _camcamdata;
  }
  const CameraData& cameraMatrices() const {
    return _camcamdata;
  }

  //////////////////////////////////////////////////////////////////////////////

  virtual bool UIEventHandler(const ui::Event& EV) = 0;
  virtual void draw(Context* pT)                   = 0;

  virtual void SetFromWorldSpaceMatrix(const fmtx4&) = 0;

  virtual float ViewLengthToWorldLength(const fvec4& pos, float ViewLength)                            = 0;
  virtual void GenerateDepthRay(const fvec2& pos2D, fvec3& rayN, fvec3& rayF, const fmtx4& IMat) const = 0;

  std::string get_full_name();

  void CommonPostSetup(void);

  void SetName(const std::string& Name) {
    instance_name = Name;
  }
  const std::string& GetName() const {
    return instance_name;
  }
};

///////////////////////////////////////////////////////////////////////////////

struct CamEvTrackData {
  int ipushX;
  int ipushY;
  int icurX;
  int icurY;

  fvec3 vPushCenter;
};

///////////////////////////////////////////////////////////////////////////////

class EzUiCam : public UiCamera {
  RttiDeclareConcrete(EzUiCam, UiCamera);

public: //
  enum erotmode {
    EROT_SCREENXY = 0,
    EROT_SCREENZ,
  };

  erotmode meRotMode;

  CamEvTrackData mEvTrackData;

  float aper;
  float tx, ty, tz;
  float far_max;
  float near_min;

  fmtx4 mRot, mTrans;

  fvec4 CamBaseLoc;

  int beginx, beginy;
  int leftbutton, middlebutton, rightbutton;

  bool mDoRotate;
  bool mDoDolly;
  bool mDoPan;

  fvec3 _pushNZ, _pushNX, _pushNY;
  svar32_t _private;
  
  bool UIEventHandler(const ui::Event& EV) final;
  void draw(Context* pT) final;

  void SetFromWorldSpaceMatrix(const fmtx4& matrix) final;

  float ViewLengthToWorldLength(const fvec4& pos, float ViewLength) final;
  void GenerateDepthRay(const fvec2& pos2D, fvec3& rayN, fvec3& rayF, const fmtx4& IMat) const final;

  void DrawBillBoardQuad(fvec4& inpt, float size);

  void updateMatrices(void);

  void PanBegin(const CamEvTrackData& ed);
  void PanUpdate(const CamEvTrackData& ed);
  void PanEnd();

  void RotBegin(const CamEvTrackData& ed);
  void RotEnd();

  void DollyBegin(const CamEvTrackData& ed);
  void DollyEnd();

  EzUiCam();
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2

///////////////////////////////////////////////////////////////////////////////
