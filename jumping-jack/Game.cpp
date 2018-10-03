#include "Game.hpp"
#include "Library/ResourcePath.hpp"

#include <sstream>
#include <iomanip>

Game Game::m_instance;

Game::Game() :
    m_game_title("JUMPING JACK"),
    m_background_count(0),
    m_curr_hazard(0),
    m_sheet_block_size(128),
    m_tile_height(32),
    m_dt(1/125.0f),
    m_view_size(800, 600),
    m_global_timer(0),
    m_timescale(1),
    m_slow_mo_timescale(0.25f),
    m_level(0),
    m_last_level(20),
    m_game_over(false),
    m_changing_level(false),
    m_changing_level_time(6),
    m_start_health(6),
    m_floor_count(8),
    m_max_hole_count(8),
    m_score_base(5),
    m_highscore(0),
    m_new_high(false),
    m_effect_color(sf::Color::Transparent) {}

void Game::init() {
    m_line_height = m_view_size.y / m_floor_count;
    
    m_effect_rect.setSize(getViewSize());
    
    loadAssets();
    
    m_music->setVolume(50);
    m_music->play();
    
    changeLevel(0);
    
    // Create window
    m_window.create(sf::VideoMode(m_view_size.x, m_view_size.y), m_game_title, sf::Style::Default);
    m_window.setVerticalSyncEnabled(true);
    
    // Create texture for hole rendering
    m_hole_texture.create(m_view_size.x, m_view_size.y);
    m_sprites["holes"].setTexture(m_hole_texture.getTexture());
}

// Add a new event to the events list
void Game::trigger(GAME_EVENT event) { m_events.push_back(event); }

void Game::run() {
    // Initialize the game
    init();
    
    // Game loop
    sf::Clock clock;
    float accumulator = 0;
    while(m_window.isOpen()) {
        // Poll events, only for close event
        sf::Event event;
        while(m_window.pollEvent(event))
            if(event.type == sf::Event::Closed) m_window.close();
        
        // Update
        accumulator += clock.restart().asSeconds();
        while(accumulator > m_dt) {
            accumulator -= m_dt;
            update();
        }
        
        // Render
        render();
    }
    
    // Clean-up
    m_music->stop(); delete m_music;
    m_sound_buffers.clear();
    m_looping_sounds.clear();
    m_sounds.clear();
}

void Game::update() {
    m_global_timer += m_dt;
    
    float timescaled_time = m_timescale * m_dt;
    
    // Update entities
    for(auto& e : m_holes) e->update(timescaled_time);
    for(auto& e : m_hazards) e->update(timescaled_time);
    if(!inInfoScreen()) m_player->update(m_dt);
    
    // Check game over condition
    if(m_game_over) {
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Return)) restart();
    }
    else if(m_changing_level) {
        if(m_global_timer >= m_changing_level_time ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::Return)) nextLevel();
    }
    
    // Change Level Cheat
    {
        // Previous level
        static bool k_prev;
        bool k_now = sf::Keyboard::isKeyPressed(sf::Keyboard::K);
        if(!k_prev && k_now) prevLevel();
        k_prev = k_now;
        
        // Next level
        static bool l_prev;
        bool l_now = sf::Keyboard::isKeyPressed(sf::Keyboard::L);
        if(!l_prev && l_now) nextLevel();
        l_prev = l_now;
        
        // Game Over
        static bool j_prev;
        bool j_now = sf::Keyboard::isKeyPressed(sf::Keyboard::J);
        if(!j_prev && j_now) trigger(GAME_EVENT::GAME_OVER);
        j_prev = j_now;
        
        // Finish Level
        static bool h_prev;
        bool h_now = sf::Keyboard::isKeyPressed(sf::Keyboard::H);
        if(!h_prev && h_now) trigger(GAME_EVENT::REACHED_TO_TOP);
        h_prev = h_now;
    }
    
    checkGameEvents();
}

void Game::render() {
    // Clear
    m_window.clear();

    // Draw everything
    drawGameplay();
    drawUI();
    if(inInfoScreen()) drawInfoScreen();
    
    // Display
    m_window.display();
}

void Game::drawGameplay() {
    // Draw background
    m_window.draw(m_sprites["background"]);
    
    // Draw tiles
    for(float y = 0; y < m_floor_count * m_line_height; y += m_line_height) {
        for(float x = 0; x < m_view_size.x; x += m_tile_height) {
            m_sprites["tile"].setPosition(x, y);
            m_window.draw(m_sprites["tile"]);
        }
    }
    
    // Draw holes to a texture, black and white
    // This is done to prevent overlapping rectangles looking darker
    m_hole_texture.clear(sf::Color::Transparent);
    for(auto& e : m_holes) e->render(m_hole_texture);
    m_hole_texture.display();
    // Draw the hole texture on top of the tiles
    m_sprites["holes"].setColor(sf::Color(255, 255, 255, 160));
    m_window.draw(m_sprites["holes"], sf::BlendAlpha);
    
    // Render other entities
    for(auto& e : m_hazards) e->render(m_window);
    m_player->render(m_window);
    
    // Screen effect on slow mo
    if(m_effect_color != sf::Color::Transparent) {
        m_effect_color.a = 50 + 100*(0.5f + 0.5f*sin(50*m_global_timer));
        m_effect_rect.setFillColor(m_effect_color);
        
        m_window.draw(m_effect_rect);
    }
}

std::string scoreText(unsigned score) {
    std::stringstream buffer;
    buffer << std::setw(5) << std::setfill('0') << score;
    return buffer.str();
}

void Game::drawUI() {
    float bottom = getViewSize().y - 44;
    
    drawText("HI" + scoreText(m_highscore), sf::Vector2f(550, bottom));
    drawText("SC" + scoreText(m_score), sf::Vector2f(675, bottom));
    
    // Draw health
    bottom -= 15;
    float health_offset = 25;
    for(unsigned i = 1; i <= m_health; ++i) {
        m_sprites["health"].setPosition(health_offset*i, bottom);
        m_window.draw(m_sprites["health"]);
    }
}

void Game::drawText(const std::string& text, const sf::Vector2f& pos, bool centered, const sf::Color& color, const sf::Color& background_color) {
    sf::Text t;
    t.setFont(m_font);
    t.setCharacterSize(24);
    t.setFillColor(color);
    
    t.setString(text);
    if(centered) t.setOrigin(sf::Vector2f(t.getLocalBounds().width, t.getLocalBounds().height)*0.5f);
    t.setPosition(pos);
    
    if(background_color != sf::Color::Transparent) {
        sf::RectangleShape rect;
        
        float offset = 20;
        rect.setSize(sf::Vector2f(t.getGlobalBounds().width + offset*2, t.getGlobalBounds().height + offset*2));
        rect.setPosition(sf::Vector2f(t.getGlobalBounds().left - offset, t.getGlobalBounds().top - offset));
        rect.setFillColor(background_color);
        
        m_window.draw(rect);
    }
    
    m_window.draw(t);
}

bool Game::inInfoScreen() {
    return m_changing_level || m_game_over;
}

void Game::drawInfoScreen() {
    sf::RectangleShape bg_filter;
    bg_filter.setSize(m_view_size);
    bg_filter.setFillColor(sf::Color(0, 0, 0, 200));
    m_window.draw(bg_filter);
    
    sf::Vector2f center = m_view_size*0.5f;
    
    // Game title
    drawText(m_game_title, sf::Vector2f(center.x, m_view_size.y*0.2f), true, sf::Color::Black, sf::Color::Green);
    
    // Game over
    if(m_game_over) {
        drawText("FINAL SCORE   " + scoreText(m_score), sf::Vector2f(center.x, m_view_size.y*0.4f), true, sf::Color::Black, sf::Color::Cyan);
        drawText("WITH " + std::to_string(m_hazards.size()) + " HAZARDS", sf::Vector2f(center.x, m_view_size.y*0.5f), true, sf::Color::Black, sf::Color::Cyan);
        
        if(m_new_high) {
            float flash_interval = 0.5f;
            bool flash = fmod(m_global_timer, flash_interval) > flash_interval*0.5f;
            sf::Color c1 = sf::Color::White, c2 = sf::Color::Magenta;
            drawText("NEW HIGH", sf::Vector2f(center.x, m_view_size.y*0.7f), true, flash ? c1 : c2, flash ? c2 : c1);
        }
        
        drawText("Press ENTER to replay", sf::Vector2f(center.x, m_view_size.y*0.9f), true, sf::Color::White);
    }
    else if(m_changing_level) {
        if(m_level <= m_last_level) {
            std::size_t hazard_count = m_hazards.size() + 1;
            drawText("NEXT LEVEL -  "  + std::to_string(hazard_count) + " " + (hazard_count == 1 ? "HAZARD" : "HAZARDS"),
                     sf::Vector2f(center.x, m_view_size.y*0.4f), true, sf::Color::Blue, sf::Color::White);
        }
        
        // Story
        auto& lines = m_story_texts[m_level];
        for(std::size_t i = 0; i < lines.size(); ++i) {
            drawText(lines[i], sf::Vector2f(m_view_size.x*0.3f, m_view_size.y*(0.7f + i*0.05f)));
        }
    }
}

void Game::spawnHole() {
    if(m_holes.size() >= m_max_hole_count) return;
    
    int direction =
    // First two are in reversed directions
    m_holes.size() == 0 ? 1 :
    m_holes.size() == 1 ? -1 :
    // Next 3 holes descend, last 3 ascend
    m_holes.size() <= 4 ? 1 : -1;
    
    m_holes.push_back(std::make_unique<Hole>());
    
    m_holes.back()->spawn(true, "", direction);
}

void Game::spawnHazard() {
    if(++m_curr_hazard >= m_hazard_names.size()) m_curr_hazard = 0;
    
    m_hazards.push_back(std::make_unique<Entity>());
    
    // Always goes left
    m_hazards.back()->spawn(true, m_hazard_names[m_curr_hazard], -1);
}

void Game::gameOver() {
    m_game_over = true;
    
    if(m_score > m_highscore) {
        m_highscore = m_score;
        m_new_high = true;
    }
}

void Game::nextLevel() {
    changeLevel(m_level + 1);
    m_changing_level = false;
}

void Game::prevLevel() {
    changeLevel(m_level - 1);
    m_changing_level = false;
}

void Game::resetEffects() {
    // Reset variables
    m_timescale = 1;
    m_effect_color = sf::Color::Transparent;
}
void Game::changeLevel(int level) {
    resetEffects();
    
    // Don't exceed the level limit
    level = std::min(std::max(level, 0), m_last_level);
    m_level = level;
    
    // Theme
    changeTheme(level);
    
    // Reset score
    if(m_level == 0) m_score = 0;
    // Set the score increase amount depending on the level
    m_score_increase = m_score_base * (1 + m_level);
    
    // Spawn hazards
    m_hazards.clear();
    for(int i = 0; i < m_level; ++i) spawnHazard();
    
    // Spawn holes
    m_holes.clear();
    for(unsigned i = 0; i < 2; ++i) spawnHole();
    
    // Health
    if(m_level == 0) m_health = m_start_health;
    // Bonus health at 6, 11, 16
    else if(m_level != 1 && (m_level - 1) % 5 == 0) ++m_health;
    
    // Player
    m_player = std::make_unique<Player>();
    m_player->spawn(false, "pink");
}


void Game::changeTheme(int level) {
    unsigned theme_id = level % m_background_count;
    std::string theme = theme_id == 0 ? "grass" : theme_id == 1 ? "desert" : "shroom";
    
    // Set correct background
    std::string curr_bg = "bg_" + theme;
    m_sprites["background"].setTexture(m_textures[curr_bg]);
    
    float scale = m_view_size.x/m_textures[curr_bg].getSize().x;
    m_sprites["background"].setScale(scale, scale);
    m_sprites["background"].setColor(sf::Color(100, 100, 100));
    
    // Set correct tile
    sf::Vector2i tex_coord;
    
    if(theme == "desert") { tex_coord.x = 4; tex_coord.y = 14; }
    else if(theme == "grass")  { tex_coord.x = 0; tex_coord.y = 6; }
    else if(theme == "shroom") { tex_coord.x = 1; tex_coord.y = 8; }
    
    m_sprites["tile"].setTexture(m_textures["spritesheet_ground"]);
    m_sprites["tile"].setTextureRect(sf::IntRect(tex_coord.x*m_sheet_block_size, tex_coord.y*m_sheet_block_size,
                                                 m_sheet_block_size, m_sheet_block_size));
    
    float sh_scale = m_tile_height / m_sheet_block_size;
    m_sprites["tile"].setScale(sh_scale, sh_scale);
}

void Game::restart() {
    changeLevel(0);
    m_game_over = false;
    m_new_high = false;
}

void Game::levelFinished() {
    m_changing_level = true;
    m_global_timer = 0;
}

void Game::checkGameEvents() {
    // Events
    while(!m_events.empty()) {
        GAME_EVENT event = m_events.back();
        m_events.pop_back();
        
        switch(event) {
            case GAME_EVENT::REACHED_TO_TOP:
                playSound("end_win");
                levelFinished();
                break;
                
            case GAME_EVENT::GAME_OVER:
                playSound("end_lose");
                gameOver();
                break;
                
            case GAME_EVENT::DROPPED_TO_BOTTOM:
                if(--m_health <= 0) {
                    m_health = 0;
                    trigger(GAME_EVENT::GAME_OVER);
                }
                break;
                
            case GAME_EVENT::STOPPED_JUMPING:
                spawnHole();
                resetEffects();
                break;
                
            case GAME_EVENT::STOPPED_HIT_HEAD:
            case GAME_EVENT::STOPPED_HAZARD_HIT:
            case GAME_EVENT::STOPPED_FALLING:
                playSound("fall_land");
                resetEffects();
                break;
                
            case GAME_EVENT::STARTED_FALLING:
                m_timescale = m_slow_mo_timescale;
                m_effect_color = sf::Color::Transparent;
                playSound("fall");
                break;
                
            case GAME_EVENT::STARTED_JUMPING:
                m_timescale = m_slow_mo_timescale;
                m_effect_color = sf::Color::Transparent;
                playSound("jump");
                addScore();
                break;
                
            case GAME_EVENT::HIT_BY_HAZARD:
                m_timescale = 0;
                m_effect_color = sf::Color::Red;
                playSound("hit");
                break;
                
            case GAME_EVENT::HIT_HEAD:
                m_timescale = m_slow_mo_timescale;
                m_effect_color = sf::Color::White;
                playSound("bump");
                break;
                
            case GAME_EVENT::STOPPED_STUN:
                playSound("get_up");
                break;
                
            case GAME_EVENT::PLAYER_TURNED:
                playSound("turn");
                break;
                
            case GAME_EVENT::STARTED_WALKING:
                setSoundLoop("walk", true);
                break;
                
            case GAME_EVENT::STOPPED_WALKING:
                setSoundLoop("walk", false);
                break;
        }
    }
}

// Getters
const std::vector<std::unique_ptr<Entity>>& Game::getHazards() { return m_hazards; }
const std::vector<std::unique_ptr<Hole>>& Game::getHoles() { return m_holes; }
float Game::getTileHeight() { return m_tile_height - 14; }
float Game::getGlobalTimer() { return m_global_timer; }
float Game::getFloorHeight() { return m_line_height; }
int Game::getBottomFloor() { return m_floor_count - 1; }
sf::Vector2f Game::getViewSize() { return m_view_size; }
void Game::addScore() { m_score += m_score_increase; }
float Game::getSpritesheetBlockSize() { return m_sheet_block_size; }
const Animation& Game::getAnimation(std::string name) { return m_animations[name]; }





















// LOAD ASSETS
void loadFailed(const std::string& file_name) {
    std::cerr << "Could not load: " << file_name << std::endl;
    exit(2);
}

void Game::loadAssets() {
    m_music = new sf::Music();
    if(!m_music->openFromFile(resourcePath() + "data/musics/music.ogg")) loadFailed("music.ogg");
    m_music->setLoop(true);
    
    m_font.loadFromFile(resourcePath() + "data/fonts/sansation.ttf");
    
    if(!m_textures["bg_desert"].loadFromFile(resourcePath() + "data/images/bg_desert.png")) loadFailed("bg_desert.png");
    ++m_background_count;
    if(!m_textures["bg_grass"].loadFromFile(resourcePath() + "data/images/bg_grass.png")) loadFailed("bg_grass.png");
    ++m_background_count;
    if(!m_textures["bg_shroom"].loadFromFile(resourcePath() + "data/images/bg_shroom.png")) loadFailed("bg_shroom.png");
    ++m_background_count;
    
    if(!m_textures["spritesheet_ground"].loadFromFile(resourcePath() + "data/images/spritesheet_ground.png")) loadFailed("spritesheet_ground.png");
    m_textures["spritesheet_ground"].setSmooth(true);
    
    if(!m_textures["spritesheet_players"].loadFromFile(resourcePath() + "data/images/spritesheet_players.png")) loadFailed("spritesheet_players.png");
    m_textures["spritesheet_players"].setSmooth(true);
    
    loadAnimations();
    
    m_sprites["health"].setTexture(m_textures["spritesheet_players"]);
    m_sprites["health"].setTextureRect(m_animations["pink_stand_mid"].getFrame(0));
    m_sprites["health"].setScale(0.175f, 0.175f);
    
    loadStory();

    loadSound("hit");
    loadSound("jump");
    loadSound("turn");
    loadSound("bump");
    loadSound("fall");
    loadSound("fall_land");
    loadSound("end_lose");
    loadSound("end_win");
    loadSound("get_up");
    loadSound("walk"); m_looping_sounds["walk"].setPitch(1.5f); m_looping_sounds["walk"].setVolume(30);
}

void Game::playSound(const std::string& name) {
    // Erase finished sounds
    m_sounds.erase(std::remove_if(m_sounds.begin(), m_sounds.end(), [](const sf::Sound& s) { return s.getStatus() == sf::Sound::Stopped; }), m_sounds.end());

    // Play new sound
    m_sounds.emplace_back(m_sound_buffers[name]);
    m_sounds.back().play();
}

void Game::loadSound(const std::string& name) {
    if(!m_sound_buffers[name].loadFromFile(resourcePath() + "data/sounds/" + name + ".wav")) loadFailed(name + ".wav");
    
    m_looping_sounds[name].setBuffer(m_sound_buffers[name]);
    m_looping_sounds[name].setLoop(true);
}

void Game::setSoundLoop(const std::string &name, bool loop) {
    if(loop) m_looping_sounds[name].play();
    else m_looping_sounds[name].pause();
}

void Game::loadStory() {
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("Jumping Jack is quick and bold");
    m_story_texts.back().push_back("With skill his story will unfold");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("THE BALLAD OF JUMPING JACK");
    m_story_texts.back().push_back("A daring explorer named Jack...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("Once found a peculiar track...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("There were dangers galore...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("Even holes in the floor...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("So he kept falling flat on");
    m_story_texts.back().push_back("his back...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("Quite soon he got used to");
    m_story_texts.back().push_back("the place...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("He could jump to escape from");
    m_story_texts.back().push_back("the chase...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("But without careful thought...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("His leaps came to nought...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("And he left with a much");
    m_story_texts.back().push_back("wider face...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("Things seemed just as bad as");
    m_story_texts.back().push_back("could be...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("Hostile faces were all Jack");
    m_story_texts.back().push_back("could see...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("He tried to stay calm...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("And come to no harm");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("But more often got squashed");
    m_story_texts.back().push_back("like a flea...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("By now Jack was in a");
    m_story_texts.back().push_back("great flap...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("He felt like a rat in a trap");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("If only he'd guessed...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("That soon he could rest...");
    m_story_texts.emplace_back();
    m_story_texts.back().push_back("After jumping the very");
    m_story_texts.back().push_back("very last gap.  - WELL DONE");
}

void Game::loadAnimations() {
    sf::Texture& spritesheet = m_textures["spritesheet_players"];
    
    const unsigned size_x = m_sheet_block_size;
    const unsigned size_y = m_sheet_block_size * 2;
    
    // Pink, player character
    m_animations["pink_stand_mid"].setSpriteSheet(spritesheet);
    m_animations["pink_stand_mid"].addFrame(sf::IntRect((4-1)*size_x, (6-1)*size_y, size_x, size_y));
    
    m_animations["pink_stand_side"].setSpriteSheet(spritesheet);
    m_animations["pink_stand_side"].addFrame(sf::IntRect((4-1)*size_x, (3-1)*size_y, size_x, size_y));
    
    m_animations["pink_walk"].setSpriteSheet(spritesheet);
    m_animations["pink_walk"].addFrame(sf::IntRect((3-1)*size_x, (7-1)*size_y, size_x, size_y));
    m_animations["pink_walk"].addFrame(sf::IntRect((3-1)*size_x, (8-1)*size_y, size_x, size_y));
    m_animations["pink_walk"].addFrame(sf::IntRect((4-1)*size_x, (1-1)*size_y, size_x, size_y));
    m_animations["pink_walk"].addFrame(sf::IntRect((4-1)*size_x, (2-1)*size_y, size_x, size_y));
    
    m_animations["pink_stun"].setSpriteSheet(spritesheet);
    m_animations["pink_stun"].addFrame(sf::IntRect((4-1)*size_x, (7-1)*size_y, size_x, size_y));
    
    m_animations["pink_climb"].setSpriteSheet(spritesheet);
    m_animations["pink_climb"].addFrame(sf::IntRect((4-1)*size_x, (8-1)*size_y, size_x, size_y));
    m_animations["pink_climb"].addFrame(sf::IntRect((5-1)*size_x, (1-1)*size_y, size_x, size_y));
    
    m_animations["pink_jump"].setSpriteSheet(spritesheet);
    m_animations["pink_jump"].addFrame(sf::IntRect((7-1)*size_x, (7-1)*size_y, size_x, size_y));
    
    m_animations["pink_fall"].setSpriteSheet(spritesheet);
    m_animations["pink_fall"].addFrame(sf::IntRect((4-1)*size_x, (5-1)*size_y, size_x, size_y));

    
    
    // Green character
    m_hazard_names.push_back("green");
    
    m_animations["green_stand_mid"].setSpriteSheet(spritesheet);
    m_animations["green_stand_mid"].addFrame(sf::IntRect((6-1)*size_x, (1-1)*size_y, size_x, size_y));
    
    m_animations["green_stand_side"].setSpriteSheet(spritesheet);
    m_animations["green_stand_side"].addFrame(sf::IntRect((5-1)*size_x, (6-1)*size_y, size_x, size_y));
    
    m_animations["green_walk"].setSpriteSheet(spritesheet);
    m_animations["green_walk"].addFrame(sf::IntRect((5-1)*size_x, (2-1)*size_y, size_x, size_y));
    m_animations["green_walk"].addFrame(sf::IntRect((5-1)*size_x, (3-1)*size_y, size_x, size_y));
    m_animations["green_walk"].addFrame(sf::IntRect((5-1)*size_x, (4-1)*size_y, size_x, size_y));
    m_animations["green_walk"].addFrame(sf::IntRect((5-1)*size_x, (5-1)*size_y, size_x, size_y));
    
    m_animations["green_stun"].setSpriteSheet(spritesheet);
    m_animations["green_stun"].addFrame(sf::IntRect((6-1)*size_x, (2-1)*size_y, size_x, size_y));
    
    m_animations["green_climb"].setSpriteSheet(spritesheet);
    m_animations["green_climb"].addFrame(sf::IntRect((6-1)*size_x, (3-1)*size_y, size_x, size_y));
    m_animations["green_climb"].addFrame(sf::IntRect((6-1)*size_x, (4-1)*size_y, size_x, size_y));
    
    m_animations["green_jump"].setSpriteSheet(spritesheet);
    m_animations["green_jump"].addFrame(sf::IntRect((5-1)*size_x, (7-1)*size_y, size_x, size_y));
    
    m_animations["green_fall"].setSpriteSheet(spritesheet);
    m_animations["green_fall"].addFrame(sf::IntRect((5-1)*size_x, (8-1)*size_y, size_x, size_y));
    
    
    
    // Gray character
    m_hazard_names.push_back("gray");
    
    m_animations["gray_stand_mid"].setSpriteSheet(spritesheet);
    m_animations["gray_stand_mid"].addFrame(sf::IntRect((1-1)*size_x, (8-1)*size_y, size_x, size_y));
    
    m_animations["gray_stand_side"].setSpriteSheet(spritesheet);
    m_animations["gray_stand_side"].addFrame(sf::IntRect((1-1)*size_x, (5-1)*size_y, size_x, size_y));
    
    m_animations["gray_walk"].setSpriteSheet(spritesheet);
    m_animations["gray_walk"].addFrame(sf::IntRect((1-1)*size_x, (1-1)*size_y, size_x, size_y));
    m_animations["gray_walk"].addFrame(sf::IntRect((1-1)*size_x, (2-1)*size_y, size_x, size_y));
    m_animations["gray_walk"].addFrame(sf::IntRect((1-1)*size_x, (3-1)*size_y, size_x, size_y));
    m_animations["gray_walk"].addFrame(sf::IntRect((1-1)*size_x, (4-1)*size_y, size_x, size_y));
    
    m_animations["gray_stun"].setSpriteSheet(spritesheet);
    m_animations["gray_stun"].addFrame(sf::IntRect((2-1)*size_x, (1-1)*size_y, size_x, size_y));
    
    m_animations["gray_climb"].setSpriteSheet(spritesheet);
    m_animations["gray_climb"].addFrame(sf::IntRect((2-1)*size_x, (2-1)*size_y, size_x, size_y));
    m_animations["gray_climb"].addFrame(sf::IntRect((2-1)*size_x, (3-1)*size_y, size_x, size_y));
    
    m_animations["gray_jump"].setSpriteSheet(spritesheet);
    m_animations["gray_jump"].addFrame(sf::IntRect((1-1)*size_x, (6-1)*size_y, size_x, size_y));
    
    m_animations["gray_fall"].setSpriteSheet(spritesheet);
    m_animations["gray_fall"].addFrame(sf::IntRect((1-1)*size_x, (7-1)*size_y, size_x, size_y));
    
    
    
    // Yellow character
    m_hazard_names.push_back("yellow");
    
    m_animations["yellow_stand_mid"].setSpriteSheet(spritesheet);
    m_animations["yellow_stand_mid"].addFrame(sf::IntRect((3-1)*size_x, (3-1)*size_y, size_x, size_y));
    
    m_animations["yellow_stand_side"].setSpriteSheet(spritesheet);
    m_animations["yellow_stand_side"].addFrame(sf::IntRect((2-1)*size_x, (8-1)*size_y, size_x, size_y));
    
    m_animations["yellow_walk"].setSpriteSheet(spritesheet);
    m_animations["yellow_walk"].addFrame(sf::IntRect((2-1)*size_x, (4-1)*size_y, size_x, size_y));
    m_animations["yellow_walk"].addFrame(sf::IntRect((2-1)*size_x, (5-1)*size_y, size_x, size_y));
    m_animations["yellow_walk"].addFrame(sf::IntRect((2-1)*size_x, (6-1)*size_y, size_x, size_y));
    m_animations["yellow_walk"].addFrame(sf::IntRect((2-1)*size_x, (7-1)*size_y, size_x, size_y));
    
    m_animations["yellow_stun"].setSpriteSheet(spritesheet);
    m_animations["yellow_stun"].addFrame(sf::IntRect((3-1)*size_x, (4-1)*size_y, size_x, size_y));
    
    m_animations["yellow_climb"].setSpriteSheet(spritesheet);
    m_animations["yellow_climb"].addFrame(sf::IntRect((3-1)*size_x, (5-1)*size_y, size_x, size_y));
    m_animations["yellow_climb"].addFrame(sf::IntRect((3-1)*size_x, (6-1)*size_y, size_x, size_y));
    
    m_animations["yellow_jump"].setSpriteSheet(spritesheet);
    m_animations["yellow_jump"].addFrame(sf::IntRect((3-1)*size_x, (1-1)*size_y, size_x, size_y));
    
    m_animations["yellow_fall"].setSpriteSheet(spritesheet);
    m_animations["yellow_fall"].addFrame(sf::IntRect((3-1)*size_x, (2-1)*size_y, size_x, size_y));
    
    
    
    // Blue character
    m_hazard_names.push_back("blue");
    
    m_animations["blue_stand_mid"].setSpriteSheet(spritesheet);
    m_animations["blue_stand_mid"].addFrame(sf::IntRect((7-1)*size_x, (4-1)*size_y, size_x, size_y));
    
    m_animations["blue_stand_side"].setSpriteSheet(spritesheet);
    m_animations["blue_stand_side"].addFrame(sf::IntRect((7-1)*size_x, (1-1)*size_y, size_x, size_y));
    
    m_animations["blue_walk"].setSpriteSheet(spritesheet);
    m_animations["blue_walk"].addFrame(sf::IntRect((6-1)*size_x, (5-1)*size_y, size_x, size_y));
    m_animations["blue_walk"].addFrame(sf::IntRect((6-1)*size_x, (6-1)*size_y, size_x, size_y));
    m_animations["blue_walk"].addFrame(sf::IntRect((6-1)*size_x, (7-1)*size_y, size_x, size_y));
    m_animations["blue_walk"].addFrame(sf::IntRect((6-1)*size_x, (8-1)*size_y, size_x, size_y));
    
    m_animations["blue_stun"].setSpriteSheet(spritesheet);
    m_animations["blue_stun"].addFrame(sf::IntRect((7-1)*size_x, (5-1)*size_y, size_x, size_y));
    
    m_animations["blue_climb"].setSpriteSheet(spritesheet);
    m_animations["blue_climb"].addFrame(sf::IntRect((4-1)*size_x, (4-1)*size_y, size_x, size_y));
    m_animations["blue_climb"].addFrame(sf::IntRect((7-1)*size_x, (6-1)*size_y, size_x, size_y));
    
    m_animations["blue_jump"].setSpriteSheet(spritesheet);
    m_animations["blue_jump"].addFrame(sf::IntRect((7-1)*size_x, (2-1)*size_y, size_x, size_y));
    
    m_animations["blue_fall"].setSpriteSheet(spritesheet);
    m_animations["blue_fall"].addFrame(sf::IntRect((7-1)*size_x, (3-1)*size_y, size_x, size_y));
}
