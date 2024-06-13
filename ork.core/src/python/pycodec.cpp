////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/kernel/prop.h>
#include <ork/kernel/opq.h>
#include <ork/kernel/datablock.h>
#include <ork/kernel/fixedstring.hpp>
#include <ork/python/pycodec.inl>
#include <iostream>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ork::python {
////////////////////////////////////////////////////////////////////////////////

struct PyCodecItem {
  py::object _pytype;
  ork::TypeId _orktype;
  encoderfn_t _encoder;
  decoderfn_t _decoder;
};
struct PyCodecItem64 {
  py::object _pytype;
  ork::TypeId _orktype;
  encoderfn64_t _encoder;
  decoderfn64_t _decoder;
};
struct PyCodecImpl {
  varval_t decode(const py::object& val) const;
  svar64_t decode64(const py::object& val) const;
  py::object encode(const varval_t& val) const;
  std::unordered_map<ork::TypeId::hashtype_t, PyCodecItem> _codecs_by_orktype;
  std::unordered_map<ork::TypeId::hashtype_t, PyCodecItem64> _codecs64_by_orktype;
};
////////////////////////////////////////////////////////////////////////////////
py::object PyCodecImpl::encode(const varval_t& val) const {
  py::object rval;
  if (not val.isSet()) {
    return py::none();
  }
  auto orktypeid = val.getOrkTypeId();
  auto it        = _codecs_by_orktype.find(orktypeid._hashed);
  if (it != _codecs_by_orktype.end()) {
    auto& codec = it->second;
    codec._encoder(val, rval);
  } else {
    // try primitives
    if (auto as_bool = val.tryAs<bool>()) {
      return py::bool_(as_bool.value());
    } else if (auto as_float = val.tryAs<float>()) {
      return py::float_(as_float.value());
    } else if (auto as_double = val.tryAs<double>()) {
      return py::float_(as_double.value());
    } else if (auto as_int = val.tryAs<int>()) {
      return py::int_(as_int.value());
    } else if (auto as_uint32_t = val.tryAs<uint32_t>()) {
      return py::int_(as_uint32_t.value());
    } else if (auto as_uint64_t = val.tryAs<uint64_t>()) {
      return py::int_(as_uint64_t.value());
    } else if (auto as_str = val.tryAs<std::string>()) {
      return py::str(as_str.value());
    } else if (auto as_np = val.tryAs<std::nullptr_t>()) {
      return py::none();
    } else if (auto as_reflcodec = val.tryAs<refl_codec_adapter_ptr_t>()) {
      return as_reflcodec.value()->encode();
    } else if (auto as_vmap = val.tryAs<varmap::VarMap>()) {
      return py::none();
    } else {
      printf( "UNKNOWNTYPE<%s>\n", val.typeName() );
      OrkAssert(false);
      throw std::runtime_error("pycodec-encode: unregistered type");
    }
  }
  return rval;
}
////////////////////////////////////////////////////////////////////////////////
varval_t PyCodecImpl::decode(const py::object& val) const {
  varval_t rval;
  auto type = val.get_type();
  bool done = false;
  for (auto& codec_item : _codecs_by_orktype) {
    const auto& codec = codec_item.second;
    if (type.is(codec._pytype)) {
      codec._decoder(val, rval);
      return rval;
    }
  }
  std::cout << "BadValue: " << val.cast<std::string>() << std::endl;
  throw std::runtime_error("pycodec-decode: unregistered type");
  OrkAssert(false); // unknown type!
  return rval;
}
////////////////////////////////////////////////////////////////////////////////
svar64_t PyCodecImpl::decode64(const py::object& val) const {
  svar64_t rval;
  auto type = val.get_type();
  bool done = false;
  for (auto& codec_item : _codecs64_by_orktype) {
    const auto& codec = codec_item.second;
    if (type.is(codec._pytype)) {
      codec._decoder(val, rval);
      return rval;
    }
  }
  std::cout << "decode64 :: BadValue: " << val.cast<std::string>() << std::endl;
  throw std::runtime_error("pycodec-decode: unregistered type");
  OrkAssert(false); // unknown type!
  return rval;
}

////////////////////////////////////////////////////////////////////////////////
TypeCodec::TypeCodec() {
  _impl.make<PyCodecImpl>();
  /////////////////////////////////////////////////
  // builtin decoders
  /////////////////////////////////////////////////
  auto builtins   = py::module::import("builtins");
  auto bool_type   = builtins.attr("bool");
  auto int_type   = builtins.attr("int");
  auto float_type = builtins.attr("float");
  auto str_type   = builtins.attr("str");
  ///////////////////////////////
  registerCodec(
      bool_type, //
      TypeId::of<bool>(),
      [](const varval_t& inpval, pybind11::object& outval) { // encoder
        outval = py::bool_(inpval.get<bool>());
      },
      [](const py::object& inpval, varval_t& outval) { // decoder
        outval.set<bool>(inpval.cast<bool>());
      });
  ///////////////////////////////
  registerCodec(
      int_type, //
      TypeId::of<int>(),
      [](const varval_t& inpval, pybind11::object& outval) { // encoder
        outval = py::int_(inpval.get<int>());
      },
      [](const py::object& inpval, varval_t& outval) { // decoder
        outval.set<int>(inpval.cast<int>());
      });
  ///////////////////////////////
  registerCodec(
      float_type, //
      TypeId::of<float>(),
      [](const varval_t& inpval, pybind11::object& outval) { // encoder
        outval = py::float_(inpval.get<float>());
      },
      [](const py::object& inpval, varval_t& outval) { // decoder
        outval.set<float>(inpval.cast<float>());
      });
  ///////////////////////////////
  registerCodec(
      str_type, //
      TypeId::of<std::string>(),
      [](const varval_t& inpval, pybind11::object& outval) { // encoder
        outval = py::str(inpval.get<std::string>());
      },
      [](const py::object& inpval, varval_t& outval) { // decoder
        outval.set<std::string>(inpval.cast<std::string>());
      });
  ///////////////////////////////
  registerCodec64(
      bool_type, //
      TypeId::of<bool>(),
      [](const svar64_t& inpval, pybind11::object& outval) { // encoder
        outval = py::bool_(inpval.get<bool>());
      },
      [](const py::object& inpval, svar64_t& outval) { // decoder
        outval.set<bool>(inpval.cast<bool>());
      });
  ///////////////////////////////
  registerCodec64(
      int_type, //
      TypeId::of<int>(),
      [](const svar64_t& inpval, pybind11::object& outval) { // encoder
        outval = py::int_(inpval.get<int>());
      },
      [](const py::object& inpval, svar64_t& outval) { // decoder
        outval.set<int>(inpval.cast<int>());
      });
  ///////////////////////////////
  registerCodec64(
      float_type, //
      TypeId::of<float>(),
      [](const svar64_t& inpval, pybind11::object& outval) { // encoder
        outval = py::float_(inpval.get<float>());
      },
      [](const py::object& inpval, svar64_t& outval) { // decoder
        outval.set<float>(inpval.cast<float>());
      });
  ///////////////////////////////
  registerCodec64(
      str_type, //
      TypeId::of<std::string>(),
      [](const svar64_t& inpval, pybind11::object& outval) { // encoder
        outval = py::str(inpval.get<std::string>());
      },
      [](const py::object& inpval, svar64_t& outval) { // decoder
        outval.set<std::string>(inpval.cast<std::string>());
      });
  }
//////////////////////////////////
void TypeCodec::registerCodec(
    const pybind11::object& pytype, //
    const ork::TypeId& orktypeid,
    encoderfn_t efn,
    decoderfn_t dfn) {
  auto& item    = _impl.get<PyCodecImpl>()._codecs_by_orktype[orktypeid._hashed];
  item._pytype  = pytype;
  item._orktype = orktypeid;
  item._encoder = efn;
  item._decoder = dfn;
}
//////////////////////////////////
void TypeCodec::registerCodec64(
    const pybind11::object& pytype, //
    const ork::TypeId& orktypeid,
    encoderfn64_t efn,
    decoderfn64_t dfn) {
  auto& item    = _impl.get<PyCodecImpl>()._codecs64_by_orktype[orktypeid._hashed];
  item._pytype  = pytype;
  item._orktype = orktypeid;
  item._encoder = efn;
  item._decoder = dfn;
}
//////////////////////////////////
py::object TypeCodec::encode(const varval_t& val) const {
  return _impl.get<PyCodecImpl>().encode(val);
}
//////////////////////////////////
varval_t TypeCodec::decode(const py::object& val) const {
  return _impl.get<PyCodecImpl>().decode(val);
}
//////////////////////////////////
svar64_t TypeCodec::decode64(const py::object& val) const {
  return _impl.get<PyCodecImpl>().decode64(val);
}
//////////////////////////////////
std::shared_ptr<TypeCodec> TypeCodec::instance() { // static
  struct TypeCodecFactory : public TypeCodec {
    TypeCodecFactory()
        : TypeCodec(){};
  };
  static auto _instance = std::make_shared<TypeCodecFactory>();
  return _instance;
}

} // namespace ork::python
