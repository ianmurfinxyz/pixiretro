#include <SDL2/SDL.h>
#include <thread>
#include <sstream>
#include "engine.h"
#include "log.h"
#include "app.h"
#include "input.h"
#include "gfx.h"

namespace pxr
{

Engine::Duration_t Engine::RealClock::update()
{
  auto old = _now;
  _now = Clock_t::now();
  return _now - old;
}

void Engine::GameClock::update(Duration_t realDt)
{
  if(!_isPaused)
    _now += Duration_t{static_cast<int64_t>(realDt.count() * _scale)};
}

Engine::Ticker::Ticker(Callback_t onTick, Engine* tickCtx, Duration_t tickPeriod, 
                       int maxTicksPerFrame, bool isChasingGameNow) :
  _onTick{onTick},
  _tickCtx{tickCtx},
  _tickerNow{0},
  _lastMeasureNow{0},
  _tickPeriod{tickPeriod},
  _tickPeriodSeconds{static_cast<double>(tickPeriod.count()) / oneSecond.count()},
  _ticksDoneTotal{0},
  _ticksDoneThisSecond{0},
  _ticksDoneThisFrame{0},
  _maxTicksPerFrame{maxTicksPerFrame},
  _ticksAccumulated{0},
  _measuredTickFrequency{0},
  _isChasingGameNow{isChasingGameNow}
{}

void Engine::Ticker::doTicks(Duration_t gameNow, Duration_t realNow)
{
  Duration_t now = _isChasingGameNow ? gameNow : realNow;

  while(_tickerNow + _tickPeriod < now){
    _tickerNow += _tickPeriod;
    ++_ticksAccumulated;
  }

  _ticksDoneThisFrame = 0;
  while(_ticksAccumulated > 0 && _ticksDoneThisFrame < _maxTicksPerFrame){
    ++_ticksDoneThisFrame;
    --_ticksAccumulated;
    (_tickCtx->*_onTick)(_tickPeriodSeconds);
  }

  _ticksDoneThisSecond += _ticksDoneThisFrame;
  _ticksDoneTotal += _ticksDoneThisFrame;

  if((realNow - _lastMeasureNow) >= oneSecond){
    _measuredTickFrequency = (static_cast<double>(_ticksDoneThisSecond) / 
                             (realNow - _lastMeasureNow).count()) * 
                             oneSecond.count();
    _ticksDoneThisSecond = 0;
    _lastMeasureNow = realNow;
  }
}

void Engine::initialize(std::unique_ptr<App> app)
{
  log::initialize();
  input::initialize();

  if(!_rc.load(EngineRC::filename))
    _rc.write(EngineRC::filename);    // generate a default rc file if one doesn't exist.

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    log::log(log::FATAL, log::msg_fail_sdl_init, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  _updateTicker = Ticker{&Engine::onUpdateTick, this, fpsLockHz, 5, true};
  _drawTicker = Ticker{&Engine::onDrawTick, this, fpsLockHz, 1, false};

  _app = std::move(app);
  _app->onInit();

  std::stringstream ss {};
  ss << _app->getName() 
     << " version:" 
     << _app->getVersionMajor() 
     << "." 
     << _app->getVersionMinor();

  gfx::Configuration gfxconfig {};
  gfxconfig._windowTitle = std::string{ss.str()};
  gfxconfig._windowSize._x = _rc.getIntValue(EngineRC::KEY_WINDOW_WIDTH);
  gfxconfig._windowSize._y = _rc.getIntValue(EngineRC::KEY_WINDOW_HEIGHT);
  gfxconfig._layerSize[gfx::LAYER_BACKGROUND] = _app->getBackgroundLayerSize();
  gfxconfig._layerSize[gfx::LAYER_STAGE] = _app->getStageLayerSize();
  gfxconfig._layerSize[gfx::LAYER_UI] = _app->getUiLayerSize();
  gfxconfig._layerSize[gfx::LAYER_ENGINE_STATS] = engineStatsLayerSize;
  gfxconfig._fullscreen = _rc.getBoolValue(EngineRC::KEY_FULLSCREEN);

  gfx::initialize(gfxconfig);

  gfx::ResourceManifest_t manifest {{engineFontKey, engineFontName}};
  gfx::loadFonts(manifest);

  _framesDone = 0;
  _framesDoneThisSecond = 0;
  _measuredFrameFrequency = 0;
  _lastFrameMeasureNow = Duration_t::zero();
  _isDrawingEngineStats = false;
  _isDone = false;
}

void Engine::shutdown()
{
  gfx::shutdown();
  log::shutdown();
}

void Engine::run()
{
  _realClock.reset();
  while(!_isDone) mainloop();
}

void Engine::mainloop()
{
  auto frameStart = Clock_t::now();

  _gameClock.update(_realClock.update()); 
  auto gameNow = _gameClock.getNow();
  auto realNow = _realClock.getNow();

  SDL_Event event;
  while(SDL_PollEvent(&event) != 0){
    switch(event.type){
      case SDL_QUIT:
        _isDone = true;
        return;
      case SDL_WINDOWEVENT:
        if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
          gfx::onWindowResize(Vector2i{event.window.data1, event.window.data2});
        break;
      case SDL_KEYDOWN:
        if(event.key.keysym.sym == decrementGameClockScaleKey){
          _gameClock.incrementScale(-0.1);
          break;
        }
        else if(event.key.keysym.sym == incrementGameClockScaleKey){
          _gameClock.incrementScale(0.1);
          break;
        }
        else if(event.key.keysym.sym == resetGameClockScaleKey){
          _gameClock.setScale(1.f);
          break;
        }
        else if(event.key.keysym.sym == pauseGameClockKey){
          _gameClock.togglePause();
          break;
        }
        else if(event.key.keysym.sym == toggleDrawEngineStatsKey){
          _isDrawingEngineStats = !_isDrawingEngineStats;
          break;
        }
        // FALLTHROUGH
      case SDL_KEYUP:
        input::onKeyEvent(event);
        break;
    }
  }

  _updateTicker.doTicks(gameNow, realNow);
  _drawTicker.doTicks(gameNow, realNow);

  ++_framesDone;
  ++_framesDoneThisSecond;
  if((realNow - _lastFrameMeasureNow) >= oneSecond){
    _measuredFrameFrequency = (static_cast<double>(_framesDoneThisSecond) / 
                              (realNow - _lastFrameMeasureNow).count()) * 
                              oneSecond.count();
    _lastFrameMeasureNow = realNow;
    _framesDoneThisSecond = 0;
  }

  auto framePeriod = Clock_t::now() - frameStart;
  if(framePeriod < minFramePeriod)
    std::this_thread::sleep_for(minFramePeriod - framePeriod); 
}

void Engine::drawEngineStats()
{
}

void Engine::drawPauseDialog()
{
}

void Engine::onUpdateTick(float tickPeriodSeconds)
{
  double nowSeconds = durationToSeconds(_gameClock.getNow());
  _app->onUpdate(nowSeconds, tickPeriodSeconds);
  input::onUpdate();
}

void Engine::onDrawTick(float tickPeriodSeconds)
{
  gfx::clearWindow(gfx::colors::gainsboro);

  double nowSeconds = durationToSeconds(_gameClock.getNow());
  _app->onDraw(nowSeconds, tickPeriodSeconds);

  if(_gameClock.isPaused())
    drawPauseDialog();

  if(_isDrawingEngineStats)
    drawEngineStats();

  gfx::present();
}

double Engine::durationToMilliseconds(Duration_t d)
{
  return static_cast<double>(d.count()) / static_cast<double>(oneMillisecond.count());
}

double Engine::durationToSeconds(Duration_t d)
{
  return static_cast<double>(d.count()) / static_cast<double>(oneSecond.count());
}

double Engine::durationToMinutes(Duration_t d)
{
  return static_cast<double>(d.count()) / static_cast<double>(oneMinute.count());
}

} // namespace pxr

