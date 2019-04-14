#pragma once

#include "f4se_common/Utilities.h"
#include "f4se/GameEvents.h"
#include "f4se/ScaleformTypes.h"
#include <xmmintrin.h>

class GFxMovieRoot;

// 20
class GFxValue
{
public:
	GFxValue()					: objectInterface(NULL), type(kType_Undefined), unk18(nullptr)	{ data.obj = nullptr; }
	GFxValue(int v)				: objectInterface(NULL), type(kType_Int), unk18(nullptr)		{ data.number = v; }
	GFxValue(UInt32 v)			: objectInterface(NULL), type(kType_UInt), unk18(nullptr)		{ data.u32 = v; }
	GFxValue(SInt32 v)			: objectInterface(NULL), type(kType_Int), unk18(nullptr)		{ data.s32 = v; }
	GFxValue(double v)			: objectInterface(NULL), type(kType_Number), unk18(nullptr)		{ data.number = v; }
	GFxValue(bool v)			: objectInterface(NULL), type(kType_Bool), unk18(nullptr)		{ data.boolean = v; }
	GFxValue(const char* ps)	: objectInterface(NULL), type(kType_String), unk18(nullptr)		{ data.string = ps; }

	GFxValue(GFxValue * value)  : objectInterface(value->objectInterface), type(value->type), unk18(value->unk18) { data.obj = value->data.obj; }
	~GFxValue();

	enum Type
	{
		kType_Undefined =	0,
		kType_Null,
		kType_Bool,
		kType_Int,
		kType_UInt,
		kType_Number,
		kType_String,
		kType_Unknown7,
		kType_Object,
		kType_Array,
		kType_DisplayObject,
		kType_Function,

		kTypeFlag_Managed =	1 << 6,

		kMask_Type =		0x8F,	// not sure why it checks the top bit
	};

	union Data
	{
		UInt32			u32;
		SInt32			s32;
		double			number;
		bool			boolean;
		const char		* string;
		const char		** managedString;
		void			* obj;
	};

	// D8
	class DisplayInfo
	{
	public:
		DisplayInfo() : _varsSet(0), unkD0(0), unkD6(0) {}
		enum
		{
			kChange_x				= (1 << 0),
			kChange_y				= (1 << 1),
			kChange_rotation		= (1 << 2),
			kChange_xscale			= (1 << 3),
			kChange_yscale			= (1 << 4),
			kChange_alpha			= (1 << 5),
			kChange_visible			= (1 << 6),
			kChange_z				= (1 << 7),
			kChange_xrotation		= (1 << 8),
			kChange_yrotation		= (1 << 9),
			kChange_zscale			= (1 << 10),
			kChange_FOV				= (1 << 11),
			kChange_projMatrix3D	= (1 << 12),
			kChange_viewMatrix3D	= (1 << 13)
		};

		double		_x;				// 00
		double		_y;				// 08
		double		_rotation;		// 10
		double		_xScale;		// 18
		double		_yScale;		// 20
		double		_alpha;			// 28
		bool		_visible;		// 30
		double		_z;				// 38
		double		_xRotation;		// 40
		double		_yRotation;		// 48
		double		_zScale;		// 50
		double		_perspFOV;		// 58
		GMatrix3F	_viewMatrix3D;	// 60
		GMatrix4F	_perspectiveMatrix3D;	// A0
		UInt32		unkD0;			// D0
		UInt16		_varsSet;		// D4
		UInt16		unkD6;			// D6

		void SetX(double x)					{ SetFlags(kChange_x); _x = x; }
		void SetY(double y)					{ SetFlags(kChange_y); _y = y; }
		void SetRotation(double degrees)	{ SetFlags(kChange_rotation); _rotation = degrees; }
		void SetXScale(double xscale)		{ SetFlags(kChange_xscale); _xScale = xscale; }
		void SetYScale(double yscale)		{ SetFlags(kChange_yscale); _yScale = yscale; }
		void SetAlpha(double alpha)			{ SetFlags(kChange_alpha); _alpha = alpha; }
		void SetVisible(bool visible)		{ SetFlags(kChange_visible); _visible = visible; }
		void SetZ(double z)					{ SetFlags(kChange_z); _z = z; }
		void SetXRotation(double degrees)	{ SetFlags(kChange_xrotation); _xRotation = degrees; }
		void SetYRotation(double degrees)	{ SetFlags(kChange_yrotation); _yRotation = degrees; }
		void SetZScale(double zscale)		{ SetFlags(kChange_zscale); _zScale = zscale; }
		void SetFOV(double fov)				{ SetFlags(kChange_FOV); _perspFOV = fov; }
		void SetProjectionMatrix3D(const GMatrix4F *pmat)  
		{ 
			if (pmat) {
				SetFlags(kChange_projMatrix3D);
				_perspectiveMatrix3D = *pmat;
			} else
				ClearFlags(kChange_projMatrix3D); 
		}
		void SetViewMatrix3D(const GMatrix3F *pmat) 
		{ 
			if (pmat) { 
				SetFlags(kChange_viewMatrix3D); 
				_viewMatrix3D = *pmat;
			} else
				ClearFlags(kChange_viewMatrix3D); 
		}

		// Convenience functions
		void SetPosition(double x, double y) { SetFlags(kChange_x | kChange_y); _x = x; _y = y; }
		void SetScale(double xscale, double yscale)  { SetFlags(kChange_xscale | kChange_yscale); _xScale = xscale; _yScale = yscale; }

		void SetFlags(UInt32 flags) { _varsSet |= flags; }
		void ClearFlags(UInt32 flags) { _varsSet &= ~flags; }
	};

	class ObjectInterface
	{
	public:
		class ArrayVisitor
        {
        public:
            virtual ~ArrayVisitor() {}
            virtual void Visit(UInt32 idx, GFxValue * val) = 0;
        };
		class ObjVisitor
		{
		public:
			virtual ~ObjVisitor() { }
			virtual bool ShowDisplayMembers(void) { return false; }
			virtual void Visit(const char * member, GFxValue * value) = 0;
		};

		void * unk1;

		MEMBER_FN_PREFIX(ObjectInterface);
		//DEFINE_MEMBER_FN(HasMember, bool, 0x020AFB90, void* pData, const char* name);
		//DEFINE_MEMBER_FN(HasMember, bool, 0x1b575a0, void* pData, const char* name);
		//DEFINE_MEMBER_FN(HasMember, bool, 0x01be5c30+0x110, void* pData, const char* name);
		//DEFINE_MEMBER_FN(GetMember, bool, 0x020A7F40, void* pData, const char* name, GFxValue* pValue, bool isDisplayObj);
		//DEFINE_MEMBER_FN(GetMember, bool, 0x1b578d0, void* pData, const char* name, GFxValue* pValue, bool isDisplayObj);
		//DEFINE_MEMBER_FN(GetMember, bool, 0x01be5c30+0x440, void* pData, const char* name, GFxValue* pValue, bool isDisplayObj);
		//DEFINE_MEMBER_FN(SetMember, bool, 0x020D04C0, void* pData, const char* name, const GFxValue* pValue, bool isDisplayObj);
		//DEFINE_MEMBER_FN(SetMember, bool, 0x1B57C90, void* pData, const char* name, const GFxValue* pValue, bool isDisplayObj
		//DEFINE_MEMBER_FN(SetMember, bool, 0x01be5c30+0x800, void* pData, const char* name, const GFxValue* pValue, bool isDisplayObj);
		//DEFINE_MEMBER_FN(Invoke, bool, 0x020B1C00, void * pData, GFxValue * result, const char * name, GFxValue * args, UInt64 numArgs, UInt8 isDisplayObj);
		//DEFINE_MEMBER_FN(Invoke, bool, 0x1b580d0, void * pData, GFxValue * result, const char * name, GFxValue * args, UInt64 numArgs, UInt8 isDisplayObj);
		//DEFINE_MEMBER_FN(Invoke, bool, 0x01be5c30+0xc40, void * pData, GFxValue * result, const char * name, GFxValue * args, UInt64 numArgs, UInt8 isDisplayObj);
		////////////////
		DEFINE_MEMBER_FN(CreateEmptyMovieClip, bool, 0x02089FA0, void* pData, GFxValue* pValue, const char* instanceName, SInt32 depth);
		DEFINE_MEMBER_FN(AttachMovie, bool, 0x02081510, void* pData, GFxValue* pValue, const char* symbolName, const char* instanceName, SInt32 depth, const void * initArgs);
		DEFINE_MEMBER_FN(GetArraySize, UInt32, 0x020A1D40, void * pData);
		// ref CC19A4FFD76032A42FBBC61E80011469E50993D7 (+4)
		DEFINE_MEMBER_FN(SetArraySize, bool, 0x0219C770, void * pData, UInt32 size);
		DEFINE_MEMBER_FN(GetElement, bool, 0x020A5A30, void * pData, UInt32 index, GFxValue * value);
		DEFINE_MEMBER_FN(PushBack, bool, 0x020C2850, void * pData, GFxValue * value);
		DEFINE_MEMBER_FN(PopBack, bool, 0x020BE770, void * pData, GFxValue * value);
		DEFINE_MEMBER_FN(VisitElements, void, 0x020DAFC0, void * pData, ArrayVisitor * visitor, UInt32 idx, SInt32 count);
		DEFINE_MEMBER_FN(GotoLabeledFrame, bool, 0x020AF5D0, void * pData, const char * frameLabel, bool stop);
		// ref 1A7DD5D4A014A3E7CBF9A53D55DA751C11218613 (+1E7)
		DEFINE_MEMBER_FN(VisitMembers, void, 0x020DB0F0, void * pData, ObjVisitor * visitor, bool isDisplayObj);
		DEFINE_MEMBER_FN(GetText, bool, 0x020AD7B0, void * pData, GFxValue * value, bool html);
		DEFINE_MEMBER_FN(SetText, bool, 0x020D35A0, void * pData, const char * text, bool html);
		DEFINE_MEMBER_FN(GetDisplayInfo, bool, 0x020A5120, void * pData, DisplayInfo * displayInfo);
		DEFINE_MEMBER_FN(SetDisplayInfo, bool, 0x020CEDD0, void * pData, DisplayInfo * displayInfo);
		//DEFINE_MEMBER_FN(AddManaged_Internal, void, 0x020B9B10, GFxValue * value, void * pData);
		//DEFINE_MEMBER_FN(AddManaged_Internal, void, 0x01B57490, GFxValue * value, void * pData);
		//DEFINE_MEMBER_FN(AddManaged_Internal, void, 0x01be5c30, GFxValue * value, void * pData);
		//DEFINE_MEMBER_FN(ReleaseManaged_Internal, void, 0x020B9B60, GFxValue * value, void * pData);
		//DEFINE_MEMBER_FN(ReleaseManaged_Internal, void, 0x01B57510, GFxValue * value, void * pData);
		//DEFINE_MEMBER_FN(ReleaseManaged_Internal, void, 0x01be5c30+0x80, GFxValue * value, void * pData);

		///
		/*DEFINE_MEMBER_FN(AddManaged_Internal, void, 0x1b78e90, GFxValue * value, void * pData);
		DEFINE_MEMBER_FN(ReleaseManaged_Internal, void, 0x1b78e90 + 0x80, GFxValue * value, void * pData);
		DEFINE_MEMBER_FN(HasMember, bool, 0x1b78e90 + 0x110, void* pData, const char* name);
		DEFINE_MEMBER_FN(GetMember, bool, 0x1b78e90 + 0x440, void* pData, const char* name, GFxValue* pValue, bool isDisplayObj);
		DEFINE_MEMBER_FN(SetMember, bool, 0x1b78e90 + 0x800, void* pData, const char* name, const GFxValue* pValue, bool isDisplayObj);
		DEFINE_MEMBER_FN(Invoke, bool, 0x1b78e90 + 0xc40, void * pData, GFxValue * result, const char * name, GFxValue * args, UInt64 numArgs, UInt8 isDisplayObj);*/
		DEFINE_MEMBER_FN(AddManaged_Internal, void, 0x1C1C220, GFxValue * value, void * pData);
		DEFINE_MEMBER_FN(ReleaseManaged_Internal, void, 0x1C1C220 + 0x80, GFxValue * value, void * pData);
		DEFINE_MEMBER_FN(HasMember, bool, 0x1C1C220 + 0x110, void* pData, const char* name);
		DEFINE_MEMBER_FN(GetMember, bool, 0x1C1C220 + 0x440, void* pData, const char* name, GFxValue* pValue, bool isDisplayObj);
		DEFINE_MEMBER_FN(SetMember, bool, 0x1C1C220 + 0x800, void* pData, const char* name, const GFxValue* pValue, bool isDisplayObj);
		DEFINE_MEMBER_FN(Invoke, bool, 0x1C1C220 + 0xc40, void * pData, GFxValue * result, const char * name, GFxValue * args, UInt64 numArgs, UInt8 isDisplayObj);
	};
	void*unk00;
	void*unk08;
	//UInt64 unk00 = -1;
	//UInt64 unk08 = -1;
	ObjectInterface	* objectInterface;	// 00
	Type			type;				// 08
	Data			data;				// 10
	void			* unk18;			// 18


	UInt32	GetType(void) const		{ return type & kMask_Type; }
	bool	IsManaged(void) const { return (type & kTypeFlag_Managed) != 0; }
	void	CleanManaged(void);
	void	AddManaged(void);

	bool	IsUndefined() const		{ return GetType() == kType_Undefined; }
	bool	IsNull() const			{ return GetType() == kType_Null; }
	bool	IsBool() const			{ return GetType() == kType_Bool; }
	bool	IsNumber() const		{ return GetType() == kType_Number; }
	bool	IsString() const		{ return GetType() == kType_String; }
	bool	IsObject() const		{ return (GetType() == kType_Object) ||  GetType() == kType_Array ||  GetType() == kType_DisplayObject; }
	bool	IsArray() const			{ return GetType() == kType_Array; }
	bool	IsDisplayObject() const	{ return GetType() == kType_DisplayObject; }
	bool	IsFunction() const		{ return GetType() == kType_Function; }

	void	SetUndefined(void);
	void	SetNull(void);
	void	SetBool(bool value);
	void	SetInt(SInt32 value);
	void	SetUInt(UInt32 value);
	void	SetNumber(double value);
	void	SetString(const char * value);

	bool			GetBool(void) const;
	const char *	GetString(void) const;
	double			GetNumber(void) const;
	SInt32			GetInt(void) const;
	UInt32			GetUInt(void) const;

	bool	HasMember(const char * name);
	bool	SetMember(const char * name, GFxValue * value);
	bool	GetMember(const char * name, GFxValue * value);
	bool	Invoke(const char * name, GFxValue * result, GFxValue * args, UInt32 numArgs);

	bool	CreateEmptyMovieClip(GFxValue* pValue, const char* instanceName, SInt32 depth);
	bool	AttachMovie(GFxValue* pValue, const char* symbolName, const char* instanceName, SInt32 depth, const void * initArgs);
	bool	GotoLabeledFrame(const char * frameLabel, bool stop);

	UInt32	GetArraySize();
	bool	SetArraySize(UInt32 size);
	bool	GetElement(UInt32 index, GFxValue * value);
	bool	PushBack(GFxValue * value);
	bool	PopBack(GFxValue * value);
	void	VisitElements(ObjectInterface::ArrayVisitor * visitor, UInt32 idx, SInt32 count);
	void	VisitMembers(ObjectInterface::ObjVisitor * visitor);
	bool	GetText(GFxValue * value, bool html);
	bool	SetText(const char * text, bool html);
	bool	GetDisplayInfo(DisplayInfo * displayInfo);
	bool	SetDisplayInfo(DisplayInfo * displayInfo);

	MEMBER_FN_PREFIX(GFxValue);
	DEFINE_MEMBER_FN(RemoveChild_Internal, void, 0x0210D220, GFxValue * name);
};

// 38
class BSGFxObject : public GFxValue
{
public:
	BSGFxObject() : unk20(0), unk28(0), unk30(0) { }
	BSGFxObject(GFxValue * value) : GFxValue(value), unk20(0), unk28(0), unk30(0)
	{
		if(value->IsManaged())
			value->AddManaged();
	}

	UInt64	unk20;	// 20
	UInt64	unk28;	// 28
	UInt64	unk30;	// 30
};

// 50
class BSGFxDisplayObject : public BSGFxObject
{
public:
	BSGFxDisplayObject() : parent(nullptr), unk48(0), unk4C(0) { }
	BSGFxDisplayObject(GFxValue * value) : BSGFxObject(value), parent(nullptr)
	{
		_MESSAGE("this will crash");
		GFxValue width, height;
		GetMember("width", &width);
		GetMember("height", &height);
		unk48 = width.GetNumber();
		unk4C = height.GetNumber();
	}
	virtual ~BSGFxDisplayObject()
	{
		if (parent)
		{
			_MESSAGE("this will crash");
			CALL_MEMBER_FN(parent, RemoveChild_Internal)(this);
		}
	};

	struct BSDisplayInfo
	{
		BSGFxDisplayObject		* displayObject;	// 00
		BSGFxDisplayObject		* unk08;			// 08
		GFxValue::DisplayInfo	displayInfo1;		// 10
		UInt64					unkE8;				// E8
		GFxValue::DisplayInfo	displayInfo2;		// F0
	};

	GFxValue	* parent;	// 40
	float		unk48;		// 48
	float		unk4C;		// 4C
};
STATIC_ASSERT(sizeof(BSGFxDisplayObject) == 0x50);
STATIC_ASSERT(offsetof(BSGFxDisplayObject::BSDisplayInfo, displayInfo2) == 0xF0);

class BSGFxShaderFXTarget;

struct FilterColor
{
	float r, g, b;
};

// B0
class BSGFxShaderFXTarget : public BSGFxDisplayObject,
							public BSTEventSink<ApplyColorUpdateEvent>
{
public:
	BSGFxShaderFXTarget() { }
	BSGFxShaderFXTarget(GFxValue * source) : BSGFxDisplayObject(source), 
		unk58(0), unk60(0), unk68(0), unk6C(0), unk70(0), unk74(0), unk78(0), unk7C(0), red(0), 
		green(0), blue(0), multiplier(0), unk94(0), unk98(0), unkA0(0), colorType(0), unkAC(0) {  }//{ CALL_MEMBER_FN(this, Impl_ctor)(source); }
	virtual ~BSGFxShaderFXTarget();// { CALL_MEMBER_FN(this, Impl_dtor)(); };

	virtual void Unk_01(void * unk1, void * unk2)
	{
		_MESSAGE("this will crash");
		Impl_Fn1(unk1, unk2);
	};

	virtual	EventResult	ReceiveEvent(ApplyColorUpdateEvent * evn, void * dispatcher);

	enum ColorTypes
	{
		kColorUnk1 = 0,
		kColorUnk2,
		kColorNormal,
		kColorUnk3,
		kColorUnk4,
		kColorWarning,
		kColorUnk6,
		kColorUnk7
	};

	UInt64	unk58;			// 58
	UInt64	unk60;			// 60
	float	unk68;			// 68
	float	unk6C;			// 6C
	float	unk70;			// 70
	float	unk74;			// 74
	float	unk78;			// 78
	float	unk7C;			// 7C
	float	red;			// 80
	float	green;			// 84
	float	blue;			// 88
	UInt32	colorFlags;		// 8C
	float	multiplier;		// 90
	UInt32	unk94;			// 94
	UInt64	unk98;			// 98
	UInt64	unkA0;			// A0
	UInt32	colorType;		// A8
	UInt32	unkAC;			// AC

	DEFINE_STATIC_HEAP(Heap_Allocate, Heap_Free)

	void SetFilterColor(bool isHostile);

	// 98B654B565F35633CBE8804A5CBF84646AE30A1B+9
	DEFINE_MEMBER_FN_1(Impl_ctor, BSGFxShaderFXTarget *, 0x020F1650, GFxValue * source);
	DEFINE_MEMBER_FN_0(Impl_dtor, void, 0x020F15B0);
	DEFINE_MEMBER_FN_2(Impl_Fn1, void, 0x020F1AD0, void * unk1, void * unk2);
};
STATIC_ASSERT(offsetof(BSGFxShaderFXTarget, red) == 0x80);
STATIC_ASSERT(sizeof(BSGFxShaderFXTarget) == 0xB0);

// This function acquires the HUD color by type e.g. normal, PA, hostile
typedef FilterColor * (* _GetFilterColorByType)(BSGFxShaderFXTarget * component, FilterColor * color);
extern RelocAddr <_GetFilterColorByType> GetFilterColorByType;

// Sets explicit component filter color
typedef void (* _ApplyColorFilter)(BSGFxShaderFXTarget * component, FilterColor * color, float unk1);
extern RelocAddr <_ApplyColorFilter> ApplyColorFilter;

typedef void (* _SetDefaultColors)(BSGFxShaderFXTarget * component);
extern RelocAddr <_SetDefaultColors> SetDefaultColors;

typedef void * (* _GetExtDisplayInfo)(BSGFxDisplayObject::BSDisplayInfo * dInfo, BSGFxDisplayObject * target);
extern RelocAddr <_GetExtDisplayInfo> GetExtDisplayInfo;

typedef void (* _SetExtDisplayInfoAlpha)(void * dInfo, double alpha);
extern RelocAddr <_SetExtDisplayInfoAlpha> SetExtDisplayInfoAlpha;

typedef void (* _SetExtDisplayInfo)(BSGFxDisplayObject::BSDisplayInfo * dInfo);
extern RelocAddr <_SetExtDisplayInfo> SetExtDisplayInfo;

typedef void(* _PlayUISound)(const char *);
extern RelocAddr<_PlayUISound>	PlayUISound;

typedef void(*_CreateBaseShaderTarget)(BSGFxShaderFXTarget * & component, GFxValue & stage);
extern RelocAddr<_CreateBaseShaderTarget>	CreateBaseShaderTarget;