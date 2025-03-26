#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <iostream>

class Animation {
private:
    sf::Sprite m_sprite;
    std::vector<sf::IntRect> m_frames;
    int m_currentFrame  = 0;

    // frames per second (FPS) (not ms/frame)
    int   m_speed       = 0;      
    float m_elapsedTime = 0.f;  
    bool  m_repeat      = true;
    std::string m_name;

public:
    Animation() = default;

    Animation(const sf::Texture& texture, 
              int frameWidth, 
              int frameHeight, 
              int frameCount, 
              int speed,      // frames per second
              bool repeat = true, 
              const std::string& name = "")
        : m_speed(speed), m_repeat(repeat), m_name(name)
    {
        m_sprite.setTexture(texture);

        // Fill out the vector of frames (all on one row)
        for (int i = 0; i < frameCount; ++i) {
            m_frames.emplace_back(i * frameWidth, 0, frameWidth, frameHeight);
        }

        if (!m_frames.empty()) {
            m_sprite.setTextureRect(m_frames[0]);
        }

        std::cout << "[DEBUG] Loaded Animation: " << m_name 
                  << " with " << frameCount << " frames.\n";
    }

    // interpret 'speed' as frames/second ---
    void update(float deltaTime) {
        if (m_frames.empty() || m_speed <= 0) {
            return;
        }

        // Accumulate elapsed time in *seconds*
        m_elapsedTime += deltaTime;

        // Each frame lasts 1 / m_speed seconds
        float frameDuration = 1.f / static_cast<float>(m_speed);

        // If enough time passed to go to the next frame:
        while (m_elapsedTime >= frameDuration) {
            m_elapsedTime -= frameDuration;

            if (m_repeat) {
                m_currentFrame = (m_currentFrame + 1) % m_frames.size();
            } else {
                // Move until the last frame, then stop
                if (m_currentFrame < static_cast<int>(m_frames.size()) - 1) {
                    m_currentFrame++;
                }
            }

            m_sprite.setTextureRect(m_frames[m_currentFrame]);
            //std::cout << "[DEBUG] Animation Updated: " << m_name 
            //          << " Frame: " << m_currentFrame << "\n";
        }
    }

    void reset() { 
        m_currentFrame = 0; 
        m_elapsedTime  = 0.f;
        if (!m_frames.empty()) {
            m_sprite.setTextureRect(m_frames[0]);
        }
    }

    void setFrame(int frame) {
        if (!m_frames.empty()) {
            m_currentFrame = std::clamp(frame, 0, (int)m_frames.size() - 1);
            m_sprite.setTextureRect(m_frames[m_currentFrame]);
        }
    }

    void setSpeed(int speed)       { m_speed = speed;  }  // frames/second
    int getSpeed()           const { return m_speed;   }
    int getFrameCount()      const { return (int)m_frames.size(); }
    int getCurrentFrame()    const { return m_currentFrame; }
    bool hasEnded()          const { return !m_repeat && m_currentFrame == (int)m_frames.size()-1; }

    // Accessors for drawing
    const sf::Sprite& getSprite()  const { return m_sprite; }
    sf::Sprite& getMutableSprite()       { return m_sprite; }

    // Misc
    const std::string& getName() const { return m_name; }
    sf::Vector2i getSize() const {
        return !m_frames.empty()
            ? sf::Vector2i(m_frames[0].width, m_frames[0].height)
            : sf::Vector2i(0, 0);
    }
};
