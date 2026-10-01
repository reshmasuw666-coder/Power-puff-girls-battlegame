// =============================================================================
//  GameManager.cpp - menu, 3 levels, waves, bosses, health, stars, sound, saving
// =============================================================================
#include "GameManager.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace {

constexpr float W  = cfg::kWidth;
constexpr float H  = cfg::kHeight;
constexpr float PI = cfg::kPi;

const char*     kSaveFile = "ppg_progress.txt";   // saved next to the working directory
const sf::Color kText(122, 46, 87);
const sf::Color kGold(255, 205, 80);

// ---------------------------------------------------------------------------
//  Level definitions
// ---------------------------------------------------------------------------
enum EType { Drift = 0, Strafe = 1, Dive = 2 };
struct SpawnDef { int type; float x; };

struct LevelDef {
    Heroine     hero = Heroine::Bubbles;
    const char* title = "";
    EnemyStyle  a, b, boss;
    int         bossHp = 100;
    sf::Color   groundA, groundB, rim, tuft;
    std::vector<std::vector<SpawnDef>> waves;
};

EnemyStyle style(sf::Color body, sf::Color accent, sf::Color bullet, float interval, float speed, int hp)
{
    EnemyStyle s;
    s.body = body; s.accent = accent; s.bullet = bullet;
    s.fireInterval = interval; s.bulletSpeed = speed; s.hp = hp;
    return s;
}

const std::array<LevelDef, 3>& levels()
{
    static const std::array<LevelDef, 3> L = [] {
        std::array<LevelDef, 3> a;
        const sf::Color green(120, 210, 140), pink(255, 130, 150), blue(120, 190, 255);

        LevelDef& l1 = a[0];                       // Bubbles vs Buttercup & Blossom's army
        l1.hero = Heroine::Bubbles;  l1.title = "Meadow Skirmish";
        l1.a = style(green, {255, 255, 200}, {90, 210, 120}, 2.4f, 185.f, 3);
        l1.b = style(pink,  {255, 230, 235}, {240, 75, 95},  2.2f, 190.f, 4);
        l1.boss = style({150, 100, 200}, {255, 200, 255}, {200, 90, 255}, 1.15f, 200.f, 0);
        l1.bossHp = 90;
        l1.groundA = {106, 152, 86}; l1.groundB = {98, 143, 79};
        l1.rim = {60, 90, 48};       l1.tuft = {70, 125, 60};
        l1.waves = {
            {{Drift, 200.f}, {Drift, 400.f}, {Drift, 600.f}},
            {{Drift, 150.f}, {Drift, 650.f}, {Strafe, 300.f}, {Strafe, 500.f}},
            {{Drift, 200.f}, {Drift, 400.f}, {Drift, 600.f}, {Strafe, 250.f}, {Strafe, 550.f}},
        };

        LevelDef& l2 = a[1];
        l2.hero = Heroine::Buttercup; l2.title = "Dusty Desert";
        l2.a = style(blue, {230, 245, 255}, {80, 150, 255}, 2.0f, 205.f, 5);
        l2.b = style(pink, {255, 230, 235}, {240, 75, 95},  1.8f, 215.f, 6);
        l2.boss = style({170, 80, 150}, {255, 190, 230}, {255, 90, 170}, 1.0f, 215.f, 0);
        l2.bossHp = 120;
        l2.groundA = {224, 198, 142}; l2.groundB = {213, 187, 131};
        l2.rim = {150, 120, 80};      l2.tuft = {160, 140, 70};
        l2.waves = {
            {{Drift, 200.f}, {Drift, 600.f}, {Dive, 400.f}},
            {{Strafe, 200.f}, {Strafe, 600.f}, {Dive, 300.f}, {Dive, 500.f}},
            {{Drift, 150.f}, {Drift, 400.f}, {Drift, 650.f}, {Strafe, 300.f}, {Dive, 500.f}},
            {{Strafe, 200.f}, {Strafe, 400.f}, {Strafe, 600.f}, {Dive, 300.f}, {Dive, 500.f}},
        };

        LevelDef& l3 = a[2];
        l3.hero = Heroine::Blossom; l3.title = "Volcano Rim";
        l3.a = style(blue,  {230, 245, 255}, {80, 150, 255}, 1.7f, 235.f, 7);
        l3.b = style(green, {255, 255, 200}, {90, 210, 120}, 1.6f, 245.f, 8);
        l3.boss = style({200, 70, 70}, {255, 210, 150}, {255, 140, 50}, 0.9f, 230.f, 0);
        l3.bossHp = 240;
        l3.groundA = {74, 54, 62}; l3.groundB = {64, 47, 55};
        l3.rim = {120, 60, 50};    l3.tuft = {235, 120, 50};
        l3.waves = {
            {{Dive, 200.f}, {Dive, 600.f}, {Strafe, 400.f}},
            {{Drift, 150.f}, {Drift, 650.f}, {Dive, 300.f}, {Dive, 500.f}, {Strafe, 400.f}},
            {{Strafe, 200.f}, {Strafe, 400.f}, {Strafe, 600.f}, {Dive, 250.f}, {Dive, 550.f}},
            {{Drift, 100.f}, {Drift, 300.f}, {Drift, 500.f}, {Drift, 700.f}, {Dive, 400.f},
             {Strafe, 250.f}, {Strafe, 550.f}},
        };
        return a;
    }();
    return L;
}

// ---------------------------------------------------------------------------
//  Small drawing helpers
// ---------------------------------------------------------------------------
void drawStar(sf::RenderTarget& t, sf::Vector2f c, float r, sf::Color color)
{
    sf::VertexArray va(sf::TriangleFan);
    va.append(sf::Vertex(c, color));
    for (int i = 0; i <= 10; ++i) {
        const float ang = -PI / 2.f + static_cast<float>(i) * PI / 5.f;
        const float rr  = (i % 2 == 0) ? r : r * 0.45f;
        va.append(sf::Vertex({c.x + std::cos(ang) * rr, c.y + std::sin(ang) * rr}, color));
    }
    t.draw(va);
}

void drawHeart(sf::RenderTarget& t, sf::Vector2f c, float s, sf::Color color)
{
    for (float sx : {-1.f, 1.f}) {
        sf::CircleShape k(s * 0.5f, 18);
        k.setOrigin(s * 0.5f, s * 0.5f);
        k.setPosition(c.x + sx * s * 0.5f, c.y - s * 0.2f);
        k.setFillColor(color);
        t.draw(k);
    }
    sf::ConvexShape tri(3);
    tri.setPoint(0, {c.x - s * 0.95f, c.y + s * 0.02f});
    tri.setPoint(1, {c.x + s * 0.95f, c.y + s * 0.02f});
    tri.setPoint(2, {c.x, c.y + s * 1.05f});
    tri.setFillColor(color);
    t.draw(tri);
}

void drawLock(sf::RenderTarget& t, sf::Vector2f c)
{
    sf::CircleShape sh(11.f, 24);
    sh.setOrigin(11.f, 11.f);
    sh.setPosition(c.x, c.y - 10.f);
    sh.setFillColor(sf::Color::Transparent);
    sh.setOutlineThickness(5.f);
    sh.setOutlineColor(sf::Color(210, 210, 220));
    t.draw(sh);
    sf::RectangleShape body({38.f, 28.f});
    body.setOrigin(19.f, 14.f);
    body.setPosition(c.x, c.y + 6.f);
    body.setFillColor(sf::Color(190, 190, 205));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(sf::Color(110, 110, 130));
    t.draw(body);
    sf::CircleShape hole(4.f, 12);
    hole.setOrigin(4.f, 4.f);
    hole.setPosition(c.x, c.y + 4.f);
    hole.setFillColor(sf::Color(70, 70, 90));
    t.draw(hole);
}

void drawBar(sf::RenderTarget& t, sf::Vector2f pos, sf::Vector2f size, float ratio, sf::Color fill)
{
    ratio = std::clamp(ratio, 0.f, 1.f);
    sf::RectangleShape bg(size);
    bg.setPosition(pos);
    bg.setFillColor(sf::Color(40, 20, 40, 190));
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(255, 255, 255, 220));
    t.draw(bg);
    sf::RectangleShape fg({size.x * ratio, size.y});
    fg.setPosition(pos);
    fg.setFillColor(fill);
    t.draw(fg);
}

// ---------------------------------------------------------------------------
//  Procedural sound synthesis (no audio files needed)
// ---------------------------------------------------------------------------
struct Tone { float f0, f1, dur, vol; int wave; float noise; };   // wave: 0 sine 1 square 2 saw

bool synth(sf::SoundBuffer& buf, const std::vector<Tone>& seq)
{
    constexpr unsigned sr = 44100;
    std::vector<sf::Int16> samples;
    float phase = 0.f;
    for (const Tone& t : seq) {
        const std::size_t n = static_cast<std::size_t>(t.dur * sr);
        for (std::size_t i = 0; i < n; ++i) {
            const float k = static_cast<float>(i) / static_cast<float>(n);
            const float f = t.f0 + (t.f1 - t.f0) * k;
            phase += 2.f * PI * f / static_cast<float>(sr);
            const float cyc = std::fmod(phase, 2.f * PI) / (2.f * PI);
            float s = 0.f;
            if (t.wave == 0)      s = std::sin(phase);
            else if (t.wave == 1) s = cyc < 0.5f ? 1.f : -1.f;
            else                  s = 2.f * cyc - 1.f;
            s = s * (1.f - t.noise) + util::randomRange(-1.f, 1.f) * t.noise;
            const float env = std::min(1.f, k * 25.f) * std::pow(1.f - k, 1.5f);
            samples.push_back(static_cast<sf::Int16>(s * env * t.vol * 30000.f));
        }
    }
    if (samples.empty()) return false;
    return buf.loadFromSamples(samples.data(), samples.size(), 1, sr);
}

const char* const kFontPaths[] = {
    "C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/segoeui.ttf",
    "C:/Windows/Fonts/calibri.ttf", "font.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
    "/System/Library/Fonts/Supplemental/Arial Bold.ttf"};

constexpr float kCardW = 220.f, kCardH = 270.f, kCardY = 130.f, kCardGap = 30.f;
sf::FloatRect cardRect(int i) { return {40.f + static_cast<float>(i) * (kCardW + kCardGap), kCardY, kCardW, kCardH}; }
const sf::FloatRect kStartBtn{300.f, 440.f, 200.f, 56.f};

} // namespace

// =============================================================================
//  Construction
// =============================================================================
GameManager::GameManager()
{
    for (const char* p : kFontPaths)
        if (m_font.loadFromFile(p)) { m_fontOk = true; break; }

    for (int i = 0; i < 3; ++i) {
        auto p = std::make_unique<Player>(levels()[static_cast<std::size_t>(i)].hero,
                                          sf::Vector2f(cardRect(i).left + kCardW / 2.f, kCardY + 118.f));
        p->setShowcase(true);
        p->setDisplayScale(1.25f);
        m_showcase.push_back(std::move(p));
    }

    buildSounds();
    loadProgress();
    pickDefaultSelection();
    buildDecor(0);
}

// =============================================================================
//  Sound
// =============================================================================
void GameManager::buildSounds()
{
    auto S = [this](Sfx s) -> sf::SoundBuffer& { return m_buffers[static_cast<std::size_t>(s)]; };
    synth(S(Sfx::ShootBubble), {{500.f, 1100.f, 0.10f, 0.35f, 0, 0.f}});
    synth(S(Sfx::ShootSlash),  {{900.f, 300.f, 0.16f, 0.30f, 2, 0.45f}});
    synth(S(Sfx::ShootRay),    {{1400.f, 900.f, 0.07f, 0.22f, 1, 0.f}});
    synth(S(Sfx::EnemyHit),    {{300.f, 200.f, 0.05f, 0.30f, 1, 0.5f}});
    synth(S(Sfx::EnemyDie),    {{500.f, 90.f, 0.32f, 0.45f, 2, 0.5f}});
    synth(S(Sfx::PlayerHurt),  {{220.f, 90.f, 0.30f, 0.50f, 1, 0.25f}});
    synth(S(Sfx::Pickup),      {{660.f, 660.f, 0.07f, 0.35f, 0, 0.f}, {990.f, 990.f, 0.12f, 0.35f, 0, 0.f}});
    synth(S(Sfx::Victory),     {{523.f, 523.f, 0.14f, 0.4f, 1, 0.f}, {659.f, 659.f, 0.14f, 0.4f, 1, 0.f},
                                {784.f, 784.f, 0.14f, 0.4f, 1, 0.f}, {1047.f, 1047.f, 0.45f, 0.4f, 1, 0.f}});
    synth(S(Sfx::Defeat),      {{440.f, 440.f, 0.22f, 0.4f, 2, 0.f}, {370.f, 370.f, 0.22f, 0.4f, 2, 0.f},
                                {294.f, 294.f, 0.22f, 0.4f, 2, 0.f}, {196.f, 150.f, 0.55f, 0.4f, 2, 0.f}});
    synth(S(Sfx::Click),       {{800.f, 800.f, 0.05f, 0.3f, 0, 0.f}});
    synth(S(Sfx::BossAlert),   {{110.f, 80.f, 0.6f, 0.55f, 2, 0.15f}});
}

void GameManager::playSfx(Sfx s, float volume, float pitch)
{
    sf::Sound& v = m_voices[m_nextVoice];
    m_nextVoice = (m_nextVoice + 1) % m_voices.size();
    v.setBuffer(m_buffers[static_cast<std::size_t>(s)]);
    v.setVolume(volume);
    v.setPitch(pitch);
    v.play();
}

// =============================================================================
//  Saving / selection
// =============================================================================
void GameManager::loadProgress()
{
    std::ifstream f(kSaveFile);
    for (int& s : m_stars) {
        int v = 0;
        if (f >> v) s = std::clamp(v, 0, 3);
    }
}

void GameManager::saveProgress() const
{
    std::ofstream f(kSaveFile);
    for (int s : m_stars) f << s << '\n';
}

void GameManager::pickDefaultSelection()
{
    m_selected = 0;
    for (int i = 0; i < 3; ++i)
        if (unlocked(i) && m_stars[static_cast<std::size_t>(i)] == 0) { m_selected = i; return; }
    m_selected = 2;   // everything done: highlight the last level
}

void GameManager::moveSelection(int step)
{
    int i = m_selected + step;
    while (i >= 0 && i < 3) {
        if (unlocked(i)) { m_selected = i; playSfx(Sfx::Click, 60.f); return; }
        i += step;
    }
}

// =============================================================================
//  Flow
// =============================================================================
bool GameManager::key(sf::Keyboard::Key k) const
{
    const int i = static_cast<int>(k);
    return i >= 0 && i < static_cast<int>(m_keys.size()) && m_keys[static_cast<std::size_t>(i)];
}

void GameManager::startLevel(int index)
{
    m_levelIdx = index;
    const LevelDef& d = levels()[static_cast<std::size_t>(index)];
    m_player = std::make_unique<Player>(d.hero);
    m_enemies.clear(); m_shots.clear(); m_pickups.clear(); m_particles.clear(); m_queue.clear();
    m_wave = 0; m_score = 0; m_bossSpawned = false;
    m_waveTimer = 1.8f; m_endTimer = 0.f; m_regenTimer = 0.f; m_levelTime = 0.f;
    m_outcome = Outcome::None;
    m_keys.fill(false);
    buildDecor(index);
    m_state = State::Playing;
    showBanner("LEVEL " + std::to_string(index + 1) + " - " + traitsFor(d.hero).name, 2.4f);
    playSfx(Sfx::Click);
}

void GameManager::toMenu()
{
    m_state = State::Menu;
    m_keys.fill(false);
    m_resetConfirm = 0.f;
    buildDecor(0);
    pickDefaultSelection();
}

void GameManager::setButtons(const std::vector<std::pair<std::string, Action>>& list)
{
    m_buttons.clear();
    const float w = 190.f, h = 54.f, gap = 20.f;
    const float total = static_cast<float>(list.size()) * w + static_cast<float>(list.size() - 1) * gap;
    float x = (W - total) / 2.f;
    for (const auto& item : list) {
        m_buttons.push_back({{x, 430.f, w, h}, item.first, item.second});
        x += w + gap;
    }
}

void GameManager::doAction(Action a)
{
    playSfx(Sfx::Click);
    switch (a) {
    case Action::Start:
    case Action::Replay:
    case Action::Retry:  startLevel(a == Action::Start ? m_selected : m_levelIdx); break;
    case Action::Next:   startLevel(std::min(2, m_levelIdx + 1)); break;
    case Action::Resume: m_state = State::Playing; break;
    case Action::Menu:   toMenu(); break;
    }
}

void GameManager::finishLevel()
{
    if (m_outcome == Outcome::Won) {
        const int hp = m_player->hp();
        m_runStars = hp >= 4 ? 3 : (hp >= 2 ? 2 : 1);
        m_score += hp * 200;
        const std::size_t li = static_cast<std::size_t>(m_levelIdx);
        m_newBest = m_runStars > m_stars[li];
        if (m_newBest) { m_stars[li] = m_runStars; saveProgress(); }
        m_state = State::Victory;
        std::vector<std::pair<std::string, Action>> b;
        if (m_levelIdx < 2) b.push_back({"NEXT LEVEL", Action::Next});
        b.push_back({"REPLAY", Action::Replay});
        b.push_back({"MENU", Action::Menu});
        setButtons(b);
    } else {
        m_state = State::GameOver;
        setButtons({{"TRY AGAIN", Action::Retry}, {"MENU", Action::Menu}});
    }
    m_keys.fill(false);
}

void GameManager::showBanner(const std::string& text, float seconds)
{
    m_bannerText = text;
    m_bannerTime = m_bannerDur = seconds;
}

// =============================================================================
//  Input
// =============================================================================
void GameManager::handleKeyPressed(sf::Keyboard::Key k)
{
    const int ki = static_cast<int>(k);
    if (ki < 0 || ki >= static_cast<int>(m_keys.size())) return;
    m_keys[static_cast<std::size_t>(ki)] = true;

    using K = sf::Keyboard;
    switch (m_state) {
    case State::Menu:
        if (k == K::Left || k == K::A)       moveSelection(-1);
        else if (k == K::Right || k == K::D) moveSelection(+1);
        else if ((k == K::Return || k == K::Space) && unlocked(m_selected)) doAction(Action::Start);
        else if (k == K::R) {
            if (m_resetConfirm > 0.f) {
                m_stars.fill(0); saveProgress(); m_resetConfirm = 0.f; pickDefaultSelection();
                playSfx(Sfx::Defeat, 60.f);
            } else {
                m_resetConfirm = 3.f;
            }
        }
        break;
    case State::Playing:
        if (k == K::Escape || k == K::P) {
            m_state = State::Paused;
            setButtons({{"RESUME", Action::Resume}, {"MENU", Action::Menu}});
        }
        break;
    case State::Paused:
        if (k == K::Escape || k == K::P || k == K::Return) doAction(Action::Resume);
        else if (k == K::M) doAction(Action::Menu);
        break;
    case State::Victory:
    case State::GameOver:
        if (k == K::Return && !m_buttons.empty()) doAction(m_buttons.front().action);
        else if (k == K::Escape || k == K::M) doAction(Action::Menu);
        break;
    }
}

void GameManager::handleKeyReleased(sf::Keyboard::Key k)
{
    const int ki = static_cast<int>(k);
    if (ki >= 0 && ki < static_cast<int>(m_keys.size())) m_keys[static_cast<std::size_t>(ki)] = false;
}

sf::Vector2f GameManager::toGameCoords(sf::Vector2f p) const
{
    const float s = std::min(static_cast<float>(m_winSize.x) / W, static_cast<float>(m_winSize.y) / H);
    const float ox = (static_cast<float>(m_winSize.x) - W * s) / 2.f;
    const float oy = (static_cast<float>(m_winSize.y) - H * s) / 2.f;
    return {(p.x - ox) / s, (p.y - oy) / s};
}

void GameManager::handleMousePressed(sf::Vector2f windowPixel)
{
    const sf::Vector2f p = toGameCoords(windowPixel);
    if (m_state == State::Menu) {
        for (int i = 0; i < 3; ++i) {
            if (!cardRect(i).contains(p)) continue;
            if (!unlocked(i)) { playSfx(Sfx::Defeat, 40.f, 2.f); return; }
            if (m_selected == i) doAction(Action::Start);
            else { m_selected = i; playSfx(Sfx::Click, 60.f); }
            return;
        }
        if (kStartBtn.contains(p) && unlocked(m_selected)) doAction(Action::Start);
    } else if (m_state != State::Playing) {
        for (const Button& b : m_buttons)
            if (b.rect.contains(p)) { doAction(b.action); return; }
    }
}

// =============================================================================
//  Simulation
// =============================================================================
void GameManager::update(float dt, sf::Vector2u windowSize)
{
    m_winSize = windowSize;
    dt = std::min(dt, 0.05f);
    m_time += dt;
    if (m_resetConfirm > 0.f) m_resetConfirm = std::max(0.f, m_resetConfirm - dt);
    if (m_bannerTime > 0.f)   m_bannerTime -= dt;

    if (m_state == State::Menu) {
        m_scroll += 25.f * dt;
        for (auto& p : m_showcase) p->update(dt);
    } else if (m_state == State::Playing) {
        updatePlaying(dt);
    }
}

void GameManager::burst(sf::Vector2f p, sf::Color c, int count, float speed, float size)
{
    for (int i = 0; i < count; ++i) {
        const float a = util::randomRange(0.f, 2.f * PI);
        const float v = util::randomRange(speed * 0.3f, speed);
        const float life = util::randomRange(0.35f, 0.8f);
        m_particles.push_back({p, {std::cos(a) * v, std::sin(a) * v}, life, life,
                               util::randomRange(size * 0.6f, size), c});
    }
}

void GameManager::hurtPlayer()
{
    playSfx(Sfx::PlayerHurt);
    burst(m_player->position(), sf::Color(255, 120, 150), 14, 200.f, 5.f);
    m_regenTimer = 0.f;
}

void GameManager::spawn(int type, float x)
{
    const LevelDef& d = levels()[static_cast<std::size_t>(m_levelIdx)];
    if (type == Drift)       m_enemies.push_back(std::make_unique<DrifterEnemy>(x, d.a));
    else if (type == Strafe) m_enemies.push_back(std::make_unique<StrafeEnemy>(x, d.b));
    else                     m_enemies.push_back(std::make_unique<DiveEnemy>(x, d.b));
}

void GameManager::nextWave()
{
    const LevelDef& d = levels()[static_cast<std::size_t>(m_levelIdx)];
    if (m_wave < static_cast<int>(d.waves.size())) {
        float delay = 0.f;
        for (const SpawnDef& s : d.waves[static_cast<std::size_t>(m_wave)]) {
            m_queue.push_back({delay, s.type, s.x});
            delay += 0.45f;
        }
        ++m_wave;
        showBanner("WAVE " + std::to_string(m_wave) + " / " + std::to_string(d.waves.size()), 1.6f);
    } else {
        m_enemies.push_back(std::make_unique<BossEnemy>(d.boss, d.bossHp));
        m_bossSpawned = true;
        showBanner("BOSS BATTLE!", 2.4f);
        playSfx(Sfx::BossAlert);
    }
}

void GameManager::updatePlaying(float dt)
{
    m_scroll += 55.f * dt;
    m_levelTime += dt;

    // ---- player --------------------------------------------------------------
    sf::Vector2f dir(0.f, 0.f);
    using K = sf::Keyboard;
    if (key(K::Left)  || key(K::A)) dir.x -= 1.f;
    if (key(K::Right) || key(K::D)) dir.x += 1.f;
    if (key(K::Up)    || key(K::W)) dir.y -= 1.f;
    if (key(K::Down)  || key(K::S)) dir.y += 1.f;
    m_player->setMoveInput(dir, key(K::LShift) || key(K::RShift));
    m_player->update(dt);

    if (m_player->isAlive() && m_player->tryShoot(key(K::Space), m_shots)) {
        switch (m_player->heroine()) {
        case Heroine::Bubbles:   playSfx(Sfx::ShootBubble, 70.f, util::randomRange(0.9f, 1.15f)); break;
        case Heroine::Buttercup: playSfx(Sfx::ShootSlash, 80.f); break;
        default:                 playSfx(Sfx::ShootRay, 45.f, util::randomRange(0.95f, 1.1f)); break;
        }
    }

    // slow health regeneration when not being hit
    if (m_player->isAlive() && m_player->hp() < m_player->maxHp()) {
        m_regenTimer += dt;
        if (m_regenTimer >= 9.f) {
            m_regenTimer = 0.f;
            m_player->heal(1);
            playSfx(Sfx::Pickup, 50.f);
            burst(m_player->position(), sf::Color(255, 150, 190), 10, 90.f, 4.f);
        }
    }

    // ---- waves -----------------------------------------------------------------
    for (Pending& p : m_queue) p.delay -= dt;
    for (std::size_t i = 0; i < m_queue.size();) {
        if (m_queue[i].delay <= 0.f) { spawn(m_queue[i].type, m_queue[i].x); m_queue.erase(m_queue.begin() + static_cast<long>(i)); }
        else ++i;
    }
    if (m_outcome == Outcome::None && !m_bossSpawned) {
        if (!m_queue.empty() || !m_enemies.empty()) m_waveTimer = 1.5f;
        else {
            m_waveTimer -= dt;
            if (m_waveTimer <= 0.f) { nextWave(); m_waveTimer = 1.5f; }
        }
    }

    // ---- enemies ---------------------------------------------------------------
    const sf::Vector2f target = m_player->position();
    for (auto& e : m_enemies) {
        e->setTarget(target);
        e->update(dt);
        if (m_player->isAlive()) e->tryShoot(target, m_shots);
    }

    // ---- projectiles -------------------------------------------------------------
    for (Projectile& s : m_shots) s.update(dt);

    for (Projectile& s : m_shots) {
        if (!s.alive) continue;
        if (s.friendly) {
            for (auto& e : m_enemies) {
                if (!e->isAlive()) continue;
                if (std::find(s.hitList.begin(), s.hitList.end(), static_cast<const void*>(e.get())) != s.hitList.end())
                    continue;
                const float reach = s.radius * (s.shape == ProjectileShape::Slash ? 1.3f : 1.f) + e->radius() * 0.9f;
                const sf::Vector2f d = e->position() - s.pos;
                if (d.x * d.x + d.y * d.y < reach * reach) {
                    e->takeDamage(s.damage);
                    s.hitList.push_back(e.get());
                    burst(s.pos, util::withAlpha(s.color, 255), 4, 120.f, 3.f);
                    playSfx(Sfx::EnemyHit, 45.f, util::randomRange(0.9f, 1.2f));
                    if (s.pierce-- <= 0) { s.alive = false; break; }
                }
            }
        } else if (m_player->isAlive()) {
            const float reach = s.radius + m_player->radius();
            const sf::Vector2f d = m_player->position() - s.pos;
            if (d.x * d.x + d.y * d.y < reach * reach && m_player->takeDamage(s.damage)) {
                s.alive = false;
                hurtPlayer();
            }
        }
    }

    // enemies touching the player
    if (m_player->isAlive()) {
        for (auto& e : m_enemies) {
            const float reach = e->radius() * 0.75f + m_player->radius();
            const sf::Vector2f d = m_player->position() - e->position();
            if (d.x * d.x + d.y * d.y < reach * reach && m_player->takeDamage(1)) hurtPlayer();
        }
    }

    // defeated enemies
    for (std::size_t i = 0; i < m_enemies.size();) {
        Enemy& e = *m_enemies[i];
        if (e.isAlive()) { ++i; continue; }
        m_score += e.scoreValue();
        burst(e.position(), sf::Color(255, 230, 120), e.isBoss() ? 60 : 18, e.isBoss() ? 320.f : 220.f, 6.f);
        playSfx(Sfx::EnemyDie, e.isBoss() ? 100.f : 75.f, e.isBoss() ? 0.6f : util::randomRange(0.9f, 1.15f));
        const int drops = e.isBoss() ? 2 : (util::randomRange(0.f, 1.f) < 0.22f ? 1 : 0);
        for (int k = 0; k < drops; ++k)
            m_pickups.push_back({{e.position().x + static_cast<float>(k) * 30.f - 15.f, e.position().y}, 0.f, true});
        m_enemies.erase(m_enemies.begin() + static_cast<long>(i));
    }

    // heart pickups
    for (Pickup& p : m_pickups) {
        p.age += dt;
        p.pos.y += 70.f * dt;
        if (p.pos.y > H + 30.f || p.age > 14.f) p.alive = false;
        if (p.alive && m_player->isAlive()) {
            const sf::Vector2f d = m_player->position() - p.pos;
            if (d.x * d.x + d.y * d.y < 30.f * 30.f) {
                p.alive = false;
                m_player->heal(1);
                m_score += 50;
                playSfx(Sfx::Pickup);
                burst(p.pos, sf::Color(255, 150, 190), 10, 110.f, 4.f);
            }
        }
    }

    for (Particle& p : m_particles) { p.pos += p.vel * dt; p.vel *= 0.96f; p.life -= dt; }

    m_shots.erase(std::remove_if(m_shots.begin(), m_shots.end(), [](const Projectile& s) { return !s.alive; }), m_shots.end());
    m_pickups.erase(std::remove_if(m_pickups.begin(), m_pickups.end(), [](const Pickup& p) { return !p.alive; }), m_pickups.end());
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) { return p.life <= 0.f; }), m_particles.end());

    // ---- win / lose ----------------------------------------------------------------
    if (m_outcome == Outcome::None) {
        if (!m_player->isAlive()) {
            m_outcome = Outcome::Lost;
            m_endTimer = 1.3f;
            burst(m_player->position(), sf::Color(255, 150, 190), 40, 260.f, 6.f);
            playSfx(Sfx::Defeat);
        } else if (m_bossSpawned && m_enemies.empty()) {
            m_outcome = Outcome::Won;
            m_endTimer = 1.7f;
            for (Projectile& s : m_shots)
                if (!s.friendly) { burst(s.pos, s.color, 3, 80.f, 3.f); s.alive = false; }
            playSfx(Sfx::Victory);
        }
    } else {
        m_endTimer -= dt;
        if (m_endTimer <= 0.f) finishLevel();
    }
}

// =============================================================================
//  Battlefield look
// =============================================================================
void GameManager::buildDecor(int level)
{
    (void)level;
    m_decor.clear();
    auto rnd = [](float a, float b) { return util::randomRange(a, b); };
    // kinds: 0 crater, 1 rock, 2 sandbags, 3 tank trap, 4 flag, 5 tuft
    for (int i = 0; i < 9; ++i)  m_decor.push_back({0, {rnd(40, W - 40), rnd(0, H + 160)}, rnd(26, 52), 0.f, 0});
    for (int i = 0; i < 10; ++i) m_decor.push_back({1, {rnd(20, W - 20), rnd(0, H + 160)}, rnd(8, 18), rnd(0, 360), 0});
    for (int i = 0; i < 6; ++i)  m_decor.push_back({2, {(i % 2 ? rnd(W - 110, W - 30) : rnd(30, 110)), rnd(0, H + 160)}, 1.f, 0.f, 0});
    for (int i = 0; i < 5; ++i)  m_decor.push_back({3, {(i % 2 ? rnd(W - 140, W - 40) : rnd(40, 140)), rnd(0, H + 160)}, rnd(16, 22), rnd(0, 40), 0});
    for (int i = 0; i < 4; ++i)  m_decor.push_back({4, {(i % 2 ? rnd(W - 60, W - 20) : rnd(20, 60)), rnd(0, H + 160)}, 1.f, 0.f, i % 3});
    for (int i = 0; i < 26; ++i) m_decor.push_back({5, {rnd(10, W - 10), rnd(0, H + 160)}, rnd(7, 12), rnd(-20, 20), 0});
}

void GameManager::drawBackground(sf::RenderTarget& t, int level)
{
    const LevelDef& d = levels()[static_cast<std::size_t>(level)];
    constexpr float ts = 50.f;
    const float off = std::fmod(m_scroll, ts * 2.f);
    sf::VertexArray va(sf::Quads);
    for (int j = -2; j < 14; ++j) {
        for (int i = 0; i < 16; ++i) {
            const float x = static_cast<float>(i) * ts, y = static_cast<float>(j) * ts + off;
            const sf::Color c = ((i + j) & 1) ? d.groundB : d.groundA;
            va.append(sf::Vertex({x, y}, c));
            va.append(sf::Vertex({x + ts, y}, c));
            va.append(sf::Vertex({x + ts, y + ts}, c));
            va.append(sf::Vertex({x, y + ts}, c));
        }
    }
    t.draw(va);

    // darker trench strips at the edges for a "front line" feeling
    for (float x : {0.f, W - 38.f}) {
        sf::RectangleShape r({38.f, H});
        r.setPosition(x, 0.f);
        r.setFillColor(util::withAlpha(d.rim, 90));
        t.draw(r);
    }
}

void GameManager::drawDecor(sf::RenderTarget& t, int level)
{
    const LevelDef& d = levels()[static_cast<std::size_t>(level)];
    const sf::Color dark = util::mix(d.groundA, sf::Color::Black, 0.35f);
    const sf::Color flagCols[3] = {{255, 130, 150}, {120, 190, 255}, {120, 215, 145}};
    const float span = H + 160.f;

    for (const Decor& o : m_decor) {
        const float y = std::fmod(o.pos.y + m_scroll, span) - 80.f;
        const sf::Vector2f p(o.pos.x, y);
        switch (o.kind) {
        case 0: {   // crater
            sf::CircleShape rim(o.size, 28);
            rim.setOrigin(o.size, o.size); rim.setPosition(p); rim.setScale(1.f, 0.5f);
            rim.setFillColor(util::mix(d.groundA, sf::Color::Black, 0.15f));
            rim.setOutlineThickness(3.f); rim.setOutlineColor(util::mix(d.groundA, sf::Color::White, 0.25f));
            t.draw(rim);
            sf::CircleShape in(o.size * 0.7f, 28);
            in.setOrigin(o.size * 0.7f, o.size * 0.7f); in.setPosition(p.x, p.y + 2.f); in.setScale(1.f, 0.5f);
            in.setFillColor(dark);
            t.draw(in);
            break;
        }
        case 1: {   // rock
            sf::CircleShape r(o.size, 6);
            r.setOrigin(o.size, o.size); r.setPosition(p); r.setRotation(o.rot); r.setScale(1.2f, 0.85f);
            r.setFillColor(sf::Color(128, 128, 138)); r.setOutlineThickness(2.f); r.setOutlineColor(sf::Color(80, 80, 92));
            t.draw(r);
            sf::CircleShape h(o.size * 0.4f, 6);
            h.setOrigin(o.size * 0.4f, o.size * 0.4f); h.setPosition(p.x - o.size * 0.25f, p.y - o.size * 0.25f);
            h.setFillColor(sf::Color(170, 170, 180));
            t.draw(h);
            break;
        }
        case 2: {   // sandbags
            for (int row = 0; row < 2; ++row) {
                for (int k = 0; k < 3 - row; ++k) {
                    sf::RectangleShape bag({34.f, 15.f});
                    bag.setOrigin(17.f, 7.5f);
                    bag.setPosition(p.x + static_cast<float>(k) * 33.f - 33.f + static_cast<float>(row) * 16.f, p.y - static_cast<float>(row) * 13.f);
                    bag.setFillColor(sf::Color(196, 168, 112));
                    bag.setOutlineThickness(2.f); bag.setOutlineColor(sf::Color(130, 105, 66));
                    t.draw(bag);
                }
            }
            break;
        }
        case 3: {   // anti-tank "hedgehog"
            for (float a : {0.f, 60.f, 120.f}) {
                sf::RectangleShape bar({o.size * 2.f, 5.f});
                bar.setOrigin(o.size, 2.5f); bar.setPosition(p); bar.setRotation(a + o.rot);
                bar.setFillColor(sf::Color(90, 95, 110)); bar.setOutlineThickness(1.5f); bar.setOutlineColor(sf::Color(40, 42, 55));
                t.draw(bar);
            }
            break;
        }
        case 4: {   // flag
            sf::RectangleShape pole({3.f, 46.f});
            pole.setOrigin(1.5f, 46.f); pole.setPosition(p); pole.setFillColor(sf::Color(90, 70, 60));
            t.draw(pole);
            const float wave = std::sin(m_time * 3.f + o.pos.x) * 3.f;
            sf::ConvexShape fl(3);
            fl.setPoint(0, {p.x + 1.f, p.y - 46.f}); fl.setPoint(1, {p.x + 26.f, p.y - 38.f + wave}); fl.setPoint(2, {p.x + 1.f, p.y - 28.f});
            fl.setFillColor(flagCols[o.variant]);
            t.draw(fl);
            break;
        }
        default: {  // grass tuft / embers
            for (float a : {-25.f, 0.f, 25.f}) {
                sf::RectangleShape blade({2.5f, o.size});
                blade.setOrigin(1.25f, o.size); blade.setPosition(p); blade.setRotation(a + o.rot);
                blade.setFillColor(d.tuft);
                t.draw(blade);
            }
        }
        }
    }
}

// =============================================================================
//  Text / buttons
// =============================================================================
void GameManager::drawText(sf::RenderTarget& t, const std::string& s, sf::Vector2f pos, unsigned size,
                           sf::Color color, bool center) const
{
    if (!m_fontOk) return;
    sf::Text tx(s, m_font, size);
    tx.setFillColor(color);
    tx.setOutlineColor(sf::Color(60, 25, 55, 230));
    tx.setOutlineThickness(std::max(1.f, static_cast<float>(size) / 14.f));
    const sf::FloatRect b = tx.getLocalBounds();
    tx.setOrigin(center ? b.left + b.width / 2.f : b.left, b.top + b.height / 2.f);
    tx.setPosition(pos);
    t.draw(tx);
}

void GameManager::drawButtons(sf::RenderTarget& t) const
{
    bool first = true;
    for (const Button& b : m_buttons) {
        sf::RectangleShape r({b.rect.width, b.rect.height});
        r.setPosition(b.rect.left, b.rect.top);
        r.setFillColor(first ? sf::Color(255, 190, 220) : sf::Color(255, 226, 238));
        r.setOutlineThickness(3.f);
        r.setOutlineColor(sf::Color(255, 140, 190));
        t.draw(r);
        drawText(t, b.label, {b.rect.left + b.rect.width / 2.f, b.rect.top + b.rect.height / 2.f}, 22, sf::Color::White);
        first = false;
    }
}

// =============================================================================
//  Drawing
// =============================================================================
void GameManager::render(sf::RenderWindow& window)
{
    const float s = std::min(static_cast<float>(m_winSize.x) / W, static_cast<float>(m_winSize.y) / H);
    const float vw = W * s / static_cast<float>(m_winSize.x);
    const float vh = H * s / static_cast<float>(m_winSize.y);
    sf::View view(sf::FloatRect(0.f, 0.f, W, H));
    view.setViewport(sf::FloatRect((1.f - vw) / 2.f, (1.f - vh) / 2.f, vw, vh));
    window.setView(view);

    if (m_state == State::Menu) {
        drawMenu(window);
        return;
    }
    drawWorld(window);
    drawHud(window);
    if (m_state != State::Playing) drawOverlay(window);
}

void GameManager::drawWorld(sf::RenderTarget& t)
{
    drawBackground(t, m_levelIdx);
    drawDecor(t, m_levelIdx);

    for (const Pickup& p : m_pickups) {
        const float bob = std::sin(p.age * 5.f) * 3.f;
        sf::CircleShape glow(20.f, 20);
        glow.setOrigin(20.f, 20.f); glow.setPosition(p.pos.x, p.pos.y + bob);
        glow.setFillColor(sf::Color(255, 255, 255, 90));
        t.draw(glow);
        drawHeart(t, {p.pos.x, p.pos.y + bob - 4.f}, 12.f, sf::Color(255, 90, 140));
    }
    for (const auto& e : m_enemies) e->draw(t);
    if (m_player && m_player->isAlive()) {
        m_player->draw(t);
        // small health bar under the heroine
        const float r = static_cast<float>(m_player->hp()) / static_cast<float>(m_player->maxHp());
        drawBar(t, {m_player->position().x - 22.f, m_player->position().y + 48.f}, {44.f, 6.f}, r,
                util::mix(sf::Color(255, 80, 80), sf::Color(90, 220, 120), r));
    }
    for (const Projectile& s : m_shots) s.draw(t);
    for (const Particle& p : m_particles) {
        const float k = p.life / p.maxLife;
        sf::CircleShape c(p.size * k + 0.5f, 10);
        c.setOrigin(c.getRadius(), c.getRadius());
        c.setPosition(p.pos);
        c.setFillColor(util::withAlpha(p.color, static_cast<sf::Uint8>(255.f * k)));
        t.draw(c);
    }
}

void GameManager::drawHud(sf::RenderTarget& t)
{
    const LevelDef& d = levels()[static_cast<std::size_t>(m_levelIdx)];
    const float r = static_cast<float>(m_player->hp()) / static_cast<float>(m_player->maxHp());

    // top-left: health
    drawHeart(t, {24.f, 20.f}, 11.f, sf::Color(255, 90, 140));
    drawBar(t, {44.f, 10.f}, {200.f, 20.f}, r, util::mix(sf::Color(255, 80, 80), sf::Color(90, 220, 120), r));
    for (int i = 1; i < m_player->maxHp(); ++i) {                       // one tick per heart
        sf::RectangleShape tick({2.f, 20.f});
        tick.setPosition(44.f + 200.f * static_cast<float>(i) / static_cast<float>(m_player->maxHp()) - 1.f, 10.f);
        tick.setFillColor(sf::Color(40, 20, 40, 200));
        t.draw(tick);
    }
    drawText(t, std::to_string(m_player->hp()) + " / " + std::to_string(m_player->maxHp()), {144.f, 20.f}, 14, sf::Color::White);
    drawText(t, traitsFor(d.hero).name + " - " + m_player->power().name(), {44.f, 46.f}, 15, sf::Color::White, false);

    // top-right: score
    drawText(t, "SCORE " + std::to_string(m_score), {W - 100.f, 20.f}, 20, kGold);
    // top-center: level / wave
    drawText(t, "LEVEL " + std::to_string(m_levelIdx + 1) + "  " + d.title, {W / 2.f, 18.f}, 16, sf::Color::White);

    // boss health
    for (const auto& e : m_enemies) {
        if (!e->isBoss()) continue;
        const float br = static_cast<float>(e->hp()) / static_cast<float>(e->maxHp());
        drawBar(t, {W / 2.f - 200.f, 52.f}, {400.f, 16.f}, br, sf::Color(200, 90, 255));
        drawText(t, "BOSS", {W / 2.f, 60.f}, 12, sf::Color::White);
    }

    // banner
    if (m_bannerTime > 0.f) {
        const float k = std::min(1.f, std::min(m_bannerTime, m_bannerDur - m_bannerTime) * 4.f);
        drawText(t, m_bannerText, {W / 2.f, 240.f}, 40, util::withAlpha(sf::Color::White, static_cast<sf::Uint8>(255.f * std::max(0.f, k))));
    }
    if (m_levelTime < 7.f && m_state == State::Playing)
        drawText(t, "Arrows / WASD: move     Space: attack     Shift: slow & precise     Esc: pause",
                 {W / 2.f, H - 16.f}, 14, sf::Color(255, 255, 255, 230));
}

void GameManager::drawOverlay(sf::RenderTarget& t)
{
    sf::RectangleShape shade({W, H});
    shade.setFillColor(sf::Color(30, 10, 30, 165));
    t.draw(shade);

    if (m_state == State::Paused) {
        drawText(t, "PAUSED", {W / 2.f, 250.f}, 54, sf::Color::White);
        drawText(t, "Esc / Enter: resume     M: menu", {W / 2.f, 330.f}, 18, sf::Color::White);
    } else if (m_state == State::Victory) {
        drawText(t, "VICTORY!", {W / 2.f, 120.f}, 60, kGold);
        for (int i = 0; i < 3; ++i)
            drawStar(t, {W / 2.f + (static_cast<float>(i) - 1.f) * 90.f, 230.f}, 38.f,
                     i < m_runStars ? kGold : sf::Color(120, 100, 120));
        drawText(t, "Score: " + std::to_string(m_score), {W / 2.f, 320.f}, 24, sf::Color::White);
        drawText(t, m_newBest ? "New best result saved!" : "Your best is kept - replay to beat it",
                 {W / 2.f, 360.f}, 18, sf::Color(255, 220, 240));
        drawText(t, "3 stars: lose at most 1 heart   2 stars: keep 2+ hearts", {W / 2.f, 395.f}, 14, sf::Color(230, 210, 230));
        drawButtons(t);
    } else if (m_state == State::GameOver) {
        drawText(t, "DEFEATED", {W / 2.f, 170.f}, 60, sf::Color(255, 110, 130));
        drawText(t, "The level was not completed. Try again!", {W / 2.f, 270.f}, 22, sf::Color::White);
        drawText(t, "Score: " + std::to_string(m_score), {W / 2.f, 320.f}, 22, sf::Color(255, 220, 240));
        drawButtons(t);
    }
    if (m_state == State::Paused) drawButtons(t);
}

void GameManager::drawMenu(sf::RenderTarget& t)
{
    drawBackground(t, 0);
    drawDecor(t, 0);
    sf::RectangleShape shade({W, H});
    shade.setFillColor(sf::Color(255, 235, 245, 120));
    t.draw(shade);

    drawText(t, "POWER PUFF GIRLS", {W / 2.f, 55.f}, 48, sf::Color(255, 120, 175));
    drawText(t, "BATTLEFIELD", {W / 2.f, 100.f}, 20, kText);

    for (int i = 0; i < 3; ++i) {
        const sf::FloatRect r = cardRect(i);
        const bool open = unlocked(i), sel = (i == m_selected);
        const int stars = m_stars[static_cast<std::size_t>(i)];
        const LevelDef& d = levels()[static_cast<std::size_t>(i)];

        sf::RectangleShape card({r.width, r.height});
        card.setPosition(r.left, r.top);
        card.setFillColor(sf::Color(255, 244, 249, 235));
        card.setOutlineThickness(sel ? 5.f : 3.f);
        card.setOutlineColor(sel ? kGold : sf::Color(255, 170, 205));
        t.draw(card);

        drawText(t, "LEVEL " + std::to_string(i + 1), {r.left + r.width / 2.f, r.top + 22.f}, 18, kText);
        m_showcase[static_cast<std::size_t>(i)]->draw(t);
        drawText(t, traitsFor(d.hero).name, {r.left + r.width / 2.f, r.top + 190.f}, 22, kText);
        drawText(t, m_showcase[static_cast<std::size_t>(i)]->power().name(), {r.left + r.width / 2.f, r.top + 214.f}, 14, kText);
        for (int s = 0; s < 3; ++s)
            drawStar(t, {r.left + r.width / 2.f + (static_cast<float>(s) - 1.f) * 40.f, r.top + 242.f}, 14.f,
                     s < stars ? kGold : sf::Color(200, 185, 200));
        if (!open) {
            sf::RectangleShape lockShade({r.width, r.height});
            lockShade.setPosition(r.left, r.top);
            lockShade.setFillColor(sf::Color(40, 25, 45, 190));
            t.draw(lockShade);
            drawLock(t, {r.left + r.width / 2.f, r.top + r.height / 2.f - 10.f});
            drawText(t, "LOCKED", {r.left + r.width / 2.f, r.top + r.height / 2.f + 40.f}, 20, sf::Color::White);
            drawText(t, "Beat level " + std::to_string(i) + " first", {r.left + r.width / 2.f, r.top + r.height / 2.f + 66.f}, 13, sf::Color(230, 210, 230));
        } else {
            drawText(t, stars > 0 ? "COMPLETED" : "NOT PLAYED YET",
                     {r.left + r.width / 2.f, r.top + 266.f - 6.f}, 12, stars > 0 ? sf::Color(60, 140, 80) : kText);
        }
    }

    const bool canStart = unlocked(m_selected);
    sf::RectangleShape btn({kStartBtn.width, kStartBtn.height});
    btn.setPosition(kStartBtn.left, kStartBtn.top);
    btn.setFillColor(canStart ? sf::Color(255, 150, 200) : sf::Color(200, 190, 200));
    btn.setOutlineThickness(4.f);
    btn.setOutlineColor(sf::Color(255, 255, 255));
    t.draw(btn);
    drawText(t, "START", {kStartBtn.left + kStartBtn.width / 2.f, kStartBtn.top + kStartBtn.height / 2.f}, 30, sf::Color::White);

    drawText(t, "Click START or press ENTER      Left / Right: choose level", {W / 2.f, 525.f}, 16, kText);
    drawText(t, m_resetConfirm > 0.f ? "Press R again to ERASE all progress" : "R: reset saved progress",
             {W / 2.f, 555.f}, 13, m_resetConfirm > 0.f ? sf::Color(220, 60, 90) : sf::Color(150, 90, 125));
}
