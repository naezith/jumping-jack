#include "Hole.hpp"
#include <SFML/Graphics.hpp>

#include "Game.hpp"

// Render model, same for all holes
sf::RectangleShape Hole::m_rect;

Hole::Hole() : Entity(true, 72) {
    m_draw_offset_y = -Game::i().getFloorHeight();
    m_rect.setSize(sf::Vector2f(m_collision_size_x, Game::i().getTileHeight()));
    m_rect.setOrigin(m_rect.getSize().x*0.5f, 0);
    m_rect.setFillColor(sf::Color::Black);
}

int Hole::getLowestFloor() { return Game::i().getBottomFloor(); }

void Hole::drawSelf(sf::RenderTarget& target) {
    // Draw a black rectangle
    m_rect.setPosition(getPosition());
    target.draw(m_rect, sf::BlendAlpha);
}
