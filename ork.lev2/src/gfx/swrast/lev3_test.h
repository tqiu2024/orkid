////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

//#include <ork/math/raytracer.h>
#include <ork/dataflow/dataflow.h>
#include <ork/dataflow/scheduler.h>
#include <ork/math/frustum.h>
#include <ork/kernel/thread_pool.h>
//#include <ork/util/avi_utils.h>

class QTimer;

template <class Interface> inline void SafeRelease(Interface** ppInterfaceToRelease) {
  if (*ppInterfaceToRelease != NULL) {
    (*ppInterfaceToRelease)->Release();

    (*ppInterfaceToRelease) = NULL;
  }
}

///////////////////////////////////////////////////////////////////////////////

class render_graph;
class thread_pool;

///////////////////////////////////////////////////////////////////////////////

class DemoApp {
public:
  DemoApp(int iw, int ih);
  ~DemoApp();

  // Process and dispatch messages
  void Run();

private:
  void Render1();
  void Render2();

  int miNumAviFrames                         = 0;
  int miFrameIndex                           = 0;
  u32* mpFrameBuffer                         = nullptr;
  render_graph* mRenderGraph                 = nullptr;
  ork::threadpool::thread_pool* mpThreadPool = nullptr;
  QTimer* mpTimer                            = nullptr;
  int miWidth                                = 0;
  int miHeight                               = 0;
};
