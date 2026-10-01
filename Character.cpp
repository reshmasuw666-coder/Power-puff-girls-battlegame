// =============================================================================
//  Character.cpp - implementation of projectiles, powers, heroines & enemies
// =============================================================================
#include "Character.h"

#include <algorithm>
#include <cmath>
#include <random>

// ----------------------------------------------------------------------------
//  util
// ----------------------------------------------------------------------------
namespace util {
static std::mt19937& engine()
{
    static std::mt19937 e{std::random_device{}()};
    return e;
}
float randomRange(float lo, float hi)
{
    std::uniform_real_distribution<float> d(lo, hi);
    return d(engine());
}
sf::Color mix(sf::Color a, sf::Color b, float t)
{
    t = std::clamp(t, 0.f, 1.f);
    auto m = [t](sf::Uint8 x, sf::Uint8 y) {
        return static_cast<sf::Uint8>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return sf::Color(m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a));
}
sf::Color withAlpha(sf::Color c, sf::Uint8 alpha)
{
    c.a = alpha;
    return c;
}
} // namespace util

// ----------------------------------------------------------------------------
//  Tiny drawing helper: everything is expressed in a local coordinate space
//  (offsets from an origin, multiplied by a scale) so one drawing routine works
//  for menu showcase (big), gameplay (normal) and boss (huge).
// ----------------------------------------------------------------------------
namespace {
struct Brush {
    sf::RenderTarget& target;
    sf::Vector2f      origin;
    float             scale;

    sf::Vector2f at(sf::Vector2f o) const { return {origin.x + o.x * scale, origin.y + o.y * scale}; }

    void disc(sf::Vector2f off, float r, sf::Color fill, float sx = 1.f, float sy = 1.f,
              float outline = 0.f, sf::Color oc = sf::Color::Transparent) const
    {
        sf::CircleShape s(r * scale, 28);
        s.setOrigin(r * scale, r * scale);
        s.setPosition(at(off));
        s.setScale(sx, sy);
        s.setFillColor(fill);
        if (outline > 0.f) {
            s.setOutlineThickness(outline * scale);
            s.setOutlineColor(oc);
        }
        target.draw(s);
    }
    void poly(std::initializer_list<sf::Vector2f> pts, sf::Color fill) const   // must be convex
    {
        sf::ConvexShape s(pts.size());
        std::size_t i = 0;
        for (const auto& p : pts) s.setPoint(i++, at(p));
        s.setFillColor(fill);
        target.draw(s);
    }
    void rect(sf::Vector2f off, sf::Vector2f size, sf::Color fill) const
    {
        sf::RectangleShape s({size.x * scale, size.y * scale});
        s.setOrigin(size.x * scale / 2.f, size.y * scale / 2.f);
        s.setPosition(at(off));
        s.setFillColor(fill);
        target.draw(s);
    }
};

const sf::Color kSkin(255, 228, 208);
const sf::Color kInk(70, 40, 70);
} // namespace

// ----------------------------------------------------------------------------
//  Projectile
// ----------------------------------------------------------------------------
void Projectile::update(float dt)
{
    pos += vel * dt;
    age += dt;
    const float m = 90.f;
    if (pos.x < -m || pos.x > cfg::kWidth + m || pos.y < -m || pos.y > cfg::kHeight + m)
        alive = false;
}

void Projectile::draw(sf::RenderTarget& t) const
{
    switch (shape) {
    case ProjectileShape::Bubble: {
        const float wob = 1.f + 0.07f * std::sin(age * 14.f);      // jelly-like wobble
        sf::CircleShape c(radius, 24);
        c.setOrigin(radius, radius);
        c.setPosition(pos);
        c.setScale(wob, 2.f - wob);
        c.setFillColor(util::withAlpha(color, 95));
        c.setOutlineThickness(2.f);
        c.setOutlineColor(sf::Color(255, 255, 255, 230));
        t.draw(c);
        sf::CircleShape h(radius * 0.25f, 12);
        h.setOrigin(radius * 0.25f, radius * 0.25f);
        h.setPosition(pos.x - radius * 0.35f, pos.y - radius * 0.35f);
        h.setFillColor(sf::Color(255, 255, 255, 235));
        t.draw(h);
        break;
    }
    case ProjectileShape::Slash: {
        // Crescent = triangle strip between an outer and an inner arc.
        constexpr int   N = 12;
        constexpr float A = 1.15f;                                  // half arc angle (rad)
        const float     R = radius * 1.35f;
        auto build = [&](float outerScale, float innerK, sf::Color col) {
            sf::VertexArray va(sf::TriangleStrip);
            for (int i = 0; i <= N; ++i) {
                const float a = -A + 2.f * A * static_cast<float>(i) / N;
                const sf::Vector2f outer(std::sin(a) * R * 1.3f * outerScale, -std::cos(a) * R * outerScale);
                const sf::Vector2f inner(std::sin(a) * R * 1.3f * outerScale,
                                         -R * outerScale * (innerK * std::cos(a) + (1.f - innerK) * std::cos(A)));
                va.append(sf::Vertex(outer, col));
                va.append(sf::Vertex(inner, col));
            }
            return va;
        };
        sf::Transform tr;
        tr.translate(pos);
        tr.rotate(std::atan2(vel.y, vel.x) * 180.f / cfg::kPi + 90.f);
        t.draw(build(1.15f, 0.30f, util::withAlpha(color, 90)), tr);                       // glow
        t.draw(build(1.00f, 0.35f, color), tr);                                            // body
        t.draw(build(0.92f, 0.60f, util::mix(color, sf::Color::White, 0.7f)), tr);         // highlight
        break;
    }
    case ProjectileShape::HeatBeam: {
        const float len = radius * 4.2f, w = radius * 0.9f;
        const float rot = std::atan2(vel.y, vel.x) * 180.f / cfg::kPi + 90.f;
        auto bar = [&](float bw, float bl, sf::Color col) {
            sf::RectangleShape r({bw, bl});
            r.setOrigin(bw / 2.f, bl / 2.f);
            r.setPosition(pos);
            r.setRotation(rot);
            r.setFillColor(col);
            t.draw(r);
        };
        bar(w * 2.4f, len * 1.1f, util::withAlpha(color, 80));
        bar(w * 1.1f, len, color);
        bar(w * 0.45f, len * 0.85f, sf::Color(255, 250, 220));
        break;
    }
    case ProjectileShape::EnemyOrb: {
        const float pulse = 1.f + 0.12f * std::sin(age * 12.f);
        sf::CircleShape glow(radius * 1.8f * pulse, 20);
        glow.setOrigin(radius * 1.8f * pulse, radius * 1.8f * pulse);
        glow.setPosition(pos);
        glow.setFillColor(util::withAlpha(color, 70));
        t.draw(glow);
        sf::CircleShape body(radius, 20);
        body.setOrigin(radius, radius);
        body.setPosition(pos);
        body.setFillColor(color);
        body.setOutlineThickness(2.f);
        body.setOutlineColor(sf::Color(255, 255, 255, 220));
        t.draw(body);
        sf::CircleShape core(radius * 0.4f, 12);
        core.setOrigin(radius * 0.4f, radius * 0.4f);
        core.setPosition(pos);
        core.setFillColor(sf::Color::White);
        t.draw(core);
        break;
    }
    }
}

// ----------------------------------------------------------------------------
//  Heroines & powers
// ----------------------------------------------------------------------------
const HeroineTraits& traitsFor(Heroine h)
{
    static const HeroineTraits bubbles  {"Bubbles",   {120, 190, 255}, {255, 226, 120}, {80, 150, 255},  {110, 180, 255}};
    static const HeroineTraits buttercup{"Buttercup", {120, 215, 145}, {55, 55, 80},    {70, 195, 105},  {90, 210, 120}};
    static const HeroineTraits blossom  {"Blossom",   {255, 130, 145}, {255, 165, 105}, {235, 100, 150}, {240, 75, 95}};
    switch (h) {
    case Heroine::Bubbles:   return bubbles;
    case Heroine::Buttercup: return buttercup;
    default:                 return blossom;
    }
}

void BubblePower::fire(sf::Vector2f o, std::vector<Projectile>& out) const
{
    for (float a : {-0.17f, 0.f, 0.17f}) {
        Projectile p;
        p.pos    = o;
        p.vel    = {std::sin(a) * 480.f, -std::cos(a) * 480.f};
        p.radius = (a == 0.f) ? 11.f : 8.f;
        p.damage = 1;
        p.shape  = ProjectileShape::Bubble;
        p.color  = sf::Color(140, 200, 255);
        out.push_back(p);
    }
}

void SlashPower::fire(sf::Vector2f o, std::vector<Projectile>& out) const
{
    Projectile p;
    p.pos    = o;
    p.vel    = {0.f, -560.f};
    p.radius = 22.f;
    p.damage = 3;
    p.pierce = 2;                                   // can cut through up to 3 enemies
    p.shape  = ProjectileShape::Slash;
    p.color  = sf::Color(110, 225, 135);
    out.push_back(p);
}

void HeatRayPower::fire(sf::Vector2f o, std::vector<Projectile>& out) const
{
    for (float dx : {-9.f, 9.f}) {
        Projectile p;
        p.pos    = {o.x + dx, o.y};
        p.vel    = {0.f, -820.f};
        p.radius = 7.f;
        p.damage = 1;
        p.shape  = ProjectileShape::HeatBeam;
        p.color  = sf::Color(255, 100, 100);
        out.push_back(p);
    }
}

std::unique_ptr<SuperPower> makePower(Heroine h)
{
    switch (h) {
    case Heroine::Bubbles:   return std::make_unique<BubblePower>();
    case Heroine::Buttercup: return std::make_unique<SlashPower>();
    default:                 return std::make_unique<HeatRayPower>();
    }
}

// ----------------------------------------------------------------------------
//  Character
// ----------------------------------------------------------------------------
Character::Character(sf::Vector2f pos, float radius, int maxHp)
    : m_pos(pos), m_radius(radius), m_hp(maxHp), m_maxHp(maxHp) {}

bool Character::takeDamage(int amount)
{
    if (!isAlive()) return false;
    m_hp -= amount;
    m_hitFlash = 0.08f;
    return true;
}

void Character::tickCommon(float dt)
{
    m_time += dt;
    m_hitFlash = std::max(0.f, m_hitFlash - dt);
}

// ----------------------------------------------------------------------------
//  Player
// ----------------------------------------------------------------------------
Player::Player(Heroine heroine, sf::Vector2f pos)
    : Character(pos, 9.f, 5),                 // 9px hit-box: forgiving "bullet-hell" style
      m_heroine(heroine),
      m_traits(traitsFor(heroine)),
      m_power(makePower(heroine)) {}

void Player::setMoveInput(sf::Vector2f dir, bool focus)
{
    m_dir   = dir;
    m_focus = focus;
}

void Player::update(float dt)
{
    tickCommon(dt);
    m_cooldown = std::max(0.f, m_cooldown - dt);
    m_invuln   = std::max(0.f, m_invuln - dt);
    m_muzzle   = std::max(0.f, m_muzzle - dt);
    if (m_showcase) return;

    sf::Vector2f d = m_dir;
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len > 1.f) d /= len;                              // no faster diagonals
    const float speed = m_focus ? 140.f : 310.f;          // Shift = precise movement
    m_pos += d * speed * dt;
    m_pos.x = std::clamp(m_pos.x, 26.f, cfg::kWidth - 26.f);
    m_pos.y = std::clamp(m_pos.y, 90.f, cfg::kHeight - 40.f);
}

bool Player::tryShoot(bool trigger, std::vector<Projectile>& out)
{
    if (!trigger || m_cooldown > 0.f) return false;
    m_power->fire({m_pos.x, m_pos.y - 28.f}, out);
    m_cooldown = m_power->cooldown();
    m_muzzle   = 0.12f;
    return true;
}

bool Player::takeDamage(int amount)
{
    if (m_invuln > 0.f || !isAlive()) return false;
    m_hp       = std::max(0, m_hp - amount);
    m_invuln   = 1.6f;                                    // brief mercy period
    m_hitFlash = 0.2f;
    return true;
}

void Player::heal(int amount) { m_hp = std::min(m_maxHp, m_hp + amount); }

void Player::draw(sf::RenderTarget& t) const
{
    if (!m_showcase && m_invuln > 0.f && static_cast<int>(m_invuln * 12.f) % 2 == 0)
        return;                                            // blink while invulnerable

    const float bob = std::sin(m_time * 4.f) * 2.f * m_scale;
    const Brush b{t, {m_pos.x, m_pos.y + bob}, m_scale};
    const sf::Color hair = m_traits.hair;
    const sf::Color hairDark = util::mix(hair, sf::Color::Black, 0.2f);
    const sf::Color white(255, 255, 255);

    // Hit-box / focus indicator (only while precise-moving in gameplay)
    if (!m_showcase && m_focus)
        b.disc({0, 0}, 9.f, sf::Color(255, 255, 255, 120), 1, 1, 1.5f, util::withAlpha(m_traits.accent, 230));

    b.disc({0, 36}, 15.f, sf::Color(0, 0, 0, 28), 1.f, 0.3f);                       // shadow

    // ---- hair (back layer) --------------------------------------------------
    switch (m_heroine) {
    case Heroine::Bubbles:    // twin pigtails
        b.disc({-28, -2}, 11.f, hair, 1.f, 1.2f, 1.f, hairDark);
        b.disc({ 28, -2}, 11.f, hair, 1.f, 1.2f, 1.f, hairDark);
        break;
    case Heroine::Buttercup:  // short bob
        b.disc({0, -6}, 25.f, hair);
        break;
    case Heroine::Blossom:    // long flowing hair
        b.disc({0, -8}, 24.f, hair);
        b.rect({0, 14}, {46.f, 34.f}, hair);
        break;
    }

    // ---- legs, shoes, dress, arms --------------------------------------------
    b.rect({-6, 31}, {5, 10}, kSkin);
    b.rect({ 6, 31}, {5, 10}, kSkin);
    b.rect({-6, 37}, {8, 4}, kInk);
    b.rect({ 6, 37}, {8, 4}, kInk);
    b.poly({{-10, 6}, {10, 6}, {19, 30}, {-19, 30}}, m_traits.dress);
    b.rect({0, 9}, {21, 4}, kInk);                                                  // belt
    b.disc({-14, 14}, 4.f, kSkin);
    b.disc({ 14, 14}, 4.f, kSkin);

    // ---- head ---------------------------------------------------------------
    b.disc({0, -10}, 22.f, hair);                                                   // hair cap
    b.disc({0, -4}, 19.5f, m_hitFlash > 0.f ? sf::Color(255, 200, 200) : kSkin);    // face
    b.poly({{-20, -17}, {-2, -19}, {-11, -4}}, hair);                               // bangs
    b.poly({{-2, -19}, {20, -17}, {9, -4}}, hair);

    for (float s : {-1.f, 1.f}) {                                                   // big anime eyes
        b.disc({s * 8.5f, -3.f}, 6.2f, white, 1.f, 1.25f, 0.8f, kInk);
        b.disc({s * 8.5f, -2.f}, 4.4f, m_traits.eyes, 1.f, 1.25f);
        b.disc({s * 8.5f, -1.5f}, 2.2f, kInk, 1.f, 1.2f);
        b.disc({s * 8.5f - 1.6f, -4.6f}, 1.7f, white);
        b.disc({s * 13.f, 5.f}, 3.2f, sf::Color(255, 150, 170, 150), 1.3f, 0.8f);   // blush
    }
    b.disc({0, 7}, 1.8f, sf::Color(205, 85, 115), 1.4f, 0.8f);                      // smile

    // ---- per-heroine accessories (front layer) -------------------------------
    switch (m_heroine) {
    case Heroine::Bubbles:
        b.disc({-21, -10}, 3.5f, m_traits.accent, 1, 1, 1.f, white);
        b.disc({ 21, -10}, 3.5f, m_traits.accent, 1, 1, 1.f, white);
        break;
    case Heroine::Buttercup:
        b.poly({{-16, -26}, {-9, -38}, {-2, -27}}, hair);
        b.poly({{-5, -28}, {3, -41}, {10, -28}}, hair);
        b.poly({{7, -27}, {16, -37}, {20, -24}}, hair);
        break;
    case Heroine::Blossom: {
        const sf::Color bow = m_traits.accent;
        b.poly({{17, -24}, {6, -32}, {6, -16}}, bow);                               // big red bow
        b.poly({{17, -24}, {28, -32}, {28, -16}}, bow);
        b.disc({17, -24}, 3.5f, util::mix(bow, sf::Color::Black, 0.25f));
        break;
    }
    }

    if (m_muzzle > 0.f)                                                             // cast sparkle
        b.disc({0, -32}, 7.f + m_muzzle * 40.f, util::withAlpha(m_traits.accent, 150));
}

// ----------------------------------------------------------------------------
//  Enemy base (Template Method)
// ----------------------------------------------------------------------------
Enemy::Enemy(sf::Vector2f pos, float radius, const EnemyStyle& style)
    : Character(pos, radius, style.hp),
      m_style(style),
      m_fireTimer(util::randomRange(0.9f, 1.8f)),
      m_phase(util::randomRange(0.f, 6.28f)) {}

void Enemy::update(float dt)
{
    tickCommon(dt);
    m_fireTimer -= dt;
    move(dt);                      // <- varies per subclass
}

bool Enemy::tryShoot(sf::Vector2f target, std::vector<Projectile>& out)
{
    if (m_fireTimer > 0.f || !canShoot()) return false;
    fire(target, out);             // <- varies per subclass
    m_fireTimer = m_style.fireInterval * util::randomRange(0.8f, 1.25f);
    return true;
}

Projectile Enemy::makeOrb(sf::Vector2f from, float angle, float speed) const
{
    Projectile p;
    p.pos      = from;
    p.vel      = {std::cos(angle) * speed, std::sin(angle) * speed};
    p.radius   = 7.f;
    p.damage   = 1;
    p.friendly = false;
    p.shape    = ProjectileShape::EnemyOrb;
    p.color    = m_style.bullet;
    return p;
}

sf::Vector2f Enemy::drawPos() const
{
    return {m_pos.x, m_pos.y + std::sin(m_time * 3.f + m_phase) * 2.f};
}

void Enemy::draw(sf::RenderTarget& t) const
{
    drawBehind(t);

    const Brush b{t, drawPos(), m_radius / 22.f};
    sf::Color body = bodyColor();
    if (m_hitFlash > 0.f) body = util::mix(body, sf::Color::White, 0.75f);
    const sf::Color outline = util::mix(body, sf::Color::Black, 0.3f);
    const sf::Color white(255, 255, 255);

    // ears
    b.poly({{-17, -12}, {-10, -31}, {-2, -19}}, body);
    b.poly({{ 17, -12}, { 10, -31}, { 2, -19}}, body);
    b.poly({{-13, -17}, {-10, -25}, {-6, -19}}, m_style.accent);
    b.poly({{ 13, -17}, { 10, -25}, { 6, -19}}, m_style.accent);
    // body + belly
    b.disc({0, 0}, 22.f, body, 1.f, 0.95f, 2.5f, outline);
    b.disc({0, 8}, 12.f, m_style.accent, 1.f, 0.8f);

    // grumpy eyes that follow the player
    const float lx = std::clamp((m_target.x - m_pos.x) / 200.f, -1.f, 1.f) * 1.8f;
    const float ly = std::clamp((m_target.y - m_pos.y) / 300.f, -0.5f, 1.f) * 1.5f;
    for (float s : {-1.f, 1.f}) {
        b.disc({s * 8.f, -4.f}, 5.5f, white, 1.f, 1.1f, 1.f, kInk);
        b.disc({s * 8.f + lx, -3.f + ly}, 2.8f, kInk);
        b.disc({s * 14.f, 4.f}, 3.f, sf::Color(255, 150, 170, 150), 1.3f, 0.8f);
    }
    b.poly({{-14, -14}, {-3, -9.5f}, {-3, -6.5f}, {-14, -11}}, kInk);              // angry brows
    b.poly({{ 14, -14}, { 3, -9.5f}, { 3, -6.5f}, { 14, -11}}, kInk);
    b.disc({0, 8}, 4.f, kInk, 1.4f, 0.7f);                                          // mouth + fangs
    b.poly({{-4, 6.5f}, {-1, 6.5f}, {-2.5f, 11}}, white);
    b.poly({{ 1, 6.5f}, { 4, 6.5f}, { 2.5f, 11}}, white);

    drawFront(t);

    if (m_hp < m_maxHp) {                                                           // HP bar
        const float w = std::max(36.f, m_radius * 1.8f);
        const sf::Vector2f p = drawPos();
        sf::RectangleShape bg({w, 5.f});
        bg.setOrigin(w / 2.f, 2.5f);
        bg.setPosition(p.x, p.y - m_radius - 16.f);
        bg.setFillColor(sf::Color(255, 255, 255, 190));
        t.draw(bg);
        sf::RectangleShape fg({w * static_cast<float>(m_hp) / static_cast<float>(m_maxHp), 5.f});
        fg.setOrigin(w / 2.f, 2.5f);
        fg.setPosition(p.x, p.y - m_radius - 16.f);
        fg.setFillColor(sf::Color(255, 110, 150));
        t.draw(fg);
    }
}

// ----------------------------------------------------------------------------
//  DrifterEnemy
// ----------------------------------------------------------------------------
DrifterEnemy::DrifterEnemy(float x, const EnemyStyle& s)
    : Enemy({x, -40.f}, 22.f, s), m_anchorX(std::clamp(x, 130.f, cfg::kWidth - 130.f)),
      m_hoverY(util::randomRange(100.f, 200.f)), m_amp(util::randomRange(60.f, 110.f)) {}

void DrifterEnemy::move(float dt)
{
    if (m_pos.y < m_hoverY) m_pos.y = std::min(m_hoverY, m_pos.y + 110.f * dt);
    m_pos.x = m_anchorX + std::sin(m_time * 1.4f + m_phase) * m_amp;
}

void DrifterEnemy::fire(sf::Vector2f target, std::vector<Projectile>& out)
{
    const float a = std::atan2(target.y - m_pos.y, target.x - m_pos.x) + util::randomRange(-0.05f, 0.05f);
    out.push_back(makeOrb(m_pos, a, m_style.bulletSpeed));
}

void DrifterEnemy::drawFront(sf::RenderTarget& t) const     // little antenna
{
    const Brush b{t, drawPos(), m_radius / 22.f};
    b.rect({0, -30}, {2.f, 10.f}, kInk);
    b.disc({0, -37}, 4.f, m_style.bullet, 1, 1, 1.f, sf::Color::White);
}

// ----------------------------------------------------------------------------
//  StrafeEnemy
// ----------------------------------------------------------------------------
StrafeEnemy::StrafeEnemy(float x, const EnemyStyle& s)
    : Enemy({x, -40.f}, 21.f, s), m_rowY(util::randomRange(90.f, 170.f)),
      m_vx(util::randomRange(0.f, 1.f) < 0.5f ? -125.f : 125.f) {}

void StrafeEnemy::move(float dt)
{
    if (m_pos.y < m_rowY) m_pos.y = std::min(m_rowY, m_pos.y + 120.f * dt);
    m_pos.x += m_vx * dt;
    if (m_pos.x < 40.f)                { m_pos.x = 40.f;                m_vx =  std::abs(m_vx); }
    if (m_pos.x > cfg::kWidth - 40.f)  { m_pos.x = cfg::kWidth - 40.f;  m_vx = -std::abs(m_vx); }
}

void StrafeEnemy::fire(sf::Vector2f target, std::vector<Projectile>& out)
{
    const float base = std::atan2(target.y - m_pos.y, target.x - m_pos.x);
    for (float d : {-0.30f, 0.f, 0.30f})
        out.push_back(makeOrb(m_pos, base + d, m_style.bulletSpeed * 0.92f));
}

void StrafeEnemy::drawBehind(sf::RenderTarget& t) const     // flapping wings
{
    const Brush b{t, drawPos(), m_radius / 22.f};
    const float flap = 0.55f + 0.45f * std::abs(std::sin(m_time * 12.f));
    const sf::Color wing = util::withAlpha(sf::Color::White, 210);
    b.disc({-27, -6}, 11.f, wing, 1.f, flap, 1.5f, m_style.accent);
    b.disc({ 27, -6}, 11.f, wing, 1.f, flap, 1.5f, m_style.accent);
}

// ----------------------------------------------------------------------------
//  DiveEnemy
// ----------------------------------------------------------------------------
DiveEnemy::DiveEnemy(float x, const EnemyStyle& s)
    : Enemy({x, -40.f}, 22.f, s), m_hoverDuration(util::randomRange(2.2f, 3.6f)),
      m_anchorX(std::clamp(x, 100.f, cfg::kWidth - 100.f)) {}

void DiveEnemy::move(float dt)
{
    m_stateTime += dt;
    switch (m_state) {
    case State::Hover:
        if (m_pos.y < m_hoverY) m_pos.y = std::min(m_hoverY, m_pos.y + 130.f * dt);
        m_pos.x = m_anchorX + std::sin(m_time * 1.1f + m_phase) * 60.f;
        if (m_stateTime > m_hoverDuration && m_pos.y >= m_hoverY - 1.f) {
            m_state = State::Telegraph;
            m_stateTime = 0.f;
        }
        break;
    case State::Telegraph:                       // shake + flash red: a fair warning
        m_pos.x += std::sin(m_time * 90.f) * 1.6f;
        if (m_stateTime > 0.75f) {
            sf::Vector2f d = m_target - m_pos;
            d.y = std::max(d.y, 120.f);          // always dive downward
            const float len = std::sqrt(d.x * d.x + d.y * d.y);
            m_diveDir = d / len;
            m_state = State::Dive;
            m_stateTime = 0.f;
        }
        break;
    case State::Dive:
        m_pos += m_diveDir * 430.f * dt;
        if (m_pos.y > cfg::kHeight + 50.f || m_pos.x < -50.f || m_pos.x > cfg::kWidth + 50.f) {
            m_anchorX = util::randomRange(100.f, cfg::kWidth - 100.f);   // re-enter from the top
            m_pos = {m_anchorX, -40.f};
            m_state = State::Hover;
            m_stateTime = 0.f;
            m_hoverDuration = util::randomRange(2.2f, 3.6f);
        }
        break;
    }
}

void DiveEnemy::fire(sf::Vector2f target, std::vector<Projectile>& out)
{
    const float a = std::atan2(target.y - m_pos.y, target.x - m_pos.x);
    out.push_back(makeOrb(m_pos, a, m_style.bulletSpeed * 1.05f));
}

sf::Color DiveEnemy::bodyColor() const
{
    if (m_state == State::Telegraph && static_cast<int>(m_time * 16.f) % 2 == 0)
        return sf::Color(255, 120, 120);
    return m_style.body;
}

void DiveEnemy::drawBehind(sf::RenderTarget& t) const       // spiky halo
{
    const Brush b{t, drawPos(), m_radius / 22.f};
    const sf::Color spike = util::mix(m_style.body, sf::Color::Black, 0.25f);
    for (int k = 0; k < 8; ++k) {
        const float a = static_cast<float>(k) * cfg::kPi / 4.f + m_time * 0.6f;
        auto P = [&](float ang, float r) { return sf::Vector2f(std::cos(ang) * r, std::sin(ang) * r); };
        b.poly({P(a - 0.30f, 19.f), P(a, 33.f), P(a + 0.30f, 19.f)}, spike);
    }
}

// ----------------------------------------------------------------------------
//  BossEnemy
// ----------------------------------------------------------------------------
BossEnemy::BossEnemy(const EnemyStyle& style, int bossHp)
    : Enemy({cfg::kWidth / 2.f, -80.f}, 46.f, [&] { EnemyStyle s = style; s.hp = bossHp; return s; }()) {}

void BossEnemy::move(float dt)
{
    if (m_pos.y < 115.f) m_pos.y = std::min(115.f, m_pos.y + 70.f * dt);
    const float speed = enraged() ? 1.15f : 0.7f;
    m_pos.x = cfg::kWidth / 2.f + std::sin(m_time * speed) * 260.f;
}

void BossEnemy::fire(sf::Vector2f target, std::vector<Projectile>& out)
{
    const float spd = m_style.bulletSpeed;
    if (m_pattern++ % 2 == 0) {                               // aimed fan
        const float base = std::atan2(target.y - m_pos.y, target.x - m_pos.x);
        const int   n    = enraged() ? 7 : 5;
        for (int i = 0; i < n; ++i)
            out.push_back(makeOrb(m_pos, base + (static_cast<float>(i) - (n - 1) / 2.f) * 0.24f, spd));
    } else {                                                  // rotating ring
        const int n = enraged() ? 16 : 12;
        m_ringOffset += 0.21f;
        for (int i = 0; i < n; ++i)
            out.push_back(makeOrb(m_pos, m_ringOffset + 2.f * cfg::kPi * static_cast<float>(i) / n, spd * 0.72f));
    }
    m_fireTimer = 0.f;   // (interval is applied by tryShoot)
}

void BossEnemy::drawBehind(sf::RenderTarget& t) const     // pulsing aura
{
    const Brush b{t, drawPos(), m_radius / 22.f};
    const float p = 1.f + 0.08f * std::sin(m_time * 5.f);
    b.disc({0, 0}, 34.f * p, util::withAlpha(m_style.accent, enraged() ? 120 : 70));
}

void BossEnemy::drawFront(sf::RenderTarget& t) const      // golden crown
{
    const Brush b{t, drawPos(), m_radius / 22.f};
    const sf::Color gold(255, 214, 90);
    b.rect({0, -24}, {34.f, 6.f}, gold);
    b.poly({{-17, -27}, {-13, -42}, {-6, -27}}, gold);
    b.poly({{ -7, -27}, {  0, -46}, { 7, -27}}, gold);
    b.poly({{  6, -27}, { 13, -42}, {17, -27}}, gold);
    b.disc({0, -24}, 2.8f, sf::Color(255, 90, 130));
}
