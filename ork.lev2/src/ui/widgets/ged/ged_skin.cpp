////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/lev2/ui/ged/ged.h>
#include <ork/lev2/ui/ged/ged_node.h>
#include <ork/lev2/ui/ged/ged_skin.h>
#include <ork/lev2/ui/ged/ged_container.h>
#include <ork/lev2/ui/ged/ged_surface.h>
#include <ork/kernel/core_interface.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/lev2/gfx/gfxmaterial_ui.h>
#include <ork/math/misc_math.h>
//#include <ork/reflect/properties/ObjectProperty.h>

//#include <orktool/ged/ged.h>
//#include <orktool/ged/ged_delegate.h>
//#include <orktool/qtui/qtmainwin.h>

////////////////////////////////////////////////////////////////
namespace ork::lev2::ged {
////////////////////////////////////////////////////////////////

static std::set<void*>& GetObjSet() {
  static std::set<void*> gObjSet;
  return gObjSet;
}
void ClearObjSet() {
  GetObjSet().clear();
}
void AddToObjSet(void* pobj) {
  GetObjSet().insert(pobj);
}
bool IsObjInSet(void* pobj) {
  bool rval = false;
  rval      = (GetObjSet().find(pobj) != GetObjSet().end());
  return rval;
}

using vtx_t = SVtxV16T16C16 ;

GedSkin::GedSkin()
    : _scrollY(0)
    , _gedVP(nullptr)
    , _font(nullptr)
    , _char_w(0)
    , _char_h(0) {
}

void GedSkin::gpuInit(lev2::Context* ctx) {
  _material = std::make_shared<FreestyleMaterial>();
  _material->gpuInit(ctx, "orkshader://ui2");
  _tekpick     = _material->technique("ui_picking");
  _tekvtxcolor = _material->technique("ui_vtxcolor");
  _tekvtxpick  = _material->technique("ui_vtxpicking");
  _tekmodcolor = _material->technique("ui_modcolor");
  _parmvp      = _material->param("mvp");
  _parmodcolor = _material->param("modcolor");
  _parobjid    = _material->param("objid");
  _material->dump();
}

struct GedText {
  typedef ork::ArrayString<128> StringType;

  int ix = 0;
  int iy = 0;
  StringType mString;
};

//'ork::tool::ged::GedSkin::PrimContainer *' to
//'ork::tool::ged::GedSkin::PrimContainer *'

void GedSkin::AddPrim(const GedPrim& cb) {
  int isort                   = calcsort(cb.miSortKey);
  PrimContainers::iterator it = mPrimContainers.find(isort);
  if (it == mPrimContainers.end()) {
    PrimContainer* pcontainer = mPrimContainerPool.allocate();
    OrkAssert(pcontainer != 0);
    it = mPrimContainers.AddSorted(isort, pcontainer);
  }

  PrimContainer* pctr = it->second;
  if (cb.mDrawCB) {
    GedPrim* pooledprim = pctr->mPrimPool.allocate();
    *pooledprim         = cb;
    pctr->mCustomPrims.push_back(pooledprim);
  } else
    switch (cb.meType) {
      case PrimitiveType::LINES: {
        GedPrim* pooledprim = pctr->mPrimPool.allocate();
        *pooledprim         = cb;
        pctr->mLinePrims.push_back(pooledprim);
        break;
      }
      case PrimitiveType::QUADS: {
        GedPrim* pooledprim = pctr->mPrimPool.allocate();
        *pooledprim         = cb;
        pctr->mQuadPrims.push_back(pooledprim);
        break;
      }
    }
}

void GedSkin::PrimContainer::clear() {
  mPrimPool.clear();
  mLinePrims.clear();
  mQuadPrims.clear();
  mCustomPrims.clear();
}

void GedSkin::clear() {
  for (int i = 0; i < int(mPrimContainerPool.capacity()); i++) {
    mPrimContainerPool.direct_access(i).clear();
  }
  mPrimContainerPool.clear();
  mPrimContainers.clear();
}

///////////////////////////////////////////////////////////////////////////////
struct GedSkin0 : public GedSkin { ///////////////////////////////////////////////////////////////////
  bool _is_pickmode = false;
  orkvector<GedText> _text_vect;
  GedSkin0(ork::lev2::Context* ctx)
      : GedSkin() {
    gpuInit(ctx);
    _font  = lev2::FontMan::GetFont("i14");
    _char_w = _font->GetFontDesc().miAdvanceWidth;
    _char_h = _font->GetFontDesc().miAdvanceHeight;
  }
  ///////////////////////////////////////////////////////////////////
  fvec4 GetStyleColor(GedObject* pnode, ESTYLE ic) {
   // GedMapNode* pmapnode = rtti::autocast(pnode);

    bool bismapnode = false;//(pmapnode != 0);

    fvec3 color;
    bool bsc        = true;
    bool balternate = true;
    switch (ic) {
      case ESTYLE_BACKGROUND_1:
        color = bismapnode ? fvec3(0.8f, 0.8f, 0.78f) : fvec3(0.6f, 0.6f, 0.58f);
        break;
      case ESTYLE_BACKGROUND_2:
        color = fvec3(0.9f, 0.9f, 0.88f);
        break;
      case ESTYLE_BACKGROUND_3:
        color = fvec3(1.0f, 0.7f, 0.7f);
        break;
      case ESTYLE_BACKGROUND_4:
        color = fvec3(0.8f, 0.5f, 0.8f);
        break;
      case ESTYLE_BACKGROUND_5:
        color = fvec3(0.8f, 0.8f, 0.5f);
        break;
      case ESTYLE_BACKGROUND_OPS:
        color      = fvec3(0.8f, 0.4f, 0.4f);
        balternate = false;
        break;
      case ESTYLE_BACKGROUND_OBJNODE_LABEL:
        color      = fvec3(0.4f, 0.4f, 1.0f);
        balternate = false;
        break;
      case ESTYLE_BACKGROUND_GROUP_LABEL:
        color      = fvec3(0.5f, 0.5f, 0.7f);
        balternate = false;
        break;
      case ESTYLE_BACKGROUND_MAPNODE_LABEL:
        color      = fvec3(0.7f, 0.5f, 0.5f);
        balternate = false;
        break;
      case ESTYLE_DEFAULT_HIGHLIGHT:
        color = fvec3(0.85f, 0.82f, 0.8f);
        break;
      case ESTYLE_DEFAULT_OUTLINE:
        color = fvec3(0.5f, 0.5f, 0.5f);
        break;
      case ESTYLE_DEFAULT_CHECKBOX:
        color = fvec3(0.0f, 0.0f, 0.0f);
        bsc   = false;
        break;
      case ESTYLE_BUTTON_OUTLINE:
        color = fvec3(0.0f, 0.0f, 0.0f);
        bsc   = false;
        break;
      default:
        color = fvec3(1.0f, 0.0f, 0.0f);
        break;
    }

    float fdepth = (bsc ? float(pnode->GetDepth()) / 16.0f : 1.0f);

    if (fdepth > 0.9f)
      fdepth = 0.9f;

    fdepth = (1.0f - fdepth);

    fdepth = powf(fdepth, 0.20f);

    int icidx = pnode->GetDecoIndex();

    if (balternate && icidx & 1) {
      float fg = color.y;
      float fb = color.z;
      color.y = fb;
      color.z = fg;
    }

    return (color * fdepth);
  }
  ///////////////////////////////////////////////////////////////////
  void DrawBgBox(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic, int isort) {
    GedPrim prim;
    prim.ix1 = ix;
    prim.ix2 = ix + iw;
    prim.iy1 = iy;
    prim.iy2 = iy + ih;

    fvec4 uobj = _gedVP->AssignPickId(pnode);

    if (_is_pickmode) {
      AddToObjSet((void*)pnode);
      // printf( "insert obj<%p>\n", (void*) pnode );
    }

    prim._ucolor   = _is_pickmode ? uobj : GetStyleColor(pnode, ic); // Default Outline
    prim.meType    = PrimitiveType::QUADS;
    prim.miSortKey = calcsort(isort);
    AddPrim(prim);
  }
  ///////////////////////////////////////////////////////////////////
  void DrawOutlineBox(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic, int isort) {
    if (false == _is_pickmode) {
      GedPrim prim;
      prim._ucolor   = GetStyleColor(pnode, ic);
      prim.meType    = PrimitiveType::LINES;
      prim.miSortKey = calcsort(isort + 1);

      prim.ix1 = ix;
      prim.ix2 = ix + iw;
      prim.iy1 = iy;
      prim.iy2 = iy;
      AddPrim(prim);

      prim.ix1 = ix + iw;
      prim.ix2 = ix + iw;
      prim.iy1 = iy;
      prim.iy2 = iy + ih;
      AddPrim(prim);

      prim.ix1 = ix + iw;
      prim.ix2 = ix;
      prim.iy1 = iy + ih;
      prim.iy2 = iy + ih;
      AddPrim(prim);

      prim.ix1 = ix;
      prim.ix2 = ix;
      prim.iy1 = iy + ih;
      prim.iy2 = iy;
      AddPrim(prim);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void DrawLine(GedObject* pnode, int ix, int iy, int ix2, int iy2, ESTYLE ic) {
    if (false == _is_pickmode) {
      GedPrim prim;
      prim._ucolor   = GetStyleColor(pnode, ic);
      prim.meType    = PrimitiveType::LINES;
      prim.ix1       = ix;
      prim.ix2       = ix2;
      prim.iy1       = iy;
      prim.iy2       = iy2;
      prim.miSortKey = calcsort(4);
      AddPrim(prim);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void DrawCheckBox(GedObject* pnode, int ix, int iy, int iw, int ih) {
    if (false == _is_pickmode) {
      int ixi  = ix + 2;
      int iyi  = iy + 2;
      int ixi2 = ix + iw - 2;
      int iyi2 = iy + ih - 2;
      DrawLine(pnode, ixi, iyi, ixi2, iyi2, ESTYLE_DEFAULT_CHECKBOX);
      DrawLine(pnode, ixi2, iyi, ixi, iyi2, ESTYLE_DEFAULT_CHECKBOX);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void DrawDownArrow(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic) {
    DrawLine(pnode, ix, iy, ix + iw, iy, GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix, iy, ix + (iw >> 1), iy + ih, GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix + iw, iy, ix + (iw >> 1), iy + ih, GedSkin::ESTYLE_BUTTON_OUTLINE);
  }
  ///////////////////////////////////////////////////////////////////
  void DrawRightArrow(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic) {
    DrawLine(pnode, ix, iy, ix, iy + ih, GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix, iy, ix + iw, iy + (ih >> 1), GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix, iy + ih, ix + iw, iy + (ih >> 1), GedSkin::ESTYLE_BUTTON_OUTLINE);
  }
  ///////////////////////////////////////////////////////////////////
  virtual void DrawText(GedObject* pnode, int ix, int iy, const char* ptext) {
    if (false == _is_pickmode) {
      GedText text;
      text.ix      = ix;
      text.iy      = iy;
      text.mString = GedText::StringType(ptext);
      _text_vect.push_back(text);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void Begin(Context* pTARG, GedSurface* pVP) {
    _is_pickmode     = pTARG->FBI()->isPickState();
    _gedVP = pVP;
    _text_vect.clear();
    clear();
    ClearObjSet();
  }
  ///////////////////////////////////////////////////////////////////
  void End(Context* pTARG) {
    int iw                               = _gedVP->width();
    int ih                               = _gedVP->height();
    miRejected                           = 0;
    miAccepted                           = 0;
    lev2::DynamicVertexBuffer<vtx_t>& VB = lev2::GfxEnv::GetSharedDynamicV16T16C16();
    ////////////////////////
    ork::fmtx4 mtxW = pTARG->MTXI()->RefMMatrix();
    pTARG->MTXI()->PushUIMatrix(iw, ih);
    pTARG->MTXI()->PushMMatrix(mtxW);
    auto uimatrix = pTARG->MTXI()->uiMatrix(iw, ih);
    ////////////////////////
    // pTARG->IMI()->QueFlush();
    RenderContextFrameData RCFD(pTARG);
    for (auto itc : mPrimContainers) {
      const PrimContainer* primcontainer = itc.second;

      int inumlines = (int)primcontainer->mLinePrims.size();
      int inumquads = (int)primcontainer->mQuadPrims.size();
      int inumcusts = (int)primcontainer->mCustomPrims.size();

      // uimat.SetUIColorMode(UiColorMode::VTX);
      // uimat._rasterstate.SetBlending(lev2::Blending::OFF);

      const float fZ = 0.0f;

      ///////////////////////////////////////////////////////////
      // int ivbase = VB.GetNum();
      int icount = 0;
      lev2::VtxWriter<vtx_t> vw;
      vw.Lock(pTARG, &VB, inumquads * 6);
      {
        for (int i = 0; i < inumquads; i++) {
          const GedPrim* prim = primcontainer->mQuadPrims[i];
          if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
            // printf( "DrawQuad ix1<%d> iy1<%d> ix2<%d> iy2<%d>\n", prim->ix1, prim->iy1, prim->ix2, prim->iy2 );

            vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy1, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy1, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy2, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy1, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy2, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy2, fZ), fvec4(), prim->_ucolor));
            icount += 6;
          }
        }
      }
      vw.UnLock(pTARG);

      _material->begin(_is_pickmode ? _tekvtxpick : _tekvtxcolor, RCFD);
      _material->bindParamMatrix(_parmvp, uimatrix);
      pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::TRIANGLES);
      _material->end(RCFD);
      icount = 0;
      // ivbase += inumquads*6;
      ///////////////////////////////////////////////////////////
      if (false == _is_pickmode) {
        for (int i = 0; i < inumcusts; i++) {
          const GedPrim* prim = primcontainer->mCustomPrims[i];
          if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
            prim->mDrawCB(this, prim->mpNode, pTARG);
          }
        }
      }
      if (false == _is_pickmode) {
        if (inumlines) {
          icount = 0;
          vw.Lock(pTARG, &VB, inumlines * 2);
          {
            for (int i = 0; i < inumlines; i++) {
              const GedPrim* prim = primcontainer->mLinePrims[i];
              if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
                vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy1, fZ), fvec4(), prim->_ucolor));
                vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy2, fZ), fvec4(), prim->_ucolor));
                icount += 2;
              }
            }
          }
          vw.UnLock(pTARG);
          if (icount) {
            _material->begin(_is_pickmode ? _tekvtxpick : _tekvtxcolor, RCFD);
            _material->bindParamMatrix(_parmvp, uimatrix);
            pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::LINES);
            _material->end(RCFD);
          }
        }
      }
    }
    ///////////////////////////////////////////////////////////
    ////////////////////////
    if (false == _is_pickmode) { ////////////////////////
      lev2::GfxMaterialUI uimat(pTARG);
      uimat.SetUIColorMode(UiColorMode::MOD);
      // pTARG->PushModColor(fcolor4(0.0f,0.0f,0.2f));
      pTARG->PushModColor(fcolor4::Black());
      lev2::FontMan::PushFont(_font);
      lev2::FontMan::beginTextBlock(pTARG);
      {
        int inumt = (int)_text_vect.size();
        for (int it = 0; it < inumt; it++) {
          const GedText& text = _text_vect[it];
          if (IsVisible(pTARG, text.iy, text.iy + 8)) {
            lev2::FontMan::DrawText(pTARG, text.ix, text.iy, text.mString.c_str());
          }
        }
      }
      lev2::FontMan::endTextBlock(pTARG);
      lev2::FontMan::PopFont();
      pTARG->PopModColor();
    }
    ////////////////////////
    pTARG->MTXI()->PopMMatrix();
    pTARG->MTXI()->PopUIMatrix();
    _gedVP = 0;
  }
};
///////////////////////////////////////////////////////////////////////////////
struct GedSkin1 : public GedSkin { ///////////////////////////////////////////////////////////////////
  bool _is_pickmode = false;
  orkvector<GedText> _text_vect;
  ///////////////////////////////////////////////////////////////////
  GedSkin1(ork::lev2::Context* ctx) {
    gpuInit(ctx);
    _font  = lev2::FontMan::GetFont("i14");
    _char_w = _font->GetFontDesc().miAdvanceWidth;
    _char_h = _font->GetFontDesc().miAdvanceHeight;
  }
  ///////////////////////////////////////////////////////////////////
  fvec4 GetStyleColor(GedObject* pnode, ESTYLE ic) {
    //GedMapNode* pmapnode = rtti::autocast(pnode);

    bool bismapnode = false; //(pmapnode != 0);

    fvec3 color;
    bool bsc        = true;
    bool balternate = true;
    switch (ic) {
      case ESTYLE_BACKGROUND_1:
        color = bismapnode ? fvec3(0.2f, 0.2f, 0.3f) : fvec3(0.4f, 0.4f, 0.5f);
        break;
      case ESTYLE_BACKGROUND_2:
        color = fvec3(0.0f, 0.0f, 0.0f);
        break;
      case ESTYLE_BACKGROUND_3:
        color = fvec3(0.0f, 0.0f, 0.3f);
        break;
      case ESTYLE_BACKGROUND_4:
        color = fvec3(0.3f, 0.0f, 0.3f);
        break;
      case ESTYLE_BACKGROUND_5:
        color = fvec3(0.7f, 0.0f, 0.0f);
        break;
      case ESTYLE_BACKGROUND_OPS:
        color      = fvec3(0.8f, 0.4f, 0.4f);
        balternate = false;
        break;
      case ESTYLE_BACKGROUND_OBJNODE_LABEL:
        color      = fvec3(0.4f, 0.4f, 1.0f);
        balternate = false;
        break;
      case ESTYLE_BACKGROUND_GROUP_LABEL:
        color      = fvec3(0.5f, 0.5f, 0.7f);
        balternate = false;
        break;
      case ESTYLE_BACKGROUND_MAPNODE_LABEL:
        color      = fvec3(0.7f, 0.5f, 0.5f);
        balternate = false;
        break;
      case ESTYLE_DEFAULT_HIGHLIGHT:
        color = fvec3(0.1f, 0.2f, 0.3f);
        break;
      case ESTYLE_DEFAULT_OUTLINE:
        color = fvec3(0.7f, 0.7f, 0.7f);
        break;
      case ESTYLE_BUTTON_OUTLINE:
        color      = fvec3(0.6f, 0.6f, 0.7f);
        balternate = false;
        bsc        = false;
        break;
      case ESTYLE_DEFAULT_CHECKBOX:
        color = fvec3(0.6f, 0.6f, 0.7f);
        bsc   = false;
        break;
      default:
        color = fvec3(0.0f, 0.0f, 0.0f);
        break;
    }

    float fdepth = bsc ? float(pnode->GetDepth()) / 12.0f : 1.0f;

    fdepth = powf(fdepth, 0.5f);

    int icidx = pnode->GetDecoIndex();

    if (balternate && icidx & 1) {
      float fg = color.y;
      float fb = color.z;
      color.y = fb;
      color.z = fg;
    }

    return (color * fdepth);
  }
  ///////////////////////////////////////////////////////////////////
  void DrawBgBox(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic, int isort) {
    GedPrim prim;
    prim.ix1 = ix;
    prim.ix2 = ix + iw;
    prim.iy1 = iy;
    prim.iy2 = iy + ih;

    fvec4 uobj = _gedVP->AssignPickId(pnode);

    if (_is_pickmode) {
      AddToObjSet((void*)pnode);
      // printf( "insert obj<%p>\n", (void*) pnode );
    }

    prim._ucolor   = _is_pickmode ? uobj : GetStyleColor(pnode, ic); // Default Outline
    prim.meType    = PrimitiveType::QUADS;
    prim.miSortKey = calcsort(isort);
    AddPrim(prim);
  }
  ///////////////////////////////////////////////////////////////////
  void DrawOutlineBox(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic, int isort) {
    if (false == _is_pickmode) {
      GedPrim prim;
      prim._ucolor   = GetStyleColor(pnode, ic);
      prim.meType    = PrimitiveType::LINES;
      prim.miSortKey = calcsort(isort + 1);

      prim.ix1 = ix;
      prim.ix2 = ix + iw;
      prim.iy1 = iy;
      prim.iy2 = iy;
      AddPrim(prim);

      prim.ix1 = ix + iw;
      prim.ix2 = ix + iw;
      prim.iy1 = iy;
      prim.iy2 = iy + ih;
      AddPrim(prim);

      prim.ix1 = ix + iw;
      prim.ix2 = ix;
      prim.iy1 = iy + ih;
      prim.iy2 = iy + ih;
      AddPrim(prim);

      prim.ix1 = ix;
      prim.ix2 = ix;
      prim.iy1 = iy + ih;
      prim.iy2 = iy;
      AddPrim(prim);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void DrawLine(GedObject* pnode, int ix, int iy, int ix2, int iy2, ESTYLE ic) {
    if (false == _is_pickmode) {
      GedPrim prim;
      prim._ucolor   = GetStyleColor(pnode, ic);
      prim.meType    = PrimitiveType::LINES;
      prim.ix1       = ix;
      prim.ix2       = ix2;
      prim.iy1       = iy;
      prim.iy2       = iy2;
      prim.miSortKey = calcsort(4);
      AddPrim(prim);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void DrawCheckBox(GedObject* pnode, int ix, int iy, int iw, int ih) {
    if (false == _is_pickmode) {
      int ixi  = ix + 2;
      int iyi  = iy + 2;
      int ixi2 = ix + iw - 2;
      int iyi2 = iy + ih - 2;
      DrawLine(pnode, ixi, iyi, ixi2, iyi2, ESTYLE_DEFAULT_CHECKBOX);
      DrawLine(pnode, ixi2, iyi, ixi, iyi2, ESTYLE_DEFAULT_CHECKBOX);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void DrawDownArrow(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic) {
    DrawLine(pnode, ix, iy, ix + iw, iy, GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix, iy, ix + (iw >> 1), iy + ih, GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix + iw, iy, ix + (iw >> 1), iy + ih, GedSkin::ESTYLE_BUTTON_OUTLINE);
  }
  ///////////////////////////////////////////////////////////////////
  void DrawRightArrow(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic) {
    DrawLine(pnode, ix, iy, ix, iy + ih, GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix, iy, ix + iw, iy + (ih >> 1), GedSkin::ESTYLE_BUTTON_OUTLINE);
    DrawLine(pnode, ix, iy + ih, ix + iw, iy + (ih >> 1), GedSkin::ESTYLE_BUTTON_OUTLINE);
  }
  ///////////////////////////////////////////////////////////////////
  virtual void DrawText(GedObject* pnode, int ix, int iy, const char* ptext) {
    if (false == _is_pickmode) {
      GedText text;
      text.ix      = ix;
      text.iy      = iy;
      text.mString = GedText::StringType(ptext);
      _text_vect.push_back(text);
    }
  }
  ///////////////////////////////////////////////////////////////////
  void Begin(Context* pTARG, GedSurface* pVP) {
    _is_pickmode     = pTARG->FBI()->isPickState();
    _gedVP = pVP;
    _text_vect.clear();
    clear();
    ClearObjSet();
  }
  ///////////////////////////////////////////////////////////////////
  void End(Context* pTARG) {
    int iw                               = _gedVP->width();
    int ih                               = _gedVP->height();
    miRejected                           = 0;
    miAccepted                           = 0;
    lev2::DynamicVertexBuffer<vtx_t>& VB = lev2::GfxEnv::GetSharedDynamicV16T16C16();
    ////////////////////////
    const ork::fmtx4& mtxW = pTARG->MTXI()->RefMMatrix();
    auto uimatrix          = pTARG->MTXI()->uiMatrix(iw, ih);
    pTARG->MTXI()->PushUIMatrix(iw, ih);
    pTARG->MTXI()->PushMMatrix(mtxW);
    ////////////////////////
    RenderContextFrameData RCFD(pTARG);
    ////////////////////////
    for (auto itc : mPrimContainers) {
      const PrimContainer* primcontainer = itc.second;

      int inumlines = (int)primcontainer->mLinePrims.size();
      int inumquads = (int)primcontainer->mQuadPrims.size();
      int inumcusts = (int)primcontainer->mCustomPrims.size();

      const float fZ = 0.0f;

      ///////////////////////////////////////////////////////////
      // int ivbase = VB.GetNum();

      int icount = 0;
      lev2::VtxWriter<vtx_t> vw;
      vw.Lock(pTARG, &VB, inumquads * 6);
      {
        for (int i = 0; i < inumquads; i++) {
          const GedPrim* prim = primcontainer->mQuadPrims[i];
          if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
            vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy1, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy1, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy2, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy1, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy2, fZ), fvec4(), prim->_ucolor));
            vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy2, fZ), fvec4(), prim->_ucolor));
            icount += 6;
          }
        }
      }
      vw.UnLock(pTARG);

      _material->begin(_is_pickmode ? _tekvtxpick : _tekvtxcolor, RCFD);
      _material->bindParamMatrix(_parmvp, mtxW * uimatrix);
      pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::TRIANGLES);
      _material->end(RCFD);
      icount = 0;
      // ivbase += inumquads*6;
      ///////////////////////////////////////////////////////////
      if (false == _is_pickmode) {
        for (int i = 0; i < inumcusts; i++) {
          const GedPrim* prim = primcontainer->mCustomPrims[i];
          if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
            prim->mDrawCB(this, prim->mpNode, pTARG);
          }
        }
      }
      if (false == _is_pickmode) {
        if (inumlines) {
          icount = 0;
          vw.Lock(pTARG, &VB, inumlines * 2);
          {
            for (int i = 0; i < inumlines; i++) {
              const GedPrim* prim = primcontainer->mLinePrims[i];
              if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
                vw.AddVertex(vtx_t(fvec4(prim->ix1, prim->iy1, fZ), fvec4(), prim->_ucolor));
                vw.AddVertex(vtx_t(fvec4(prim->ix2, prim->iy2, fZ), fvec4(), prim->_ucolor));
                icount += 2;
              }
            }
          }
          vw.UnLock(pTARG);
          if (icount) {
            _material->begin(_is_pickmode ? _tekvtxpick : _tekvtxcolor, RCFD);
            _material->bindParamMatrix(_parmvp, mtxW * uimatrix);
            pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::LINES);
            _material->end(RCFD);
          }
        }
      }
    }
    ///////////////////////////////////////////////////////////
    if (not _is_pickmode) {
      //lev2::GfxMaterialUI uimat(pTARG);
      //uimat.SetUIColorMode(UiColorMode::MOD);
      lev2::FontMan::PushFont(_font);
      lev2::FontMan::beginTextBlock(pTARG);
      pTARG->PushModColor(fcolor4(0.8f, 0.9f, 1.0f));
      {
        int inumt = (int)_text_vect.size();
        for (int it = 0; it < inumt; it++) {
          const GedText& text = _text_vect[it];
          if (IsVisible(pTARG, text.iy, text.iy + 8)) {
            lev2::FontMan::DrawText(pTARG, text.ix, text.iy, text.mString.c_str());
          }
        }
      }
      lev2::FontMan::endTextBlock(pTARG);
      lev2::FontMan::PopFont();
      pTARG->PopModColor();
    }
    ////////////////////////
    pTARG->MTXI()->PopMMatrix();
    pTARG->MTXI()->PopUIMatrix();
    _gedVP = 0;
  }
};
////////////////////////////////////////////////////////////////
orkvector<GedSkin*> instantiateSkins(ork::lev2::Context* ctx) {
  orkvector<GedSkin*> skins;
  auto skin0 = new GedSkin0(ctx);
  auto skin1 = new GedSkin1(ctx);
  skins.push_back(skin0);
  skins.push_back(skin1);
    return skins;
}

////////////////////////////////////////////////////////////////
} //namespace ork::lev2::ged {
////////////////////////////////////////////////////////////////
