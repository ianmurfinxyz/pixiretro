#ifndef _PIXIRETRO_PARTICLE_ENGINE_H_
#define _PIXIRETRO_PARTICLE_ENGINE_H_

#include <array>
#include "pxr_vec.h"
#include "pxr_rand.h"
#include "pxr_color.h"

namespace pxr
{

//
// A simple particle engine which spawns and manages particles homogeneous in color and
// mass.
//
// The particle engine allows the spawning of up to MAX_PARTICLE_COUNT with optionally
// randomised velocities and or accelerations.
//
class ParticleEngine
{
public:

  //
  // Defines an upper limit on the number of particles any particle engine is allowed to spawn.
  // Used to avoid excessive memory usage by particle engines.
  //
  static constexpr int HARD_MAX_PARTICLES {1000};

  //
  // Configuration struct used to construct a particle engine.
  //
  // PROPERTY                   ROLE
  // --------                   ----
  //
  // _maxParticles              Controls the limit of spawned particles. Once the limit is 
  //                            reached no further particles can be spawned until old particles
  //                            die.
  //
  // _loVelocityComponent       Controls the lowest possible value of a randomly generated
  //                            velocities components. This value applies to both the X and Y
  //                            velocity components.
  //
  // _hiVelocityComponent       Controls the highest possible value of a randomly generated
  //                            velocities components.
  //
  // _loAccelerationComponent   The same as _loVelocityComponent but for acceleration.
  //
  // _hiAccelerationComponent   The same as _hiVelocityComponent but for acceleration.
  //
  // _loLifetime                The lowest possible randomly generated lifetime of a spawned
  //                            particle.
  //
  // _hiLifetime                The highest possible randomly generated lifetime of a spawned
  //                            particle.
  //
  // _damping                   The damping applied to each particle during each integration;
  //                            damping is used to take energy out of the system to prevent
  //                            particle movement becoming chaotic due to float math errors.
  //
  // _Color                     The color of all particles.
  //
  struct Configuration
  {
    int     _maxParticles            = 100;
    float   _loVelocityComponent     = -10.f;
    float   _hiVelocityComponent     = 10.f;
    float   _loAccelerationComponent = -10.f;
    float   _hiAccelerationComponent = 10.f;
    float   _loLifetime              = 0.5f;
    float   _hiLifetime              = 1.5f;
    float   _damping                 = 0.99f;
    gfx::Color4u _color              = gfx::colors::barbiepink;
  };

  ParticleEngine(Configuration config);
  ~ParticleEngine();

  //
  // Must call every update tick to integrate particle positions and velocities.
  //
  void update(float dt);

  //
  // Must call every draw tick to draw all particles. Particles are drawn as single pixels
  // (points) to the virtual screen. The real size of the particle in the window depends on 
  // the size of the virtual screens pixels.
  //
  void draw(int screenid);

  //
  // Spawns a particle. The version of this function called determines whether the particle
  // is assigned a random velocity and/or acceleration; if the function doesn't take the
  // argument the argument is randomised.
  //
  void spawnParticle(Vector2f position, Vector2f velocity, Vector2f acceleration);
  void spawnParticle(Vector2f position, Vector2f velocity);
  void spawnParticle(Vector2f position);

  //
  // Takes effect upon the next draw call and will change the color of all spawned particles,
  // past and future.
  //
  void setColor(gfx::Color4u color) {_config._color = color;}

  //
  // Takes effect for future integrations.
  //
  void setDamping(float damping);

  const gfx::Color4u& getParticleColor() const {return _config._color;}
  float getDamping() const {return _config._damping;}

private:

  struct Particle
  {
    Vector2f _position      = Vector2f{0.f, 0.f};
    Vector2f _velocity      = Vector2f{0.f, 0.f};
    Vector2f _acceleration  = Vector2f{0.f, 0.f};
    float _lifetime         = 0.f;
    float _clock            = 0.f;
    bool _isAlive           = false;
  };

private:

  Configuration _config;

  //
  // Raw particle data.
  //
  Particle* _particles;
  int _numParticles;
};

} // namespace pxr 

#endif
