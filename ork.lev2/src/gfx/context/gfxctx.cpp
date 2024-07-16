////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/gfx/renderer/rendercontext.h>
#include <ork/kernel/string/string.h>
#include <ork/lev2/ui/event.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/object/AutoConnector.h>
#include <ork/lev2/gfx/ctxbase.h>

///////////////////////////////////////////////////////////////////////////////

bool sbExit = false;

ImplementReflectionX(ork::lev2::Context, "Context");

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace lev2 {

///////////////////////////////////////////////////////////////////////////////

void Context::enqueueGpuEvent(gpuevent_ptr_t evt) {
  _gpuEventQueue.push(evt);
}
void Context::registerGpuEventSink(gpueventsink_ptr_t sink) {
  _gpuEventSinks.atomicOp([sink](gpueventsink_map_t& unlocked){
    unlocked.insert(std::make_pair(sink->_eventID,sink));
  });
}

///////////////////////////////////////////////////////////////////////////////

bool Context::hiDPI() const {
  return _HIDPI();
}

float Context::currentDPI() const {
  return _currentDPI();
}

void Context::describeX(class_t* clazz) {
}

void Context::triggerFrameDebugCapture(){
  _isFrameDebugCapture = true;
  _doTriggerFrameDebugCapture();
}

///////////////////////////////////////////////////////////////////////////////

void Context::beginFrame(void) {

  makeCurrentContext();

  bool keep_going = true;
  while(keep_going) {
    keep_going = false;
    auto it = _stickyCallbacks.begin();
    if( it != _stickyCallbacks.end() ){
      auto cb = *it;
      bool processed = cb();
      if(processed){
        _stickyCallbacks.erase(it);
        keep_going = true;
      }
    }   
  }

  auto mainrect = mainSurfaceRectAtOrigin();
  FBI()->setViewport(mainrect);
  FBI()->setScissor(mainrect);

  FBI()->BeginFrame();
  GBI()->BeginFrame();
  FXI()->BeginFrame();

  PushModColor(fcolor4::White());
  MTXI()->PushMMatrix(fmtx4::Identity());
  MTXI()->PushVMatrix(fmtx4::Identity());
  MTXI()->PushPMatrix(fmtx4::Identity());

  mpCurrentObject = 0;

  mRenderContextInstData = 0;

  _doBeginFrame();

  for (auto l : _onBeginFrameCallbacks)
    l();

  _gpuEventSinks.atomicOp([this](gpueventsink_map_t& unlocked){
    while(not _gpuEventQueue.empty() ){
      auto event = _gpuEventQueue.front();
      auto it = unlocked.find(event->_eventID);
      if( it != unlocked.end() ){
        auto sink = it->second;
        if(sink->_onEvent){
          sink->_onEvent(event);
        }
        //it->second->onGpuEvent(event);
      }
      _gpuEventQueue.pop();
    }
  });
  
}

///////////////////////////////////////////////////////////////////////////////

void Context::endFrame(void) {

  for (auto l : _onEndFrameCallbacks)
    l();



  GBI()->EndFrame();
  MTXI()->PopMMatrix();
  MTXI()->PopVMatrix();
  MTXI()->PopPMatrix();
  FBI()->EndFrame();

  PopModColor();
  mbPostInitializeContext = false;

  for (auto l : _onBeforeDoEndFrameOneShotCallbacks)
    l();

  _onBeforeDoEndFrameOneShotCallbacks.clear();

  _doEndFrame();

  miTargetFrame++;
  _isFrameDebugCapture = false;
}

/////////////////////////////////////////////////////////////////////////

Context::Context()
    : meTargetType(TargetType::NONE)
    , miW(0)
    , miH(0)
    , miModColorStackIndex(0)
    , miTargetFrame(0)
    , miDrawLock(0)
    , mbPostInitializeContext(true)
    , mFramePerfItem(CreateFormattedString("<target:%p>", this)) {

  static CompositingData _gdata;
  static auto _gimpl = _gdata.createImpl();
  auto RCFD   = std::make_shared<RenderContextFrameData>(this);
  RCFD->pushCompositor(_gimpl);
  _defaultrcfd       = RCFD;
  pushRenderContextFrameData(RCFD);
}

///////////////////////////////////////////////////////////////////////////////

Context::~Context() {
}

///////////////////////////////////////////////////////////////////////////////

orkvector<DisplayMode*> Context::mDisplayModes;

bool Context::SetDisplayMode(unsigned int index) {
  if (index < mDisplayModes.size())
    return SetDisplayMode(mDisplayModes[index]);
  return false;
}

///////////////////////////////////////////////////////////////////////////////

void* Context::BeginLoad() {
  return _doBeginLoad();
}
void Context::EndLoad(void* ploadtok) {
  _doEndLoad(ploadtok);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2
