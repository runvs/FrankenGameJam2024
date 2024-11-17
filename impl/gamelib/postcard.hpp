
#ifndef POSTCARD_HPP
#define POSTCARD_HPP

#include "animation.hpp"
#include <game_object.hpp>

class Postcard : public jt::GameObject {
public:
    void doCreate() override;

    void doUpdate(float const elapsed) override;
    void doDraw() const override;

    std::shared_ptr<jt::Animation> m_animation;
    bool picked { false };
};

#endif // POSTCARD_HPP
