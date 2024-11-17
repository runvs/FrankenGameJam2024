
#include "postcard.hpp"
#include "random/random.hpp"

void Postcard::doCreate()
{
    m_animation = std::make_shared<jt::Animation>();
    m_animation->loadFromAseprite("assets/postcards.aseprite", textureManager());

    m_animation->play(std::to_string(jt::Random::getInt(0, 4)));

    m_animation->setOrigin(jt::OriginMode::CENTER);
}

void Postcard::doUpdate(float const elapsed) { m_animation->update(elapsed); }

void Postcard::doDraw() const { m_animation->draw(renderTarget()); }
