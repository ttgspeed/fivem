/*
 * This file is part of the CitizenFX project - http://citizen.re/
 *
 * See LICENSE and MENTIONS in the root of the source tree for information
 * regarding licensing.
 */

#pragma once

#ifdef COMPILING_NUI_CORE
#define OVERLAY_DECL __declspec(dllexport)
#else
#define OVERLAY_DECL __declspec(dllimport)
#endif

#include <memory>

#include "grcTexture.h"

#include <include/cef_app.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>

#include <queue>

class NUIExtensionHandler : public CefV8Handler
{
public:
	NUIExtensionHandler();

	virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) override;

	void InvokeNUICallback(const char* type, const CefString& name, const CefV8ValueList& arguments);

	void EnterV8Context(const char* type);
	void ExitV8Context(const char* type);

private:
	std::map<std::string, CefRefPtr<CefV8Value>> _callbackFunctions;
	std::map<std::string, CefRefPtr<CefV8Context>> _callbackContexts;

	IMPLEMENT_REFCOUNTING(NUIExtensionHandler);
};

namespace nui
{
	//void EnterV8Context(const char* type);
	//void LeaveV8Context(const char* type);
	//void InvokeNUICallback(const char* type, const CefString& name, const CefV8ValueList& arguments);
	void OVERLAY_DECL ReloadNUI();

	void OVERLAY_DECL CreateFrame(fwString frameName, fwString frameURL);
	void OVERLAY_DECL DestroyFrame(fwString frameName);
	bool OVERLAY_DECL HasFrame(const std::string& frameName);
	void OVERLAY_DECL SignalPoll(fwString frameName);

	void OVERLAY_DECL GiveFocus(bool hasFocus, bool hasCursor = false);
	void OVERLAY_DECL OverrideFocus(bool hasFocus);
	bool OVERLAY_DECL HasMainUI();
	void OVERLAY_DECL SetMainUI(bool enable);

	void ProcessInput();

	void OVERLAY_DECL ExecuteRootScript(const std::string& scriptBit);

	void OVERLAY_DECL PostFrameMessage(const std::string& frameName, const std::string& jsonData);

	void OVERLAY_DECL PostRootMessage(const std::string& jsonData);

	OVERLAY_DECL CefBrowser* GetBrowser();

	bool OnPreLoadGame(void* cefSandbox);

	// window API
	OVERLAY_DECL CefBrowser* GetNUIWindowBrowser(fwString windowName);

	OVERLAY_DECL void CreateNUIWindow(fwString windowName, int width, int height, fwString windowURL);
	OVERLAY_DECL void DestroyNUIWindow(fwString windowName);
	OVERLAY_DECL void ExecuteWindowScript(const std::string& windowName, const std::string& scriptBit);
	OVERLAY_DECL void SetNUIWindowURL(fwString windowName, fwString url);

	OVERLAY_DECL rage::grcTexture* GetWindowTexture(fwString windowName);

	extern
		OVERLAY_DECL
		fwEvent<const wchar_t*, const wchar_t*> OnInvokeNative;

	extern
		OVERLAY_DECL
		fwEvent<bool> OnDrawBackground;
}

#define REQUIRE_IO_THREAD()   assert(CefCurrentlyOn(TID_IO));

struct nui_s
{
	bool initialized;

	DWORD nuiWidth;
	DWORD nuiHeight;
};

extern
	OVERLAY_DECL
	fwEvent<const char*, CefRefPtr<CefRequest>, CefRefPtr<CefResourceHandler>&> OnSchemeCreateRequest;
