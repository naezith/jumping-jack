#include "Player.hpp"

#include <SFML/Graphics.hpp>

#include "Game.hpp"

Player::Player() :
    Entity(false),
    m_state(PLAYER_STATE::FREE),
    m_timer(0),
    m_stun_timer(0),
    m_move_time(1),
    m_hit_head_time(1),
    m_hit_by_hazard_time(0.5f),
    m_stun_time(0.7f),
    m_hazard_hit_stun_time(m_stun_time * 2),
    m_long_stun_time(m_stun_time * 3),
    m_last_facing(m_facing),
    m_facing_timer(0),
    m_facing_interval(0.7f) {}

void Player::updateFacing(float dt) {
    // If controllable
    if(m_state == PLAYER_STATE::FREE) {
        // If standing still
        if(m_direction == 0) {
            m_facing_timer += dt;
            
            // If it is time to turn
            if(m_facing_timer > m_facing_interval) {
                m_facing_timer = 0;
                
                // Change direction of look
                m_facing = m_facing == 0 ? -m_last_facing : 0;
                
                Game::i().trigger(Game::GAME_EVENT::PLAYER_TURNED);
            }
        }
        // If moving
        else {
            // Look in the direction of movement
            Entity::updateFacing(dt);
            m_facing_timer = 0;
        }
    }
    
    // Save the last facing direction
    if(m_facing != 0) m_last_facing = m_facing;
}

void Player::update(float dt) {
    updateDirection();
    Entity::update(dt);
    checkInteractions();
    updateState(dt);
}

void Player::changeState(PLAYER_STATE new_state) {
    switch(new_state) {
        case PLAYER_STATE::FREE:
            if(m_state == PLAYER_STATE::JUMPING) {
                Game::i().trigger(Game::GAME_EVENT::STOPPED_JUMPING);
                
                // Finished the level
                if(m_floor == -1) Game::i().trigger(Game::GAME_EVENT::REACHED_TO_TOP);
            }
            else if(m_state == PLAYER_STATE::STUNNED)
                Game::i().trigger(Game::GAME_EVENT::STOPPED_STUN);
            
            m_sprite_color = sf::Color::White;
            break;
            
        case PLAYER_STATE::JUMPING:
            Game::i().trigger(Game::GAME_EVENT::STARTED_JUMPING);
            m_sprite_color = sf::Color::White;
            m_timer = m_move_time;
            break;

        case PLAYER_STATE::FALLING:
            Game::i().trigger(Game::GAME_EVENT::STARTED_FALLING);
            m_sprite_color = sf::Color::White;
            m_timer = m_move_time;
            break;
            
        case PLAYER_STATE::HIT_HEAD:
            Game::i().trigger(Game::GAME_EVENT::HIT_HEAD);
            m_timer = m_hit_head_time;
            m_sprite_color = sf::Color::Magenta;
            break;
            
        case PLAYER_STATE::HIT_BY_HAZARD:
            Game::i().trigger(Game::GAME_EVENT::HIT_BY_HAZARD);
            m_sprite_color = sf::Color::Red;
            m_timer = m_hit_by_hazard_time;
            break;
            
        case PLAYER_STATE::STUNNED:
            // Alert about the end of previous state
            if(m_state == PLAYER_STATE::FALLING)
                Game::i().trigger(Game::GAME_EVENT::STOPPED_FALLING);
            else if(m_state == PLAYER_STATE::HIT_BY_HAZARD)
                Game::i().trigger(Game::GAME_EVENT::STOPPED_HAZARD_HIT);
            else if(m_state == PLAYER_STATE::HIT_HEAD)
                Game::i().trigger(Game::GAME_EVENT::STOPPED_HIT_HEAD);
            
            // Dropped to the bottom floor
            if(m_floor == Game::i().getBottomFloor())
                Game::i().trigger(Game::GAME_EVENT::DROPPED_TO_BOTTOM);
            
            // Stack up the stun time
            if(m_stun_timer < 0) m_stun_timer = 0;
            
            if(m_state == PLAYER_STATE::HIT_BY_HAZARD)
                m_stun_timer += m_hazard_hit_stun_time;
            else if(m_state == PLAYER_STATE::HIT_HEAD)
                m_stun_timer += m_long_stun_time;
            else if(m_floor == Game::i().getBottomFloor()) // Dropped to the bottom floor
                m_stun_timer += m_long_stun_time;
            else
                m_stun_timer += m_stun_time;
            
            m_sprite_color = sf::Color::Yellow;
            break;
    }
    
    m_state = new_state;
}

void Player::updateState(float dt) {
    m_timer -= dt;
    const bool time_is_up = m_timer <= 0;
    
    m_stun_timer -= dt;
    const bool stun_time_is_up = m_stun_timer <= 0;
    
    switch(m_state) {
        case PLAYER_STATE::FREE:
            m_draw_offset_y = 0;
            break;
            
        case PLAYER_STATE::JUMPING:
            m_draw_offset_y = Game::i().getFloorHeight()*(m_timer/m_move_time);
            if(time_is_up) changeState(PLAYER_STATE::FREE);
            break;
        
        case PLAYER_STATE::FALLING:
            m_draw_offset_y = -Game::i().getFloorHeight()*(m_timer/m_move_time);
            if(time_is_up) changeState(PLAYER_STATE::STUNNED);
            break;
        
        case PLAYER_STATE::HIT_BY_HAZARD:
            m_draw_offset_y = 0;
            if(time_is_up) changeState(PLAYER_STATE::STUNNED);
            break;
            
        case PLAYER_STATE::HIT_HEAD:
            m_draw_offset_y = -0.25f*Game::i().getFloorHeight()*(m_timer/m_hit_head_time);
            if(time_is_up) changeState(PLAYER_STATE::STUNNED);
            break;
        
        case PLAYER_STATE::STUNNED:
            m_draw_offset_y = 0;
            if(stun_time_is_up) changeState(PLAYER_STATE::FREE);
            break;
    }
}

void Player::updateDirection() {
    const int dir_before = m_direction;
    
    // Controllable
    if(m_state == PLAYER_STATE::FREE) {
        const bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        const bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        
        m_direction = !(right ^ left) ? 0 : right ? 1 : -1;
    }
    // Not controllable
    else m_direction = 0;
    
    // Started or Stopped walking
         if(dir_before == 0 && m_direction != 0) Game::i().trigger(Game::GAME_EVENT::STARTED_WALKING);
    else if(dir_before != 0 && m_direction == 0) Game::i().trigger(Game::GAME_EVENT::STOPPED_WALKING);
}

void Player::checkInteractions() {
    // Holes
    const bool jump = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
    bool jump_result = false;
    for(auto& hole : Game::i().getHoles()) {
        // Jump
        if(jump) {
            if(m_state == PLAYER_STATE::FREE) {
                if(hole->collides(m_floor, getPosition().x)) {
                    moveUp(true);
                    changeState(PLAYER_STATE::JUMPING);
                    jump_result = true;
                }
            }
        }
        
        // Fall
        if(m_state == PLAYER_STATE::FREE || m_state == PLAYER_STATE::STUNNED) {
            if(hole->collides(m_floor + 1, getPosition().x)) {
                moveDown();
                changeState(PLAYER_STATE::FALLING);
            }
        }
    }
    
    // Hit head to ceiling
    if(m_state == PLAYER_STATE::FREE && jump && !jump_result)
        changeState(PLAYER_STATE::HIT_HEAD);
    
    // Hit by hazard
    if(m_state == PLAYER_STATE::FREE) {
        for(auto& hazard : Game::i().getHazards()) {
            if(hazard->collides(m_floor, getPosition().x))
                changeState(PLAYER_STATE::HIT_BY_HAZARD);
        }
    }
}

void Player::changeAnimations() {
    if(m_state == PLAYER_STATE::STUNNED)
        play(Game::i().getAnimation(m_sprite_name + "_stun"));
    else if(m_state == PLAYER_STATE::FALLING || m_state == PLAYER_STATE::HIT_BY_HAZARD)
        play(Game::i().getAnimation(m_sprite_name + "_fall"));
    else if(m_state == PLAYER_STATE::JUMPING)
        play(Game::i().getAnimation(m_sprite_name + "_climb"));
    else if(m_state == PLAYER_STATE::HIT_HEAD)
        play(Game::i().getAnimation(m_sprite_name + "_jump"));
    else Entity::changeAnimations();
}


// Getters
int Player::getLowestFloor() { return Game::i().getBottomFloor(); }
int Player::getSpawnFloor() { return getLowestFloor(); }
