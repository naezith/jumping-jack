#include "Entity.hpp"
#include "Library/Utility.hpp"

#include "Game.hpp"

Entity::Entity() : Entity(true) {}

Entity::Entity(bool changes_floor_on_edge, float collision_size_x) :
    m_direction(-1),
    m_collision_size_x(collision_size_x),
    m_facing(m_direction),
    m_draw_offset_y(0),
    m_sprite_color(sf::Color::White),
    m_movement_speed(300),
    m_changes_floor_on_edge(changes_floor_on_edge),
    m_scale(0.35) {
    
    // Middle bottom is the origin
    setOrigin(sf::Vector2f(0.5f*Game::i().getSpritesheetBlockSize(),
                           2.0f*Game::i().getSpritesheetBlockSize()));
    setScale(m_scale, m_scale);
    
    // Animation
    setFrameTime(sf::seconds(0.1f));
}

void Entity::spawn(bool random_position, const std::string& name, int direction) {
    m_sprite_name = name;
    m_floor = getSpawnFloor();
    
    // Pick middle or a random position
    setPosition(
        random_position ? random_float(0, Game::i().getViewSize().x) : Game::i().getViewSize().x*0.5f,
        m_floor * Game::i().getFloorHeight()
    );
    
    // Set direction if given, if not, pick random
    m_direction = direction != PICK_RANDOMLY ? direction : random_int(0, 1) ? 1 : -1;
}

void Entity::moveUp(bool allow_top_climb) {
    // Reached to top, teleport to bottom
    if(!allow_top_climb && m_floor <= 0) m_floor = getLowestFloor();
    // Go to upper floor
    else --m_floor;
    
    // Update sprite Y position
    setPositionY(m_floor * Game::i().getFloorHeight());

}

void Entity::moveDown() {
    // Reached to bottom, teleport to top
    if(m_floor >= getLowestFloor()) m_floor = 0;
    // Go to lower floor
    else ++m_floor;
    
    // Update sprite Y position
    setPositionY(m_floor * Game::i().getFloorHeight());
}

void Entity::updateFacing(float /* dt */) {
    // Movement direction is the facing direction by default
    m_facing = m_direction;
}

void Entity::update(float dt) {
    // Move
    setPositionX(getPosition().x + m_movement_speed * m_direction * dt);
    
    // If going left
    if(m_direction == -1) {
        // Reached to the left edge
        if(getPosition().x < 0) {
            setPositionX(Game::i().getViewSize().x);
            
            // Move to upper floor
            if(m_changes_floor_on_edge) moveUp();
        }
    }
    // If going right
    else if(m_direction == 1) {
        // Reached to the right edge
        if(getPosition().x > Game::i().getViewSize().x) {
            setPositionX(0);
            
            // Move to bottom floor
            if(m_changes_floor_on_edge) moveDown();
        }
    }
    
    updateFacing(dt);
    
    // If this entity has a sprite
    if(m_sprite_name != "") {
        changeAnimations();
        AnimatedSprite::updateAnim(dt);
    }
    
    // Update transparency
    if(m_sprite_color.r > 230 &&
       m_sprite_color.g > 230 &&
       m_sprite_color.b > 230) m_sprite_color.a = 255;
    else m_sprite_color.a = 55 + 200*(0.5f + 0.5f*sin(35*Game::i().getGlobalTimer()));
    
    // Turn to facing direction
    setScale((m_facing == 0 ? 1 : m_facing) * m_scale, m_scale);
}

bool Entity::collides(int floor, float x) {
    // Check if it's same floor and given x is inside the bounds of this object
    return floor == m_floor && (x >= getPosition().x - m_collision_size_x*0.5f &&
                                x <= getPosition().x + m_collision_size_x*0.5f);
}

void Entity::changeAnimations() {
    // If standing
    if(m_direction == 0) {
        if(m_facing == 0) play(Game::i().getAnimation(m_sprite_name + "_stand_mid"));
        else play(Game::i().getAnimation(m_sprite_name + "_stand_side"));
    }
    // If walking
    else {
        play(Game::i().getAnimation(m_sprite_name + "_walk"));
    }
}

void Entity::drawSelf(sf::RenderTarget& target) {
    target.draw(*this);
}

void Entity::render(sf::RenderTarget& target) {
    setColor(m_sprite_color);
    
    // Save location
    sf::Vector2f pos = getPosition();
    
    // Draw original
    float animated_y = pos.y + Game::i().getFloorHeight() + m_draw_offset_y;
    setPositionY(animated_y);
    drawSelf(target);
    
    // Screen Wrapping
    {
        // Draw left copy
        setPositionX(pos.x - Game::i().getViewSize().x);
        if(m_changes_floor_on_edge) setPositionY(animated_y + (m_floor == getLowestFloor() ? -getLowestFloor() : 1)*Game::i().getFloorHeight());
        drawSelf(target);
        
        // Draw right copy
        setPositionX(pos.x + Game::i().getViewSize().x);
        if(m_changes_floor_on_edge) setPositionY(animated_y - (m_floor == 0 ? -getLowestFloor() : 1)*Game::i().getFloorHeight());
        drawSelf(target);
    }
    
    // Recover location
    setPosition(pos);
}

// Getters
int Entity::getLowestFloor() { return Game::i().getBottomFloor() - 1; }
int Entity::getSpawnFloor() { return random_int(0, getLowestFloor()); }

// Setters
void Entity::setPositionX(float x) { setPosition(x, getPosition().y); }
void Entity::setPositionY(float y) { setPosition(getPosition().x, y); }
