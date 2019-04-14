#include <map>
#include <string>

#include "common/IFileStream.h"

#include "f4se/PluginAPI.h"
#include "f4se/GameData.h"
#include "f4se/InternalSerialization.h"
#include "f4se/Serialization.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusObjects.h"
#include "f4se/PapyrusDelayFunctors.h"

#include "f4se/CustomMenu.h"

// Internal

static UInt8	s_savefileIndexMap[0xFF];
static UInt8	s_numSavefileMods = 0;

static UInt16	s_savefileLightIndexMap[0xFFF];
static UInt16	s_numSavefileLightMods = 0;

void LoadModList(const F4SESerializationInterface * intfc)
{
	_MESSAGE("Loading mod list:");

	char name[0x104] = { 0 };
	UInt16 nameLen = 0;

	intfc->ReadRecordData(&s_numSavefileMods, sizeof(s_numSavefileMods));
	for (UInt32 i = 0; i < s_numSavefileMods; i++)
	{
		intfc->ReadRecordData(&nameLen, sizeof(nameLen));
		intfc->ReadRecordData(&name, nameLen);
		name[nameLen] = 0;

		UInt8 newIndex = (*g_dataHandler)->GetLoadedModIndex(name);
		s_savefileIndexMap[i] = newIndex;
		_MESSAGE("\t(%d -> %d)\t%s", i, newIndex, &name);
	}
}

void SaveModList(const F4SESerializationInterface * intfc)
{
	UInt8 modCount = (*g_dataHandler)->modList.loadedMods.count;

	intfc->OpenRecord('MODS', 0);
	intfc->WriteRecordData(&modCount, sizeof(modCount));

	_MESSAGE("Saving mod list:");

	for (UInt32 i = 0; i < modCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.loadedMods[i];
		UInt16 nameLen = strlen(modInfo->name);
		intfc->WriteRecordData(&nameLen, sizeof(nameLen));
		intfc->WriteRecordData(modInfo->name, nameLen);
		_MESSAGE("\t(%d)\t%s", i, &modInfo->name);
	}
}

void LoadLightModList(const F4SESerializationInterface * intfc, bool fixedSize)
{
	_MESSAGE("Loading light mod list:");

	char name[0x104] = { 0 };
	UInt16 nameLen = 0;

	if(fixedSize)
	{
		UInt16 numMods = 0;
		intfc->ReadRecordData(&numMods, sizeof(numMods));

		s_numSavefileLightMods = numMods;
	}
	else
	{
		UInt8 numMods = 0;
		intfc->ReadRecordData(&numMods, sizeof(numMods));

		s_numSavefileLightMods = numMods;
	}

	for (UInt32 i = 0; i < s_numSavefileLightMods; i++)
	{
		intfc->ReadRecordData(&nameLen, sizeof(nameLen));
		intfc->ReadRecordData(&name, nameLen);
		name[nameLen] = 0;

		UInt16 newIndex = (*g_dataHandler)->GetLoadedLightModIndex(name);
		s_savefileLightIndexMap[i] = newIndex;
		_MESSAGE("\t(%d -> %d)\t%s", i, newIndex, &name);
	}
}

void SaveLightModList(const F4SESerializationInterface * intfc)
{
	UInt16 modCount = (*g_dataHandler)->modList.lightMods.count;

	intfc->OpenRecord('LIMD', 0);
	intfc->WriteRecordData(&modCount, sizeof(modCount));

	_MESSAGE("Saving light mod list:");

	for (UInt32 i = 0; i < modCount; i++)
	{
		ModInfo * modInfo = (*g_dataHandler)->modList.lightMods[i];
		UInt16 nameLen = strlen(modInfo->name);
		intfc->WriteRecordData(&nameLen, sizeof(nameLen));
		intfc->WriteRecordData(modInfo->name, nameLen);
		_MESSAGE("\t(%d)\t%s", i, &modInfo->name);
	}
}

UInt8 ResolveModIndex(UInt8 modIndex)
{
	return (modIndex < s_numSavefileMods) ? s_savefileIndexMap[modIndex] : 0xFF;
}

UInt16 ResolveLightModIndex(UInt16 modIndex)
{
	return (modIndex < s_numSavefileLightMods) ? s_savefileLightIndexMap[modIndex] : 0xFFFF;
}

//// Callbacks

void Core_RevertCallback(const F4SESerializationInterface * intfc)
{
	g_inputKeyEventRegs.Clear();
	g_inputControlEventRegs.Clear();
	g_externalEventRegs.Clear();
	g_cameraEventRegs.Clear();
	g_furnitureEventRegs.Clear();

	F4SEDelayFunctorManagerInstance().OnRevert();
	F4SEObjectStorageInstance().ClearAndRelease();

	// Unregister all custom menus
	g_customMenuLock.LockForReadAndWrite();
	for(auto & menuData : g_customMenuData)
	{
		BSFixedString menuName(menuData.first.c_str());
		if(!(*g_ui)->UnregisterMenu(menuName)) {
			_DMESSAGE("Failed to unregister %s, instance still exists", menuName.c_str());
		}
	}
	g_customMenuData.clear();
	g_customMenuLock.Unlock();
}

void Core_SaveCallback(const F4SESerializationInterface * intfc)
{
	using namespace Serialization;

	SaveModList(intfc);
	SaveLightModList(intfc);

	_MESSAGE("Saving key input event registrations...");
	g_inputKeyEventRegs.Save(intfc, 'KEYR', InternalEventVersion::kCurrentVersion);

	_MESSAGE("Saving control input event registrations...");
	g_inputControlEventRegs.Save(intfc, 'CTLR', InternalEventVersion::kCurrentVersion);

	_MESSAGE("Saving external event registrations...");
	g_externalEventRegs.Save(intfc, 'EXEV', InternalEventVersion::kCurrentVersion);

	_MESSAGE("Saving camera event registrations...");
	g_cameraEventRegs.Save(intfc, 'CAMR', InternalEventVersion::kCurrentVersion);

	_MESSAGE("Saving furniture event registrations...");
	g_furnitureEventRegs.Save(intfc, 'FRNR', InternalEventVersion::kCurrentVersion);

	_MESSAGE("Saving SKSEPersistentObjectStorage data...");
	SaveClassHelper(intfc, 'OBMG', F4SEObjectStorageInstance());

	_MESSAGE("Saving SKSEDelayFunctorManager data...");
	SaveClassHelper(intfc, 'DFMG', F4SEDelayFunctorManagerInstance());
}

void Core_LoadCallback(const F4SESerializationInterface * intfc)
{
	UInt32 type, version, length;

	while (intfc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		// Mod list
		case 'MODS':
			LoadModList(intfc);
			break;

		// Light Mod list (legacy)
		case 'LMOD':
			LoadLightModList(intfc, false);
			break;

		// Light mod list (16-bit size)
		case 'LIMD':
			LoadLightModList(intfc, true);
			break;

			// Key input events
		case 'KEYR':
			_MESSAGE("Loading key input event registrations...");
			g_inputKeyEventRegs.Load(intfc, InternalEventVersion::kCurrentVersion);
			break;

			// Control input events
		case 'CTLR':
			_MESSAGE("Loading control input event registrations...");
			g_inputControlEventRegs.Load(intfc, InternalEventVersion::kCurrentVersion);
			break;

			// External events
		case 'EXEV':
			_MESSAGE("Loading external event registrations...");
			g_externalEventRegs.Load(intfc, InternalEventVersion::kCurrentVersion);
			break;

			// Camera events
		case 'CAMR':
			_MESSAGE("Loading camera event registrations...");
			g_cameraEventRegs.Load(intfc, InternalEventVersion::kCurrentVersion);
			break;

			// Furniture events
		case 'FRNR':
			_MESSAGE("Loading furniture event registrations...");
			g_furnitureEventRegs.Load(intfc, InternalEventVersion::kCurrentVersion);
			break;

			// SKSEPersistentObjectStorage
		case 'OBMG':
			_MESSAGE("Loading F4SEPersistentObjectStorage data...");
			F4SEObjectStorageInstance().Load(intfc, version);
			break;

			// SKSEDelayFunctorManager
		case 'DFMG':
			_MESSAGE("Loading F4SEDelayFunctorManager data...");
			F4SEDelayFunctorManagerInstance().Load(intfc, version);
			break;

		default:
			_MESSAGE("Unhandled chunk type in Core_LoadCallback: %08X (%.4s)", type, &type);
			continue;
		}
	}
}

void Init_CoreSerialization_Callbacks()
{
	Serialization::SetUniqueID(0, 0);
	Serialization::SetRevertCallback(0, Core_RevertCallback);
	Serialization::SetSaveCallback(0, Core_SaveCallback);
	Serialization::SetLoadCallback(0, Core_LoadCallback);
}
