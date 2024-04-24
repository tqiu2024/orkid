////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/math/plane.h>
#include <ork/lev2/gfx/meshutil/submesh.h>
#include <ork/util/xxhash.inl>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace meshutil {
///////////////////////////////////////////////////////////////////////////////

static const std::string gnomatch("");

///////////////////////////////////////////////////////////////////////////////

const std::string& AnnoMap::GetAnnotation(const std::string& annoname) const {
  orkmap<std::string, std::string>::const_iterator it = _annotations.find(annoname);
  if (it != _annotations.end()) {
    return it->second;
  }
  return gnomatch;
}

void AnnoMap::SetAnnotation(const std::string& key, const std::string& val) {
  _annotations[key] = val;
}

///////////////////////////////////////////////////////////////////////////////

AnnoMap* AnnoMap::Fork() const {
  AnnoMap* newannomap      = new AnnoMap;
  newannomap->_annotations = _annotations;
  return newannomap;
}

///////////////////////////////////////////////////////////////////////////////

AnnoMap::AnnoMap() {
}

///////////////////////////////////////////////////////////////////////////////

AnnoMap::~AnnoMap() {
}

///////////////////////////////////////////////////////////////////////////////

U64 annopolyposlut::HashItem(const submesh& tmesh, const Polygon& ply) const {
  XXH64HASH xxh;
  xxh.init();
  ply.visitVertices([&](vertex_ptr_t v) {
    int ivi           = v->_poolindex;
    auto vtx = tmesh.vertex(ivi);
    xxh.accumulateItem(vtx->mPos);
    xxh.accumulateItem(vtx->mNrm);
  });
  xxh.finish();
  return xxh.result();
}

///////////////////////////////////////////////////////////////////////////////

const AnnoMap* annopolylut::Find(const submesh& tmesh, const Polygon& ply) const {
  const AnnoMap* rval                            = 0;
  U64 uhash                                      = HashItem(tmesh, ply);
  orkmap<U64, const AnnoMap*>::const_iterator it = mAnnoMap.find(uhash);
  if (it != mAnnoMap.end()) {
    rval = it->second;
  }
  return rval;
}

///////////////////////////////////////////////////////////////////////////////

vertexpool::vertexpool() {
}

///////////////////////////////////////////////////////////////////////////////

vertex_ptr_t vertexpool::mergeVertex(const vertex& vtx) {
  vertex_ptr_t rval;
  U64 vhash = vtx.hash();
  auto it   = _vtxmap.find(vhash);
  if (_vtxmap.end() != it) {
    rval = it->second;
  } else {
    rval             = std::make_shared<vertex>(vtx);
    rval->_poolindex = uint32_t(_orderedVertices.size());
    _orderedVertices.push_back(rval);
    _vtxmap[vhash] = rval;
  }
  return rval;
}

///////////////////////////////////////////////////////////////////////////////

void vertexpool::rehash(){
  //auto copy_of_ordered = _orderedVertices;
  //_orderedVertices.clear();
  _vtxmap.clear();
  for( auto v : _orderedVertices ){
    uint64_t h = v->hash();
    auto it = _vtxmap.find(h);
    OrkAssert(it==_vtxmap.end());
    _vtxmap[h] = v;
  }

}


///////////////////////////////////////////////////////////////////////////////
}} // namespace ork::meshutil
///////////////////////////////////////////////////////////////////////////////
