#ifndef _PIXIRETRO_APP_H_
#define _PIXIRETRO_APP_H_

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

//#include "pxr_gfx.h"

namespace pxr
{

namespace gfx
{
	using ScreenID_t = int;
}

class Game;

//
// Virtual base class for app states. Derive from this class to create app 'modes'
// that can be switched between, e.g. a splash screen, a menu, a gameplay state etc.
//
class Scene
{
public:
	Scene(Game* owner) : _owner(owner) {}

	virtual ~Scene() = default;
	virtual bool onInit() = 0;
	virtual void onUpdate(double now, float dt) = 0;
	virtual void onDraw(double now, float dt, const std::vector<gfx::ScreenID_t>& screens) = 0;
	virtual void onEnter() = 0;
	virtual void onExit() = 0;

	virtual std::string getName() const = 0;

protected:
	Game* _owner;
};

//
// Virtual base class for applications. Derive from this class and setup some
// states to create a game.
//
class Game
{
public:
	Game() = default;
	virtual ~Game() = default;

	//
	// Invoked by the engine on boot. For use by derived classes to instantiate and
	// add all their app states, as well as setting their initial state.
	// 
	// Must also create all the gfx screens the app required for drawing.
	//
	virtual bool onInit() = 0;

	//
	// Invoked by the engine on shutdown.
	//
	virtual void onShutdown() = 0;

	//
	// Invoked by the engine during the update tick.
	//
	void onUpdate(double now, float dt)
	{
		_activeScene->onUpdate(now, dt);
	}

	//
	// Invoked by the engine during the draw tick.
	//
	void onDraw(double now, float dt)
	{
		_activeScene->onDraw(now, dt, _screens);
	}

	//
	// For use by app states to switch between other states (game state, menu states etc).
	//
	void switchScene(const std::string& name)
	{
		_activeScene->onExit();
		_activeScene = _scenes[name];
		_activeScene->onEnter();
	}

	//
	// Accessors to provide information to the engine about your application. Used, for example,
	// to set the window title.
	//
	// The implementation of these access functions must be useable prior to the call to onInit() 
	// as the engine will call these accessors before onInit.
	//
	virtual std::string getName() const = 0;
	virtual int getVersionMajor() const = 0;
	virtual int getVersionMinor() const = 0;

protected:
	std::unordered_map<std::string, std::shared_ptr<Scene>> _scenes;
	std::shared_ptr<Scene> _activeScene;
	std::vector<gfx::ScreenID_t> _screens;
};

} // namespace pxr

#endif
