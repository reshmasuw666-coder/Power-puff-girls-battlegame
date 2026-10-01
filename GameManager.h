// =============================================================================
//  GameManager.h - menu, levels, waves, collisions, HUD, sound and saving.
//  Uses the entities in Character.h (Player / Enemy / Projectile).
// =============================================================================
#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "Character.h"

class GameManager {
public:
    GameManager();

    void update(float dt, sf::Vector2u windowSize);
    void render(sf::RenderWindow& window);

    void handleKeyPressed(sf::Keyboard::Key key);
    void handleKeyReleased(sf::Keyboard::Key key);
    // position in SFML-window pixels (GameWindow converts it from Qt)
    void handleMousePressed(sf::Vector2f windowPixel);

private:
    enum class State   { Menu, Playing, Paused, Victory, GameOver };
    enum class Outcome { None, Won, Lost };
    enum class Action  { Start, Resume, Next, Replay, Retry, Menu };
    enum class Sfx     { ShootBubble, ShootSlash, ShootRay, EnemyHit, EnemyDie, PlayerHurt,
                         Pickup, Victory, Defeat, Click, BossAlert, Count };

    struct Button   { sf::FloatRect rect; std::string label; Action action; };
    struct Pickup   { sf::Vector2f pos; float age = 0.f; bool alive = true; };
    struct Particle { sf::Vector2f pos, vel; float life, maxLife, size; sf::Color color; };
    struct Pending  { float delay; int type; float x; };
    struct Decor    { int kind; sf::Vector2f pos; float size; float rot; int variant; };

    // ---- flow ----------------------------------------------------------------
    void startLevel(int index);
    void toMenu();
    void finishLevel();
    void doAction(Action a);
    void setButtons(const std::vector<std::pair<std::string, Action>>& list);
    bool unlocked(int level) const { return level == 0 || m_stars[level - 1] > 0; }
    void moveSelection(int step);
    void pickDefaultSelection();

    // ---- simulation ------------------------------------------------------------
    void updatePlaying(float dt);
    void nextWave();
    void spawn(int type, float x);
    void burst(sf::Vector2f p, sf::Color c, int count, float speed, float size);
    void hurtPlayer();
    bool key(sf::Keyboard::Key k) const;

    // ---- drawing -----------------------------------------------------------------
    void drawWorld(sf::RenderTarget& t);
    void drawHud(sf::RenderTarget& t);
    void drawMenu(sf::RenderTarget& t);
    void drawOverlay(sf::RenderTarget& t);
    void drawBackground(sf::RenderTarget& t, int level);
    void drawDecor(sf::RenderTarget& t, int level);
    void buildDecor(int level);
    void drawText(sf::RenderTarget& t, const std::string& s, sf::Vector2f pos, unsigned size,
                  sf::Color color, bool center = true) const;
    void drawButtons(sf::RenderTarget& t) const;
    void showBanner(const std::string& text, float seconds);
    sf::Vector2f toGameCoords(sf::Vector2f windowPixel) const;

    // ---- sound & saving ----------------------------------------------------------
    void buildSounds();
    void playSfx(Sfx s, float volume = 100.f, float pitch = 1.f);
    void loadProgress();
    void saveProgress() const;

    // ---- state ---------------------------------------------------------------------
    State   m_state   = State::Menu;
    Outcome m_outcome = Outcome::None;

    std::array<int, 3> m_stars{{0, 0, 0}};     // best stars per level (0 = not completed)
    int   m_selected     = 0;
    float m_resetConfirm = 0.f;
    int   m_levelIdx     = 0;
    int   m_runStars     = 0;
    bool  m_newBest      = false;

    std::unique_ptr<Player>              m_player;
    std::vector<std::unique_ptr<Enemy>>  m_enemies;
    std::vector<Projectile>              m_shots;
    std::vector<Pickup>                  m_pickups;
    std::vector<Particle>                m_particles;
    std::vector<Decor>                   m_decor;
    std::vector<Pending>                 m_queue;
    std::vector<Button>                  m_buttons;
    std::vector<std::unique_ptr<Player>> m_showcase;

    int   m_wave = 0, m_score = 0;
    bool  m_bossSpawned = false;
    float m_waveTimer = 1.8f, m_endTimer = 0.f, m_regenTimer = 0.f;
    float m_scroll = 0.f, m_time = 0.f, m_levelTime = 0.f;
    float m_bannerTime = 0.f, m_bannerDur = 1.f;
    std::string m_bannerText;

    std::array<bool, sf::Keyboard::KeyCount> m_keys{};
    sf::Vector2u m_winSize{800, 600};

    sf::Font m_font;
    bool     m_fontOk = false;

    std::array<sf::SoundBuffer, static_cast<std::size_t>(Sfx::Count)> m_buffers;
    std::array<sf::Sound, 16> m_voices;
    std::size_t m_nextVoice = 0;
};
