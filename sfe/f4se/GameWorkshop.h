#pragma once

#include "f4se/GameTypes.h"

class TESObjectREFR;
class LocationData;
class BSExtraData;
class NiPoint3;
class bhkWorld;

typedef void(*_LinkPower)(BSExtraData* workshopExtraData, TESObjectREFR* akRef1, TESObjectREFR* akRef2, TESObjectREFR* akWireRef); // Wire optional
extern RelocAddr <_LinkPower> LinkPower_Internal;

typedef void(*_LinkPower2)(TESObjectREFR* akRef, BSExtraData* workshopExtraData);
extern RelocAddr <_LinkPower2> LinkPower2_Internal;

// Wire related calls
typedef void(*_LinkPower3)(BSExtraData* workshopExtraData, TESObjectREFR* akWireRef);
extern RelocAddr <_LinkPower3> LinkPower3_Internal;

typedef void(*_LinkPower4)(TESObjectREFR* akWireRef);
extern RelocAddr <_LinkPower4> LinkPower4_Internal;

typedef void(*_SetWireEndpoints)(TESObjectREFR* akRef1, SInt32 unk2, TESObjectREFR* akRef2, SInt32 unk4, TESObjectREFR* akWireRef);	// unk2 and unk4 always 0 - Adds the ExtraData
extern RelocAddr <_SetWireEndpoints> SetWireEndpoints_Internal;

typedef void(*_FinalizeWireLink)(LocationData* locationData, TESObjectREFR* akWireRef, TESObjectREFR* akRef1, int unk4, TESObjectREFR* akRef2, int unk6);	// unk4 and unk6 always 0
extern RelocAddr <_FinalizeWireLink> FinalizeWireLink;

typedef TESObjectREFR * (*_GetObjectAtConnectPoint)(TESObjectREFR * source, NiPoint3 * connectPos, bhkWorld * world, float unk1);
extern RelocAddr <_GetObjectAtConnectPoint> GetObjectAtConnectPoint;

struct MaterialsReturned
{
	TESForm	* form;
	UInt32	amount;
};

typedef void(*_ScrapReference)(LocationData* locationData, TESObjectREFR** akRef, tArray<MaterialsReturned> * materials);
extern RelocAddr <_ScrapReference> ScrapReference;
