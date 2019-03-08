#include <StdInc.h>
#include <CefOverlay.h>

#include <ICoreGameInit.h>

#include <Resource.h>

#include <gameSkeleton.h>

#include <HostSystem.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <ResourceManager.h>
#include <ResourceMetaDataComponent.h>

#include <ScriptEngine.h>

#include <GameInit.h>

#include <Error.h>

// 1365
// 1493
// 1604?
#define NUM_DLC_CALLS 36

using fx::Resource;

static bool frameOn = false;
static bool primedMapLoad = false;

static void InvokeNUIScript(const std::string& eventName, rapidjson::Document& json)
{
	if (!frameOn)
	{
		return;
	}

	json.AddMember("eventName", rapidjson::Value(eventName.c_str(), json.GetAllocator()), json.GetAllocator());

	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
	
	if (json.Accept(writer))
	{
		nui::PostFrameMessage("loadingScreen", sb.GetString());
	}
}

static void DestroyFrame()
{
	if (frameOn)
	{
		nui::DestroyFrame("loadingScreen");

		frameOn = false;
	}
}

int CountRelevantDataFileEntries();
extern fwEvent<int, const char*> OnReturnDataFileEntry;
extern fwEvent<int, void*, void*> OnInstrumentedFunctionCall;

#include <scrEngine.h>

static HookFunction hookFunction([]()
{
	rage::scrEngine::OnScriptInit.Connect([]()
	{
		static bool endedLoadingScreens = false;
		static bool autoShutdownNui = true;

		auto endLoadingScreens = [=]()
		{
			if (!endedLoadingScreens)
			{
				endedLoadingScreens = true;

				DestroyFrame();

				nui::OverrideFocus(false);
			}
		};

		{
			// override SHUTDOWN_LOADING_SCREEN
			auto handler = fx::ScriptEngine::GetNativeHandler(0x078EBE9809CCD637);

			if (!handler)
			{
				FatalError("Couldn't find SHUTDOWN_LOADING_SCREEN to hook!");
			}

			fx::ScriptEngine::RegisterNativeHandler("SET_MANUAL_SHUTDOWN_LOADING_SCREEN_NUI", [](fx::ScriptContext& ctx)
			{
				autoShutdownNui = !ctx.GetArgument<bool>(0);
			});

			fx::ScriptEngine::RegisterNativeHandler("SHUTDOWN_LOADING_SCREEN_NUI", [=](fx::ScriptContext& ctx)
			{
				endLoadingScreens();
			});

			fx::ScriptEngine::RegisterNativeHandler(0x078EBE9809CCD637, [=](fx::ScriptContext& ctx)
			{
				(*handler)(ctx);

				if (autoShutdownNui)
				{
					endLoadingScreens();
				}
			});
		}

		{
			Instance<ICoreGameInit>::Get()->OnTriggerError.Connect([=](const std::string& errorMessage)
			{
				endLoadingScreens();

				return true;
			});
		}

		OnKillNetworkDone.Connect([=]()
		{
			nui::DestroyFrame("loadingScreen");
		});

		Instance<ICoreGameInit>::Get()->OnGameRequestLoad.Connect([]()
		{
			endedLoadingScreens = false;
			autoShutdownNui = true;
		});

		{
			// override LOAD_ALL_OBJECTS_NOW
			auto handler = fx::ScriptEngine::GetNativeHandler(0xBD6E84632DD4CB3F);

			if (!handler)
			{
				FatalError("Couldn't find LOAD_ALL_OBJECTS_NOW to hook!");
			}

			fx::ScriptEngine::RegisterNativeHandler(0xBD6E84632DD4CB3F, [=](fx::ScriptContext& ctx)
			{
				if (!endedLoadingScreens)
				{
					trace("Skipping LOAD_ALL_OBJECTS_NOW as loading screens haven't ended yet!\n");
					return;
				}

				(*handler)(ctx);
			});
		}
	});
});

extern int dlcIdx;

static InitFunction initFunction([] ()
{
	OnKillNetworkDone.Connect([] ()
	{
		DestroyFrame();
	});

	Instance<ICoreGameInit>::Get()->OnGameRequestLoad.Connect([] ()
	{
		frameOn = true;

		std::vector<std::string> loadingScreens = { "nui://game/ui/loadscreen/index.html" };

		Instance<fx::ResourceManager>::Get()->ForAllResources([&](fwRefContainer<fx::Resource> resource)
		{
			auto mdComponent = resource->GetComponent<fx::ResourceMetaDataComponent>();
			auto entries = mdComponent->GetEntries("loadscreen");

			if (entries.begin() != entries.end())
			{
				std::string path = entries.begin()->second;

				if (path.find("://") != std::string::npos)
				{
					loadingScreens.push_back(path);
				}
				else
				{
					loadingScreens.push_back("nui://" + resource->GetName() + "/" + path);
				}
			}
		});

		nui::CreateFrame("loadingScreen", loadingScreens.back());
		nui::OverrideFocus(true);

		nui::PostRootMessage(R"({ "type": "focusFrame", "frameName": "loadingScreen" })");
	}, 100);

	rage::OnInitFunctionStart.Connect([] (rage::InitFunctionType type)
	{
		if (type == rage::INIT_AFTER_MAP_LOADED)
		{
			rapidjson::Document doc;
			doc.SetObject();

			InvokeNUIScript("endDataFileEntries", doc);
		}

		{
			rapidjson::Document doc;
			doc.SetObject();
			doc.AddMember("type", rapidjson::Value(rage::InitFunctionTypeToString(type), doc.GetAllocator()), doc.GetAllocator());

			InvokeNUIScript("startInitFunction", doc);
		}
	});

	rage::OnInitFunctionStartOrder.Connect([] (rage::InitFunctionType type, int order, int count)
	{
		if (type == rage::INIT_SESSION && order == 3)
		{
			count += NUM_DLC_CALLS;

			dlcIdx = 0;
		}

		rapidjson::Document doc;
		doc.SetObject();
		doc.AddMember("type", rapidjson::Value(rage::InitFunctionTypeToString(type), doc.GetAllocator()), doc.GetAllocator());
		doc.AddMember("order", order, doc.GetAllocator());
		doc.AddMember("count", count, doc.GetAllocator());

		InvokeNUIScript("startInitFunctionOrder", doc);
	});

	rage::OnInitFunctionInvoking.Connect([] (rage::InitFunctionType type, int idx, rage::InitFunctionData& data)
	{
		if (type == rage::INIT_SESSION && data.initOrder == 3 && idx >= 15 && data.shutdownOrder != 42)
		{
			idx += NUM_DLC_CALLS;
		}

		rapidjson::Document doc;
		doc.SetObject();
		doc.AddMember("type", rapidjson::Value(rage::InitFunctionTypeToString(type), doc.GetAllocator()), doc.GetAllocator());
		doc.AddMember("name", rapidjson::Value(data.GetName(), doc.GetAllocator()), doc.GetAllocator());
		doc.AddMember("idx", idx, doc.GetAllocator());

		InvokeNUIScript("initFunctionInvoking", doc);
	});

	rage::OnInitFunctionInvoked.Connect([] (rage::InitFunctionType type, const rage::InitFunctionData& data)
	{
		rapidjson::Document doc;
		doc.SetObject();
		doc.AddMember("type", rapidjson::Value(rage::InitFunctionTypeToString(type), doc.GetAllocator()), doc.GetAllocator());
		doc.AddMember("name", rapidjson::Value(data.GetName(), doc.GetAllocator()), doc.GetAllocator());

		InvokeNUIScript("initFunctionInvoked", doc);
	});

	rage::OnInitFunctionEnd.Connect([] (rage::InitFunctionType type)
	{
		{
			rapidjson::Document doc;
			doc.SetObject();
			doc.AddMember("type", rapidjson::Value(rage::InitFunctionTypeToString(type), doc.GetAllocator()), doc.GetAllocator());

			InvokeNUIScript("endInitFunction", doc);
		}

		if (type == rage::INIT_BEFORE_MAP_LOADED)
		{
			primedMapLoad = true;
		}
		else if (type == rage::INIT_SESSION)
		{
			if (Instance<ICoreGameInit>::Get()->HasVariable("networkInited"))
			{
				DestroyFrame();
			}
		}
	});

	OnHostStateTransition.Connect([] (HostState newState, HostState oldState)
	{
		auto printLog = [] (const std::string& message)
		{
			rapidjson::Document doc;
			doc.SetObject();
			doc.AddMember("message", rapidjson::Value(message.c_str(), message.size(), doc.GetAllocator()), doc.GetAllocator());

			InvokeNUIScript("onLogLine", doc);
		};

		if (newState == HS_FATAL)
		{
			DestroyFrame();
		}
		else if (newState == HS_JOINING)
		{
			printLog("Entering session");
		}
		else if (newState == HS_WAIT_HOSTING)
		{
			printLog("Setting up game");
		}
		else if (newState == HS_JOIN_FAILURE)
		{
			printLog("Adjusting settings for best experience");
		}
		else if (newState == HS_HOSTING)
		{
			printLog("Initializing session");
		}
	});

	OnInstrumentedFunctionCall.Connect([] (int idx, void* origin, void* target)
	{
        static std::set<int> hadIndices;

		if (primedMapLoad)
		{
			rapidjson::Document doc;
			doc.SetObject();
			doc.AddMember("count", CountRelevantDataFileEntries(), doc.GetAllocator());

			InvokeNUIScript("startDataFileEntries", doc);

			primedMapLoad = false;
		}

        if (hadIndices.find(idx) == hadIndices.end())
		{
			rapidjson::Document doc;
			doc.SetObject();
			doc.AddMember("idx", idx, doc.GetAllocator());

			InvokeNUIScript("performMapLoadFunction", doc);

            hadIndices.insert(idx);
		}
	});

	OnReturnDataFileEntry.Connect([] (int type, const char* name)
	{
		// counting 0, 9 and 185 is odd due to multiple counts :/ (+ entries getting added all the time)

		static std::set<int> entryTypes = { 0, 9, 185, 3, 4, 20, 21, 28, 45, 48, 49, 51, 52, 53, 54, 55, 56, 59, 66, 71, 72, 73, 75, 76, 77, 84, 89, 97, 98, 99, 100, 106, 107, 112, 133, 184 };
		static thread_local std::set<std::pair<int, std::string>> hadEntries;

		if (entryTypes.find(type) != entryTypes.end())
		{
			bool isNew = false;

			if (hadEntries.find({ type, name }) == hadEntries.end())
			{
				hadEntries.insert({ type, name });

				if (type != 0 && type != 9 && type != 185)
				{
					isNew = true;
				}
			}

			rapidjson::Document doc;
			doc.SetObject();
			doc.AddMember("type", type, doc.GetAllocator());
			doc.AddMember("isNew", isNew, doc.GetAllocator());
			doc.AddMember("name", rapidjson::Value(name, doc.GetAllocator()), doc.GetAllocator());

			InvokeNUIScript("onDataFileEntry", doc);
		}
	});
});
