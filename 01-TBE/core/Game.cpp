#include <cstring>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Game.h"
#include "SaveSystem.h"
#include "UIRenderer.h"


Game::Game()
{
	bPlay = true;
	std::memset(keys, 0, sizeof(keys));
	std::memset(justPressedKeys_, 0, sizeof(justPressedKeys_));
	std::memset(mouseButtons_, 0, sizeof(mouseButtons_));
	std::memset(justPressedMouse_, 0, sizeof(justPressedMouse_));
	mouseX_ = 0;
	mouseY_ = 0;
	windowW_ = SCREEN_WIDTH;
	windowH_ = SCREEN_HEIGHT;
	mouseMovedThisFrame_ = false;
}

void Game::init()
{
	bPlay = true;
	std::memset(keys, 0, sizeof(keys));
	std::memset(justPressedKeys_, 0, sizeof(justPressedKeys_));
	std::memset(mouseButtons_, 0, sizeof(mouseButtons_));
	std::memset(justPressedMouse_, 0, sizeof(justPressedMouse_));

	glClearColor(0.06f, 0.08f, 0.16f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SaveSystem::instance().init();
	scene.init();
	menus.init(&scene);
}

bool Game::update(int deltaTime)
{
	if(!menus.update(deltaTime))
		bPlay = false;

	// Clear edge-triggered input flags after this update tick has consumed them
	std::memset(justPressedKeys_, 0, sizeof(justPressedKeys_));
	std::memset(justPressedMouse_, 0, sizeof(justPressedMouse_));
	mouseMovedThisFrame_ = false;

	return bPlay;
}

void Game::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GameState st = menus.getState();
	if(st == STATE_PLAYING || st == STATE_PAUSED || st == STATE_WORLD_CLEAR || st == STATE_GAME_OVER ||
	   menus.rendersSceneBehind())
	{
		scene.render();
	}

	menus.render();
}

void Game::keyPressed(int key)
{
	if(key >= 0 && key <= GLFW_KEY_LAST)
	{
		if(!keys[key])
			justPressedKeys_[key] = true;
		keys[key] = true;
	}
}

void Game::keyReleased(int key)
{
	if(key >= 0 && key <= GLFW_KEY_LAST)
		keys[key] = false;
}

void Game::mouseMove(int x, int y)
{
	if(x != mouseX_ || y != mouseY_)
		mouseMovedThisFrame_ = true;
	mouseX_ = x;
	mouseY_ = y;
}

void Game::mousePress(int button)
{
	if(button >= 0 && button < MAX_MOUSE_BUTTONS)
	{
		if(!mouseButtons_[button])
			justPressedMouse_[button] = true;
		mouseButtons_[button] = true;
	}
}

void Game::mouseRelease(int button)
{
	if(button >= 0 && button < MAX_MOUSE_BUTTONS)
		mouseButtons_[button] = false;
}

void Game::windowResized(int width, int height)
{
	if(width > 0 && height > 0)
	{
		windowW_ = width;
		windowH_ = height;
	}
}

bool Game::getKey(int key) const
{
	if(key < 0 || key > GLFW_KEY_LAST)
		return false;
	return keys[key];
}

bool Game::justPressed(int key) const
{
	if(key < 0 || key > GLFW_KEY_LAST)
		return false;
	return justPressedKeys_[key];
}

glm::vec2 Game::getMouseVirtualPos() const
{
	float vx = float(mouseX_) * float(UIRenderer::VIRTUAL_W) / float(windowW_ > 0 ? windowW_ : SCREEN_WIDTH);
	float vy = float(mouseY_) * float(UIRenderer::VIRTUAL_H) / float(windowH_ > 0 ? windowH_ : SCREEN_HEIGHT);
	return glm::vec2(vx, vy);
}

bool Game::mouseJustPressed(int button) const
{
	if(button < 0 || button >= MAX_MOUSE_BUTTONS)
		return false;
	return justPressedMouse_[button];
}
