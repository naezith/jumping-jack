#ifndef Hole_hpp
#define Hole_hpp

#include <SFML/Graphics/RectangleShape.hpp>

#include "Entity.hpp"

class Hole : public Entity {
public:
    Hole();

protected:
    // Gameplay
    virtual int getLowestFloor();

    // Render
    virtual void drawSelf(sf::RenderTarget& target);
    
private:
    // Render
    static sf::RectangleShape m_rect;
};

#endif /* Hole_hpp */
