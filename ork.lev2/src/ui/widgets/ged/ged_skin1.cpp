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
#include <ork/lev2/gfx/gfxvtxbuf.inl>

////////////////////////////////////////////////////////////////
namespace ork::lev2::ged {
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////

GedSkin1::GedSkin1(ork::lev2::Context* ctx)
  : GedSkin(ctx) {
  gpuInit(ctx);
  _font   = lev2::FontMan::GetFont("i14");
  _char_w = _font->GetFontDesc().miAdvanceWidth;
  _char_h = _font->GetFontDesc().miAdvanceHeight;
  _bannerHeight = _char_h + 4;
}
///////////////////////////////////////////////////////////////////
fvec4 GedSkin1::GetStyleColor(GedObject* pnode, ESTYLE ic) {
  // GedMapNode* pmapnode = rtti::autocast(pnode);

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
      color      = fvec3(0.8f, 0.8f, 0.9f);
      balternate = false;
      bsc        = false;
      break;
    case ESTYLE_DEFAULT_CHECKBOX:
      color = fvec3(0.6f, 0.6f, 0.7f);
      bsc   = false;
      break;
    case ESTYLE_CUSTOM_COLOR: {
      color      = _colorStack.top();
      balternate = false;
      bsc        = false;
      break;
    }
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
    color.y  = fb;
    color.z  = fg;
  }

  return fvec4((color * fdepth), 1);
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawBgBox(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic, int isort) {
  fvec4 color = GetStyleColor(pnode, ic);
  DrawColorBox(pnode, ix, iy, iw, ih, color, isort);
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawOutlineBox(GedObject* pnode, int ix, int iy, int iw, int ih, ESTYLE ic, int isort) {

  if (not _is_pickmode) {
    GedPrim prim;
    prim._ucolor   = GetStyleColor(pnode, ic);
    prim.meType    = PrimitiveType::LINES;
    prim.miSortKey = calcsort(isort + 1);

    int ix2 = ix + iw - 1;
    int iy2 = iy + ih - 1;

    // top
    prim.ix1 = ix;
    prim.ix2 = ix2+1;
    prim.iy1 = iy;
    prim.iy2 = iy;
    AddPrim(prim);

    // right
    prim.ix1 = ix2;
    prim.ix2 = ix2;
    prim.iy1 = iy;
    prim.iy2 = iy2+1;
    AddPrim(prim);

    // bottom
    prim.ix1 = ix;
    prim.ix2 = ix2+1;
    prim.iy1 = iy2;
    prim.iy2 = iy2;
    AddPrim(prim);

    // left
    prim.ix1 = ix;
    prim.ix2 = ix;
    prim.iy1 = iy;
    prim.iy2 = iy2+1;
    AddPrim(prim);
  }
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawLine(GedObject* pnode, int ix, int iy, int ix2, int iy2, ESTYLE ic) {
  if (not _is_pickmode) {
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
void GedSkin1::DrawCheckBox(GedObject* pnode, int ix, int iy, int iw, int ih) {
  if (not _is_pickmode) {
    int ixi  = ix + 2;
    int iyi  = iy + 2;
    int ixi2 = ix + iw - 2;
    int iyi2 = iy + ih - 2;
    DrawLine(pnode, ixi, iyi, ixi2, iyi2, ESTYLE_DEFAULT_CHECKBOX);
    DrawLine(pnode, ixi2, iyi, ixi, iyi2, ESTYLE_DEFAULT_CHECKBOX);
  }
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawDownArrow(GedObject* pnode, int ix, int iy, ESTYLE ic) {
  DrawTexBoxCrc( pnode, ix, iy, "downarrow"_crcu, ic );
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawRightArrow(GedObject* pnode, int ix, int iy, ESTYLE ic) {
  DrawTexBoxCrc( pnode, ix, iy, "rightarrow"_crcu, ic );
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawUpArrow(GedObject* pnode, int ix, int iy, ESTYLE ic) {
  DrawTexBoxCrc( pnode, ix, iy, "uparrow"_crcu, ic );
}
///////////////////////////////////////////////////////////////////
void GedSkin1::DrawText(GedObject* pnode, int ix, int iy, const char* ptext) {
  if (not _is_pickmode) {
    GedText text;
    text.ix      = ix;
    text.iy      = iy;
    text.mString = GedText::StringType(ptext);
    _text_vect.push_back(text);
  }
}
///////////////////////////////////////////////////////////////////
void GedSkin1::Begin(Context* pTARG, GedSurface* pVP) {
  _is_pickmode = pTARG->FBI()->isPickState();
  _gedVP       = pVP;
  _text_vect.clear();
  clear();
  ClearObjSet();
  _colorStack = std::stack<fcolor3>();
  _colorStack.push(fcolor3::White());
}
///////////////////////////////////////////////////////////////////
void GedSkin1::End(Context* pTARG) {
  int iw                               = _gedVP->width();
  int ih                               = _gedVP->height();
  miRejected                           = 0;
  miAccepted                           = 0;
  lev2::DynamicVertexBuffer<vtx_t>& VB = lev2::GfxEnv::GetSharedDynamicV16T16C16();
  ////////////////////////
  _mMatrix.setTranslation(0.0f, float(_scrollY), 0.0f);
  //_mMatrix.setTranslation(0.0f, 0.0f, 0.0f);
  _uiMatrix = pTARG->MTXI()->uiMatrix(iw, ih);

  _uiMVPMatrix = _uiMatrix * _mMatrix;

  pTARG->MTXI()->PushUIMatrix(iw, ih);
  pTARG->MTXI()->PushMMatrix(_mMatrix);
  ////////////////////////
  auto RCFD = std::make_shared<RenderContextFrameData>(pTARG);
  ////////////////////////
  for (auto itc : mPrimContainers) {
    const PrimContainer* primcontainer = itc.second;

    int inumlines = (int)primcontainer->mLinePrims.size();
    int inumquads = (int)primcontainer->mQuadPrims.size();
    int inumcusts = (int)primcontainer->mCustomPrims.size();

    const float fZ = 0.0f;
    const float fbias = +0.375f;
    //const float fbias = +0.5f;

    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    // render quad primitive
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////

    int icount = 0;
    lev2::VtxWriter<vtx_t> vw;
    vw.Lock(pTARG, &VB, inumquads * 6);
    {
      for (int i = 0; i < inumquads; i++) {
        const GedPrim* prim = primcontainer->mQuadPrims[i];
        if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
          vw.AddVertex(vtx_t(fvec4(prim->ix1+fbias, prim->iy1+fbias, fZ), fvec4(), prim->_ucolor));
          vw.AddVertex(vtx_t(fvec4(prim->ix2+fbias, prim->iy2+fbias, fZ), fvec4(), prim->_ucolor));
          vw.AddVertex(vtx_t(fvec4(prim->ix2+fbias, prim->iy1+fbias, fZ), fvec4(), prim->_ucolor));
          vw.AddVertex(vtx_t(fvec4(prim->ix1+fbias, prim->iy1+fbias, fZ), fvec4(), prim->_ucolor));
          vw.AddVertex(vtx_t(fvec4(prim->ix1+fbias, prim->iy2+fbias, fZ), fvec4(), prim->_ucolor));
          vw.AddVertex(vtx_t(fvec4(prim->ix2+fbias, prim->iy2+fbias, fZ), fvec4(), prim->_ucolor));
          icount += 6;
        }
      }
    }
    vw.UnLock(pTARG);

    _material->_rasterstate.SetRGBAWriteMask(true, true);
    _material->_rasterstate.SetDepthTest(EDepthTest::OFF);
    _material->begin(_tekvtxcolor, RCFD);
    _material->bindParamMatrix(_parmvp, _uiMVPMatrix);
    pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::TRIANGLES);
    _material->end(RCFD);
    icount = 0;

    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    // render texquad primitive
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////

    for( const auto& item : primcontainer->mTexQuadPrims ){
        auto texture = item.first;
        const auto& prim_vect = item.second;          
        int inumtexquads = (int)prim_vect.size();

        int texw = texture->_width;
        int texh = texture->_height;

        float fu0 = 0;
        float fv0 = 0;
        float fu1 = texw;
        float fv1 = texh;
        int icount = 0;
        lev2::VtxWriter<vtx_t> vw;
        vw.Lock(pTARG, &VB, inumtexquads * 6);
        {
          for (int i = 0; i < inumtexquads; i++) {
            const GedPrim* prim = prim_vect[i];

            float fx0 = prim->ix1 + fbias;
            float fx1 = prim->ix2 + fbias + 0.5f;
            float fy0 = prim->iy1 + fbias;
            float fy1 = prim->iy2 + fbias + 0.5f;

            auto V0 = fvec4(fx0, fy0, fZ);
            auto V1 = fvec4(fx1, fy1, fZ);
            auto V2 = fvec4(fx1, fy0, fZ);
            auto V3 = fvec4(fx0, fy1, fZ);
            auto T0 = fvec4(fu0, fv0, 0.0f, 0.0f);
            auto T1 = fvec4(fu1, fv1, 0.0f, 0.0f);
            auto T2 = fvec4(fu1, fv0, 0.0f, 0.0f);
            auto T3 = fvec4(fu0, fv1, 0.0f, 0.0f);
            if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
              vw.AddVertex(vtx_t(V0, T0, prim->_ucolor));
              vw.AddVertex(vtx_t(V1, T1, prim->_ucolor));
              vw.AddVertex(vtx_t(V2, T2, prim->_ucolor));
              vw.AddVertex(vtx_t(V0, T0, prim->_ucolor));
              vw.AddVertex(vtx_t(V3, T3, prim->_ucolor));
              vw.AddVertex(vtx_t(V1, T1, prim->_ucolor));
              icount += 6;
            }
          }
        }
        vw.UnLock(pTARG);

        _material->_rasterstate.SetRGBAWriteMask(true, true);
        _material->_rasterstate.SetDepthTest(EDepthTest::OFF);
        _material->_rasterstate.SetBlending(Blending::ALPHA);

        _material->begin(_is_pickmode ? _tekvtxcolor : _tektexcolor, RCFD);
        _material->bindParamMatrix(_parmvp, _uiMVPMatrix);
        _material->bindParamCTex(_partexture, texture.get());
        pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::TRIANGLES);
        _material->end(RCFD);
        icount = 0;
    }

    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    // render custom primitive
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////

    if (not _is_pickmode) {
      for (int i = 0; i < inumcusts; i++) {
        const GedPrim* prim = primcontainer->mCustomPrims[i];
        if (prim->_renderLambda) {
          prim->_renderLambda();
        }

        // if (IsVisible(pTARG, prim->iy1, prim->iy2) and prim->_renderLambda) {
        // prim->_renderLambda();
        // }
      }
    }

    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    // render line primitive
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////

    if (not _is_pickmode) {
      if (inumlines) {
        icount = 0;
        vw.Lock(pTARG, &VB, inumlines * 2);
        {
          for (int i = 0; i < inumlines; i++) {
            const GedPrim* prim = primcontainer->mLinePrims[i];
            if (IsVisible(pTARG, prim->iy1, prim->iy2)) {
              vw.AddVertex(vtx_t(fvec4(prim->ix1+fbias, prim->iy1+fbias, fZ), fvec4(), prim->_ucolor));
              vw.AddVertex(vtx_t(fvec4(prim->ix2+fbias, prim->iy2+fbias, fZ), fvec4(), prim->_ucolor));
              icount += 2;
            }
          }
        }
        vw.UnLock(pTARG);
        if (icount) {
          _material->_rasterstate.SetRGBAWriteMask(true, true);
          _material->_rasterstate.SetDepthTest(EDepthTest::OFF);
          _material->_rasterstate.SetBlending(Blending::OFF);

          _material->begin(_tekvtxcolor, RCFD);
          _material->bindParamMatrix(_parmvp, _uiMVPMatrix);
          pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::LINES);
          _material->end(RCFD);
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////
  if (not _is_pickmode) {
    lev2::FontMan::PushFont(_font);
    lev2::FontMan::beginTextBlock(pTARG);
    pTARG->PushModColor(fcolor4(0.8f, 0.9f, 1.0f));
    {
      int inumt = (int)_text_vect.size();
      for (int it = 0; it < inumt; it++) {
        const GedText& text = _text_vect[it];
        if (IsVisible(pTARG, text.iy, text.iy + 8)) {
          lev2::FontMan::DrawText(pTARG, text.ix, text.iy + 1, text.mString.c_str());
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

} // namespace ork::lev2::ged
