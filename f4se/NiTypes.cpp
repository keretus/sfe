#include "f4se/NiTypes.h"

NiPoint3::NiPoint3()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

NiPoint3 NiPoint3::operator- () const
{
	return NiPoint3(-x, -y, -z);
}

NiPoint3 NiPoint3::operator+ (const NiPoint3& pt) const
{
	return NiPoint3(x + pt.x, y + pt.y, z + pt.z);
}

NiPoint3 NiPoint3::operator- (const NiPoint3& pt) const
{
	return NiPoint3(x - pt.x, y - pt.y, z - pt.z);
}

NiPoint3& NiPoint3::operator+= (const NiPoint3& pt)
{
	x += pt.x;
	y += pt.y;
	z += pt.z;
	return *this;
}
NiPoint3& NiPoint3::operator-= (const NiPoint3& pt)
{
	x -= pt.x;
	y -= pt.y;
	z -= pt.z;
	return *this;
}

// Scalar operations
NiPoint3 NiPoint3::operator* (float scalar) const
{
	return NiPoint3(scalar * x, scalar * y, scalar * z);
}
NiPoint3 NiPoint3::operator/ (float scalar) const
{
	float invScalar = 1.0f / scalar;
	return NiPoint3(invScalar * x, invScalar * y, invScalar * z);
}

NiPoint3& NiPoint3::operator*= (float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}
NiPoint3& NiPoint3::operator/= (float scalar)
{
	float invScalar = 1.0f / scalar;
	x *= invScalar;
	y *= invScalar;
	z *= invScalar;
	return *this;
}

NiPoint3 NiMatrix43::operator* (const NiPoint3& pt) const
{
	return NiPoint3
		(
		data[0][0]*pt.x+data[0][1]*pt.y+data[0][2]*pt.z,
		data[1][0]*pt.x+data[1][1]*pt.y+data[1][2]*pt.z,
		data[2][0]*pt.x+data[2][1]*pt.y+data[2][2]*pt.z
		);
}

NiMatrix43 NiMatrix43::Transpose() const
{
	NiMatrix43 result;
	result.data[0][0] = data[0][0];
	result.data[0][1] = data[1][0];
	result.data[0][2] = data[2][0];
	result.data[0][3] = data[0][3];
	result.data[1][0] = data[0][1];
	result.data[1][1] = data[1][1];
	result.data[1][2] = data[2][1];
	result.data[1][3] = data[1][3];
	result.data[2][0] = data[0][2];
	result.data[2][1] = data[1][2];
	result.data[2][2] = data[2][2];
	result.data[2][3] = data[2][3];
	return result;
}

NiPoint3 NiTransform::operator*(const NiPoint3 & pt) const
{
	return (((rot * pt) * scale) + pos);
}
