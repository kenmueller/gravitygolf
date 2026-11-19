#include "Misc/NetworkVersion.h"
#include "Modules/ModuleManager.h"

namespace
{
/**
 * The game's own network version. Two copies play together only if this number is the same in both.
 *
 * The engine's own number is made from the engine build, and the desktop game (the installed engine) and the web
 * game (the engine built from source) report different ones for the same code: a browser's join was refused as
 * "an incompatible version of the game". A hole is kept in step by commands on a simulation both sides compute
 * alike, so what has to match is this game's code, not the engine build. Raise the number whenever the simulation,
 * a course, or anything sent between players changes in a way an older copy would not follow.
 */
constexpr uint32 GGNetVersion = 0x47470003; // "GG", then the count
} // namespace

class FGravityGolfModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FNetworkVersion::GetLocalNetworkVersionOverride.BindLambda([]() { return GGNetVersion; });
	}
	virtual void ShutdownModule() override
	{
		FNetworkVersion::GetLocalNetworkVersionOverride.Unbind();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FGravityGolfModule, GravityGolf, "GravityGolf");
