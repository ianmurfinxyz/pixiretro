#include <memory>
#include "engine.h"
#include "app.h"
#include "gfx.h"
#include "math.h"

#include <iostream>

class SplashState final : public pxr::AppState
{
public:
  static constexpr const char* name {"splash"};

  enum SpriteID
  {
    SPRITEID_SQUID,
    SPRITEID_CRAB
  };

public:
  SplashState(pxr::App* owner) : AppState(owner){}

  bool onInit()
  {
    pxr::gfx::ResourceManifest_t manifest {
      {SPRITEID_SQUID, "squid"},
      {SPRITEID_CRAB, "crab"},
    };

    pxr::gfx::loadSprites(manifest);

    _alienPosition = pxr::Vector2i{20, 20};
    _alienSpeedX = 40;
  }

  void onUpdate(double now, float dt)
  {
    _alienPosition._x += _alienSpeedX * dt;
    if(_alienPosition._x < 0 || _alienPosition._x > (_owner->getStageLayerSize()._x - 20))
      _alienSpeedX *= -1;
  }

  void onDraw(double now, float dt)
  {
    pxr::gfx::clearLayer(pxr::gfx::LAYER_BACKGROUND);
    pxr::gfx::fastFillLayer(20, pxr::gfx::LAYER_BACKGROUND);

    pxr::gfx::clearLayer(pxr::gfx::LAYER_STAGE);
    pxr::gfx::drawSprite(_alienPosition, SPRITEID_SQUID, 0, pxr::gfx::LAYER_STAGE);
    pxr::gfx::drawSprite(pxr::Vector2i{50, 20}, SPRITEID_CRAB, 0, pxr::gfx::LAYER_STAGE);
  }

  void onReset()
  {

  }

  std::string getName() const {return name;}

private:
  pxr::Vector2f _alienPosition;
  float _alienSpeedX; // px per seconds
};


class ExampleApp final : public pxr::App
{
public:
  static constexpr int versionMajor {1};
  static constexpr int versionMinor {0};
  static constexpr const char* name {"example-app"};
  static constexpr pxr::Vector2i worldSize {512, 512};

public:
  ExampleApp() = default;
  ~ExampleApp() = default;

  bool onInit()
  {
    _active = std::shared_ptr<pxr::AppState>(new SplashState(this));
    _active->onInit();
    _states.emplace(_active->getName(), _active);
  }

  virtual std::string getName() const {return std::string{name};}
  virtual int getVersionMajor() const {return versionMajor;}
  virtual int getVersionMinor() const {return versionMinor;}

  virtual pxr::Vector2i getBackgroundLayerSize() const {return worldSize;}
  virtual pxr::Vector2i getStageLayerSize() const {return worldSize;}
  virtual pxr::Vector2i getUiLayerSize() const {return worldSize;}
};

pxr::Engine engine;

int main()
{
  engine.initialize(std::unique_ptr<pxr::App>(new ExampleApp()));
  engine.run();
  engine.shutdown();
}
