#ifndef JAMTEMPLATE_LEVEL_HPP
#define JAMTEMPLATE_LEVEL_HPP

#include "postcard.hpp"
#include <bee.hpp>
#include <box2dwrapper/box2d_object.hpp>
#include <box2dwrapper/box2d_world_interface.hpp>
#include <exit.hpp>
#include <game_object.hpp>
#include <killbox.hpp>
#include <moving_platform.hpp>
#include <shape.hpp>
#include <tilemap/tile_layer.hpp>
#include <tilemap/tileson_loader.hpp>
#include <functional>

class Level : public jt::GameObject {
public:
    Level(std::string const& fileName, std::weak_ptr<jt::Box2DWorldInterface> world);
    jt::Vector2f getPlayerStart() const;

    void checkIfPlayerIsInKillbox(
        jt::Vector2f const& playerPosition, std::function<void()> callback) const;

    void checkIfPlayerIsInPostcard(jt::Vector2f const& playerPosition,
        std::function<void(std::shared_ptr<Postcard>)> callback);

    void checkIfPlayerIsInExit(
        jt::Vector2f const& playerPosition, std::function<void(std::string const&)> callback);

    jt::Vector2f getLevelSizeInPixel() const;

private:
    void doCreate() override;
    void doUpdate(float const elapsed) override;
    void doDraw() const override;

    std::shared_ptr<jt::Shape> m_background { nullptr };
    std::string m_fileName { "" };
    std::weak_ptr<jt::Box2DWorldInterface> m_world {};

    std::vector<std::shared_ptr<jt::Box2DObject>> m_colliders {};
    std::shared_ptr<jt::tilemap::TileLayer> m_tileLayerGround { nullptr };
    jt::Vector2f m_playerStart { 0.0f, 0.0f };

    // TODO create enemy interface/base class
    std::vector<std::shared_ptr<Bee>> m_bees {};

    std::vector<std::shared_ptr<Killbox>> m_killboxes {};
    std::vector<Exit> m_exits {};
    std::vector<std::shared_ptr<Postcard>> m_postcards {};
    jt::Vector2f m_levelSizeInPixel { 0.0f, 0.0f };

    std::vector<std::shared_ptr<MovingPlatform>> m_movingPlatforms {};

    void loadLevelSettings(jt::tilemap::TilesonLoader& loader);
    void loadLevelTileLayer(jt::tilemap::TilesonLoader& loader);
    void loadLevelCollisions(jt::tilemap::TilesonLoader& loader);
    void loadLevelKillboxes(jt::tilemap::TilesonLoader& loader);
    void loadLevelPostcards(jt::tilemap::TilesonLoader& loader);
    void loadLevelSize(jt::tilemap::TilesonLoader const& loader);
    void loadMovingPlatforms(jt::tilemap::TilesonLoader& loader);
};

#endif // JAMTEMPLATE_LEVEL_HPP
