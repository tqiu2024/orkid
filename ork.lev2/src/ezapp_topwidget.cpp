#include <ork/lev2/ezapp.h>
#include <ork/lev2/ui/viewport.h>
#include <ork/lev2/ui/layoutgroup.inl>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/vr/vr.h>
#include <ork/lev2/imgui/imgui.h>
#include <ork/lev2/imgui/imgui_impl_glfw.h>
#include <ork/lev2/imgui/imgui_impl_opengl3.h>
#include <boost/program_options.hpp>
#include <ork/kernel/environment.h>
#include <ork/util/logger.h>

using namespace std::string_literals;

namespace ork::lev2 {
static logchannel_ptr_t logchan_ezapp = logger()->createChannel("ezapp", fvec3(0.7, 0.7, 0.9));
///////////////////////////////////////////////////////////////////////////////
EzTopWidget::EzTopWidget(EzMainWin* mainwin)
    : ui::Group("ezviewport", 1, 1, 1, 1)
    , _mainwin(mainwin) {
  _geometry._w = 1;
  _geometry._h = 1;
  _initstate.store(0);
  lev2::DrawableBuffer::ClearAndSyncWriters();
  _mainwin->_render_timer.Start();
  _mainwin->_render_prevtime = _mainwin->_render_timer.SecsSinceStart();
}
/////////////////////////////////////////////////
void EzTopWidget::_doGpuInit(ork::lev2::Context* pTARG) {

  pTARG->FBI()->SetClearColor(fcolor4(0.0f, 0.0f, 0.0f, 0.0f));
  _initstate.store(1);

}
/////////////////////////////////////////////////
void EzTopWidget::DoDraw(ui::drawevent_constptr_t drwev) {
  //////////////////////////////////////////////////////
  // ensure onUpdateInit called before onGpuInit!
  //////////////////////////////////////////////////////
  auto ezapp = (OrkEzApp*)OrkEzAppBase::get();
  if (not ezapp->checkAppState(KAPPSTATEFLAG_UPDRUNNING))
    return;
  ///////////////////////////
  drwev->GetTarget()->makeCurrentContext();
  ///////////////////////////
  while (ezapp->_rthreadq->Process()) {
  }
  ///////////////////////////
  if (_mainwin->_onDraw) {
    _mainwin->_onDraw(drwev);
    auto ctxbase = drwev->GetTarget()->mCtxBase;
    drwev->GetTarget()->swapBuffers(ctxbase);
    ezapp->_render_count.fetch_add(1);
  }
  ///////////////////////////
  double this_time           = _mainwin->_render_timer.SecsSinceStart();
  _mainwin->_render_prevtime = this_time;
  if (this_time >= 5.0) {
    double FPS = _mainwin->_render_state_numiters / this_time;
    logchan_ezapp->log("FPS<%g>", FPS);
    _mainwin->_render_state_numiters  = 0.0;
    _mainwin->_render_timer.Start();
  } else {
    _mainwin->_render_state_numiters += 1.0;
  }
  ///////////////////////////
}
/////////////////////////////////////////////////
void EzTopWidget::_doOnResized() {
  if (_mainwin->_onResize) {
    _mainwin->_onResize(width(), height());
  }
  _topLayoutGroup->SetSize(width(), height());
}
/////////////////////////////////////////////////
ui::HandlerResult EzTopWidget::DoOnUiEvent(ui::event_constptr_t ev) {
  if (_mainwin->_onUiEvent) {
    auto hacked_event      = std::make_shared<ui::Event>();
    *hacked_event          = *ev;
    hacked_event->_vpdim.x = width();
    hacked_event->_vpdim.y = height();
    return _mainwin->_onUiEvent(hacked_event);
  } else
    return ui::HandlerResult();
}
///////////////////////////////////////////////////////////////////////////////
} //namespace ork::lev2 {
