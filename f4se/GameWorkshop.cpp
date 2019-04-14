#include "f4se/GameWorkshop.h"

RelocAddr <_LinkPower> LinkPower_Internal(0x001F6D70); // Power related natives
RelocAddr <_LinkPower2> LinkPower2_Internal(0x00201A60); // Usually paired with LinkPower
RelocAddr <_GetObjectAtConnectPoint> GetObjectAtConnectPoint(0x001FF2B0); // Acquires objects that are touching attach points
RelocAddr <_LinkPower3> LinkPower3_Internal(0x001F67E0); // Wire related calls
RelocAddr <_LinkPower4> LinkPower4_Internal(0x00204560);
RelocAddr <_SetWireEndpoints> SetWireEndpoints_Internal(0x00200DA0);
RelocAddr <_FinalizeWireLink> FinalizeWireLink(0x00200AA0);
RelocAddr <_ScrapReference> ScrapReference(0x002083D0);
