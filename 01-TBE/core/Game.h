#ifndef _GAME_INCLUDE
#define _GAME_INCLUDE


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Scene.h"
#include "MenuSystem.h"


#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 720


// Game is a singleton (a class with a single instance) that represents our whole application
class Game
{

private:
	Game();

public:
	static Game &instance()
	{
		static Game G;
		return G;
	}

	void init();
	bool update(int deltaTime);
	void render();

	// Input callback methods
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMove(int x, int y);
	void mousePress(int button);
	void mouseRelease(int button);
	void windowResized(int width, int height);

	bool getKey(int key) const;
	bool justPressed(int key) const;

	glm::vec2 getMouseVirtualPos() const;
	bool didMouseMove() const { return mouseMovedThisFrame_; }
	bool mouseJustPressed(int button) const;
	void quit() { bPlay = false; }

private:
	static const int MAX_MOUSE_BUTTONS = 8;

	bool bPlay; // Continue to play game?
	bool keys[GLFW_KEY_LAST + 1];
	bool justPressedKeys_[GLFW_KEY_LAST + 1];
	bool mouseButtons_[MAX_MOUSE_BUTTONS];
	bool justPressedMouse_[MAX_MOUSE_BUTTONS];
	int mouseX_, mouseY_;
	int windowW_, windowH_;
	bool mouseMovedThisFrame_;

	Scene scene;
	MenuSystem menus;

};


#endif // _GAME_INCLUDE
