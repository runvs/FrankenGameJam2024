#include "platform_player.hpp"
#include <conversions.hpp>
#include <game_interface.hpp>
#include <line.hpp>
#include <math_helper.hpp>
#include <user_data_entries.hpp>

Player::Player(std::shared_ptr<jt::Box2DWorldInterface> world)
{
    b2BodyDef bodyDef;
    bodyDef.fixedRotation = true;
    bodyDef.type = b2_dynamicBody;

    m_physicsObject = std::make_shared<jt::Box2DObject>(world, &bodyDef);
}

void Player::doCreate()
{
    m_animation = std::make_shared<jt::Animation>();
    m_animation->loadFromAseprite(
        m_playerId == 0 ? "assets/player.aseprite" : "assets/Player2.aseprite", textureManager());
    m_animation->play("idle");
    m_animation->setOrigin({ 2, 4 });

    b2FixtureDef fixtureDef;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.5f;
    b2CircleShape circleCollider {};
    circleCollider.m_radius = 4.0f;
    fixtureDef.shape = &circleCollider;
    auto playerCollider = m_physicsObject->getB2Body()->CreateFixture(&fixtureDef);
    playerCollider->SetUserData((void*)(g_userDataPlayerID + m_playerId));

    fixtureDef.isSensor = true;

    b2PolygonShape polygonShape;

    polygonShape.SetAsBox(3.0f, 0.2f, b2Vec2(0, 4), 0);
    fixtureDef.shape = &polygonShape;
    m_footSensorFixture = m_physicsObject->getB2Body()->CreateFixture(&fixtureDef);
}

void Player::updateGravity(jt::Vector2f const& currentPosition)
{
    constexpr auto worldOffset = jt::Vector2f { 100.0, 100.0 } * 8;

    m_gravityDirection = currentPosition - worldOffset;
    jt::MathHelper::normalizeMe(m_gravityDirection);
    m_gravityDirection *= -1;

    m_gravityGizmo = std::make_shared<jt::Line>(m_gravityDirection * 12);
    m_gravityGizmo->setColor({ 255, 255, 255, 100 });
    m_gravityGizmo->setPosition(currentPosition + jt::Vector2f { 0, 0 });
    m_gravityGizmo->update(0.0f);

    constexpr auto gravityStrength = 20000.0f;
    m_physicsObject->getB2Body()->ApplyForceToCenter(
        { m_gravityDirection.x * gravityStrength, m_gravityDirection.y * gravityStrength }, true);
}

std::shared_ptr<jt::Animation> Player::getAnimation() { return m_animation; }

void Player::doUpdate(float const elapsed)
{
    m_physicsObject->getB2Body()->DestroyFixture(m_footSensorFixture);
    b2FixtureDef fixtureDef;
    fixtureDef.isSensor = true;
    b2PolygonShape polygonShape;

    auto const rotDeg = -jt::MathHelper::angleOf(m_gravityDirection) - 90;
    auto const halfAxis = jt::MathHelper::rotateBy(jt::Vector2f { 3.0f, 0.2f }, rotDeg);
    auto const center = jt::MathHelper::rotateBy(jt::Vector2f { 0, 4 }, rotDeg);

    polygonShape.SetAsBox(halfAxis.x, halfAxis.y, jt::Conversion::vec(center), 0);
    fixtureDef.shape = &polygonShape;
    m_footSensorFixture = m_physicsObject->getB2Body()->CreateFixture(&fixtureDef);
    m_footSensorFixture->SetUserData((void*)(g_userDataPlayerFeetID + m_playerId));

    auto currentPosition = m_physicsObject->getPosition();
    clampPositionToLevelSize(currentPosition);
    m_physicsObject->setPosition(currentPosition);

    m_animation->setPosition(currentPosition);
    updateGravity(currentPosition);
    updateAnimation(elapsed);
    handleMovement(elapsed);

    m_wasTouchingGroundLastFrame = m_isTouchingGround;

    m_lastTouchedGroundTimer -= elapsed;
    m_lastJumpTimer -= elapsed;
    m_wantsToJumpTimer -= elapsed;
}

void Player::clampPositionToLevelSize(jt::Vector2f& currentPosition) const
{
    if (currentPosition.x < 4) {
        currentPosition.x = 4;
    }
    if (currentPosition.x > m_levelSizeInTiles.x - 4) {
        currentPosition.x = m_levelSizeInTiles.x - 4;
    }
}

void Player::updateAnimation(float elapsed)
{
    auto const rotDeg
        = (static_cast<int>(-jt::MathHelper::angleOf(m_gravityDirection)) - 90 + 360) % 360;
    if (rotDeg < 45) {
        m_animation->setRotation(0);
    } else if (rotDeg < 135) {
        m_animation->setRotation(90);
    } else if (rotDeg < 225) {
        m_animation->setRotation(180);
    } else if (rotDeg < 315) {
        m_animation->setRotation(270);
    } else {
        m_animation->setRotation(0);
    }

    auto const rotated_velocity = jt::MathHelper::rotateBy(m_physicsObject->getVelocity(), rotDeg);

    if (rotated_velocity.x > 0) {
        m_animation->play("left");
        m_isMoving = true;
    } else if (rotated_velocity.x < 0) {
        m_animation->play("right");
        m_isMoving = true;
    } else {
        m_isMoving = false;
    }
    auto const v = m_horizontalMovement ? abs(rotated_velocity.x) / 90.0f : 0.0f;
    m_animation->setAnimationSpeedFactor(v);
    m_animation->update(elapsed);

    m_walkParticlesTimer -= elapsed * v;
    if (m_walkParticlesTimer <= 0) {
        m_walkParticlesTimer = 0.15f;
        if (m_isMoving && m_isTouchingGround) {
            auto ps = m_walkParticles.lock();
            if (ps) {
                ps->fire(1, getPosition());
            }
        }
    }
}

InputState Player::queryInput()
{
    InputState result = {};
    auto const keyboard = getGame()->input().keyboard();
    if (m_playerId == 0) {
        result.isLeftPressed = keyboard->pressed(jt::KeyCode::A);
        result.isRightPressed = keyboard->pressed(jt::KeyCode::D);
        result.isUpPressed = keyboard->pressed(jt::KeyCode::W);
        result.isDownPressed = keyboard->pressed(jt::KeyCode::S);
        result.isJumpJustPressed = keyboard->justPressed(jt::KeyCode::Space);
        result.isJumpPressed = keyboard->pressed(jt::KeyCode::Space);
        result.isRespawnRequested = keyboard->pressed(jt::KeyCode::R);
    }
    if (m_playerId == 1) {
        result.isLeftPressed
            = keyboard->pressed(jt::KeyCode::J) || keyboard->pressed(jt::KeyCode::Left);
        result.isRightPressed
            = keyboard->pressed(jt::KeyCode::L) || keyboard->pressed(jt::KeyCode::Right);
        result.isUpPressed
            = keyboard->pressed(jt::KeyCode::I) || keyboard->pressed(jt::KeyCode::Up);
        result.isDownPressed
            = keyboard->pressed(jt::KeyCode::K) || keyboard->pressed(jt::KeyCode::Down);
        result.isJumpJustPressed = keyboard->justPressed(jt::KeyCode::RShift);
        result.isJumpPressed = keyboard->pressed(jt::KeyCode::RShift);
        result.isRespawnRequested = keyboard->pressed(jt::KeyCode::P);
    }

    auto const gamepad = getGame()->input().gamepad(m_playerId);
    result.isJumpJustPressed |= gamepad->justPressed(jt::GamepadButtonCode::GBA);
    result.isJumpPressed |= gamepad->pressed(jt::GamepadButtonCode::GBA);
    result.isRespawnRequested |= gamepad->justPressed(jt::GamepadButtonCode::GBX);

    return result;
}

void Player::handleMovement(float const elapsed)
{
    auto const horizontalAcceleration = 15000.0f;
    auto const maxHorizontalVelocity = 250.0f;

    auto const jumpInitialVelocity = -250.0f;
    auto const maxVerticalVelocity = maxHorizontalVelocity;

    auto const jumpDeadTime = 0.3f;
    auto const preLandJumpTimeFrame = 0.1f;

    auto b2b = getB2Body();

    m_horizontalMovement = false;

    auto degreesToHorizontalRotation = jt::MathHelper::angleOf(m_gravityDirection) + 90.0f;
    auto localRightAxis = jt::MathHelper::rotateBy(m_gravityDirection, -90.0f);

    auto v_rotated
        = jt::MathHelper::rotateBy(m_physicsObject->getVelocity(), degreesToHorizontalRotation);

    auto inputAxis = getGame()->input().gamepad(m_playerId)->getAxis(jt::GamepadAxisCode::ALeft);

    auto inputState = queryInput();
    if (inputState.isRespawnRequested) {
        setRequestRespawn(true);
    }

    if (inputState.isRightPressed) {
        inputAxis.x += 1;
    }
    if (inputState.isLeftPressed) {
        inputAxis.x -= 1;
    }
    if (inputState.isUpPressed) {
        inputAxis.y -= 1;
    }
    if (inputState.isDownPressed) {
        inputAxis.y += 1;
    }

    jt::MathHelper::normalizeMe(inputAxis);
    if (inputAxis.x > 0) {
        m_horizontalMovement = true;
    }
    if (inputAxis.x < 0) {
        m_horizontalMovement = true;
    }

    auto constexpr inputDeadZone = 0.2;
    if (jt::MathHelper::length(inputAxis) > inputDeadZone) {
        auto movement_strength = jt::MathHelper::dot(inputAxis, localRightAxis);

        b2b->ApplyForceToCenter(
            b2Vec2 { localRightAxis.x * movement_strength * horizontalAcceleration,
                localRightAxis.y * movement_strength * horizontalAcceleration },
            true);
    }

    if (inputState.isJumpPressed) {
        if (m_wantsToJumpTimer <= 0.0f) {
            m_wantsToJumpTimer = preLandJumpTimeFrame;
        }
    }

    if (m_wantsToJumpTimer >= 0.0f) {
        if (canJump()) {
            m_lastJumpTimer = jumpDeadTime;
            v_rotated.y = jumpInitialVelocity;

            auto const count = m_wasTouchingGroundLastFrame ? 10 : 25;
            auto ps = m_postJumpParticles.lock();
            if (ps) {
                ps->fire(count, getPosition() + jt::Vector2f { 0.0f, 5.0f });
            }
        }
    }

    // Jump
    m_soundTimerJump -= elapsed;
    if (inputState.isJumpJustPressed) {
        if (v_rotated.y < 0) {
            if (m_soundTimerJump < 0.0f) {
                m_soundTimerJump = 0.25f;
                auto const jumpSound = getGame()->audio().addTemporarySound(
                    "event:/jump-p" + std::to_string(m_playerId + 1));
                jumpSound->play();
            }
            b2b->ApplyForceToCenter(b2Vec2 { -m_gravityDirection.x, -m_gravityDirection.y }, true);
        }
    }

    // clamp velocity
    if (v_rotated.y >= maxVerticalVelocity) {
        v_rotated.y = maxVerticalVelocity;
    } else if (v_rotated.y <= -maxVerticalVelocity) {
        v_rotated.y = -maxVerticalVelocity;
    }

    if (v_rotated.x >= maxHorizontalVelocity) {
        v_rotated.x = maxHorizontalVelocity;
    } else if (v_rotated.x <= -maxHorizontalVelocity) {
        v_rotated.x = -maxHorizontalVelocity;
    }

    auto v = jt::MathHelper::rotateBy(v_rotated, -degreesToHorizontalRotation);
    v = v * 0.99f;
    m_physicsObject->setVelocity(v);

    ///////////// sound
    auto const l = jt::MathHelper::lengthSquared(v);
    if (m_playerId == 0) { }
    if (l > 50.0f) {
        m_soundTimerWalk -= elapsed;
    }
    if (m_soundTimerWalk <= 0.0f) {
        // TODO select good time
        m_soundTimerWalk = 0.25f;

        if (canJump()) {
            auto const walkSound = getGame()->audio().addTemporarySound(
                "event:/walking-p" + std::to_string(m_playerId + 1));
            walkSound->play();
        }
    }
}

b2Body* Player::getB2Body() { return m_physicsObject->getB2Body(); }

void Player::doDraw() const
{
    m_animation->draw(renderTarget());
    m_gravityGizmo->draw(renderTarget());
}

void Player::setTouchesGround(bool touchingGround)
{
    auto const m_postDropJumpTimeFrame = 0.2f;
    m_isTouchingGround = touchingGround;
    if (m_isTouchingGround) {
        m_lastTouchedGroundTimer = m_postDropJumpTimeFrame;
    }
}

jt::Vector2f Player::getPosOnScreen() const { return m_animation->getScreenPosition(); }

void Player::setPosition(jt::Vector2f const& pos) { m_physicsObject->setPosition(pos); }

jt::Vector2f Player::getPosition() const { return m_physicsObject->getPosition(); }

jt::Vector2f Player::getGravityDirection() const { return m_gravityDirection; }

void Player::setWalkParticleSystem(std::weak_ptr<jt::ParticleSystem<jt::Shape, 50>> ps)
{
    m_walkParticles = ps;
}

void Player::setJumpParticleSystem(std::weak_ptr<jt::ParticleSystem<jt::Shape, 50>> ps)
{
    m_postJumpParticles = ps;
}

void Player::setLevelSize(jt::Vector2f const& levelSizeInTiles)
{
    m_levelSizeInTiles = levelSizeInTiles;
}

void Player::setPlayerId(int playerId) { m_playerId = playerId; }

int Player::getPlayerId() const { return m_playerId; }

void Player::resetVelocity() const { m_physicsObject->setVelocity({ 0, 0 }); }

void Player::setRequestRespawn(bool value, bool reason)
{
    m_isRespawnRequested = value;
    if (reason) {
        auto const deathSound = getGame()->audio().addTemporarySound(
            "event:/death-p" + std::to_string(m_playerId + 1));
        deathSound->play();
    }
}

bool Player::isRespawnRequested() const { return m_isRespawnRequested; }

bool Player::canJump() const
{
    if (m_lastJumpTimer >= 0.0f) {
        return false;
    }
    if (m_isTouchingGround) {
        return true;
    }
    if (m_lastTouchedGroundTimer > 0) {
        return true;
    }
    return false;
}
