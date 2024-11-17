#include "state_menu.hpp"
#include <build_info.hpp>
#include <color/color.hpp>
#include <drawable_helpers.hpp>
#include <game_interface.hpp>
#include <game_properties.hpp>
#include <input/input_manager.hpp>
#include <lerp.hpp>
#include <log/license_info.hpp>
#include <screeneffects/vignette.hpp>
#include <state_box2d.hpp>
#include <state_manager/state_manager_transition_fade_to_black.hpp>
#include <text.hpp>
#include <tweens/tween_alpha.hpp>
#include <tweens/tween_color.hpp>
#include <tweens/tween_position.hpp>
#include <algorithm>

void StateMenu::onCreate()
{
    createMenuText();
    createShapes();
    createVignette();

    add(std::make_shared<jt::LicenseInfo>());

    getGame()->stateManager().setTransition(std::make_shared<jt::StateManagerTransitionFadeToBlack>(
        GP::GetScreenSize(), textureManager()));

    try {
        auto bgm = getGame()->audio().getPermanentSound("bgm");
        if (bgm == nullptr) {
            bgm = getGame()->audio().addPermanentSound("bgm", "event:/main-theme");
            bgm->play();
        }
    } catch (std::exception const& e) {
        getGame()->logger().error(e.what(), { "menu", "bgm" });
    }
}

void StateMenu::onEnter()
{
    createTweens();
    m_started = false;
}

void StateMenu::createVignette()
{
    m_vignette = std::make_shared<jt::Vignette>(GP::GetScreenSize());
    add(m_vignette);
}

void StateMenu::createShapes()
{
    m_background = jt::dh::createShapeRect(
        GP::GetScreenSize(), jt::Color { 70u, 63u, 68u, 255u }, textureManager());
    m_overlay = jt::dh::createShapeRect(GP::GetScreenSize(), jt::colors::Black, textureManager());
}

void StateMenu::createMenuText()
{
    createTextTitle();
    createTextStart();
    createTextExplanation();
    createTextCredits();
}

void StateMenu::createTextExplanation()
{
    m_textExplanation = jt::dh::createText(
        renderTarget(), GP::ExplanationText(), 16u, jt::Color { 245u, 203u, 92u, 255 });
    auto const half_width = GP::GetScreenSize().x / 2.0f;
    m_textExplanation->setPosition({ half_width, 100 });
    m_textExplanation->setShadow(jt::Color { 51u, 53u, 51u, 255u }, jt::Vector2f { 2, 2 });
}

void StateMenu::createTextCredits()
{
    m_textCredits = jt::dh::createText(renderTarget(),
        "Created by " + GP::AuthorName() + " for " + GP::JamName() + "\n" + GP::JamDate()
            + "\nF9 for License Information",
        14u, jt::Color { 47u, 184u, 218u, 255u });
    m_textCredits->setTextAlign(jt::Text::TextAlign::LEFT);
    m_textCredits->setPosition({ 10, GP::GetScreenSize().y - 70 });
    m_textCredits->setShadow({ 51u, 53u, 51u, 255u }, jt::Vector2f { 1, 1 });

    m_textVersion = jt::dh::createText(renderTarget(), "", 14u, { 47u, 184u, 218u, 255u });
    if (jt::BuildInfo::gitTagName() != "") {
        m_textVersion->setText(jt::BuildInfo::gitTagName());
    } else {
        m_textVersion->setText(
            jt::BuildInfo::gitCommitHash().substr(0, 6) + " " + jt::BuildInfo::timestamp());
    }
    m_textVersion->setTextAlign(jt::Text::TextAlign::RIGHT);
    m_textVersion->setPosition({ GP::GetScreenSize().x - 5.0f, GP::GetScreenSize().y - 20.0f });
    m_textVersion->setShadow(GP::PaletteFontShadow(), jt::Vector2f { 1, 1 });
}

void StateMenu::createTextStart()
{
    auto const half_width = GP::GetScreenSize().x / 2.0f;
    m_textStart = jt::dh::createText(renderTarget(), "", 16u, GP::PaletteFontFront());
    m_textStart->setPosition({ half_width, 70 });
    m_textStart->setShadow(GP::PaletteFontShadow(), jt::Vector2f { 2, 2 });
}

void StateMenu::createTextTitle()
{
    m_title = std::make_shared<jt::Sprite>("assets/title_2.png", textureManager());
    m_title->setOrigin(jt::OriginMode::CENTER);
    m_title->setPosition(GP::GetScreenSize() * 0.5f + jt::Vector2f { 0.0f, -60.0f });
}

void StateMenu::createTweens()
{
    createTweenOverlayAlpha();
    createTweenTitleAlpha();
    createTweenCreditsPosition();
    createTweenExplanation();
}

void StateMenu::createInstructionTweenColor1()
{
    auto const tc = jt::TweenColor::create(
        m_textStart, 0.5f, GP::PaletteFontFront(), GP::PalleteFrontHighlight());
    tc->addCompleteCallback([this]() { createInstructionTweenColor2(); });
    tc->setAgePercentConversion(
        [](float age) { return jt::Lerp::cubic(0.0f, 1.0f, std::clamp(age, 0.0f, 1.0f)); });
    add(tc);
}

void StateMenu::createInstructionTweenColor2()
{
    auto const tc = jt::TweenColor::create(
        m_textStart, 0.45f, GP::PalleteFrontHighlight(), GP::PaletteFontFront());
    tc->setAgePercentConversion(
        [](float age) { return jt::Lerp::cubic(0.0f, 1.0f, std::clamp(age, 0.0f, 1.0f)); });
    tc->setStartDelay(0.2f);
    tc->addCompleteCallback([this]() { createInstructionTweenColor1(); });
    add(tc);
}

void StateMenu::createTweenExplanation()
{
    auto const start = m_textStart->getPosition() + jt::Vector2f { -1000, 0 };
    auto const end = m_textStart->getPosition();

    auto const tween = jt::TweenPosition::create(m_textStart, 0.5f, start, end);
    tween->setStartDelay(0.3f);
    tween->setSkipTicks();

    tween->addCompleteCallback([this]() { createInstructionTweenColor1(); });
    add(tween);
}

void StateMenu::createTweenTitleAlpha()
{
    auto const tween = jt::TweenAlpha::create(m_title, 0.55f, 0, 255);
    tween->setStartDelay(0.2f);
    tween->setSkipTicks();
    add(tween);
}

void StateMenu::createTweenOverlayAlpha()
{
    auto const tween
        = jt::TweenAlpha::create(m_overlay, 0.5f, std::uint8_t { 255 }, std::uint8_t { 0 });
    tween->setSkipTicks();
    add(tween);
}

void StateMenu::createTweenCreditsPosition()
{
    auto const creditsPositionStart = m_textCredits->getPosition() + jt::Vector2f { 0, 150 };
    auto const creditsPositionEnd = m_textCredits->getPosition();

    auto const tweenCredits
        = jt::TweenPosition::create(m_textCredits, 0.75f, creditsPositionStart, creditsPositionEnd);
    tweenCredits->setStartDelay(0.8f);
    tweenCredits->setSkipTicks();
    add(tweenCredits);

    auto const versionPositionStart = m_textVersion->getPosition() + jt::Vector2f { 0, 150 };
    auto const versionPositionEnd = m_textVersion->getPosition();
    auto const tweenVersion
        = jt::TweenPosition::create(m_textVersion, 0.75f, versionPositionStart, versionPositionEnd);
    tweenVersion->setStartDelay(0.8f);
    tweenVersion->setSkipTicks();
    add(tweenVersion);
}

void StateMenu::onUpdate(float const elapsed)
{
    updateDrawables(elapsed);
    checkForTransitionToStateGame();
}

void StateMenu::updateDrawables(float const& elapsed)
{
    m_background->update(elapsed);
    m_title->update(elapsed);
    m_textStart->update(elapsed);
    m_textExplanation->update(elapsed);
    m_textCredits->update(elapsed);
    m_textVersion->update(elapsed);
    m_overlay->update(elapsed);
    m_vignette->update(elapsed);
}

void StateMenu::checkForTransitionToStateGame()
{
    auto const keysToTriggerTransition = { jt::KeyCode::Space, jt::KeyCode::Enter };

    if (std::any_of(std::begin(keysToTriggerTransition), std::end(keysToTriggerTransition),
            [this](auto const k) { return getGame()->input().keyboard()->justPressed(k); })
        || getGame()->input().gamepad(0)->justPressed(jt::GamepadButtonCode::GBStart)
        || getGame()->input().gamepad(1)->justPressed(jt::GamepadButtonCode::GBStart)) {
        startTransitionToStateGame();
    }
}

void StateMenu::startTransitionToStateGame()
{
    if (!m_started) {
        m_started = true;
        getGame()->stateManager().storeCurrentState("menu");
        getGame()->stateManager().switchState(std::make_shared<StatePlatformer>());

        auto const startSound = getGame()->audio().addTemporarySound("event:/start-game");
        startSound->play();
    }
}

void StateMenu::onDraw() const
{
    m_background->draw(renderTarget());

    m_title->draw(renderTarget());
    m_textStart->draw(renderTarget());
    m_textExplanation->draw(renderTarget());
    m_textCredits->draw(renderTarget());
    m_textVersion->draw(renderTarget());
    m_overlay->draw(renderTarget());
    m_vignette->draw();
}

std::string StateMenu::getName() const { return "State Menu"; }
