#include "f4se/BSGraphics.h"

// 2CA5233612B3158658DB6DB9C90FD0258F1836E2+124
RelocPtr <BSRenderer*> g_renderer(0x06759728);

// 6BF9214E9DC5338FC817F85B0716990E5CA7C862+31
RelocPtr <BSRenderManager> g_renderManager(0x06219900);

// FA43F2F87927D8F20B17E756782BC91BB6BD04C2+3B
RelocPtr <BSRenderTargetManager> g_renderTargetManager(0x03887D30);

// 4A4A200E8F9173F8CE99D39E27F4BDAF680DF52B+9C
RelocPtr <BSShaderResourceManager> g_shaderResourceManager(0x05C41F08);

// 84E19996C30AD51CE0D4AEF3E6ED8FFFF4AE4BD7+18
RelocPtr <ID3D11Device*>			g_D3D11Device(0x060D4F88);

// BFDAF477B684098E9F394A636CCDC1BBD06EDEBF+25
RelocPtr <ID3D11DeviceContext*>		g_D3D11DeviceContext(0x06216C60);
