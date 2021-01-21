#ifndef _ENGINE_H_
#define _ENGINE_H_

#include "filerc.h"

namespace pxr
{

class Engine final
{
public:
  Engine() = default;
  ~Engine() = default;

  void initialize(std::unique_ptr<App> app);
  void shutdown();
  void run();

private:
  using Clock_t = std::chrono::steady_clock;
  using TimePoint_t = std::chrono::time_point<Clock_t>;
  using Duration_t = std::chrono::nanoseconds;

  static constexpr Duration_t oneMillisecond {1'000'000};
  static constexpr Duration_t oneSecond {1'000'000'000};
  static constexpr Duration_t oneMinute {60'000'000'000};
  static constexpr Duration_t minFramePeriod {1'000'000};

  static constexpr Vector2i engineStatsLayerSize {400, 200};

  // Lock the FPS to this frequency (or less).
  static constexpr Duration_t fpsLockHz {static_cast<int64_t>(1.0e9 / 60.0)}; 

  // Clock to record the real passage of time since the app booted.
  class RealClock
  {
  public:
    RealClock() : _start{}, _now{}{}
    void reset(){_now = _start = Clock_t::now();}
    Duration_t update();
    Duration_t getNow() {return _now - _start;}

  private:
    TimePoint_t _start;
    TimePoint_t _now;
  };

  // Clock independent of real time. Can be paused and scaled. Used as the
  // timeline for game systems.
  class GameClock
  {
  public:
    GameClock() : _now{}, _scale{1.f}, _isPaused{false}{}
    void update(Duration_t realDt);
    Duration_t getNow() const {return _now;}
    void incrementScale(float increment){_scale += increment;}
    void setScale(float scale){_scale = scale;}
    float getScale() const {return _scale;}
    void pause(){_isPaused = true;}
    void unpause(){_isPaused = false;}
    void togglePause(){_isPaused = !_isPaused;}
    bool isPaused() const {return _isPaused;}

  private:
    Duration_t _now;
    float _scale;
    bool _isPaused;
  };

  // A class responsible for invoking a callback at regular (tick) intervals. Used in the
  // gameloop to manage when systems are updated; e.g. game logic and drawing. Results in
  // constant time updates.
  //
  // Can be conceptualised as a timeline in which time is quantised and chases a master 
  // clock. When the time on the master clock exceeds the next quantised time unit on the
  // ticker, the ticker time jumps forward to said unit to catch up. Invoking the callback
  // upon jumping.
  class Ticker
  {
  public:
    using Callback_t = void (Engine::*)(Duration_t, Duration_t, float);

  public:
    Ticker() = default;
    Ticker(Callback_t onTick, Engine* tickCtx, Duration_t tickPeriod, int maxTicksPerFrame, bool isChasingGameNow);
    void doTicks(Duration_t gameNow, Duration_t realNow);
    int getTicksDoneTotal() const {return _ticksDoneTotal;}
    int getTicksDoneThisFrame() const {return _ticksDoneThisFrame;}
    int getTicksAccumulated() const {return _ticksAccumulated;}
    double getMeasuredTickFrequency() const {return _measuredTickFrequency;}
    
  private:
    Callback_t _onTick;
    Engine* _tickCtx;
    Duration_t _tickerNow;           // current time in the ticker's timeline.
    Duration_t _lastMeasureNow;      // time when tick frequency was last measured.
    Duration_t _tickPeriod;          // ticker timeline is quantised; period of each jump/tick.
    float _tickPeriodSeconds;        // precalculated as passed to callback every tick.
    int _ticksDoneTotal;             // useful performance stat.
    int _ticksDoneThisSecond;        // used to calculate tick frequency.
    int _ticksDoneThisFrame;         // useful performance stat.
    int _maxTicksPerFrame;           // limit to number of ticks in each call to doTicks.
    int _ticksAccumulated;           // backlog of ticks that need to be done.
    double _measuredTickFrequency;   // ticks per second (analagous to FPS for ticks).
    bool _isChasingGameNow;          // ticker either 'chases' the real clock or the game clock.
  };

  class EngineRC final : public FileRC
  {
  public:
    static constexpr const char* filename = "enginerc";
    
    enum Key
    {
      KEY_WINDOW_WIDTH, 
      KEY_WINDOW_HEIGHT, 
      KEY_FULLSCREEN
    };

    EngineRC() : FileRC({
      //    key               name        default   min      max
      {KEY_WINDOW_WIDTH,  "windowWidth",  {500},   {300},   {1000}},
      {KEY_WINDOW_HEIGHT, "windowHeight", {500},   {300},   {1000}},
      {KEY_FULLSCREEN,    "fullscreen",   {false}, {false}, {true}}
    }){}
  };

private:
  void mainloop();
  void drawEngineStats(Duration_t realDt, Duration_t gameDt);
  void drawPauseDialog();
  void onUpdateTick(Duration_t gameNow, Duration_t realNow, float tickPeriodSeconds);
  void onDrawTick(Duration_t gameNow, Duration_t realNow, float tickPeriodSeconds);

  double durationToMilliseconds(Duration_t d);
  double durationToSeconds(Duration_t d);
  double durationToMinutes(Duration_t d);

private:
  EngineRC _rc;

  Ticker _updateTicker;
  Ticker _drawTicker;

  RealClock _realClock;
  GameClock _gameClock;

  long _framesDone;
  int _framesDoneThisSecond;
  float _measuredFrameFrequency;
  Duration_t _lastFrameMeasureNow;

  std::unique_ptr<App> _app;

  bool _isDrawingEngineStats;
  bool _isDone;
};

} // namespace pxr

#endif
