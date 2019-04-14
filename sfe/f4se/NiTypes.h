#pragma once

#include "f4se/GameAPI.h"

// 8
template <class T>
class NiPointer
{
public:
	T	* m_pObject;	// 00

	inline NiPointer(T* pObject = (T*) 0)
	{
		m_pObject = pObject;
		if(m_pObject) m_pObject->IncRef();
	}

	inline ~NiPointer()
	{
		if(m_pObject) m_pObject->DecRef();
	}

	inline operator bool() const
	{
		return m_pObject != nullptr;
	}

	inline operator T *() const
	{
		return m_pObject;
	}

	inline T & operator*() const
	{
		return *m_pObject;
	}

	inline T * operator->() const
	{
		return m_pObject;
	}

	inline NiPointer <T> & operator=(const NiPointer & rhs)
	{
		if(m_pObject != rhs.m_pObject)
		{
			if(rhs) rhs.m_pObject->IncRef();
			if(m_pObject) m_pObject->DecRef();

			m_pObject = rhs.m_pObject;
		}

		return *this;
	}

	inline NiPointer <T> & operator=(T * rhs)
	{
		if(m_pObject != rhs)
		{
			if(rhs) rhs->IncRef();
			if(m_pObject) m_pObject->DecRef();

			m_pObject = rhs;
		}

		return *this;
	}

	inline bool operator==(T * pObject) const
	{
		return m_pObject == pObject;
	}

	inline bool operator!=(T * pObject) const
	{
		return m_pObject != pObject;
	}

	inline bool operator==(const NiPointer & ptr) const
	{
		return m_pObject == ptr.m_pObject;
	}

	inline bool operator!=(const NiPointer & ptr) const
	{
		return m_pObject != ptr.m_pObject;
	}
};

#define MAKE_NI_POINTER(x)	class x; typedef NiPointer <x> x##Ptr

template <class T_to, class T_from>
T_to * niptr_cast(const T_from & src)
{
	return static_cast <T_to *>(src.m_pObject);
}

// C
class NiPoint3
{
public:
	float	x;	// 0
	float	y;	// 4
	float	z;	// 8

	NiPoint3();
	NiPoint3(float X, float Y, float Z) : x(X), y(Y), z(Z) { };

	// Negative
	NiPoint3 operator- () const;

	// Basic operations
	NiPoint3 operator+ (const NiPoint3& pt) const;
	NiPoint3 operator- (const NiPoint3& pt) const;

	NiPoint3& operator+= (const NiPoint3& pt);
	NiPoint3& operator-= (const NiPoint3& pt);

	// Scalar operations
	NiPoint3 operator* (float fScalar) const;
	NiPoint3 operator/ (float fScalar) const;

	NiPoint3& operator*= (float fScalar);
	NiPoint3& operator/= (float fScalar);
};

class __declspec(align(8)) NiPoint3A : NiPoint3
{
public:

};

// C
class NiColor
{
public:
	float	r;	// 00
	float	g;	// 04
	float	b;	// 08
};

// 10
class NiColorA
{
public:
	float	r;	// 00
	float	g;	// 04
	float	b;	// 08
	float	a;	// 0C
};

// 10
template <class T>
class NiRect
{
public:
	T	m_left;		// 00
	T	m_right;	// 04
	T	m_top;		// 08
	T	m_bottom;	// 0C
};

// 1C
class NiFrustum
{
public:
	float	m_fLeft;	// 00
	float	m_fRight;	// 04
	float	m_fTop;		// 08
	float	m_fBottom;	// 0C
	float	m_fNear;	// 10
	float	m_fFar;		// 14
	bool	m_bOrtho;	// 18
};

// 10
class NiQuaternion
{
public:
	// w is first

	float	m_fW;	// 0
	float	m_fX;	// 4
	float	m_fY;	// 8
	float	m_fZ;	// C


	void SetEulerAngles(float pitch, float roll, float yaw)
	{
		float t0 = std::cos(yaw * 0.5);
		float t1 = std::sin(yaw * 0.5);
		float t2 = std::cos(roll * 0.5);
		float t3 = std::sin(roll * 0.5);
		float t4 = std::cos(pitch * 0.5);
		float t5 = std::sin(pitch * 0.5);

		m_fW = t0 * t2 * t4 + t1 * t3 * t5;
		m_fX = t0 * t3 * t4 - t1 * t2 * t5;
		m_fY = t0 * t2 * t5 + t1 * t3 * t4;
		m_fZ = t1 * t2 * t4 - t0 * t3 * t5;
	}

	void GetEulerAngles(float& roll, float& pitch, float& yaw)
	{
		float ysqr = m_fY * m_fY;

		// roll (x-axis rotation)
		float t0 = +2.0 * (m_fW * m_fX + m_fY * m_fZ);
		float t1 = +1.0 - 2.0 * (m_fX * m_fX + ysqr);
		roll = std::atan2(t0, t1);

		// pitch (y-axis rotation)
		float t2 = +2.0 * (m_fW * m_fY - m_fZ * m_fX);
		t2 = t2 > 1.0 ? 1.0 : t2;
		t2 = t2 < -1.0 ? -1.0 : t2;
		pitch = std::asin(t2);

		// yaw (z-axis rotation)
		float t3 = +2.0 * (m_fW * m_fZ + m_fX * m_fY);
		float t4 = +1.0 - 2.0 * (ysqr + m_fZ * m_fZ);  
		yaw = std::atan2(t3, t4);
	}
};

// 8
class NiPoint2
{
public:
	float	x;	// 0
	float	y;	// 4
};

// 10
class NiBound
{
public:
	NiPoint3 m_kCenter;
	union
	{
		float m_fRadius;
		int m_iRadiusAsInt;
	};
};

class NiPoint4
{
public:
	struct NiPoint4Struct
	{
		float x;
		float y;
		float z;
		float w;
	};
	union
	{
		NiPoint4Struct v;
		float m_pt[4];
	};
};


class NiMatrix3
{
public:
	NiPoint4 m_pEntry[3];
};

// 30
class NiMatrix43
{
public:
	union
	{
		float	data[3][4];
		float   arr[12];
	};

	NiMatrix43 Transpose() const;
	NiPoint3 operator* (const NiPoint3& pt) const;
};

// math.h
#define MATH_PI       3.14159265358979323846

// 40
class NiTransform
{
public:
	NiMatrix43	rot;	// 00
	NiPoint3	pos;	// 30
	float		scale;	// 3C

	NiPoint3 operator*(const NiPoint3 & pt) const;
};
STATIC_ASSERT(sizeof(NiTransform) == 0x40);

// 10
struct NiPlane
{
	NiPoint3 m_kNormal;
	float m_fConstant;
};

// 18
template <typename T>
class NiTArray
{
public:
	NiTArray();
	virtual ~NiTArray();

	// sparse array, can have NULL entries that should be skipped
	// iterate from 0 to m_emptyRunStart - 1

//	void	** _vtbl;			// 00
	T		* m_data;			// 08
	UInt16	m_arrayBufLen;		// 10 - max elements storable in m_data
	UInt16	m_emptyRunStart;	// 12 - index of beginning of empty slot run
	UInt16	m_size;				// 14 - number of filled slots
	UInt16	m_growSize;			// 16 - number of slots to grow m_data by
};

// 20
// derives from NiTMapBase, we don't bother
template <typename T_key, typename T_data>
class NiTMap
{
public:
	virtual ~NiTMap();

	struct NiTMapItem
	{
		NiTMapItem	* next;
		T_key		key;
		T_data		data;
	};

	T_data	Get(T_key key)
	{
		UInt32	bucket = GetBucket(key);

		for(NiTMapItem * iter = buckets[bucket]; iter; iter = iter->next)
		{
			if(Compare(iter->key, key))
			{
				return iter->data;
			}
		}

		return T_data();
	}

	virtual UInt32	GetBucket(T_key key);					// return hash % numBuckets;
	virtual bool	Compare(T_key lhs, T_key rhs);			// return lhs == rhs;
	virtual void	FillItem(NiTMapItem * item, T_key key, T_data data);
	// item->key = key; item->data = data;
	virtual void	Fn_04(UInt32 arg);	// nop
	virtual NiTMapItem *	AllocItem(void);				// return new NiTMapItem;
	virtual void	FreeItem(NiTMapItem * item);			// item->data = 0; delete item;

	void RemoveAll()
	{
		for (UInt32 ui = 0; ui < numBuckets; ui++) 
		{
			while (buckets[ui])
			{
				NiTMapItem * pkSave = buckets[ui];
				buckets[ui] = buckets[ui]->next;
				DeleteItem(pkSave);
			}
		}

		numEntries = 0;
	}

	//	void		** _vtbl;	// 00
	UInt32		numBuckets;	// 08
	UInt32		unk0C;		// 0C
	NiTMapItem	** buckets;	// 10
	UInt32		numEntries;	// 18
};

// 10
template <typename T_key, typename T_data>
class NiTPointerMap : public NiTMap <T_key, T_data>
{
public:

};
