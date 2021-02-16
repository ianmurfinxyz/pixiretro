#include "particles.h"
#include "gfx.h"

namespace pxr
{

ParticleEngine::ParticleEngine(gfx::Color4u color, RandRange velocityRange, 
  RandRange accelerationRange, float damping) 
  :
  _particles{},
  _velRand{velocityRange._lo, velocityRange._hi},
  _accRand{accelerationRange._lo, accelerationRange._hi},
  _color{color},
  _damping{damping}
{
}

void ParticleEngine::update(float dt)
{
  for(auto& particle : _particles){
    particle._velocity += particle._acceleration * dt;
    particle._velocity *= _damping;
    particle._position += particle._velocity * dt; 
  }
}

void ParticleEngine::draw(int screenid)
{
  for(auto& particle : _particles)
    gfx::drawPoint(particle._position, _color, screenid);
}

void ParticleEngine::spawnParticle(Vector2f position, Vector2f velocity, Vector2f acceleration)
{
  Particle p {};
  p._position = position;
  p._velocity = velocity;
  p._acceleration = acceleration;
  _particles.push_back(p);
}

void ParticleEngine::spawnParticle(Vector2f position, Vector2f velocity)
{
  Particle p {};
  p._position = position;
  p._velocity = velocity;

  Vector2f a {};
  a._x = _accRand();
  a._y = _accRand();
  p._acceleration = a;

  _particles.push_back(p);
}

void ParticleEngine::spawnParticle(Vector2f position)
{
  Particle p {};
  p._position = position;

  Vector2f v {};
  v._x = _accRand();
  v._y = _accRand();
  p._velocity = v;

  Vector2f a {};
  a._x = _accRand();
  a._y = _accRand();
  p._acceleration = a;

  _particles.push_back(p);
}

} // namespace pxr
