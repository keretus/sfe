#include "f4se/GameMenus.h"

// 2CA5233612B3158658DB6DB9C90FD0258F1836E2+AD
RelocPtr <UI*> g_ui(0x05909918);

RelocAddr <_HasHUDContext> HasHUDContext(0x00A4F270);

RelocAddr<_GetChildElement>		GetChildElement(0x020F0CA0);

// 2CA5233612B3158658DB6DB9C90FD0258F1836E2+F1
RelocPtr <UIMessageManager*>	g_uiMessageManager(0x05909B48);

// E8B45849BEED1FD76CD4D25F030C48F09D0B41F1+90
RelocPtr <BSReadWriteLock> g_menuTableLock(0x065B04B8);

bool UI::IsMenuOpen(const BSFixedString & menuName)
{
	return CALL_MEMBER_FN(this, IsMenuOpen)(menuName);
}

bool UI::IsMenuRegistered(BSFixedString & menuName)
{
	BSReadLocker locker(g_menuTableLock);
	MenuTableItem * item = menuTable.Find(&menuName);
	if (item) {
		return true;
	}

	return false;
}

IMenu * UI::GetMenu(BSFixedString & menuName)
{
	if (!menuName.data->Get<char>())
		return nullptr;

	BSReadLocker locker(g_menuTableLock);
	MenuTableItem * item = menuTable.Find(&menuName);
	if (!item) {
		return nullptr;
	}

	IMenu * menu = item->menuInstance;
	if(!menu) {
		return nullptr;
	}

	return menu;
}

IMenu * UI::GetMenuByMovie(GFxMovieView * movie)
{
	if (!movie) {
		return nullptr;
	}

	BSReadLocker locker(g_menuTableLock);

	IMenu * menu = nullptr;
	menuTable.ForEach([movie, &menu](MenuTableItem * item)
	{
		IMenu * itemMenu = item->menuInstance;
		if(itemMenu) {
			GFxMovieView * view = itemMenu->movie;
			if(view) {
				if(movie == view) {
					menu = itemMenu;
					return false;
				}
			}
		}
		return true;
	});

	return menu;
}

bool UI::UnregisterMenu(BSFixedString & name, bool force)
{
	BSReadAndWriteLocker locker(g_menuTableLock);
	MenuTableItem * item = menuTable.Find(&name);
	if (!item || (item->menuInstance && !force)) {
		return false;
	}

	return menuTable.Remove(&name);
}

HUDComponentBase::HUDComponentBase(GFxValue * parent, const char * componentName, HUDContextArray<BSFixedString> * contextList)
{
	Impl_ctor(parent, componentName, contextList);
}

HUDComponentBase::~HUDComponentBase()
{
	for(UInt32 i = 0; i < contexts.count; i++)
	{
		contexts.entries[i].Release();
	}

	Heap_Free(contexts.entries);
	contexts.count = 0;
}

void HUDComponentBase::UpdateVisibilityContext(void * unk1)
{
	HasHUDContext(&contexts, unk1);
	bool bVisible = IsVisible();
	double alpha = 0.0;
	if(bVisible) {
		alpha = 100.0;
	}

	BSGFxDisplayObject::BSDisplayInfo dInfo;
	void * unk2 = GetExtDisplayInfo(&dInfo, this);
	SetExtDisplayInfoAlpha(unk2, alpha);
	SetExtDisplayInfo(&dInfo);

	unkEC = bVisible == false;
}

void HUDComponentBase::ColorizeComponent()
{
	SetFilterColor(isWarning);
}

GameMenuBase::GameMenuBase() : IMenu()
{
	Impl_ctor();
}

GameMenuBase::~GameMenuBase()
{
	Impl_dtor();
}