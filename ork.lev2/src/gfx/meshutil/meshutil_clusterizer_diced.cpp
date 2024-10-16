///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyrigh 1996-2004, Michael T. Mayers
// See License at OrkidRoot/license.html or http://www.tweakoz.com/orkid/license.html
///////////////////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/meshutil/submesh.h>
#include <ork/lev2/gfx/meshutil/meshutil_fixedgrid.h>
#include <ork/lev2/gfx/meshutil/clusterizer.h>

const bool gbFORCEDICE = true;
const int kDICESIZE    = 512;

namespace ork::meshutil {
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

XgmClusterizerDiced::XgmClusterizerDiced() {
}

///////////////////////////////////////////////////////////////////////////////

XgmClusterizerDiced::~XgmClusterizerDiced() {
}

///////////////////////////////////////////////////////////////////////////////

void XgmClusterizerDiced::Begin() {
}

///////////////////////////////////////////////////////////////////////////////

bool XgmClusterizerDiced::addTriangle(const XgmClusterTri& Triangle, const MeshConfigurationFlags& flags) {
  auto v0 = _preDicedMesh.mergeVertex(Triangle._vertex[0]);
  auto v1 = _preDicedMesh.mergeVertex(Triangle._vertex[1]);
  auto v2 = _preDicedMesh.mergeVertex(Triangle._vertex[2]);
  _preDicedMesh.mergeTriangle(v0,v1,v2);
  return true;
}

///////////////////////////////////////////////////////////////////////////////

void XgmClusterizerDiced::End() {

  ///////////////////////////////////////////////
  // compute ideal dice size
  ///////////////////////////////////////////////

  AABox aab     = _preDicedMesh.aabox();
  fvec3 extents = aab.Max() - aab.Min();

#if 0

	int isize = 1<<20;
	bool bdone = false;

	while( false == bdone )
	{
		int idimX = int(extents.x)/isize;
		int idimY = int(extents.y)/isize;
		int idimZ = int(extents.z)/isize;

		int inumnodes = idimX*idimY*idimZ;

		if( inumnodes > 16 )
		{
			bdone = true;
		}
		else
		{
			isize >>= 1;
			orkprintf( "idim<%d %d %d> dice size<%d>\n", idimX, idimY, idimZ, isize );
		}

	}
#else
  int isize = kDICESIZE;
  int idimX = int(extents.x) / isize;
  int idimY = int(extents.y) / isize;
  int idimZ = int(extents.z) / isize;
  if (idimX == 0)
    idimX = 1;
  if (idimY == 0)
    idimY = 1;
  if (idimZ == 0)
    idimZ = 1;
  int inumnodes = idimX * idimY * idimZ;
#endif

  ///////////////////////////////////////////////
  // END compute ideal dice size
  ///////////////////////////////////////////////

  Mesh DicedMesh;

  DicedMesh.SetMergeEdges(false);

  if (gbFORCEDICE || _preDicedMesh.numPolys() > 10000) {
    float ftimeA = float(OldSchool::GetRef().GetLoResTime());

    GridGraph thegraph(isize);
    thegraph.BeginPreMerge();
    thegraph.PreMergeMesh(_preDicedMesh);
    thegraph.EndPreMerge();
    thegraph.MergeMesh(_preDicedMesh, DicedMesh);

    float ftimeB = float(OldSchool::GetRef().GetLoResTime());

    float ftime = (ftimeB - ftimeA);
    orkprintf("<<PROFILE>> <<dicemesh %f seconds>>\n", ftime);

  } else {
    DicedMesh.MergeSubMesh(_preDicedMesh);
  }
  int inumpacc = 0;

  auto& submeshes_by_polygroup = DicedMesh.RefSubMeshLut();

  size_t inumgroups = submeshes_by_polygroup.size();
  static int igroup = 0;

  float ftimeC = float(OldSchool::GetRef().GetLoResTime());
  for (auto it : submeshes_by_polygroup) {
    const std::string& pgname = it.first;
    const submesh& pgrp       = *it.second;
    int inumpolys             = pgrp.numPolys();

    inumpacc += inumpolys;

    auto new_builder = std::make_shared<XgmRigidClusterBuilder>(*this);
    _clusters.push_back(new_builder);

    for (int ip = 0; ip < inumpolys; ip++) {
      const Polygon& ply = pgrp.RefPoly(ip);

      OrkAssert(ply.numVertices() == 3);

      XgmClusterTri ClusTri;

      ClusTri._vertex[0] = *pgrp.vertex(ply.vertexID(0));
      ClusTri._vertex[1] = *pgrp.vertex(ply.vertexID(1));
      ClusTri._vertex[2] = *pgrp.vertex(ply.vertexID(2));

      bool bOK = new_builder->addTriangle(ClusTri);

      if (false == bOK) // cluster full, make new cluster
      {
        new_builder = std::make_shared<XgmRigidClusterBuilder>(*this);
        _clusters.push_back(new_builder);
        bOK = new_builder->addTriangle(ClusTri);
        OrkAssert(bOK);
      }
    }
  }
  float ftimeD = float(OldSchool::GetRef().GetLoResTime());
  float ftime  = (ftimeD - ftimeC);
  orkprintf("<<PROFILE>> <<clusterize %f seconds>>\n", ftime);

  float favgpolyspergroup = float(inumpacc) / float(inumgroups);

  orkprintf("dicer NumGroups<%d> AvgPolysPerGroup<%d>\n", inumgroups, int(favgpolyspergroup));

  size_t inumclus = _clusters.size();
  for (size_t ic = 0; ic < inumclus; ic++) {
    auto builder = _clusters[ic];
    AABox bbox   = builder->_submesh.aabox();
    fvec3 vmin   = bbox.Min();
    fvec3 vmax   = bbox.Max();
    float fdist  = (vmax - vmin).magnitude();

    int inumv = (int)builder->_submesh.numVertices();
    orkprintf(
        "clus<%d> inumv<%d> bbmin<%g %g %g> bbmax<%g %g %g> diag<%g>\n",
        ic,
        inumv,
        vmin.x,
        vmin.y,
        vmin.z,
        vmax.x,
        vmax.y,
        vmax.z,
        fdist);
  }
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::meshutil
