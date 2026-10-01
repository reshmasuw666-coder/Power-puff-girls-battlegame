// =============================================================================
//  Character.h - game entities: projectiles, super-powers, heroines and enemies
//
//  Design patterns / OOP concepts used here
//  ----------------------------------------
//  * Strategy      : SuperPower is an abstract "firing strategy". Each heroine
//                    owns one (BubblePower / SlashPower / HeatRayPower) and the
//                    Player simply calls power().fire() - polymorphism.
//  * Template Method: Enemy::update() and Enemy::draw() define the algorithm
//                    skeleton; subclasses override only move()/fire()/decor.
//  * Inheritance   : Character -> Player | Enemy -> Drifter/Strafer/Diver/Boss.
//  * Encapsulation : state is protected/private; the outside world uses small,
//                    intention-revealing methods.
//
//  Everything is drawn procedurally with SFML shapes so the project needs no
//  external art assets to look good.
// =============================================================================
#pragma once

#include <SFML/Graphics.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace cfg {
constexpr float kWidth  = 800.f;   // logical playfield size (pixels)
constexpr float kHeight = 600.f;
constexpr float kPi     = 3.14159265358979f;
}

namespace util {
float     randomRange(float lo, float hi);
sf::Color mix(sf::Color a, sf::Color b, float t);
sf::Color withAlpha(sf::Color c, sf::Uint8 alpha);
}

// -----------------------------------------------------------------------------
//  Projectile - a plain data object with its own update/draw behaviour.
// -----------------------------------------------------------------------------
enum class ProjectileShape { Bubble, Slash, HeatBeam, EnemyOrb };

struct Projectile {
    sf::Vector2f    pos;
    sf::Vector2f    vel;
    float           radius   = 6.f;
    int             damage   = 1;
    bool            friendly = true;     // true = fired by the player
    ProjectileShape shape    = ProjectileShape::EnemyOrb;
    sf::Color       color    = sf::Color::White;
    int             pierce   = 0;        // extra enemies this projectile may pass through
    float           age      = 0.f;
    bool            alive    = true;
    std::vector<const void*> hitList;    // enemies already damaged (for piercing shots)

    void update(float dt);
    void draw(sf::RenderTarget& target) const;
};

// -----------------------------------------------------------------------------
//  Heroines & super-powers (Strategy pattern)
// -----------------------------------------------------------------------------
enum class Heroine { Bubbles, Buttercup, Blossom };

struct HeroineTraits {
    std::string name;
    sf::Color   dress, hair, eyes, accent;
};
const HeroineTraits& traitsFor(Heroine h);

class SuperPower {
public:
    virtual ~SuperPower() = default;
    virtual std::string name() const = 0;
    virtual float       cooldown() const = 0;                       // seconds between shots
    virtual void        fire(sf::Vector2f origin,
                             std::vector<Projectile>& out) const = 0;
};

class BubblePower  : public SuperPower {   // Level 1: fan of three bubbles
public:
    std::string name() const override { return "Bubble Burst"; }
    float cooldown() const override   { return 0.30f; }
    void fire(sf::Vector2f origin, std::vector<Projectile>& out) const override;
};
class SlashPower   : public SuperPower {   // Level 2: piercing energy crescent
public:
    std::string name() const override { return "Energy Slash"; }
    float cooldown() const override   { return 0.36f; }
    void fire(sf::Vector2f origin, std::vector<Projectile>& out) const override;
};
class HeatRayPower : public SuperPower {   // Level 3: rapid twin heat beams
public:
    std::string name() const override { return "Heat Ray"; }
    float cooldown() const override   { return 0.16f; }
    void fire(sf::Vector2f origin, std::vector<Projectile>& out) const override;
};

std::unique_ptr<SuperPower> makePower(Heroine h);   // simple factory

// -----------------------------------------------------------------------------
//  Character - abstract base of everything that has HP and lives on screen
// -----------------------------------------------------------------------------
class Character {
public:
    Character(sf::Vector2f pos, float radius, int maxHp);
    virtual ~Character() = default;

    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderTarget& target) const = 0;
    virtual bool takeDamage(int amount);     // returns true if damage was applied

    sf::Vector2f position() const { return m_pos; }
    float        radius()   const { return m_radius; }
    int          hp()       const { return m_hp; }
    int          maxHp()    const { return m_maxHp; }
    bool         isAlive()  const { return m_hp > 0; }

protected:
    void tickCommon(float dt);               // advances animation clock & hit flash

    sf::Vector2f m_pos;
    float        m_radius;
    int          m_hp;
    int          m_maxHp;
    float        m_time     = 0.f;
    float        m_hitFlash = 0.f;
};

// -----------------------------------------------------------------------------
//  Player - a chibi heroine controlled by the keyboard
// -----------------------------------------------------------------------------
class Player : public Character {
public:
    explicit Player(Heroine heroine,
                    sf::Vector2f pos = {cfg::kWidth / 2.f, cfg::kHeight - 90.f});

    void setMoveInput(sf::Vector2f dir, bool focus);
    bool tryShoot(bool trigger, std::vector<Projectile>& out);   // true if a volley was fired

    void update(float dt) override;
    void draw(sf::RenderTarget& target) const override;
    bool takeDamage(int amount) override;                        // honours invulnerability
    void heal(int amount);

    void setDisplayScale(float s) { m_scale = s; }
    void setShowcase(bool on)     { m_showcase = on; }           // menu-mode: no movement / blinking
    const SuperPower&    power()  const { return *m_power; }
    const HeroineTraits& traits() const { return m_traits; }
    Heroine              heroine() const { return m_heroine; }

private:
    Heroine                      m_heroine;
    HeroineTraits                m_traits;
    std::unique_ptr<SuperPower>  m_power;
    sf::Vector2f                 m_dir{0.f, 0.f};
    bool                         m_focus    = false;
    bool                         m_showcase = false;
    float                        m_cooldown = 0.f;
    float                        m_invuln   = 0.f;
    float                        m_muzzle   = 0.f;
    float                        m_scale    = 1.f;
};

// -----------------------------------------------------------------------------
//  Enemies (Template Method + polymorphism)
// -----------------------------------------------------------------------------
struct EnemyStyle {
    sf::Color body, accent, bullet;
    float     fireInterval = 2.f;
    float     bulletSpeed  = 200.f;
    int       hp           = 4;
};

class Enemy : public Character {
public:
    Enemy(sf::Vector2f pos, float radius, const EnemyStyle& style);

    void update(float dt) final;                       // template method
    void draw(sf::RenderTarget& target) const final;   // template method
    bool tryShoot(sf::Vector2f target, std::vector<Projectile>& out);
    void setTarget(sf::Vector2f t) { m_target = t; }

    virtual int  scoreValue() const { return 100; }
    virtual bool isBoss()     const { return false; }

protected:
    // Hooks that concrete enemies implement / customise:
    virtual void      move(float dt) = 0;
    virtual void      fire(sf::Vector2f target, std::vector<Projectile>& out) = 0;
    virtual bool      canShoot()  const { return m_pos.y > 30.f; }
    virtual sf::Color bodyColor() const { return m_style.body; }
    virtual void      drawBehind(sf::RenderTarget&) const {}
    virtual void      drawFront(sf::RenderTarget&)  const {}

    Projectile   makeOrb(sf::Vector2f from, float angleRad, float speed) const;
    sf::Vector2f drawPos() const;

    EnemyStyle   m_style;
    sf::Vector2f m_target{cfg::kWidth / 2.f, cfg::kHeight};
    float        m_fireTimer;
    float        m_phase;
};

// Hovers and sways sideways, firing single aimed shots.
class DrifterEnemy : public Enemy {
public:
    DrifterEnemy(float x, const EnemyStyle& style);
protected:
    void move(float dt) override;
    void fire(sf::Vector2f target, std::vector<Projectile>& out) override;
    void drawFront(sf::RenderTarget& t) const override;
private:
    float m_anchorX, m_hoverY, m_amp;
};

// Patrols left-right across the screen, firing a 3-way fan.
class StrafeEnemy : public Enemy {
public:
    StrafeEnemy(float x, const EnemyStyle& style);
protected:
    void move(float dt) override;
    void fire(sf::Vector2f target, std::vector<Projectile>& out) override;
    void drawBehind(sf::RenderTarget& t) const override;
private:
    float m_rowY, m_vx;
};

// Hovers, telegraphs, then dives at the player's position - needs dodging!
class DiveEnemy : public Enemy {
public:
    DiveEnemy(float x, const EnemyStyle& style);
protected:
    void      move(float dt) override;
    void      fire(sf::Vector2f target, std::vector<Projectile>& out) override;
    bool      canShoot() const override { return m_state == State::Hover && m_pos.y > 30.f; }
    sf::Color bodyColor() const override;
    void      drawBehind(sf::RenderTarget& t) const override;
private:
    enum class State { Hover, Telegraph, Dive };
    State        m_state = State::Hover;
    float        m_stateTime = 0.f, m_hoverDuration;
    float        m_anchorX, m_hoverY = 140.f;
    sf::Vector2f m_diveDir{0.f, 1.f};
};

// Level boss: big, two phases, alternates aimed fans and rotating rings.
class BossEnemy : public Enemy {
public:
    BossEnemy(const EnemyStyle& style, int bossHp);
    int  scoreValue() const override { return 1500; }
    bool isBoss()     const override { return true; }
protected:
    void move(float dt) override;
    void fire(sf::Vector2f target, std::vector<Projectile>& out) override;
    void drawBehind(sf::RenderTarget& t) const override;
    void drawFront(sf::RenderTarget& t) const override;
private:
    bool  enraged() const { return m_hp * 2 < m_maxHp; }
    int   m_pattern = 0;
    float m_ringOffset = 0.f;
};
