
#include "contact_callback_player_enemy.hpp"
#include <user_data_entries.hpp>

#include <iostream>

void ContactCallbackPlayerEnemy::setPlayer(std::weak_ptr<Player> player) { m_player = player; }

void ContactCallbackPlayerEnemy::setEnabled(bool enabled) { m_enabled = enabled; }

bool ContactCallbackPlayerEnemy::getEnabled() const { return m_enabled; }

void ContactCallbackPlayerEnemy::onBeginContact(b2Contact* contact)
{
    auto p = m_player.lock();
    if (!p) {
        return;
    }
    if (p->isRespawnRequested()) {
        return;
    }

    auto const fa = contact->GetFixtureA();
    auto const fb = contact->GetFixtureB();

    bool optionA = isPlayerFixture(fa) && isEnemyFixture(fb);
    bool optionB = isPlayerFixture(fb) && isEnemyFixture(fa);

    if (optionA || optionB) {
        p->setRequestRespawn(true);
    }
}

void ContactCallbackPlayerEnemy::onEndContact(b2Contact* /*contact*/) { }

bool ContactCallbackPlayerEnemy::isPlayerFixture(b2Fixture* fa) const
{
    void* fixtureUserData = fa->GetUserData();
    return (std::uint64_t)fixtureUserData == g_userDataPlayerID + m_player.lock()->getPlayerId();
}

bool ContactCallbackPlayerEnemy::isEnemyFixture(b2Fixture* fa) const
{
    void* fixtureUserData = fa->GetUserData();
    return (std::uint64_t)fixtureUserData == g_userDataEnemyID;
}
