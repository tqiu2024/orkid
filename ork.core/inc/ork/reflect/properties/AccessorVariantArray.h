////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include "IArray.h"

#include <ork/config/config.h>

namespace ork { namespace reflect {

class AccessorVariantArray : public IArray {
public:
  AccessorVariantArray(
      void (Object::*serialize_item)(ISerializer&, size_t) const,
      void (Object::*deserialize_item)(IDeserializer&, size_t),
      size_t (Object::*count)() const,
      void (Object::*resize)(size_t));

private:
  void serializeItem(ISerializer&, object_constptr_t, size_t) const override;
  void deserializeItem(IDeserializer&, object_ptr_t, size_t) const override;
  size_t count(object_constptr_t) const override;
  void resize(object_ptr_t, size_t) const override;

  void (Object::*mSerializeItem)(ISerializer&, size_t) const;
  void (Object::*mDeserializeItem)(IDeserializer&, size_t);
  size_t (Object::*mCount)() const;
  void (Object::*mResize)(size_t);
};

}} // namespace ork::reflect
