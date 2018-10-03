#ifndef ANIMATEDSPRITE_INCLUDE
#define ANIMATEDSPRITE_INCLUDE

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Vector2.hpp>

#include <vector>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <iostream>

class Animation {
public:
    Animation() : m_texture(nullptr) {}
    void addFrame(sf::IntRect rect) {
        m_frames.push_back(rect);
    }
    void setSpriteSheet(const sf::Texture& texture) {
        m_texture = &texture;
    }
    const sf::Texture* getSpriteSheet() const {
        return m_texture;
    }
    std::size_t getSize() const {
        return m_frames.size();
    }
    const sf::IntRect& getFrame(std::size_t n) const {
        return m_frames[n];
    }

private:
    std::vector<sf::IntRect> m_frames;
    const sf::Texture* m_texture;
};

class AnimatedSprite : public sf::Drawable, public sf::Transformable {
public:
    explicit AnimatedSprite(sf::Time frameTime = sf::seconds(0.2f), bool paused = false, bool looped = true);
    virtual ~AnimatedSprite(){};

    void updateAnim(float deltaTime);
    void setAnimation(const Animation& animation);
    void setFrameTime(sf::Time time);
    void setCurrentTime(const sf::Time& currentTime);
    void play();
    void play(const Animation& animation);
    void resetAnim();
    void pause();
    void stop();
    void setLooped(bool looped);
    void setColor(const sf::Color& color);
    const Animation* getAnimation() const;
    sf::FloatRect getLocalBounds() const;
    sf::FloatRect getGlobalBounds() const;
    std::size_t getCurrentFrame() const;
    sf::Time getCurrentTime() const;
    bool isLooped() const;
    bool isPlaying() const;
    sf::Time getFrameTime() const;
    void setFrame(std::size_t newFrame, bool resetTime = true);
    const sf::IntRect& getAnimFrame() const;

private:
    const Animation* m_animation;
    sf::Time m_frameTime;
    sf::Time m_currentTime;
    std::size_t m_currentFrame;
    bool m_isPaused;
    bool m_isLooped;
    const sf::Texture* m_texture;
    sf::Vertex m_vertices[4];

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
};

#endif // ANIMATEDSPRITE_INCLUDE
