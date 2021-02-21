#include <algorithm>
#include <cassert>
#include "pxr_particle.h"
#include "pxr_gfx.h"

namespace pxr
{

ParticleEngine::ParticleEngine(Configuration config) : 
  _config{config},
  _numParticles{0}
{
  assert(0 < _config._maxParticles && _config._maxParticles <= HARD_MAX_PARTICLES);
  _particles = new Particle[_config._maxParticles];
}

ParticleEngine::~ParticleEngine()
{
  if(_particles != nullptr)
    delete[] _particles;
}

void ParticleEngine::update(float dt)
{
  assert(_particles != nullptr);

  int numUpdates{0}, i{0};
  while(i < _config._maxParticles && numUpdates < _numParticles){
    auto& particle = _particles[i];

    if(!particle._isAlive)
      continue;

    particle._clock += dt;
    if(particle._clock > particle._lifetime){
      particle._isAlive = false; 
      continue;
    }

    particle._velocity += particle._acceleration * dt;
    particle._velocity *= _config._damping;
    particle._position += particle._velocity * dt; 

    ++numUpdates;
    ++i;
  }
}

void ParticleEngine::draw(int screenid)
{
  assert(_particles != nullptr);
  for(int i = 0; i < _config._maxParticles; ++i){
    auto& particle = _particles[i];
    if(particle._isAlive)
      gfx::drawPoint(particle._position, _config._color, screenid);
  }
}

void ParticleEngine::spawnParticle(Vector2f position, Vector2f velocity, Vector2f acceleration)
{
  assert(_particles != nullptr);

  for(int i = 0; i < _config._maxParticles; ++i){
    auto& particle = _particles[i];

    if(particle._isAlive)
      continue;

    particle._position = position;
    particle._velocity = velocity;
    particle._acceleration = acceleration;
    particle._lifetime = rand::uniformReal(_config._loLifetime, _config._hiLifetime);
    particle._clock = 0.f;
    particle._isAlive = true;

    break;
  }
}

void ParticleEngine::spawnParticle(Vector2f position, Vector2f velocity)
{
  assert(_particles != nullptr);
  Vector2f a {
    rand::uniformReal(_config._loAccelerationComponent, _config._hiAccelerationComponent),
    rand::uniformReal(_config._loAccelerationComponent, _config._hiAccelerationComponent)
  };
  spawnParticle(position, velocity, a);
}

void ParticleEngine::spawnParticle(Vector2f position)
{
  assert(_particles != nullptr);
  Vector2f v {
    rand::uniformReal(_config._loVelocityComponent, _config._hiVelocityComponent),
    rand::uniformReal(_config._loVelocityComponent, _config._hiVelocityComponent)
  };
  spawnParticle(position, v);
}

void ParticleEngine::setDamping(float damping)
{
  _config._damping = std::clamp(damping, 0.f, 1.f);
}

} // namespace pxr
