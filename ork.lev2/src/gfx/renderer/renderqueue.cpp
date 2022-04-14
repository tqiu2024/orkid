////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/kernel/Array.hpp>
#include <ork/lev2/gfx/renderer/renderer.h>

///////////////////////////////////////////////////////////////////////////////

template class ork::fixedvector<ork::lev2::RenderQueue::Node, ork::lev2::RenderQueue::krqmaxsize>;

namespace ork { namespace lev2 {

///////////////////////////////////////////////////////////////////////////////

void RenderQueue::enqueueRenderable(const IRenderable* renderable) {
  new (&_nodes.create()) Node(renderable);
}

///////////////////////////////////////////////////////////////////////////////

void RenderQueue::exportRenderableNodes(ork::fixedvector<const RenderQueue::Node*, krqmaxsize>& nodes) const {
  nodes.resize(Size());
  int idx = 0;
  for (const Node& n : _nodes) {
    nodes[idx++] = &n;
  }
}

///////////////////////////////////////////////////////////////////////////////

void RenderQueue::Reset() { _nodes.clear(); }

///////////////////////////////////////////////////////////////////////////////
}} // namespace ork::lev2
///////////////////////////////////////////////////////////////////////////////
