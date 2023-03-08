////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/lev2/ui/ui.h>
#include <ork/lev2/ui/viewport.h>

namespace ork { namespace ui {

struct SceneGraphViewport : public Viewport {
  RttiDeclareAbstract(SceneGraphViewport, Viewport);
public:
  SceneGraphViewport(const std::string& name, int x=0, int y=0, int w=0, int h=0);

};

}} // namespace ork::ui
