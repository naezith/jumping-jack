#ifndef Entity_hpp
#define Entity_hpp

#include "Library/AnimatedSprite.hpp"

class Entity : public AnimatedSprite {
public:
    Entity();
    Entity(bool changes_floor_on_edge, float collision_box_x = 20);
    
    // Global
    virtual void update(float dt);
    void render(sf::RenderTarget& target);
    
    // Gameplay
    static const int PICK_RANDOMLY = 1337;
    void spawn(bool random_position, const std::string& name, int direction = PICK_RANDOMLY);
    bool collides(int floor, float x);
    
protected:
// Functions
    // Gameplay
    void moveUp(bool allow_top_climb = false);
    void moveDown();
    
    virtual int getLowestFloor();
    virtual int getSpawnFloor();

    virtual void updateFacing(float dt);
    
    // Render
    virtual void changeAnimations();
    virtual void drawSelf(sf::RenderTarget& target);
    
// Variables
    // Gameplay
    int m_direction;
    int m_floor;
    const float m_collision_size_x;
    std::string m_sprite_name;
    
    // Render
    int m_facing;
    float m_draw_offset_y;
    sf::Color m_sprite_color;
    
private:
// Functions
    // Render
    void setPositionX(float x);
    void setPositionY(float y);
    
// Variables
    // Gameplay
    const float m_movement_speed;
    const bool m_changes_floor_on_edge;
    
    // Render
    const float m_scale;
};

#endif /* Entity_hpp */
