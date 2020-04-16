////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/kernel/orklut.hpp>
#include <ork/math/plane.h>
#include <ork/lev2/gfx/meshutil/submesh.h>

template class ork::orklut<std::string, ork::meshutil::submesh*>;

namespace ork::meshutil {

const vertexpool vertexpool::EmptyPool;

/////////////////////////////////////////////////////////////////////////

submesh::submesh(const vertexpool& vpool)
    : _vtxpool(vpool)
    , _surfaceArea(0)
    , _mergeEdges(true) {
  // _orderedPolys.reserve(32<<10);
  // if( _mergeEdges )
  {
    // mEdges.reserve(32<<10);
  }
  for (int i = 0; i < kmaxsidesperpoly; i++) {
    _polyTypeCounter[i] = 0;
  }
}

submesh::~submesh() {
  static size_t gc1 = 0;
  static size_t gc2 = 0;
  static size_t gc3 = 0;
  static size_t gc4 = 0;
  static size_t gc5 = 0;
  static size_t gc6 = 0;

  size_t ic1 = _polymap.size();
  size_t ic2 = _edgemap.size();
  size_t ic3 = _vtxpool._vtxmap.size();
  size_t ic4 = _vtxpool._orderedVertices.size();
  size_t ic5 = _edgemap.size();
  size_t ic6 = _orderedPolys.size();
  gc1 += ic1;

  gc2 += ic2;
  gc3 += ic3;
  gc4 += ic4;
  gc5 += ic5;
  gc6 += ic6;
  size_t is1 = sizeof(std::pair<U64, int>);
  size_t is2 = sizeof(std::pair<int, int>);
  size_t is3 = is1;
  size_t is4 = sizeof(vertex);
  size_t is5 = sizeof(edge);
  size_t is6 = sizeof(poly);

  // orkprintf( "///////////////////////////////////\n" );
  // orkprintf( "polyhash cnt<%d:%d> tot<%d:%d>\n", ic1,ic1*is1, gc1,gc1*is1 );
  // orkprintf( "polys cnt<%d:%d> tot<%d:%d>\n", ic6,ic6*is6, gc6,gc6*is6 );
  // orkprintf( "edgemap cnt<%d:%d> tot<%d:%d>\n", ic2,ic2*is2, gc2,gc2*is2 );
  // orkprintf( "edges cnt<%d:%d> tot<%d:%d>\n", ic5,ic5*is5, gc5,gc5*is5 );
  // orkprintf( "vpoolmap cnt<%d:%d> tot<%d:%d>\n", ic3,ic3*is3, gc3,gc3*is3 );
  // orkprintf( "vpool cnt<%d:%d> tot<%d:%d>\n", ic4,ic4*is4, gc4,gc4*is4 );
  // orkprintf( "///////////////////////////////////\n" );
}

svar64_t submesh::annotation(const char* annokey) const {
  static const char* defret("");
  auto it = _annotations.find(std::string(annokey));
  if (it != _annotations.end()) {
    return (*it).second;
  }
  return defret;
}

void submesh::MergeAnnos(const AnnotationMap& mrgannos, bool boverwrite) {
  for (AnnotationMap::const_iterator it = mrgannos.begin(); it != mrgannos.end(); it++) {
    const std::string& key = it->first;
    const auto& val        = it->second;

    AnnotationMap::iterator itf = _annotations.find(key);
    if (itf == _annotations.end()) {
      _annotations[key] = val;
    } else if (boverwrite) {
      itf->second = val;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void submesh::ImportPolyAnnotations(const annopolylut& apl) {
  int inumpolys = (int)_orderedPolys.size();
  for (int ip = 0; ip < inumpolys; ip++) {
    auto ply            = _orderedPolys[ip];
    const AnnoMap* amap = apl.Find(*this, *ply);
    if (amap) {
      ply->SetAnnoMap(amap);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void submesh::ExportPolyAnnotations(annopolylut& apl) const {
  int inumpolys = (int)_orderedPolys.size();
  for (int ip = 0; ip < inumpolys; ip++) {
    auto ply            = _orderedPolys[ip];
    U64 uhash           = apl.HashItem(*this, *ply);
    const AnnoMap* amap = ply->GetAnnoMap();
    apl.mAnnoMap[uhash] = amap;
  }
}

///////////////////////////////////////////////////////////////////////////////

const AABox& submesh::aabox() const {
  if (_aaBoxDirty) {
    _aaBox.BeginGrow();
    int inumvtx = (int)RefVertexPool().GetNumVertices();
    for (int i = 0; i < inumvtx; i++) {
      const vertex& v = RefVertexPool().GetVertex(i);
      _aaBox.Grow(v.mPos);
    }
    _aaBox.EndGrow();
    _aaBoxDirty = false;
  }
  return _aaBox;
}

///////////////////////////////////////////////////////////////////////////////

const edge& submesh::RefEdge(U64 edgekey) const {
  auto it = _edgemap.find(edgekey);
  OrkAssert(it != _edgemap.end());
  return *it->second;
}

///////////////////////////////////////////////////////////////////////////////

vertex_ptr_t submesh::newMergeVertex(const vertex& vtx) {
  _aaBoxDirty = true;
  return _vtxpool.newMergeVertex(vtx);
}

///////////////////////////////////////////////////////////////////////////////

poly& submesh::RefPoly(int i) {
  OrkAssert(orkvector<int>::size_type(i) < _orderedPolys.size());
  return *_orderedPolys[i];
}

///////////////////////////////////////////////////////////////////////////////

const poly& submesh::RefPoly(int i) const {
  OrkAssert(orkvector<int>::size_type(i) < _orderedPolys.size());
  return *_orderedPolys[i];
}

///////////////////////////////////////////////////////////////////////////////

const orkvector<poly_ptr_t>& submesh::RefPolys() const {
  return _orderedPolys;
}

/////////////////////////////////////////////////////////////////////////

void submesh::FindNSidedPolys(orkvector<int>& output, int inumsides) const {
  int inump = (int)_orderedPolys.size();
  for (int i = 0; i < inump; i++) {
    const poly& ply = RefPoly(i);
    if (ply.GetNumSides() == inumsides) {
      output.push_back(i);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

int submesh::GetNumPolys(int inumsides) const {
  int iret = 0;
  if (0 == inumsides) {
    iret = (int)_orderedPolys.size();
  } else {
    OrkAssert(inumsides < kmaxsidesperpoly);
    iret = _polyTypeCounter[inumsides];
  }
  return iret;
}

///////////////////////////////////////////////////////////////////////////////

void submesh::GetEdges(const poly& ply, orkvector<edge>& Edges) const {
  int icnt  = 0;
  int icntf = 0;
  for (int is = 0; is < ply.GetNumSides(); is++) {
    U64 ue  = ply.mEdges[is]->GetHashKey();
    auto it = _edgemap.find(ue);
    if (it != _edgemap.end()) {
      Edges.push_back(*it->second);
      icntf++;
    }
    icnt++;
  }
}

///////////////////////////////////////////////////////////////////////////////

void submesh::GetAdjacentPolys(int ply, orkset<int>& output) const {
  orkvector<edge> edges;
  GetEdges(RefPoly(ply), edges);
  for (orkvector<edge>::const_iterator edgeIter = edges.begin(); edgeIter != edges.end(); edgeIter++) {
    orkset<int> connectedPolys;
    GetConnectedPolys(*edgeIter, connectedPolys);
    for (orkset<int>::const_iterator it2 = connectedPolys.begin(); it2 != connectedPolys.end(); it2++) {
      int ic = *it2;
      if (ic != ply) {
        output.insert(connectedPolys.begin(), connectedPolys.end());
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

edge_constptr_t submesh::edgeBetween(int aind, int bind) const {
  const poly& a = RefPoly(aind);
  const poly& b = RefPoly(bind);
  for (int eaind = 0; eaind < a.miNumSides; eaind++)
    for (int ebind = 0; ebind < b.miNumSides; ebind++)
      if (a.mEdges[eaind] == b.mEdges[ebind])
        return std::const_pointer_cast<const edge>(a.mEdges[eaind]);
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void submesh::GetConnectedPolys(const edge& ed, orkset<int>& output) const {
  U64 keyA    = ed.GetHashKey();
  auto itfind = _edgemap.find(keyA);
  if (itfind != _edgemap.end()) {
    auto edfound = itfind->second;
    int inump    = edfound->GetNumConnectedPolys();
    for (int ip = 0; ip < inump; ip++) {
      int ipi = edfound->GetConnectedPoly(ip);
      output.insert(ipi);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void submesh::MergeSubMesh(const submesh& inp_mesh) {
  float ftimeA     = float(OldSchool::GetRef().GetLoResTime());
  int inumpingroup = inp_mesh.GetNumPolys();
  for (int i = 0; i < inumpingroup; i++) {
    const poly& ply = inp_mesh.RefPoly(i);
    int inumpv      = ply.GetNumSides();
    poly NewPoly;
    NewPoly.miNumSides = inumpv;
    for (int iv = 0; iv < inumpv; iv++) {
      int ivi               = ply.GetVertexID(iv);
      const vertex& src_vtx = inp_mesh.RefVertexPool().GetVertex(ivi);
      NewPoly._vertices[iv] = newMergeVertex(src_vtx);
    }
    NewPoly.SetAnnoMap(ply.GetAnnoMap());
    MergePoly(NewPoly);
  }
  float ftimeB = float(OldSchool::GetRef().GetLoResTime());
  float ftime  = (ftimeB - ftimeA);
  orkprintf("<<PROFILE>> <<submesh::MergeSubMesh %f seconds>>\n", ftime);
}

///////////////////////////////////////////////////////////////////////////////

void submesh::MergePoly(const poly& ply) {
  int ipolyindex = GetNumPolys();
  poly nply      = ply;
  int inumv      = ply.GetNumSides();
  OrkAssert(inumv <= kmaxsidesperpoly);
  ///////////////////////////////
  // zero area poly removal
  switch (inumv) {
    case 3: {
      if ((ply._vertices[0]->_poolindex == ply._vertices[1]->_poolindex) ||
          (ply._vertices[1]->_poolindex == ply._vertices[2]->_poolindex) ||
          (ply._vertices[2]->_poolindex == ply._vertices[0]->_poolindex)) {
        orkprintf(
            "Mesh::MergePoly() removing zero area tri<%d %d %d>\n",
            ply._vertices[0]->_poolindex,
            ply._vertices[1]->_poolindex,
            ply._vertices[2]->_poolindex);

        return;
      }
      break;
    }
    case 4: {
      if ((ply._vertices[0]->_poolindex == ply._vertices[1]->_poolindex) ||
          (ply._vertices[0]->_poolindex == ply._vertices[2]->_poolindex) ||
          (ply._vertices[0]->_poolindex == ply._vertices[3]->_poolindex) ||
          (ply._vertices[1]->_poolindex == ply._vertices[2]->_poolindex) ||
          (ply._vertices[1]->_poolindex == ply._vertices[3]->_poolindex) ||
          (ply._vertices[2]->_poolindex == ply._vertices[3]->_poolindex)) {
        orkprintf(
            "Mesh::MergePoly() removing zero area quad<%d %d %d %d>\n",
            ply._vertices[0]->_poolindex,
            ply._vertices[1]->_poolindex,
            ply._vertices[2]->_poolindex,
            ply._vertices[3]->_poolindex);

        return;
      }
      break;
    }
    default:
      break;
      // TODO n-sided polys
  }
  //////////////////////////////
  // dupe check
  U64 ucrc   = ply.HashIndices();
  auto itfhm = _polymap.find(ucrc);
  ///////////////////////////////
  if (itfhm == _polymap.end()) // no match
  {
    int inewpi = (int)_orderedPolys.size();
    //////////////////////////////////////////////////
    // connect to vertices
    for (int i = 0; i < inumv; i++) {
      auto vtx = ply._vertices[i];
      // vtx->ConnectToPoly(inewpi);
    }
    //////////////////////////////////////////////////
    // add edges
    if (_mergeEdges) {
      for (int i = 0; i < inumv; i++) {
        int i0  = (i);
        int i1  = (i + 1) % inumv;
        int iv0 = ply.GetVertexID(i0);
        int iv1 = ply.GetVertexID(i1);
        auto v0 = _vtxpool._orderedVertices[iv0];
        auto v1 = _vtxpool._orderedVertices[iv1];

        edge Edge(v0, v1);
        nply.mEdges[i] = MergeEdge(Edge, ipolyindex);
      }
    }
    nply.SetAnnoMap(ply.GetAnnoMap());
    auto new_poly = std::make_shared<poly>(nply);
    _orderedPolys.push_back(new_poly);
    _polymap[ucrc] = new_poly;
    //////////////////////////////////////////////////
    // add n sided counters
    _polyTypeCounter[inumv]++;
    //////////////////////////////////////////////////
    float farea = ply.ComputeArea(_vtxpool, ork::fmtx4::Identity());
    _surfaceArea += farea;
  }
  _aaBoxDirty = true;
}

///////////////////////////////////////////////////////////////////////////////

edge_ptr_t submesh::MergeEdge(const edge& ed, int ipolyindex) {
  U64 crcA    = ed.GetHashKey();
  auto itfind = _edgemap.find(crcA);

  edge_ptr_t rval;

  if (_edgemap.end() != itfind) {
    rval     = itfind->second;
    U64 crcB = rval->GetHashKey();
    OrkAssert(ed.Matches(*rval));
  } else {
    rval           = std::make_shared<edge>(ed);
    _edgemap[crcA] = rval;
  }
  if (ipolyindex >= 0) {
    rval->ConnectToPoly(ipolyindex);
  }

  _aaBoxDirty = true;
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
// addPoly helper methods
///////////////////////////////////////////////////////////////////////////////

void submesh::addQuad(fvec3 p0, fvec3 p1, fvec3 p2, fvec3 p3, fvec4 c) {
  vertex muvtx[4];
  muvtx[0].set(p0, fvec3(), fvec3(), fvec2(), c);
  muvtx[1].set(p1, fvec3(), fvec3(), fvec2(), c);
  muvtx[2].set(p2, fvec3(), fvec3(), fvec2(), c);
  muvtx[3].set(p3, fvec3(), fvec3(), fvec2(), c);
  auto v0 = newMergeVertex(muvtx[0]);
  auto v1 = newMergeVertex(muvtx[1]);
  auto v2 = newMergeVertex(muvtx[2]);
  auto v3 = newMergeVertex(muvtx[3]);
  MergePoly(poly(v0, v1, v2, v3));
}

void submesh::addQuad(fvec3 p0, fvec3 p1, fvec3 p2, fvec3 p3, fvec2 uv0, fvec2 uv1, fvec2 uv2, fvec2 uv3, fvec4 c) {
  vertex muvtx[4];
  fvec3 p0p1 = (p1 - p0).Normal();
  fvec3 p0p2 = (p2 - p0).Normal();
  fvec3 nrm  = p0p1.Cross(p0p2);
  // todo compute tangent space from uv gradients
  fvec3 bin = p0p1;
  muvtx[0].set(p0, nrm, bin, uv0, c);
  muvtx[1].set(p1, nrm, bin, uv1, c);
  muvtx[2].set(p2, nrm, bin, uv2, c);
  muvtx[3].set(p3, nrm, bin, uv3, c);

  auto v0 = newMergeVertex(muvtx[0]);
  auto v1 = newMergeVertex(muvtx[1]);
  auto v2 = newMergeVertex(muvtx[2]);
  auto v3 = newMergeVertex(muvtx[3]);
  MergePoly(poly(v0, v1, v2, v3));
}

void submesh::addQuad(
    fvec3 p0,
    fvec3 p1,
    fvec3 p2,
    fvec3 p3,
    fvec3 n0,
    fvec3 n1,
    fvec3 n2,
    fvec3 n3,
    fvec2 uv0,
    fvec2 uv1,
    fvec2 uv2,
    fvec2 uv3,
    fvec4 c) { /// add quad helper method
  vertex muvtx[4];
  fvec3 p0p1 = (p1 - p0).Normal();
  fvec3 bin  = p0p1;
  muvtx[0].set(p0, n0, bin, uv0, c);
  muvtx[1].set(p1, n1, bin, uv1, c);
  muvtx[2].set(p2, n2, bin, uv2, c);
  muvtx[3].set(p3, n3, bin, uv3, c);

  auto v0 = newMergeVertex(muvtx[0]);
  auto v1 = newMergeVertex(muvtx[1]);
  auto v2 = newMergeVertex(muvtx[2]);
  auto v3 = newMergeVertex(muvtx[3]);
  MergePoly(poly(v0, v1, v2, v3));
}
///////////////////////////////////////////////////////////////////////////////
/*
void SubMesh::GenIndexBuffers( void )
{
    int inumvtx = RefVertexPool().VertexPool.size();

    orkvector<int> TrianglePolyIndices;
    orkvector<int> QuadPolyIndices;

    FindNSidedPolys( TrianglePolyIndices, 3 );
    FindNSidedPolys( QuadPolyIndices, 4 );

    int inumtri( TrianglePolyIndices.size() );
    int inumquad( QuadPolyIndices.size() );

    mpBaseTriangleIndices = new U16[ inumtri*3 ];
    mpBaseQuadIndices = new U16[ inumquad*4 ];

    for( int itri=0; itri<inumtri; itri++ )
    {
        int iti = TrianglePolyIndices[itri];

        const poly & tri = RefPoly( iti );

        int i0 = tri.miVertices[0];
        int i1 = tri.miVertices[1];
        int i2 = tri.miVertices[2];

        OrkAssert( i0<inumvtx );
        OrkAssert( i1<inumvtx );
        OrkAssert( i2<inumvtx );

        mpBaseTriangleIndices[ (itri*3)+0 ] = U16(i0);
        mpBaseTriangleIndices[ (itri*3)+1 ] = U16(i1);
        mpBaseTriangleIndices[ (itri*3)+2 ] = U16(i2);
    }

    for( int iqua=0; iqua<inumquad; iqua++ )
    {
        int iqi = QuadPolyIndices[iqua];

        const poly & qu = RefPoly( iqi );

        int i0 = qu.miVertices[0];
        int i1 = qu.miVertices[1];
        int i2 = qu.miVertices[2];
        int i3 = qu.miVertices[3];

        OrkAssert( i0<inumvtx );
        OrkAssert( i1<inumvtx );
        OrkAssert( i2<inumvtx );
        OrkAssert( i3<inumvtx );

        mpBaseQuadIndices[ (iqua*4)+0 ] = U16(i0);
        mpBaseQuadIndices[ (iqua*4)+1 ] = U16(i1);
        mpBaseQuadIndices[ (iqua*4)+2 ] = U16(i2);
        mpBaseQuadIndices[ (iqua*4)+3 ] = U16(i3);
    }

}*/

} // namespace ork::meshutil
