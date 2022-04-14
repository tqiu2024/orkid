////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <utpp/UnitTest++.h>
#include <cmath>
#include <limits>
#include <string.h>

#include <ork/application/application.h>
#include <ork/reflect/serialize/JsonDeserializer.h>
#include <ork/reflect/serialize/JsonSerializer.h>
#include "reflectionclasses.inl"

using namespace ork;
using namespace ork::reflect;

std::string asset_generate() {
  auto assettest = std::make_shared<AssetTest>();

  assettest->_assetptr        = std::make_shared<asset::Asset>();
  assettest->_assetptr->_name = "dyn://yo";

  serdes::JsonSerializer ser;
  auto rootnode = ser.serializeRoot(assettest);
  return ser.output();
}

TEST(SerdesAssetProperties) {
  auto objstr = asset_generate();
  // printf("objstr<%s>\n", objstr.c_str());
  object_ptr_t instance_out;
  serdes::JsonDeserializer deser(objstr.c_str());
  deser.deserializeTop(instance_out);
  auto clone = std::dynamic_pointer_cast<AssetTest>(instance_out);
  auto asset = clone->_assetptr;
  CHECK(asset->_name == "dyn://yo");
}
