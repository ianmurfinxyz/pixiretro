#ifndef _APP_H_
#define _APP_H_

#include <memory>
#include "math.h"

namespace pxr
{

class App;

// Virtual base class for app states. Derive from this class to create app 'modes'
// that can be switched between, e.g. a splash screen, a menu, a gameplay state etc.
class AppState
{
public:
  AppState(App* owner) : _owner(owner) {}

  virtual ~AppState() = default;
  virtual bool onInit() = 0;
  virtual void onUpdate(double now, float dt) = 0;
  virtual void onDraw(double now, float dt) = 0;
  virtual void onReset() = 0;

  virtual std::string getName() const = 0;

protected:
  App* _owner;
};

// Virtual base class for applications. Derive from this class and setup some
// states to create a game.
class App
{
public:
  App() = default;
  virtual ~App() = default;

  // Invoked by the engine on boot. For use by derived classes to instantiate and
  // add all their app states, as well as setting their initial state.
  virtual bool onInit() = 0;

  void onUpdate(double now, float dt)
  {
    _active->onUpdate(now, dt);
  }

  void onDraw(double now, float dt)
  {
    _active->onDraw(now, dt);
  }

  // For use by app states to switch between other states (game state, menu states etc).
  void switchState(const std::string& name)
  {
    _active = _states[name];
    _active->onReset();
  }

  //
  // note: the implementation of these access functions must be useable prior to the call
  // to onInit(); engine will call these accessors before onInit.
  //

  virtual std::string getName() const = 0;
  virtual int getVersionMajor() const = 0;
  virtual int getVersionMinor() const = 0;

protected:
  std::unordered_map<std::string, std::shared_ptr<AppState>> _states;
  std::shared_ptr<AppState> _active;
};

} // namespace pxr

#endif
