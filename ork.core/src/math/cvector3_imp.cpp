////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 


#include <ork/pch.h>

#include <ork/math/cvector3.h>
#include <ork/math/cvector3.hpp>
#include <ork/kernel/prop.h>
#include <ork/kernel/prop.hpp>
#include <ork/reflect/Serialize.h>
#include <ork/reflect/BidirectionalSerializer.h>
#include <ork/math/misc_math.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork {

template<> float Vector3<float>::Sin( float fin )
{
	return sinf( fin );
}
template<> float Vector3<float>::Cos( float fin )
{
	return cosf( fin );
}
template<> float Vector3<float>::Sqrt( float fin )
{
	return sqrtf( fin );
}
template<> float Vector3<float>::Epsilon()
{
	return Float::FloatEpsilon();
}
template<> float Vector3<float>::Abs( float fin )
{
	return fabs( fin );
}

///////////////////////////////////////////////////////////////////////////////

template<> double Vector3<double>::Sin( double fin )
{
	return (double) sinf((float)fin);
}
template<> double Vector3<double>::Cos( double fin )
{
	return (double) cosf((float)fin);
}
template<> double Vector3<double>::Sqrt( double fin )
{
	return (double) sqrtf( (float)fin );
}
template<> double Vector3<double>::Epsilon()
{
	return Float::DoubleEpsilon();
}
template<> double Vector3<double>::Abs( double fin )
{
	return (double) fabs((float)fin);
}

// FIXED ///////////////////////////////////////////////////////////////////////

template<> const EPropType PropType<Vector3<float> >::meType   = EPROPTYPE_VEC3FLOAT;
template<> const char* PropType<Vector3<float> >::mstrTypeName = "VEC3FLOAT";
template<> void PropType<Vector3<float> >::ToString(const Vector3<float> & Value, PropTypeString& tstr )
{
	Vector3<float> v = Value;
	tstr.format("%g %g %g", float(v.GetX()), float(v.GetY()), float(v.GetZ()));
}

template<> Vector3<float> PropType<Vector3<float> >::FromString(const PropTypeString& String)
{
	float x, y, z;
	sscanf(String.c_str(), "%g %g %g", &x, &y, &z);
	return Vector3<float>(float(x), float(y), float(z));
}

///////////////////////////////////////////////////////////////////////////////

template class Vector3<float>;		// explicit template instantiation
template class PropType<Vector3<float> >;

template class Vector3<double>;		// explicit template instantiation

///////////////////////////////////////////////////////////////////////////////

namespace reflect {

template<> void Serialize( const fvec3*in, fvec3*out, reflect::BidirectionalSerializer& bidi )
{
	if( bidi.Serializing() )
	{
		bidi.Serializer()->Hint("fvec3");
		for( int i=0; i<3; i++ )
		{
			bidi | in->GetArray()[i];
		}
	}
	else
	{
		for( int i=0; i<3; i++ )
		{
			bidi | out->GetArray()[i];
		}
	}
}}

}
