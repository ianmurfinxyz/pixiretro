#ifndef _ENGINE_H_
#define _ENGINE_H_

namespace pxr
{

class Engine final
{
public:
  Engine() = default;
  ~Engine() = default;

  void initialize(std::unique_ptr<Application> app);
  void run();

private:
  using Clock_t = std::chrono::steady_clock;
  using TimePoint_t = std::chrono::time_point<Clock_t>;
  using Duration_t = std::chrono::nanoseconds;

  constexpr static Duration_t oneMillisecond {1'000'000};
  constexpr static Duration_t oneSecond {1'000'000'000};
  constexpr static Duration_t oneMinute {60'000'000'000};
  constexpr static Duration_t minFramePeriod {1'000'000};

  constexpr static Assets::Key_t engineFontKey {0}; // engine reserves this font key for itself.
  constexpr static Assets::Name_t engineFontName {"engine"};
  constexpr static Assets::Scale_t engineFontScale {1};

  class RealClock
  {
  public:
    RealClock() : _start{}, _now{}, _dt{}{}
    void start(){_now0 = _start = Clock_t::now();}
    Duration_t update();
    Duration_t getNow() {return _now - _start;}

  private:
    TimePoint_t _start;
    TimePoint_t _now;
  };

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
    Duration_t _tickerNow;
    Duration_t _lastMeasureNow;
    Duration_t _tickPeriod;
    float _tickPeriodSeconds;
    int _ticksDoneTotal;
    int _ticksDoneThisSecond;
    int _ticksDoneThisFrame;
    int _maxTicksPerFrame;
    int _ticksAccumulated;
    double _measuredTickFrequency;
    bool _isChasingGameNow;
  };

  class EngineRC final : public FileRC
  {
  public:
    static constexpr const char* filename = "enginerc";
    
    enum Key
    {
      KEY_WINDOW_WIDTH, 
      KEY_WINDOW_HEIGHT, 
      KEY_FULLSCREEN, 
      KEY_OPENGL_MAJOR, 
      KEY_OPENGL_MINOR,
    };

    Config() : Dataset({
      //    key               name        default   min      max
      {KEY_WINDOW_WIDTH,  "windowWidth",  {500},   {300},   {1000}},
      {KEY_WINDOW_HEIGHT, "windowHeight", {500},   {300},   {1000}},
      {KEY_FULLSCREEN,    "fullscreen",   {false}, {false}, {true}},
      {KEY_OPENGL_MAJOR,  "openglMajor",  {2},     {2},     {2},  },
      {KEY_OPENGL_MINOR,  "openglMinor",  {1},     {1},     {1},  }
    }){}
  };

private:
  void mainloop();
  void drawPerformanceStats(Duration_t realDt, Duration_t gameDt);
  void drawPauseDialog();
  void onLogicTick(Duration_t gameNow, Duration_t realNow, float tickPeriodSeconds);
  void onDrawTick(Duration_t gameNow, Duration_t realNow, float tickPeriodSeconds);

  double durationToMilliseconds(Duration_t d);
  double durationToSeconds(Duration_t d);
  double durationToMinutes(Duration_t d);

private:
  Ticker _logicTicker;
  Ticker _drawTicker;

  RealClock _realClock;
  GameClock _gameClock;

  long _framesDone;
  int _framesDoneThisSecond;
  float _measuredFrameFrequency;
  Duration_t _lastFrameMeasureNow;

  std::unique_ptr<Application> _app;

  bool _isDrawingPerformanceStats;
  bool _isDone;
};

} // namespace pxr

#endif
