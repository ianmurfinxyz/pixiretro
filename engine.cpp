#include "engine.h"

namespace pxr
{

Engine::Duration_t Engine::RealClock::update()
{
  Duration_t old = _now;
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
  _onTicks{onTick},
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
  Duration_t now = isChasingGameNow ? gameNow : realNow;

  while(_tickerNow + _tickPeriod < now){
    _tickerNow += _tickPeriod;
    ++_ticksAccumulated;
  }

  _ticksDoneThisFrame = 0;
  while(_ticksAccumulated > 0 && _ticksDoneThisFrame < _maxTicksPerFrame){
    ++_ticksDoneThisFrame;
    --_ticksAccumulated;
    (_tickCtx->*_onTick)(gameNow, realNow, _tickPeriodSeconds);
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
  subsys::log = std::make_unique<Log>();

  if(!_config.load(Config::filename))
    _config.write(Config::filename); // generate a default file if one doesn't exist.

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    log->log(Log::FATAL, logstr::fail_sdl_init, std::string{SDL_GetError()});
    exit(EXIT_FAILURE);
  }

  _logicTicker = Ticker{
    &onLogicTick, 
    this, 
    Duration_t{static_cast<int64_t>(1.0e9 / 60.0)}, // 60Hz tick rate.
    5,
    true
  };

  _drawTicker = Ticker{
    &onDrawTick, 
    this, 
    Duration_t{static_cast<int64_t>(1.0e9 / 60.0)}, // 60Hz tick rate.
    1,
    false
  };

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
  gfxconfig._windowSize = 

  Renderer::Config rconfig {
    std::string{ss.str()},
    _config.getIntValue(Config::KEY_WINDOW_WIDTH),
    _config.getIntValue(Config::KEY_WINDOW_HEIGHT),
    _config.getIntValue(Config::KEY_OPENGL_MAJOR),
    _config.getIntValue(Config::KEY_OPENGL_MINOR),
    _config.getBoolValue(Config::KEY_FULLSCREEN)
  };

  renderer = std::make_unique<Renderer>(rconfig);

  input = std::make_unique<Input>();
  assets = std::make_unique<Assets>();

  Assets::Manifest_t manifest {{engineFontKey, engineFontName, engineFontScale}};
  assets->loadFonts(manifest);

  Vector2i windowSize = pxr::renderer->getWindowSize();

  _frameNo = 0;
  _isSleeping = true;
  _isDrawingPerformanceStats = false;
  _isDone = false;
}

void Engine::run()
{
  _realClock.start();
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
          _app->onWindowResize(event.window.data1, event.window.data2);
        break;
      case SDL_KEYDOWN:
        if(event.key.keysym.sym == SDLK_LEFTBRACKET){
          _gameClock.incrementScale(-0.1);
          break;
        }
        else if(event.key.keysym.sym == SDLK_RIGHTBRACKET){
          _gameClock.incrementScale(0.1);
          break;
        }
        else if(event.key.keysym.sym == SDLK_KP_HASH){
          _gameClock.setScale(1.f);
          break;
        }
        else if(event.key.keysym.sym == SDLK_p && !_app->isWindowTooSmall()){
          _gameClock.togglePause();
          break;
        }
        else if(event.key.keysym.sym == SDLK_BACKQUOTE){
          _isDrawingPerformanceStats = !_isDrawingPerformanceStats;
          break;
        }
        // FALLTHROUGH
      case SDL_KEYUP:
        subsys::input->onKeyEvent(event);
        break;
    }
  }

  _logicTicker.doTicks(gameNow, realNow);
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

void Engine::drawPerformanceStats(Duration_t realDt, Duration_t gameDt)
{
  Vector2i windowSize = renderer->getWindowSize();
  renderer->setViewport({0, 0, std::min(300, windowSize._x), std::min(70, windowSize._y)});
  renderer->clearViewport(colors::blue);

  const Font& engineFont = assets->getFont(engineFontKey, engineFontScale);

  std::stringstream ss {};

  LoopTick* tick = &_loopTicks[LOOPTICK_UPDATE];
  ss << std::setprecision(3);
  ss << "UTPS:"  << tick->_tpsMeter.getTPS() << "hz"
     << "  UTA:" << tick->_ticksAccumulated
     << "  UTD:" << tick->_ticksDoneThisFrame
     << "  UTT:"  << tick->_metronome.getTotalTicks();
  renderer->blitText({5.f, 50.f}, ss.str(), engineFont, colors::white); 

  std::stringstream().swap(ss);

  tick = &_loopTicks[LOOPTICK_DRAW];
  ss << std::setprecision(3);
  ss << "DTPS:"  << tick->_tpsMeter.getTPS() << "hz"
     << "  DTA:" << tick->_ticksAccumulated
     << "  DTD:" << tick->_ticksDoneThisFrame
     << "  DTT:" << tick->_metronome.getTotalTicks();
  renderer->blitText({5.f, 40.f}, ss.str(), engineFont, colors::white); 

  std::stringstream().swap(ss);
  
  ss << std::setprecision(3);
  ss << "FPS:"  << _fpsMeter.getTPS() << "hz"
     << "  FNo:" << _frameNo;
  renderer->blitText({5.f, 30.f}, ss.str(), engineFont, colors::white); 

  std::stringstream().swap(ss);
  
  ss << std::setprecision(3);
  ss << "Gdt:"   << durationToMilliseconds(gameDt) << "ms";
  renderer->blitText({5.f, 20.f}, ss.str(), engineFont, colors::white); 

  std::stringstream().swap(ss);

  ss << std::setprecision(3);
  ss << "  Rdt:" << durationToMilliseconds(realDt) << "ms";
  renderer->blitText({120.f, 20.f}, ss.str(), engineFont, colors::white); 

  std::stringstream().swap(ss);

  ss << std::setprecision(3);
  ss << "GNow:"     << durationToMinutes(_gameClock.getNow()) << "min";
  renderer->blitText({5.f, 10.f}, ss.str(), engineFont, colors::white); 

  std::stringstream().swap(ss);

  ss << std::setprecision(3);
  ss << "  Uptime:" << durationToMinutes(_realClock.getNow()) << "min";
  renderer->blitText({120.f, 10.f}, ss.str(), engineFont, colors::white); 
}

void Engine::drawPauseDialog()
{
  Vector2i windowSize = renderer->getWindowSize();
  renderer->setViewport({0, 0, windowSize._x, windowSize._y});

  const Font& engineFont = assets->getFont(engineFontKey, engineFontScale);

  Vector2f position {};
  position._x = (windowSize._x / 2.f) - 20.f; 
  position._y = (windowSize._y / 2.f) - 5.f;

  renderer->blitText(position, "PAUSED", engineFont, colors::white); 
}

void Engine::onUpdateTick(Duration_t gameNow, Duration_t gameDt, Duration_t realDt, float tickDt)
{
  double now = durationToSeconds(gameNow);
  _app->onUpdate(now, tickDt);
  pxr::input->onUpdate();
}

void Engine::onDrawTick(Duration_t gameNow, Duration_t gameDt, Duration_t realDt, float tickDt)
{
  // TODO - temp - clear the game viewport only in the game and menu states - only clear window
  // when toggle perf stats
  pxr::renderer->clearWindow(colors::gainsboro);

  double now = durationToSeconds(gameNow);

  _app->onDraw(now, tickDt);

  if(_gameClock.isPaused())
    drawPauseDialog();

  if(_isDrawingPerformanceStats)
    drawPerformanceStats(realDt, gameDt);

  pxr::renderer->show();
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

