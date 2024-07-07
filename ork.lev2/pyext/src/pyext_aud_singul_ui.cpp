////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "pyext.h"
#include <ork/lev2/aud/audiodevice.h>
#include <ork/lev2/aud/singularity/cz1.h>
#include <ork/lev2/aud/singularity/krzdata.h>
#include <ork/lev2/aud/singularity/tx81z.h>
#include <ork/lev2/aud/singularity/fxgen.h>
#include <ork/lev2/aud/singularity/hud.h>
#include <ork/lev2/ui/widget.h>
#include <ork/lev2/ui/group.h>
#include <ork/lev2/ui/surface.h>
#include <ork/lev2/ui/viewport.h>
#include <ork/lev2/ui/layoutgroup.inl>
#include <ork/lev2/ui/anchor.h>
#include <ork/lev2/ui/box.h>

///////////////////////////////////////////////////////////////////////////////
namespace ork::audio::singularity {
///////////////////////////////////////////////////////////////////////////////
using namespace ork::lev2;

void pyinit_aud_singularity_ui(py::module& singmodule) {
  auto type_codec = python::typecodec_t::instance();
  /////////////////////////////////////////////////////////////////////////////////
  auto scopesource_type = //
      py::class_<ScopeSource, scopesource_ptr_t>(singmodule, "ScopeSource")
          .def("connect", [](scopesource_ptr_t src, scopesink_ptr_t sink) { //
            src->connect(sink);
          })
          .def("disconnect", [](scopesource_ptr_t src, scopesink_ptr_t sink) { //
            src->disconnect(sink);
          });
  type_codec->registerStdCodec<scopesource_ptr_t>(scopesource_type);
  /////////////////////////////////////////////////////////////////////////////////
  auto scopesink_type = //
      py::class_<ScopeSink, scopesink_ptr_t>(singmodule, "ScopeSink");
  type_codec->registerStdCodec<scopesink_ptr_t>(scopesink_type);
  /////////////////////////////////////////////////////////////////////////////////
  struct OSCOPE_PROXY {
    signalscope_ptr_t _instrument;
  };
  using oscope_ptr_t = std::shared_ptr<OSCOPE_PROXY>;
  auto analyzer_type = //
      py::class_<OSCOPE_PROXY, oscope_ptr_t>(singmodule, "Oscilloscope")
          .def_property_readonly(
              "sink",
              [](oscope_ptr_t scope) -> scopesink_ptr_t { //
                return scope->_instrument->_sink;
              })
          .def_static("uifactory", [type_codec](uilayoutgroup_ptr_t lg, py::list py_args) -> uilayoutitem_ptr_t { //
            auto decoded_args  = type_codec->decodeList(py_args);
            auto name          = decoded_args[0].get<std::string>();
            auto instrument    = create_oscilloscope2(lg, name);
            auto proxy         = std::make_shared<OSCOPE_PROXY>();
            proxy->_instrument = instrument;
            // retain scope in layout group
            lg->_uservars.makeValueForKey<oscope_ptr_t>("oscilloscopes." + name, proxy);
            return instrument->_layoutitem;
          });
  type_codec->registerStdCodec<oscope_ptr_t>(analyzer_type);
  /////////////////////////////////////////////////////////////////////////////////
  struct ANALYZER_PROXY {
    signalscope_ptr_t _instrument;
  };
  using analyzer_ptr_t = std::shared_ptr<ANALYZER_PROXY>;
  auto oscope_type     = //
      py::class_<ANALYZER_PROXY, analyzer_ptr_t>(singmodule, "SpectrumAnalyzer")
          .def_property_readonly(
              "sink",
              [](analyzer_ptr_t analyzer) -> scopesink_ptr_t { //
                return analyzer->_instrument->_sink;
              })
          .def_static("uifactory", [type_codec](uilayoutgroup_ptr_t lg, py::list py_args) -> uilayoutitem_ptr_t { //
            auto decoded_args  = type_codec->decodeList(py_args);
            auto name          = decoded_args[0].get<std::string>();
            auto instrument    = create_spectrumanalyzer2(lg, name);
            auto proxy         = std::make_shared<ANALYZER_PROXY>();
            proxy->_instrument = instrument;
            // retain scope in layout group
            lg->_uservars.makeValueForKey<analyzer_ptr_t>("analyzers." + name, proxy);
            return instrument->_layoutitem;
          });
  type_codec->registerStdCodec<analyzer_ptr_t>(oscope_type);
  /////////////////////////////////////////////////////////////////////////////////
  struct PROGRAMVIEW_PROXY {
    hudpanel_ptr_t _instrument;
  };
  using pgmviewproxy_ptr_t = std::shared_ptr<PROGRAMVIEW_PROXY>;
  auto pgmview_type        = //
      py::class_<PROGRAMVIEW_PROXY, pgmviewproxy_ptr_t>(singmodule, "ProgramView")
          .def(
              "setProgram",
              [](pgmviewproxy_ptr_t proxy, prgdata_ptr_t prg) { //
                auto ins             = proxy->_instrument;
                auto pgmview         = std::dynamic_pointer_cast<ProgramView>(ins->_uisurface);
                pgmview->_curprogram = prg;
                pgmview->MarkSurfaceDirty();
              })
          .def_static("uifactory", [type_codec](uilayoutgroup_ptr_t lg, py::list py_args) -> uilayoutitem_ptr_t { //
            auto decoded_args  = type_codec->decodeList(py_args);
            auto name          = decoded_args[0].get<std::string>();
            auto instrument    = createProgramView2(lg, name);
            auto proxy         = std::make_shared<PROGRAMVIEW_PROXY>();
            proxy->_instrument = instrument;
            // retain scope in layout group
            lg->_uservars.makeValueForKey<pgmviewproxy_ptr_t>("programviews." + name, proxy);
            return instrument->_layoutitem;
          });
  type_codec->registerStdCodec<pgmviewproxy_ptr_t>(pgmview_type);
  /////////////////////////////////////////////////////////////////////////////////
  struct PROFILERVIEW_PROXY {
    hudpanel_ptr_t _instrument;
  };
  using profviewproxy_ptr_t = std::shared_ptr<PROFILERVIEW_PROXY>;
  auto profview_type        = //
      py::class_<PROFILERVIEW_PROXY, profviewproxy_ptr_t>(singmodule, "ProfilerView")
          .def_static("uifactory", [type_codec](uilayoutgroup_ptr_t lg, py::list py_args) -> uilayoutitem_ptr_t { //
            auto decoded_args  = type_codec->decodeList(py_args);
            auto name          = decoded_args[0].get<std::string>();
            auto instrument    = createProfilerView2(lg, name);
            auto proxy         = std::make_shared<PROFILERVIEW_PROXY>();
            proxy->_instrument = instrument;
            // retain scope in layout group
            lg->_uservars.makeValueForKey<profviewproxy_ptr_t>("profilerviews." + name, proxy);
            return instrument->_layoutitem;
          });
  type_codec->registerStdCodec<profviewproxy_ptr_t>(profview_type);
  /////////////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
} //namespace ork::audio::singularity {
///////////////////////////////////////////////////////////////////////////////
