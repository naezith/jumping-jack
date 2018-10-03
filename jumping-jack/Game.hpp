#ifndef Game_hpp
#define Game_hpp

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>

#include "Entity.hpp"
#include "Hole.hpp"
#include "Player.hpp"

class Game {
    Game();
    static Game m_instance;
public:
    static Game& i() { return m_instance; }
    
    // Called by main.cpp
    void run();
    
    // Events
    enum GAME_EVENT {
        GAME_OVER,
        DROPPED_TO_BOTTOM, REACHED_TO_TOP,
        STARTED_JUMPING, STOPPED_JUMPING,
        STARTED_FALLING, STOPPED_FALLING,
        HIT_BY_HAZARD, STOPPED_HAZARD_HIT,
        HIT_HEAD, STOPPED_HIT_HEAD,
        STARTED_WALKING, STOPPED_WALKING,
        STOPPED_STUN, PLAYER_TURNED
    };
    
    // Game
    void trigger(GAME_EVENT event);
    float getGlobalTimer();
    sf::Vector2f getViewSize();
    float getSpritesheetBlockSize();
    float getTileHeight();
    const Animation& getAnimation(std::string name);
    
    // World
    float getFloorHeight();
    int getBottomFloor();
    
    // Objects
    const std::vector<std::unique_ptr<Entity>>& getHazards();
    const std::vector<std::unique_ptr<Hole>>& getHoles();
    
private:
// Functions
    // Global
    void init();
    void update();
    void render();
    void checkGameEvents();

    void playSound(const std::string& name);
    void setSoundLoop(const std::string& name, bool loop);
    void drawText(const std::string& text, const sf::Vector2f& pos, bool centered = false,
                  const sf::Color& color = sf::Color::White, const sf::Color& background_color = sf::Color::Transparent);

    void drawGameplay();
    void drawUI();
    void drawInfoScreen();
    
    void loadAssets();
    void loadAnimations();
    void loadStory();
    void loadSound(const std::string& name);
    
    void changeTheme(int level);
    
    // Game
    void changeLevel(int level);
    void nextLevel();
    void prevLevel();
    void restart();
    void addScore();
    void levelFinished();
    void gameOver();
    bool inInfoScreen();
    void resetEffects();

    // Objects
    void spawnHole();
    void spawnHazard();

// Variables
    // Assets
    std::string m_game_title;
    std::vector<std::vector<std::string>> m_story_texts;
    std::unordered_map<std::string, sf::Texture> m_textures;
    std::unordered_map<std::string, sf::Sprite> m_sprites;
    std::unordered_map<std::string, Animation> m_animations;
    std::unordered_map<std::string, sf::SoundBuffer> m_sound_buffers;
    std::unordered_map<std::string, sf::Sound> m_looping_sounds;
    std::vector<sf::Sound> m_sounds;
    sf::Music* m_music;
    sf::Font m_font;

    unsigned m_background_count;
    std::size_t m_curr_hazard;
    std::vector<std::string> m_hazard_names;
    const unsigned m_sheet_block_size;
    const float m_tile_height;
    
    // Global
    const float m_dt;
    const sf::Vector2f m_view_size;
    float m_line_height;
    
    float m_global_timer;
    float m_timescale;
    const float m_slow_mo_timescale;
    
    // Game
    std::vector<GAME_EVENT> m_events;
    int m_level;
    int m_last_level;
    bool m_game_over;
    bool m_changing_level;
    unsigned m_health;
    
    const float m_changing_level_time;
    const unsigned m_start_health;
    
    // Objects
    std::vector<std::unique_ptr<Entity>> m_hazards;
    std::vector<std::unique_ptr<Hole>> m_holes;
    std::unique_ptr<Player> m_player;
    const int m_floor_count;
    const std::size_t m_max_hole_count;
    
    // Score
    const unsigned m_score_base;
    unsigned m_score_increase;
    unsigned m_score;
    unsigned m_highscore;
    bool m_new_high;
    
    // Render
    sf::RenderWindow m_window;
    sf::RenderTexture m_hole_texture;
    sf::RectangleShape m_effect_rect;
    sf::Color m_effect_color;
};

#endif /* Game_hpp */
