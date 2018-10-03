#ifndef Player_hpp
#define Player_hpp

#include "Entity.hpp"

class Player : public Entity {
public:
    Player();
    
    // Global
    virtual void update(float dt);

protected:
    // Gameplay
    virtual int getLowestFloor();
    virtual int getSpawnFloor();
    virtual void updateFacing(float dt);
    
private:
// Functions
    // State
    enum PLAYER_STATE { FREE, JUMPING, FALLING, HIT_BY_HAZARD, HIT_HEAD, STUNNED };
    void changeState(PLAYER_STATE new_state);
    void updateState(float dt);
    
    // Gameplay
    void updateDirection();
    void checkInteractions();
    
    // Render
    virtual void changeAnimations();
    
// Variables
    // State
    PLAYER_STATE m_state;
    
    // Timers
    float m_timer;
    float m_stun_timer;
    
    // Constants
    const float m_move_time;
    const float m_hit_head_time;
    const float m_hit_by_hazard_time;
    const float m_stun_time;
    const float m_hazard_hit_stun_time;
    const float m_long_stun_time;
    
    // Render
    int m_last_facing;
    float m_facing_timer;
    const float m_facing_interval;
}; 

#endif /* Player_hpp */
