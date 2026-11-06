#ifndef _MENU_SYSTEM_INCLUDE
#define _MENU_SYSTEM_INCLUDE


#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "UIRenderer.h"
#include "Texture.h"
#include "SaveSystem.h"
#include "PowerUp.h"


class Scene;


// High-level application states (Pocket Bomberman style flow):
//   TITLE -> MAIN_MENU -> PLAY_SUBMENU -> SLOT_SELECT (load / save) -> WORLD_SELECT -> PLAYING
//   PLAYING <-> PAUSED, PLAYING -> WORLD_CLEAR -> WORLD_SELECT, PLAYING -> GAME_OVER
enum GameState
{
	STATE_TITLE,
	STATE_MAIN_MENU,
	STATE_PLAY_SUBMENU,
	STATE_CONTROLS,
	STATE_CREDITS,
	STATE_SLOT_SELECT,
	STATE_WORLD_SELECT,
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_WORLD_CLEAR,
	STATE_GAME_OVER
};


// MenuSystem drives the whole game flow: menus, save slots, world map, HUD,
// pause menu and result screens. Gameplay itself is delegated to Scene.
class MenuSystem
{

public:
	MenuSystem();

	void init(Scene *scene);
	bool update(int deltaTime); // Returns false when the player chooses to quit
	void render();

	GameState getState() const { return state_; }
	bool rendersSceneBehind() const;

private:
	enum SlotMode { SLOT_NEW, SLOT_LOAD, SLOT_SAVE };
	enum PendingAction { ACTION_NONE, ACTION_START_WORLD, ACTION_RESTART_WORLD, ACTION_NEXT_WORLD };
	enum ConfirmAction { CONFIRM_NONE, CONFIRM_OVERWRITE_NEW, CONFIRM_OVERWRITE_SAVE, CONFIRM_DELETE };

	// Input helpers (edge-triggered)
	bool pressedConfirm() const;
	bool pressedBack() const;
	bool pressedUp() const;
	bool pressedDown() const;
	bool pressedLeft() const;
	bool pressedRight() const;
	bool pressedPause() const;
	bool mouseInRect(const glm::vec2 &pos, const glm::vec2 &size) const;

	// State management
	void changeState(GameState newState, PendingAction action = ACTION_NONE);
	void enterState(GameState newState);
	void openSlotScreen(SlotMode mode, GameState returnTo);
	void openConfirm(ConfirmAction action, const std::string &title, const std::string &msg);
	void resolveConfirm(bool accepted);
	void showToast(const std::string &msg, float durationMs = 2000.0f);

	// Per-state logic
	void updateTitle(int deltaTime);
	void updateMainMenu(int deltaTime);
	void updatePlaySubmenu(int deltaTime);
	void updateControls(int deltaTime);
	void updateCredits(int deltaTime);
	void updateSlotSelect(int deltaTime);
	void updateWorldSelect(int deltaTime);
	void updatePlaying(int deltaTime);
	void updatePaused(int deltaTime);
	void updateWorldClear(int deltaTime);
	void updateGameOver(int deltaTime);
	void updateConfirm();

	// Game flow actions
	void startNewGame(int slot);
	void loadGame(int slot);
	void saveGameToSlot(int slot);
	void openDirectWorldSelect();
	void startWorld(bool restart);
	void onWorldCleared(int worldId);
	bool isWorldUnlocked(int worldId) const;
	void beginTravel(int fromWorld, int toWorld);

	// Rendering
	void renderTitleBackground(float dim);
	void renderLogo(float x, float y, float width);
	void renderTitle();
	void renderMainMenu();
	void renderPlaySubmenu();
	void renderControls();
	void renderCredits();
	void renderSlotSelect();
	void renderSlotCard(int slot, const glm::vec2 &pos, const glm::vec2 &size, bool selected);
	void renderWorldSelect();
	void renderWorldPaths();
	void renderWorldNodes();
	void renderWorldInfoPanel();
	void renderHUD();
	void renderIntroBanner();
	void renderPickupBanner();
	void renderPause();
	void renderWorldClear();
	void renderGameOver();
	void renderConfirm();
	void renderToast();
	void renderMenuList(const std::vector<std::string> &items, const std::vector<bool> &enabled,
	                    int selected, float centerX, float startY, float spacing, float rowWidth = 164.0f);
	int hitTestMenuList(int itemCount, float centerX, float startY, float spacing, float rowWidth = 164.0f) const;
	void renderStatsRow(const glm::vec2 &pos, int lives, int bombs, int range, int speed);
	void renderBomberman(const glm::vec2 &feetCenter, bool walking, bool faceRight, float scale);
	void renderBombCursor(const glm::vec2 &pos, float size = 12.0f);
	void renderHint(const std::string &hint);

	std::vector<glm::vec2> buildPath(int fromWorld, int toWorld) const;
	static std::string formatTime(float ms);
	static std::string intToStr(int v);

private:
	Scene *scene_;
	UIRenderer ui_;
	Texture titleBg_, logo_, worldBg_;

	GameState state_, pendingState_, slotReturnState_;
	PendingAction pendingAction_;
	float fade_;
	int fadeDir_;       // +1 fading out, -1 fading in, 0 idle
	float time_;        // Global time (ms)
	float stateTime_;   // Time since entering the current state (ms)
	bool quit_;

	int mainSel_;
	int playSubSel_;
	int slotSel_;       // 0..2 = slots, 3 = VOLVER button
	SlotMode slotMode_;
	int pauseSel_;
	int clearSel_;
	int overSel_;
	float shakeTimer_;  // Invalid action feedback
	std::string toast_;
	float toastTimer_;

	// Current save file (-1 when playing in Free World Select without a chosen slot yet)
	int currentSlot_;
	bool freePlayMode_;
	SaveSlotData save_;
	SaveSlotData levelStartSnapshot_;

	// World map
	int worldSel_;
	glm::vec2 cursorPos_;
	std::vector<glm::vec2> travelPath_;
	float travelDist_, travelLen_;
	bool traveling_;
	bool travelFaceRight_;

	// Level intro / powerup pickup / results
	float introTimer_;
	float pickupTimer_;
	PowerUpType pickupType_;
	bool clearUnlockedNew_;
	bool clearGameComplete_;
	int clearScore_;
	float clearTime_;
	int clearedWorld_;

	// Confirmation dialog
	bool confirmOpen_;
	ConfirmAction confirmAction_;
	std::string confirmTitle_, confirmMsg_;
	int confirmSel_; // 0 = SI, 1 = NO

};


#endif // _MENU_SYSTEM_INCLUDE
