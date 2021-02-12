#include <memory>
#include <cassert>
#include "engine.h"
#include "app.h"
#include "gfx.h"
#include "math.h"
#include "cutscene.h"

#include <iostream>

enum SpriteID
{
  SID_BLUE_DOUBLE_LADDER,
  SID_BLUE_SINGLE_LADDER,
  SID_RED_GIRDER_LONG_FLAT,
  SID_RED_GIRDER_LONG_RIGHT_SLOPE,
  SID_RED_GIRDER_LONG_LEFT_SLOPE,
  SID_RED_GIRDER_TOP_SLOPE,
  SID_RED_GIRDER_BOTTOM_SLOPE,
  SID_RED_GIRDER_BOTTOM_FLAT,
  SID_RED_GIRDER_SHORT,
  SID_KONG_CLIMB_PAULINE,
  SID_OIL_BARREL,
  SID_OIL_FLAMES,
  SID_COUNT
};

//
// The set of all sprite asset names required by the app.
//
//static std::array<pxr::gfx::ResourceName_t, SID_COUNT> spriteNames {
//  "blue_double_ladder",
//  "blue_single_ladder",
//  "red_girder_long_flat",
//  "red_girder_long_right_slope",
//  "red_girder_long_left_slope",
//  "red_girder_top_slope",
//  "red_girder_bottom_slope",
//  "red_girder_bottom_flat",
//  "red_girder_short",
//  "kong_climb_pauline",
//  "oil_barrel",
//  "oil_flames"
//};
//
//static std::array<pxr::gfx::ResourceKey_t, SID_COUNT> spriteKeys;

enum CutsceneKey
{
  CUTSKEY_INTRO = 0,
};

static std::vector<pxr::cut::Cutscene> cutscenes;

static constexpr int STAGE_SCREEN_ID {0};
static constexpr pxr::Vector2i WORLD_SIZE {224, 256};

class SplashState final : public pxr::AppState
{
public:
  static constexpr const char* name {"splash"};

public:
  SplashState(pxr::App* owner) : AppState(owner){}

  bool onInit()
  {
  }

  void onUpdate(double now, float dt)
  {
    cutscenes[CUTSKEY_INTRO].update(dt);
  }

  void onDraw(double now, float dt)
  {
    pxr::gfx::clearScreenShade(1, STAGE_SCREEN_ID);

    cutscenes[CUTSKEY_INTRO].draw(STAGE_SCREEN_ID);


    //pxr::gfx::drawSprite(_alienPosition, SPRITEID_SQUID, 0, STAGE_SCREEN_ID);
    //pxr::gfx::drawSprite(pxr::Vector2i{10, 10}, SPRITEID_CRAB, 0, STAGE_SCREEN_ID);
    //pxr::gfx::drawText(pxr::Vector2i{-20, 100}, "hello world", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 130}, "!\"#$%&`()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, STAGE_SCREEN_ID);
    //pxr::gfx::drawText(pxr::Vector2i{20, 120}, "[\\]^_'abcdefghijklmnopqrstuvwxyz{|}~", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 140}, "\"this is a quote\"", 0, pxr::gfx::LAYER_UI);
    //pxr::gfx::drawText(pxr::Vector2i{20, 150}, "$cat file.txt >> main.cpp -la", 0, pxr::gfx::LAYER_UI);
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

public:
  ExampleApp() = default;
  ~ExampleApp() = default;

  bool onInit()
  {
    _active = std::shared_ptr<pxr::AppState>(new SplashState(this));
    _active->onInit();
    _states.emplace(_active->getName(), _active);

    int screenid = pxr::gfx::createScreen(WORLD_SIZE);
    assert(screenid == STAGE_SCREEN_ID);

    //for(int sid = 0; sid < SID_COUNT; ++sid)
    //  spriteKeys[sid] = gfx::loadSprite(spriteNames[sid]);

    cutscenes.push_back(pxr::cut::Cutscene{});
    bool result = cutscenes.back().load("intro");
    assert(result == true);
  }

  virtual std::string getName() const {return std::string{name};}
  virtual int getVersionMajor() const {return versionMajor;}
  virtual int getVersionMinor() const {return versionMinor;}
};

pxr::Engine engine;

int main()
{
  engine.initialize(std::unique_ptr<pxr::App>(new ExampleApp()));
  engine.run();
  engine.shutdown();
}
