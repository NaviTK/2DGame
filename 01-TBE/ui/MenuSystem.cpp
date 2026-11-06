#include <cmath>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "MenuSystem.h"
#include "Scene.h"
#include "Game.h"


// UTF-8 constants decoded by PixelFont::decodeUtf8ToGlyphs (immune to source codepage)
#define UTF8_STAR   "\xE2\x98\x85"
#define UTF8_HEART  "\xE2\x99\xA5"
#define UTF8_ARR_R  "\xE2\x96\xBA"
#define UTF8_ARR_L  "\xE2\x97\x84"
#define UTF8_INV_EX "\xC2\xA1"
#define UTF8_INV_QM "\xC2\xBF"
#define UTF8_A_AC   "\xC3\x81"
#define UTF8_E_AC   "\xC3\x89"
#define UTF8_I_AC   "\xC3\x8D"
#define UTF8_O_AC   "\xC3\x93"
#define UTF8_U_AC   "\xC3\x9A"
#define UTF8_N_TLD  "\xC3\x91"


namespace
{
	static const glm::vec4 COL_WHITE(0.95f, 0.97f, 1.0f, 1.0f);
	static const glm::vec4 COL_GOLD(1.0f, 0.88f, 0.25f, 1.0f);
	static const glm::vec4 COL_CREAM(1.0f, 0.95f, 0.78f, 1.0f);
	static const glm::vec4 COL_SKY(0.55f, 0.84f, 1.0f, 1.0f);
	static const glm::vec4 COL_GREEN(0.32f, 0.92f, 0.45f, 1.0f);
	static const glm::vec4 COL_RED(1.0f, 0.34f, 0.36f, 1.0f);
	static const glm::vec4 COL_MUTED(0.56f, 0.62f, 0.74f, 1.0f);
	static const glm::vec4 COL_PANEL_BG(0.06f, 0.10f, 0.22f, 0.92f);
	static const glm::vec4 COL_PANEL_BORDER(1.0f, 0.82f, 0.25f, 1.0f);
}


MenuSystem::MenuSystem()
{
	scene_ = NULL;
	state_ = STATE_TITLE;
	pendingState_ = STATE_TITLE;
	slotReturnState_ = STATE_MAIN_MENU;
	pendingAction_ = ACTION_NONE;
	fade_ = 0.0f;
	fadeDir_ = 0;
	time_ = 0.0f;
	stateTime_ = 0.0f;
	quit_ = false;

	mainSel_ = 0;
	slotSel_ = 0;
	slotMode_ = SLOT_NEW;
	pauseSel_ = 0;
	clearSel_ = 0;
	overSel_ = 0;
	shakeTimer_ = 0.0f;
	toastTimer_ = 0.0f;

	currentSlot_ = -1;
	freePlayMode_ = false;

	worldSel_ = 1;
	cursorPos_ = glm::vec2(92.0f, 154.0f);
	travelDist_ = 0.0f;
	travelLen_ = 0.0f;
	traveling_ = false;
	travelFaceRight_ = true;

	introTimer_ = 0.0f;
	pickupTimer_ = 0.0f;
	pickupType_ = EXTRA_BOMB;
	clearUnlockedNew_ = false;
	clearGameComplete_ = false;
	clearScore_ = 0;
	clearTime_ = 0.0f;
	clearedWorld_ = 1;

	confirmOpen_ = false;
	confirmAction_ = CONFIRM_NONE;
	confirmSel_ = 1;
}

void MenuSystem::init(Scene *scene)
{
	scene_ = scene;
	ui_.init(scene_->getTexProgram(), scene_->getSimpleProgram());

	titleBg_.loadFromFile("images/menu_title_bg.png", TEXTURE_PIXEL_FORMAT_RGBA);
	titleBg_.setWrapS(GL_CLAMP_TO_EDGE);
	titleBg_.setWrapT(GL_CLAMP_TO_EDGE);
	titleBg_.setMinFilter(GL_NEAREST);
	titleBg_.setMagFilter(GL_NEAREST);

	logo_.loadFromFile("images/menu_logo.png", TEXTURE_PIXEL_FORMAT_RGBA);
	logo_.setWrapS(GL_CLAMP_TO_EDGE);
	logo_.setWrapT(GL_CLAMP_TO_EDGE);
	logo_.setMinFilter(GL_NEAREST);
	logo_.setMagFilter(GL_NEAREST);

	worldBg_.loadFromFile("images/menu_world_bg.png", TEXTURE_PIXEL_FORMAT_RGBA);
	worldBg_.setWrapS(GL_CLAMP_TO_EDGE);
	worldBg_.setWrapT(GL_CLAMP_TO_EDGE);
	worldBg_.setMinFilter(GL_NEAREST);
	worldBg_.setMagFilter(GL_NEAREST);

	const WorldDef *w1 = SaveSystem::instance().getWorldById(1);
	if(w1 != NULL)
		cursorPos_ = w1->mapNodePos;

	state_ = STATE_TITLE;
	fade_ = 1.0f;
	fadeDir_ = -1;
}

bool MenuSystem::rendersSceneBehind() const
{
	return state_ == STATE_SLOT_SELECT && slotReturnState_ == STATE_PAUSED;
}

// ---------------------------------------------------------------------------
// Input helpers
// ---------------------------------------------------------------------------

bool MenuSystem::pressedConfirm() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_ENTER) || g.justPressed(GLFW_KEY_KP_ENTER) || g.justPressed(GLFW_KEY_SPACE);
}

bool MenuSystem::pressedBack() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_ESCAPE) || g.mouseJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
}

bool MenuSystem::pressedUp() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_UP) || g.justPressed(GLFW_KEY_W);
}

bool MenuSystem::pressedDown() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_DOWN) || g.justPressed(GLFW_KEY_S);
}

bool MenuSystem::pressedLeft() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_LEFT) || g.justPressed(GLFW_KEY_A);
}

bool MenuSystem::pressedRight() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_RIGHT) || g.justPressed(GLFW_KEY_D);
}

bool MenuSystem::pressedPause() const
{
	const Game &g = Game::instance();
	return g.justPressed(GLFW_KEY_ESCAPE) || g.justPressed(GLFW_KEY_P);
}

bool MenuSystem::mouseInRect(const glm::vec2 &pos, const glm::vec2 &size) const
{
	glm::vec2 m = Game::instance().getMouseVirtualPos();
	return m.x >= pos.x && m.x <= pos.x + size.x &&
	       m.y >= pos.y && m.y <= pos.y + size.y;
}

// ---------------------------------------------------------------------------
// State transitions & modals
// ---------------------------------------------------------------------------

void MenuSystem::changeState(GameState newState, PendingAction action)
{
	if(fadeDir_ == 1)
		return;
	pendingState_ = newState;
	pendingAction_ = action;
	fadeDir_ = 1;
}

void MenuSystem::enterState(GameState newState)
{
	if(pendingAction_ == ACTION_START_WORLD)
		startWorld(false);
	else if(pendingAction_ == ACTION_RESTART_WORLD)
		startWorld(true);
	else if(pendingAction_ == ACTION_NEXT_WORLD)
	{
		worldSel_ = std::min(clearedWorld_ + 1, SaveSystem::instance().getWorldCount());
		save_.currentWorld = worldSel_;
		startWorld(false);
	}
	pendingAction_ = ACTION_NONE;

	state_ = newState;
	stateTime_ = 0.0f;
	confirmOpen_ = false;

	if(state_ == STATE_WORLD_SELECT)
	{
		SaveSystem::instance().loadAllSlots();
		const WorldDef *w = SaveSystem::instance().getWorldById(worldSel_);
		if(w != NULL && !traveling_)
			cursorPos_ = w->mapNodePos;
	}
	else if(state_ == STATE_PLAYING && scene_ != NULL)
	{
		scene_->syncPlayerInputLatch();
	}
}

void MenuSystem::openSlotScreen(SlotMode mode, GameState returnTo)
{
	SaveSystem::instance().loadAllSlots();
	slotMode_ = mode;
	slotReturnState_ = returnTo;
	confirmOpen_ = false;

	if(mode == SLOT_NEW)
	{
		slotSel_ = SaveSystem::instance().findFirstFreeOrDefaultSlot();
	}
	else if(mode == SLOT_SAVE)
	{
		slotSel_ = (currentSlot_ >= 0) ? currentSlot_ : SaveSystem::instance().findFirstFreeOrDefaultSlot();
	}
	else // SLOT_LOAD
	{
		slotSel_ = 0;
		for(int i = 0; i < SaveSystem::NUM_SLOTS; i++)
		{
			if(SaveSystem::instance().getSlot(i).occupied)
			{
				slotSel_ = i;
				break;
			}
		}
	}

	if(returnTo == STATE_PAUSED)
	{
		state_ = STATE_SLOT_SELECT;
		stateTime_ = 0.0f;
	}
	else
	{
		changeState(STATE_SLOT_SELECT);
	}
}

void MenuSystem::openConfirm(ConfirmAction action, const std::string &title, const std::string &msg)
{
	confirmOpen_ = true;
	confirmAction_ = action;
	confirmTitle_ = title;
	confirmMsg_ = msg;
	confirmSel_ = (action == CONFIRM_DELETE) ? 1 : 0;
}

void MenuSystem::resolveConfirm(bool accepted)
{
	ConfirmAction act = confirmAction_;
	confirmOpen_ = false;
	confirmAction_ = CONFIRM_NONE;

	if(!accepted)
		return;

	if(act == CONFIRM_OVERWRITE_NEW && slotSel_ >= 0 && slotSel_ < SaveSystem::NUM_SLOTS)
	{
		startNewGame(slotSel_);
	}
	else if(act == CONFIRM_OVERWRITE_SAVE && slotSel_ >= 0 && slotSel_ < SaveSystem::NUM_SLOTS)
	{
		saveGameToSlot(slotSel_);
	}
	else if(act == CONFIRM_DELETE && slotSel_ >= 0 && slotSel_ < SaveSystem::NUM_SLOTS)
	{
		SaveSystem::instance().deleteSlot(slotSel_);
		if(currentSlot_ == slotSel_)
			currentSlot_ = -1;
		showToast("RANURA " + intToStr(slotSel_ + 1) + " BORRADA");
	}
}

void MenuSystem::showToast(const std::string &msg, float durationMs)
{
	toast_ = msg;
	toastTimer_ = durationMs;
}

// ---------------------------------------------------------------------------
// Main update loop
// ---------------------------------------------------------------------------

bool MenuSystem::update(int deltaTime)
{
	float dt = float(std::min(deltaTime, 50));
	time_ += dt;
	stateTime_ += dt;

	if(shakeTimer_ > 0.0f)
		shakeTimer_ = std::max(0.0f, shakeTimer_ - dt);
	if(toastTimer_ > 0.0f)
		toastTimer_ = std::max(0.0f, toastTimer_ - dt);
	if(introTimer_ > 0.0f)
		introTimer_ = std::max(0.0f, introTimer_ - dt);
	if(pickupTimer_ > 0.0f)
		pickupTimer_ = std::max(0.0f, pickupTimer_ - dt);

	// Smooth screen fade transition (fast 160ms fade out / in)
	if(fadeDir_ != 0)
	{
		const float fadeSpeed = 0.0065f;
		fade_ += float(fadeDir_) * fadeSpeed * dt;
		if(fadeDir_ > 0 && fade_ >= 1.0f)
		{
			fade_ = 1.0f;
			enterState(pendingState_);
			fadeDir_ = -1;
		}
		else if(fadeDir_ < 0 && fade_ <= 0.0f)
		{
			fade_ = 0.0f;
			fadeDir_ = 0;
		}
		if(fadeDir_ > 0)
			return !quit_;
	}

	if(confirmOpen_)
	{
		updateConfirm();
		return !quit_;
	}

	switch(state_)
	{
	case STATE_TITLE:        updateTitle(deltaTime);       break;
	case STATE_MAIN_MENU:    updateMainMenu(deltaTime);    break;
	case STATE_CONTROLS:     updateControls(deltaTime);    break;
	case STATE_SLOT_SELECT:  updateSlotSelect(deltaTime);  break;
	case STATE_WORLD_SELECT: updateWorldSelect(deltaTime); break;
	case STATE_PLAYING:      updatePlaying(deltaTime);     break;
	case STATE_PAUSED:       updatePaused(deltaTime);      break;
	case STATE_WORLD_CLEAR:  updateWorldClear(deltaTime);  break;
	case STATE_GAME_OVER:    updateGameOver(deltaTime);    break;
	}

	return !quit_;
}

void MenuSystem::updateConfirm()
{
	if(pressedLeft() || pressedUp())
		confirmSel_ = 0;
	else if(pressedRight() || pressedDown())
		confirmSel_ = 1;

	glm::vec2 yesBox(96.0f, 134.0f), noBox(166.0f, 134.0f), btnSz(58.0f, 18.0f);
	if(Game::instance().didMouseMove())
	{
		if(mouseInRect(yesBox, btnSz)) confirmSel_ = 0;
		else if(mouseInRect(noBox, btnSz)) confirmSel_ = 1;
	}

	if(Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT))
	{
		if(mouseInRect(yesBox, btnSz)) { resolveConfirm(true); return; }
		if(mouseInRect(noBox, btnSz))  { resolveConfirm(false); return; }
	}

	if(pressedConfirm())
		resolveConfirm(confirmSel_ == 0);
	else if(pressedBack())
		resolveConfirm(false);
}

void MenuSystem::updateTitle(int deltaTime)
{
	(void)deltaTime;
	if(Game::instance().justPressed(GLFW_KEY_ESCAPE))
	{
		quit_ = true;
		return;
	}
	if(pressedConfirm() || Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT))
	{
		mainSel_ = 0;
		changeState(STATE_MAIN_MENU);
	}
}

void MenuSystem::updateMainMenu(int deltaTime)
{
	(void)deltaTime;
	const int itemCount = 5;

	if(pressedUp())
		mainSel_ = (mainSel_ + itemCount - 1) % itemCount;
	else if(pressedDown())
		mainSel_ = (mainSel_ + 1) % itemCount;

	int hovered = hitTestMenuList(itemCount, 160.0f, 114.0f, 17.0f, 180.0f);
	if(hovered >= 0 && Game::instance().didMouseMove())
		mainSel_ = hovered;

	bool clicked = (hovered >= 0 && Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT));
	if(clicked)
		mainSel_ = hovered;

	if(pressedConfirm() || clicked)
	{
		switch(mainSel_)
		{
		case 0: // EMPEZAR PARTIDA
			openSlotScreen(SLOT_NEW, STATE_MAIN_MENU);
			break;
		case 1: // CARGAR PARTIDA
			openSlotScreen(SLOT_LOAD, STATE_MAIN_MENU);
			break;
		case 2: // SELECCIONAR MUNDO
			openDirectWorldSelect();
			break;
		case 3: // CONTROLES Y OBJETOS
			changeState(STATE_CONTROLS);
			break;
		case 4: // SALIR
			quit_ = true;
			break;
		}
	}
	else if(pressedBack())
	{
		changeState(STATE_TITLE);
	}
}

void MenuSystem::updateControls(int deltaTime)
{
	(void)deltaTime;
	if(pressedConfirm() || pressedBack() || Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT))
	{
		changeState(STATE_MAIN_MENU);
	}
}

void MenuSystem::updateSlotSelect(int deltaTime)
{
	(void)deltaTime;
	const int totalRows = SaveSystem::NUM_SLOTS + 1; // 0..2 = slots, 3 = VOLVER

	if(pressedUp())
		slotSel_ = (slotSel_ + totalRows - 1) % totalRows;
	else if(pressedDown())
		slotSel_ = (slotSel_ + 1) % totalRows;

	// Mouse hit testing on the 3 slot cards + delete buttons + VOLVER button
	int hoveredSlot = -1;
	int clickedDeleteSlot = -1;
	for(int i = 0; i < SaveSystem::NUM_SLOTS; i++)
	{
		glm::vec2 cardPos(22.0f, 44.0f + float(i) * 50.0f);
		glm::vec2 cardSz(276.0f, 45.0f);
		glm::vec2 delPos(cardPos.x + cardSz.x - 22.0f, cardPos.y + 5.0f);
		glm::vec2 delSz(16.0f, 13.0f);

		if(SaveSystem::instance().getSlot(i).occupied && mouseInRect(delPos, delSz))
		{
			if(Game::instance().didMouseMove())
				slotSel_ = i;
			if(Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT))
				clickedDeleteSlot = i;
		}
		else if(mouseInRect(cardPos, cardSz))
		{
			hoveredSlot = i;
		}
	}

	glm::vec2 backPos(112.0f, 198.0f), backSz(96.0f, 18.0f);
	if(mouseInRect(backPos, backSz))
		hoveredSlot = SaveSystem::NUM_SLOTS;

	if(hoveredSlot >= 0 && Game::instance().didMouseMove())
		slotSel_ = hoveredSlot;

	if(clickedDeleteSlot >= 0)
	{
		slotSel_ = clickedDeleteSlot;
		openConfirm(CONFIRM_DELETE,
		            UTF8_INV_QM "BORRAR RANURA " + intToStr(slotSel_ + 1) + "?",
		            "SE ELIMINAR" UTF8_A_AC " ESTE PROGRESO");
		return;
	}

	// Keyboard shortcut to delete an occupied slot (DELETE / X)
	if(slotSel_ < SaveSystem::NUM_SLOTS && SaveSystem::instance().getSlot(slotSel_).occupied &&
	   (Game::instance().justPressed(GLFW_KEY_DELETE) || Game::instance().justPressed(GLFW_KEY_X)))
	{
		openConfirm(CONFIRM_DELETE,
		            UTF8_INV_QM "BORRAR RANURA " + intToStr(slotSel_ + 1) + "?",
		            "SE ELIMINAR" UTF8_A_AC " ESTE PROGRESO");
		return;
	}

	if(pressedBack())
	{
		if(slotReturnState_ == STATE_PAUSED)
			state_ = STATE_PAUSED;
		else
			changeState(slotReturnState_);
		return;
	}

	bool clicked = (hoveredSlot >= 0 && Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT));
	if(clicked)
		slotSel_ = hoveredSlot;

	if(pressedConfirm() || clicked)
	{
		if(slotSel_ == SaveSystem::NUM_SLOTS) // VOLVER
		{
			if(slotReturnState_ == STATE_PAUSED)
				state_ = STATE_PAUSED;
			else
				changeState(slotReturnState_);
			return;
		}

		const SaveSlotData &slotData = SaveSystem::instance().getSlot(slotSel_);
		if(slotMode_ == SLOT_NEW)
		{
			if(slotData.occupied)
			{
				openConfirm(CONFIRM_OVERWRITE_NEW,
				            UTF8_INV_QM "SOBRESCRIBIR RANURA " + intToStr(slotSel_ + 1) + "?",
				            "EMPEZAR" UTF8_A_AC "S DESDE EL MUNDO 1");
			}
			else
			{
				startNewGame(slotSel_);
			}
		}
		else if(slotMode_ == SLOT_LOAD)
		{
			if(!slotData.occupied)
			{
				shakeTimer_ = 280.0f;
				showToast(UTF8_INV_EX "RANURA VAC" UTF8_I_AC "A! USA EMPEZAR PARTIDA");
			}
			else
			{
				loadGame(slotSel_);
			}
		}
		else if(slotMode_ == SLOT_SAVE)
		{
			if(slotData.occupied && slotSel_ != currentSlot_)
			{
				openConfirm(CONFIRM_OVERWRITE_SAVE,
				            UTF8_INV_QM "SOBRESCRIBIR RANURA " + intToStr(slotSel_ + 1) + "?",
				            "SE REEMPLAZAR" UTF8_A_AC " LA PARTIDA ANTERIOR");
			}
			else
			{
				saveGameToSlot(slotSel_);
			}
		}
	}
}

void MenuSystem::updateWorldSelect(int deltaTime)
{
	float dt = float(std::min(deltaTime, 50));
	int worldCount = SaveSystem::instance().getWorldCount();

	// Advance animated Bomberman walking along island waypoints
	if(traveling_ && travelPath_.size() >= 2)
	{
		const float walkSpeed = 0.14f; // virtual px / ms
		travelDist_ += walkSpeed * dt;
		if(travelDist_ >= travelLen_)
		{
			cursorPos_ = travelPath_.back();
			traveling_ = false;
		}
		else
		{
			float rem = travelDist_;
			for(size_t i = 1; i < travelPath_.size(); i++)
			{
				glm::vec2 seg = travelPath_[i] - travelPath_[i - 1];
				float segLen = glm::length(seg);
				if(rem <= segLen && segLen > 0.001f)
				{
					cursorPos_ = travelPath_[i - 1] + seg * (rem / segLen);
					if(std::fabs(seg.x) > 0.5f)
						travelFaceRight_ = (seg.x >= 0.0f);
					break;
				}
				rem -= segLen;
			}
		}
	}

	// Keyboard movement between world nodes
	if((pressedLeft() || pressedUp()) && worldSel_ > 1)
	{
		beginTravel(worldSel_, worldSel_ - 1);
	}
	else if((pressedRight() || pressedDown()) && worldSel_ < worldCount)
	{
		beginTravel(worldSel_, worldSel_ + 1);
	}

	// 'U' unlocks all worlds in the current session for quick testing
	if(Game::instance().justPressed(GLFW_KEY_U))
	{
		save_.highestUnlockedWorld = worldCount;
		showToast(UTF8_INV_EX "TODOS LOS MUNDOS DESBLOQUEADOS!");
	}

	// 'G' opens manual save slot picker
	if(Game::instance().justPressed(GLFW_KEY_G))
	{
		save_.currentWorld = worldSel_;
		openSlotScreen(SLOT_SAVE, STATE_WORLD_SELECT);
		return;
	}

	// Mouse click on island world nodes or bottom JUGAR button
	bool launchRequested = pressedConfirm();
	if(Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT))
	{
		const std::vector<WorldDef> &worlds = SaveSystem::instance().getWorlds();
		glm::vec2 m = Game::instance().getMouseVirtualPos();
		for(size_t i = 0; i < worlds.size(); i++)
		{
			if(glm::length(m - worlds[i].mapNodePos) <= 18.0f)
			{
				if(worlds[i].id == worldSel_)
					launchRequested = true;
				else
					beginTravel(worldSel_, worlds[i].id);
				break;
			}
		}

		// Clickable [JUGAR] button on the bottom world card
		if(mouseInRect(glm::vec2(238.0f, 191.0f), glm::vec2(62.0f, 20.0f)))
			launchRequested = true;
	}

	if(launchRequested)
	{
		if(!isWorldUnlocked(worldSel_))
		{
			shakeTimer_ = 280.0f;
			showToast(UTF8_INV_EX "SUPERA EL MUNDO " + intToStr(worldSel_ - 1) + " PRIMERO! (U: LIBRE)");
		}
		else
		{
			traveling_ = false;
			save_.currentWorld = worldSel_;
			if(currentSlot_ >= 0)
			{
				save_.timestamp = SaveSystem::buildCurrentTimestamp();
				SaveSystem::instance().saveSlot(currentSlot_, save_);
			}
			changeState(STATE_PLAYING, ACTION_START_WORLD);
		}
	}
	else if(pressedBack())
	{
		changeState(STATE_MAIN_MENU);
	}
}

void MenuSystem::updatePlaying(int deltaTime)
{
	// Pause via ESC, P, or clicking the [II] pause button in the top-right HUD
	bool clickPause = Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
	                  mouseInRect(glm::vec2(298.0f, 2.0f), glm::vec2(19.0f, 14.0f));
	if(pressedPause() || clickPause)
	{
		pauseSel_ = 0;
		state_ = STATE_PAUSED;
		stateTime_ = 0.0f;
		return;
	}

	// Quick level testing shortcuts ('1', '2', '3', 'R')
	if(Game::instance().justPressed(GLFW_KEY_1))
	{
		worldSel_ = 1;
		startWorld(false);
		return;
	}
	if(Game::instance().justPressed(GLFW_KEY_2) && SaveSystem::instance().getWorldCount() >= 2)
	{
		worldSel_ = 2;
		startWorld(false);
		return;
	}
	if(Game::instance().justPressed(GLFW_KEY_3) && SaveSystem::instance().getWorldCount() >= 3)
	{
		worldSel_ = 3;
		startWorld(false);
		return;
	}
	if(Game::instance().justPressed(GLFW_KEY_R))
	{
		startWorld(true);
		return;
	}

	scene_->update(deltaTime);

	PowerUpType picked;
	if(scene_->consumePickedPowerUp(picked))
	{
		pickupType_ = picked;
		pickupTimer_ = 1900.0f;
	}

	int clearedWorld = 0;
	if(scene_->consumeWorldClearedEvent(clearedWorld))
	{
		onWorldCleared(clearedWorld);
		return;
	}

	if(scene_->isGameOver())
	{
		overSel_ = 0;
		state_ = STATE_GAME_OVER;
		stateTime_ = 0.0f;
	}
}

void MenuSystem::updatePaused(int deltaTime)
{
	(void)deltaTime;
	const int itemCount = 6;

	if(pressedPause())
	{
		state_ = STATE_PLAYING;
		scene_->syncPlayerInputLatch();
		return;
	}

	if(pressedUp())
		pauseSel_ = (pauseSel_ + itemCount - 1) % itemCount;
	else if(pressedDown())
		pauseSel_ = (pauseSel_ + 1) % itemCount;

	int hovered = hitTestMenuList(itemCount, 160.0f, 89.0f, 16.0f, 168.0f);
	if(hovered >= 0 && Game::instance().didMouseMove())
		pauseSel_ = hovered;

	bool clicked = (hovered >= 0 && Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT));
	if(clicked)
		pauseSel_ = hovered;

	if(pressedConfirm() || clicked)
	{
		switch(pauseSel_)
		{
		case 0: // CONTINUAR
			state_ = STATE_PLAYING;
			scene_->syncPlayerInputLatch();
			break;
		case 1: // REINICIAR MUNDO
			changeState(STATE_PLAYING, ACTION_RESTART_WORLD);
			break;
		case 2: // GUARDAR PARTIDA
			scene_->exportSaveData(save_);
			openSlotScreen(SLOT_SAVE, STATE_PAUSED);
			break;
		case 3: // CARGAR PARTIDA
			openSlotScreen(SLOT_LOAD, STATE_PAUSED);
			break;
		case 4: // SELECCIONAR MUNDO
			scene_->exportSaveData(save_);
			worldSel_ = scene_->getCurrentLevel();
			changeState(STATE_WORLD_SELECT);
			break;
		case 5: // MENU PRINCIPAL
			changeState(STATE_MAIN_MENU);
			break;
		}
	}
}

void MenuSystem::updateWorldClear(int deltaTime)
{
	(void)deltaTime;
	bool hasNext = (clearedWorld_ < SaveSystem::instance().getWorldCount());
	int itemCount = hasNext ? 4 : 3;

	if(pressedUp())
		clearSel_ = (clearSel_ + itemCount - 1) % itemCount;
	else if(pressedDown())
		clearSel_ = (clearSel_ + 1) % itemCount;

	int hovered = hitTestMenuList(itemCount, 160.0f, 124.0f, 16.0f, 172.0f);
	if(hovered >= 0 && Game::instance().didMouseMove())
		clearSel_ = hovered;

	bool clicked = (hovered >= 0 && Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT));
	if(clicked)
		clearSel_ = hovered;

	if(pressedConfirm() || clicked)
	{
		int actionIdx = hasNext ? clearSel_ : (clearSel_ + 1);
		switch(actionIdx)
		{
		case 0: // SIGUIENTE MUNDO
			changeState(STATE_PLAYING, ACTION_NEXT_WORLD);
			break;
		case 1: // MAPA DE MUNDOS
			if(hasNext)
				worldSel_ = clearedWorld_ + 1;
			changeState(STATE_WORLD_SELECT);
			break;
		case 2: // REPETIR MUNDO
			worldSel_ = clearedWorld_;
			changeState(STATE_PLAYING, ACTION_START_WORLD);
			break;
		case 3: // MENU PRINCIPAL
			changeState(STATE_MAIN_MENU);
			break;
		}
	}
}

void MenuSystem::updateGameOver(int deltaTime)
{
	(void)deltaTime;
	const int itemCount = 4;

	if(pressedUp())
		overSel_ = (overSel_ + itemCount - 1) % itemCount;
	else if(pressedDown())
		overSel_ = (overSel_ + 1) % itemCount;

	int hovered = hitTestMenuList(itemCount, 160.0f, 118.0f, 16.0f, 172.0f);
	if(hovered >= 0 && Game::instance().didMouseMove())
		overSel_ = hovered;

	bool clicked = (hovered >= 0 && Game::instance().mouseJustPressed(GLFW_MOUSE_BUTTON_LEFT));
	if(clicked)
		overSel_ = hovered;

	if(pressedConfirm() || clicked)
	{
		switch(overSel_)
		{
		case 0: // REINTENTAR MUNDO (fresh 3 lives)
			save_.lives = 3;
			levelStartSnapshot_.lives = 3;
			changeState(STATE_PLAYING, ACTION_RESTART_WORLD);
			break;
		case 1: // MAPA DE MUNDOS
			save_.lives = 3;
			changeState(STATE_WORLD_SELECT);
			break;
		case 2: // CARGAR PARTIDA
			openSlotScreen(SLOT_LOAD, STATE_MAIN_MENU);
			break;
		case 3: // MENU PRINCIPAL
			changeState(STATE_MAIN_MENU);
			break;
		}
	}
}

// ---------------------------------------------------------------------------
// Game flow actions
// ---------------------------------------------------------------------------

void MenuSystem::startNewGame(int slot)
{
	currentSlot_ = slot;
	freePlayMode_ = false;
	save_.resetDefaults();
	save_.occupied = true;
	save_.currentWorld = 1;
	save_.highestUnlockedWorld = 1;
	save_.timestamp = SaveSystem::buildCurrentTimestamp();
	SaveSystem::instance().saveSlot(slot, save_);

	worldSel_ = 1;
	traveling_ = false;
	const WorldDef *w1 = SaveSystem::instance().getWorldById(1);
	if(w1 != NULL)
		cursorPos_ = w1->mapNodePos;

	showToast("NUEVA PARTIDA - RANURA " + intToStr(slot + 1));
	changeState(STATE_WORLD_SELECT);
}

void MenuSystem::loadGame(int slot)
{
	SaveSystem::instance().loadAllSlots();
	const SaveSlotData &loaded = SaveSystem::instance().getSlot(slot);
	if(!loaded.occupied)
		return;

	currentSlot_ = slot;
	freePlayMode_ = false;
	save_ = loaded;
	worldSel_ = save_.currentWorld;
	traveling_ = false;
	const WorldDef *w = SaveSystem::instance().getWorldById(worldSel_);
	if(w != NULL)
		cursorPos_ = w->mapNodePos;

	showToast("PARTIDA CARGADA - RANURA " + intToStr(slot + 1));
	changeState(STATE_WORLD_SELECT);
}

void MenuSystem::saveGameToSlot(int slot)
{
	currentSlot_ = slot;
	freePlayMode_ = false;
	if(slotReturnState_ == STATE_PAUSED && scene_ != NULL)
		scene_->exportSaveData(save_);
	else
		save_.currentWorld = worldSel_;

	save_.occupied = true;
	save_.timestamp = SaveSystem::buildCurrentTimestamp();
	SaveSystem::instance().saveSlot(slot, save_);
	showToast(UTF8_INV_EX "GUARDADO EN RANURA " + intToStr(slot + 1) + "!");

	if(slotReturnState_ == STATE_PAUSED)
		state_ = STATE_PAUSED;
	else
		changeState(slotReturnState_);
}

void MenuSystem::openDirectWorldSelect()
{
	SaveSystem::instance().loadAllSlots();
	if(currentSlot_ < 0)
	{
		// If there is an existing save slot, load the first occupied one;
		// otherwise enable Free Select mode with all worlds unlocked for instant play!
		int found = -1;
		for(int i = 0; i < SaveSystem::NUM_SLOTS; i++)
		{
			if(SaveSystem::instance().getSlot(i).occupied)
			{
				found = i;
				break;
			}
		}
		if(found >= 0)
		{
			currentSlot_ = found;
			freePlayMode_ = false;
			save_ = SaveSystem::instance().getSlot(found);
			worldSel_ = save_.currentWorld;
		}
		else
		{
			freePlayMode_ = true;
			save_.resetDefaults();
			save_.highestUnlockedWorld = SaveSystem::instance().getWorldCount();
			worldSel_ = 1;
		}
	}
	traveling_ = false;
	const WorldDef *w = SaveSystem::instance().getWorldById(worldSel_);
	if(w != NULL)
		cursorPos_ = w->mapNodePos;
	changeState(STATE_WORLD_SELECT);
}

void MenuSystem::startWorld(bool restart)
{
	if(scene_ == NULL)
		return;

	if(restart)
		save_ = levelStartSnapshot_;
	else
	{
		if(save_.lives <= 0)
			save_.lives = 3;
		save_.currentWorld = worldSel_;
		levelStartSnapshot_ = save_;
	}

	scene_->loadLevel(worldSel_, false);
	scene_->applySaveData(save_);
	scene_->syncPlayerInputLatch();
	introTimer_ = 1700.0f;
	pickupTimer_ = 0.0f;
}

void MenuSystem::onWorldCleared(int worldId)
{
	clearedWorld_ = worldId;
	clearScore_ = scene_->getScore();
	clearTime_ = scene_->getElapsedTime();

	scene_->exportSaveData(save_);
	save_.markWorldCompleted(worldId);

	int worldCount = SaveSystem::instance().getWorldCount();
	clearUnlockedNew_ = false;
	if(worldId < worldCount && save_.highestUnlockedWorld < worldId + 1)
	{
		save_.highestUnlockedWorld = worldId + 1;
		clearUnlockedNew_ = true;
	}
	if(worldId < worldCount)
		save_.currentWorld = worldId + 1;

	clearGameComplete_ = (save_.countCompletedWorlds(worldCount) >= worldCount);

	if(currentSlot_ >= 0)
	{
		save_.timestamp = SaveSystem::buildCurrentTimestamp();
		SaveSystem::instance().saveSlot(currentSlot_, save_);
		showToast("AUTOGUARDADO EN RANURA " + intToStr(currentSlot_ + 1));
	}

	clearSel_ = 0;
	state_ = STATE_WORLD_CLEAR;
	stateTime_ = 0.0f;
}

bool MenuSystem::isWorldUnlocked(int worldId) const
{
	if(freePlayMode_ || worldId <= 1)
		return true;
	return worldId <= save_.highestUnlockedWorld;
}

std::vector<glm::vec2> MenuSystem::buildPath(int fromWorld, int toWorld) const
{
	std::vector<glm::vec2> pts;
	const WorldDef *wFrom = SaveSystem::instance().getWorldById(fromWorld);
	const WorldDef *wTo   = SaveSystem::instance().getWorldById(toWorld);
	if(wFrom == NULL || wTo == NULL)
		return pts;

	pts.push_back(wFrom->mapNodePos);
	if(toWorld > fromWorld)
	{
		for(int id = fromWorld + 1; id <= toWorld; id++)
		{
			const WorldDef *w = SaveSystem::instance().getWorldById(id);
			if(w != NULL)
			{
				for(size_t k = 0; k < w->pathFromPrev.size(); k++)
					pts.push_back(w->pathFromPrev[k]);
				pts.push_back(w->mapNodePos);
			}
		}
	}
	else if(toWorld < fromWorld)
	{
		for(int id = fromWorld; id > toWorld; id--)
		{
			const WorldDef *w = SaveSystem::instance().getWorldById(id);
			if(w != NULL)
			{
				for(int k = int(w->pathFromPrev.size()) - 1; k >= 0; k--)
					pts.push_back(w->pathFromPrev[size_t(k)]);
			}
			const WorldDef *prev = SaveSystem::instance().getWorldById(id - 1);
			if(prev != NULL)
				pts.push_back(prev->mapNodePos);
		}
	}
	return pts;
}

void MenuSystem::beginTravel(int fromWorld, int toWorld)
{
	const WorldDef *currentTarget = SaveSystem::instance().getWorldById(fromWorld);
	std::vector<glm::vec2> pts = buildPath(fromWorld, toWorld);
	worldSel_ = toWorld;
	if(pts.size() < 2)
	{
		const WorldDef *dest = SaveSystem::instance().getWorldById(toWorld);
		if(dest != NULL)
			cursorPos_ = dest->mapNodePos;
		traveling_ = false;
		return;
	}

	if(currentTarget != NULL && glm::length(cursorPos_ - pts[0]) > 2.0f)
		pts[0] = cursorPos_;

	travelPath_ = pts;
	travelDist_ = 0.0f;
	travelLen_ = 0.0f;
	for(size_t i = 1; i < travelPath_.size(); i++)
		travelLen_ += glm::length(travelPath_[i] - travelPath_[i - 1]);

	travelFaceRight_ = (toWorld >= fromWorld);
	traveling_ = (travelLen_ > 0.5f);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void MenuSystem::render()
{
	ui_.begin();

	switch(state_)
	{
	case STATE_TITLE:        renderTitle();       break;
	case STATE_MAIN_MENU:    renderMainMenu();    break;
	case STATE_CONTROLS:     renderControls();    break;
	case STATE_SLOT_SELECT:  renderSlotSelect();  break;
	case STATE_WORLD_SELECT: renderWorldSelect(); break;
	case STATE_PLAYING:      renderHUD();         break;
	case STATE_PAUSED:       renderHUD(); renderPause();      break;
	case STATE_WORLD_CLEAR:  renderHUD(); renderWorldClear(); break;
	case STATE_GAME_OVER:    renderHUD(); renderGameOver();   break;
	}

	if(confirmOpen_)
		renderConfirm();

	renderToast();

	if(fade_ > 0.001f)
		ui_.fullscreen(glm::vec4(0.03f, 0.04f, 0.10f, glm::clamp(fade_, 0.0f, 1.0f)));
}

void MenuSystem::renderTitleBackground(float dim)
{
	if(titleBg_.width() > 0)
		ui_.image(titleBg_, glm::vec2(0.0f), glm::vec2(320.0f, 240.0f));
	else
		ui_.fullscreen(glm::vec4(0.16f, 0.65f, 0.95f, 1.0f));

	// Animated Bomberman & friendly rival chasing across the meadow
	float cycle = std::fmod(time_ * 0.048f, 380.0f);
	bool right = (int(time_ * 0.048f / 380.0f) % 2) == 0;
	float bx = right ? (-24.0f + cycle) : (344.0f - cycle);
	float hillY = 212.0f - 5.0f * std::sin((bx / 320.0f) * 3.14159f);
	renderBomberman(glm::vec2(bx, hillY), true, right, 1.3f);

	int eFrame = (int(time_ / 150.0f) % 2 == 0) ? 2 : 36;
	float ex = bx + (right ? -34.0f : 34.0f);
	float ey = 212.0f - 5.0f * std::sin((ex / 320.0f) * 3.14159f) - 18.0f;
	ui_.sheet(glm::vec2(ex - 9.0f, ey), glm::vec2(18.0f, 18.0f), eFrame, 171, right ? -16 : 16, 16);

	if(dim > 0.001f)
		ui_.fullscreen(glm::vec4(0.04f, 0.07f, 0.18f, dim));
}

void MenuSystem::renderLogo(float x, float y, float width)
{
	float bob = std::floor(std::sin(time_ * 0.0032f) * 2.5f + 0.5f);
	float height = std::floor(width * (445.0f / 1036.0f));
	if(logo_.width() > 0)
	{
		ui_.image(logo_, glm::vec2(x, y + bob), glm::vec2(width, height));
	}
	else
	{
		ui_.textCentered("POCKET BOMBERMAN", 160.0f, y + 18.0f + bob, 2.0f, COL_GOLD);
	}
}

void MenuSystem::renderTitle()
{
	renderTitleBackground(0.0f);
	renderLogo(46.0f, 18.0f, 228.0f);

	// Classic blinking arcade prompt pill
	ui_.panel(glm::vec2(54.0f, 148.0f), glm::vec2(212.0f, 30.0f),
	          glm::vec4(0.06f, 0.10f, 0.24f, 0.88f), COL_PANEL_BORDER);
	if((int(time_ / 450.0f) % 2) == 0)
	{
		ui_.textCentered("PULSA ENTER O CLIC", 160.0f, 159.0f, 1.0f, COL_GOLD);
	}

	renderHint("ENTER / ESPACIO: MENU     ESC: SALIR");
}

void MenuSystem::renderMainMenu()
{
	renderTitleBackground(0.16f);
	renderLogo(62.0f, 8.0f, 196.0f);

	ui_.panel(glm::vec2(58.0f, 98.0f), glm::vec2(204.0f, 116.0f), COL_PANEL_BG, COL_PANEL_BORDER);

	std::vector<std::string> items;
	items.push_back("EMPEZAR PARTIDA");
	items.push_back("CARGAR PARTIDA");
	items.push_back("SELECCIONAR MUNDO");
	items.push_back("CONTROLES Y OBJETOS");
	items.push_back("SALIR");
	std::vector<bool> enabled(items.size(), true);

	renderMenuList(items, enabled, mainSel_, 160.0f, 110.0f, 18.0f, 180.0f);
	renderHint("FLECHAS/RAT" UTF8_O_AC "N: MOVER   ENTER: ELEGIR");
}

void MenuSystem::renderControls()
{
	renderTitleBackground(0.55f);

	ui_.panel(glm::vec2(12.0f, 10.0f), glm::vec2(296.0f, 206.0f), COL_PANEL_BG, COL_PANEL_BORDER);
	ui_.textCentered("CONTROLES Y OBJETOS", 160.0f, 19.0f, 1.0f, COL_GOLD);
	ui_.rect(glm::vec2(22.0f, 31.0f), glm::vec2(276.0f, 1.0f), glm::vec4(1.0f, 0.82f, 0.25f, 0.5f));

	// Left column: Controls
	ui_.text("CONTROLES", glm::vec2(24.0f, 38.0f), 1.0f, COL_SKY);
	ui_.text("A/D O " UTF8_ARR_L "/" UTF8_ARR_R ": MOVER", glm::vec2(24.0f, 52.0f), 1.0f, COL_WHITE);
	ui_.text("W O ARRIBA: SALTAR",     glm::vec2(24.0f, 65.0f), 1.0f, COL_WHITE);
	ui_.text("W/S EN LIANA: TREPAR",   glm::vec2(24.0f, 78.0f), 1.0f, COL_WHITE);
	ui_.text("ESPACIO: PONER BOMBA",   glm::vec2(24.0f, 91.0f), 1.0f, COL_WHITE);
	ui_.text("ESC O P: MENU PAUSA",    glm::vec2(24.0f, 104.0f), 1.0f, COL_WHITE);
	ui_.text("H: VER COLISIONES",      glm::vec2(24.0f, 117.0f), 1.0f, COL_MUTED);

	ui_.rect(glm::vec2(22.0f, 132.0f), glm::vec2(128.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.2f));
	ui_.text("TRUCO BOMBERMAN:", glm::vec2(24.0f, 138.0f), 1.0f, COL_GOLD);
	ui_.text("SALTA SOBRE TU BOMBA", glm::vec2(24.0f, 150.0f), 1.0f, COL_CREAM);
	ui_.text("PARA SUBIR MAS ALTO!", glm::vec2(24.0f, 161.0f), 1.0f, COL_CREAM);

	// Vertical divider
	ui_.rect(glm::vec2(158.0f, 36.0f), glm::vec2(1.0f, 144.0f), glm::vec4(1.0f, 0.82f, 0.25f, 0.35f));

	// Right column: Power-Ups with real sprites from caracters.png
	ui_.text("POWER-UPS", glm::vec2(166.0f, 38.0f), 1.0f, COL_SKY);
	const PowerUpType types[6] = { EXTRA_BOMB, BOMB_RANGE, SPEED_UP, EXTRA_LIFE, FULL_FIRE, VEST };
	const char *labels[6] = {
		"+1 BOMBA MAX",
		"+1 RANGO FUEGO",
		"+VELOCIDAD",
		"+1 VIDA EXTRA",
		"FUEGO MAXIMO",
		"CHALECO 10 SEG"
	};
	for(int i = 0; i < 6; i++)
	{
		float ry = 50.0f + float(i) * 21.0f;
		glm::ivec2 src = PowerUp::getSheetCoords(types[i]);
		ui_.sheet(glm::vec2(166.0f, ry), glm::vec2(16.0f, 16.0f), src.x, src.y, 16, 16);
		ui_.text(labels[i], glm::vec2(186.0f, ry + 4.0f), 1.0f, COL_WHITE);
	}

	ui_.panel(glm::vec2(106.0f, 190.0f), glm::vec2(108.0f, 18.0f),
	          glm::vec4(0.14f, 0.22f, 0.44f, 1.0f), COL_GOLD);
	ui_.textCentered("VOLVER", 160.0f, 195.0f, 1.0f, COL_GOLD);

	renderHint("PULSA ENTER, ESC O CLIC PARA VOLVER");
}

void MenuSystem::renderSlotSelect()
{
	if(slotReturnState_ == STATE_PAUSED)
		ui_.fullscreen(glm::vec4(0.03f, 0.05f, 0.14f, 0.80f));
	else
		renderTitleBackground(0.58f);

	std::string header = "CARGAR PARTIDA";
	std::string sub    = "SELECCIONA UNA PARTIDA GUARDADA";
	if(slotMode_ == SLOT_NEW)
	{
		header = "EMPEZAR PARTIDA";
		sub    = "ELIGE UNA RANURA PARA TU AVENTURA";
	}
	else if(slotMode_ == SLOT_SAVE)
	{
		header = "GUARDAR PARTIDA";
		sub    = "ELIGE D" UTF8_O_AC "NDE GUARDAR EL PROGRESO";
	}

	ui_.textCentered(header, 160.0f, 9.0f, 2.0f, COL_GOLD);
	ui_.textCentered(sub, 160.0f, 29.0f, 1.0f, COL_SKY);

	float shakeX = (shakeTimer_ > 0.0f) ? (std::sin(time_ * 0.06f) * 3.0f) : 0.0f;
	for(int i = 0; i < SaveSystem::NUM_SLOTS; i++)
	{
		float ox = (i == slotSel_) ? shakeX : 0.0f;
		glm::vec2 cardPos(22.0f + ox, 44.0f + float(i) * 50.0f);
		renderSlotCard(i, cardPos, glm::vec2(276.0f, 45.0f), slotSel_ == i);
	}

	// Bottom [ VOLVER ] button
	bool backSel = (slotSel_ == SaveSystem::NUM_SLOTS);
	glm::vec2 backPos(112.0f, 198.0f), backSz(96.0f, 18.0f);
	ui_.panel(backPos, backSz,
	          backSel ? glm::vec4(0.18f, 0.28f, 0.56f, 0.96f) : COL_PANEL_BG,
	          backSel ? COL_GOLD : COL_MUTED);
	ui_.textCentered("VOLVER", 160.0f, 203.0f, 1.0f, backSel ? COL_GOLD : COL_WHITE);
	if(backSel)
		renderBombCursor(glm::vec2(backPos.x - 14.0f, backPos.y + 3.0f), 12.0f);

	if(slotSel_ < SaveSystem::NUM_SLOTS && SaveSystem::instance().getSlot(slotSel_).occupied)
		renderHint("ENTER: ELEGIR   SUPR/X: BORRAR   ESC: VOLVER");
	else
		renderHint("ENTER/CLIC: ELEGIR     ESC: VOLVER");
}

void MenuSystem::renderSlotCard(int slot, const glm::vec2 &pos, const glm::vec2 &size, bool selected)
{
	const SaveSlotData &d = SaveSystem::instance().getSlot(slot);
	glm::vec4 border = selected ? COL_GOLD : glm::vec4(0.35f, 0.46f, 0.68f, 1.0f);
	glm::vec4 fill   = selected ? glm::vec4(0.11f, 0.18f, 0.38f, 0.95f) : COL_PANEL_BG;
	ui_.panel(pos, size, fill, border);

	if(selected)
		renderBombCursor(glm::vec2(pos.x - 15.0f, pos.y + 16.0f), 13.0f);

	// Left portrait badge
	glm::vec2 badgePos(pos.x + 7.0f, pos.y + 7.0f);
	glm::vec2 badgeSz(31.0f, 31.0f);
	const WorldDef *wDef = SaveSystem::instance().getWorldById(d.currentWorld);
	glm::vec4 badgeCol = (d.occupied && wDef != NULL) ? wDef->accentColor : glm::vec4(0.22f, 0.26f, 0.36f, 1.0f);
	ui_.rect(badgePos, badgeSz, glm::vec4(0.02f, 0.03f, 0.08f, 1.0f));
	ui_.rect(badgePos + glm::vec2(1.0f), badgeSz - glm::vec2(2.0f), badgeCol * glm::vec4(0.45f, 0.45f, 0.45f, 1.0f));
	ui_.rectOutline(badgePos + glm::vec2(1.0f), badgeSz - glm::vec2(2.0f), badgeCol, 1.0f);

	if(d.occupied)
	{
		renderBomberman(badgePos + glm::vec2(15.5f, 28.0f), selected, true, 1.2f);

		std::string slotTitle = "RANURA " + intToStr(slot + 1);
		if(slot == currentSlot_)
			slotTitle += " [ACTIVA]";
		ui_.text(slotTitle, glm::vec2(pos.x + 44.0f, pos.y + 7.0f), 1.0f, selected ? COL_GOLD : COL_WHITE);

		if(!d.timestamp.empty())
			ui_.textRight(d.timestamp, pos.x + size.x - 26.0f, pos.y + 7.0f, 1.0f, COL_SKY);

		// Small [X] delete button on top-right of occupied cards
		glm::vec2 delPos(pos.x + size.x - 22.0f, pos.y + 5.0f);
		bool delHover = mouseInRect(delPos, glm::vec2(15.0f, 12.0f));
		ui_.rect(delPos, glm::vec2(15.0f, 12.0f), delHover ? COL_RED : glm::vec4(0.42f, 0.14f, 0.18f, 0.9f));
		ui_.textCentered("X", delPos.x + 7.5f, delPos.y + 2.0f, 1.0f, COL_WHITE, false);

		std::string worldLabel = (wDef != NULL) ? (wDef->name + ": " + wDef->themeName) : ("MUNDO " + intToStr(d.currentWorld));
		ui_.text(worldLabel, glm::vec2(pos.x + 44.0f, pos.y + 19.0f), 1.0f, COL_CREAM);

		int totalW = SaveSystem::instance().getWorldCount();
		std::string stars = std::string(UTF8_STAR " ") + intToStr(d.countCompletedWorlds(totalW)) + "/" + intToStr(totalW);
		ui_.textRight(stars, pos.x + size.x - 8.0f, pos.y + 19.0f, 1.0f, COL_GOLD);

		renderStatsRow(glm::vec2(pos.x + 44.0f, pos.y + 30.0f), d.lives, d.maxBombs, d.bombRange, d.speedUps);
	}
	else
	{
		ui_.textCentered("-", badgePos.x + 15.5f, badgePos.y + 11.0f, 1.0f, COL_MUTED);
		ui_.text("RANURA " + intToStr(slot + 1), glm::vec2(pos.x + 44.0f, pos.y + 9.0f), 1.0f, selected ? COL_GOLD : COL_MUTED);
		ui_.text("--- RANURA VAC" UTF8_I_AC "A ---", glm::vec2(pos.x + 44.0f, pos.y + 24.0f), 1.0f, COL_MUTED);
		if(slotMode_ != SLOT_LOAD)
			ui_.textRight("LIBRE", pos.x + size.x - 10.0f, pos.y + 24.0f, 1.0f, COL_GREEN);
	}
}

void MenuSystem::renderWorldSelect()
{
	if(worldBg_.width() > 0)
		ui_.image(worldBg_, glm::vec2(0.0f), glm::vec2(320.0f, 240.0f));
	else
		ui_.fullscreen(glm::vec4(0.10f, 0.52f, 0.88f, 1.0f));

	// Top status bar
	ui_.panel(glm::vec2(6.0f, 4.0f), glm::vec2(308.0f, 22.0f), COL_PANEL_BG, COL_PANEL_BORDER);
	std::string slotTag = (currentSlot_ >= 0) ? ("RANURA " + intToStr(currentSlot_ + 1)) : "MODO LIBRE";
	int totalW = SaveSystem::instance().getWorldCount();
	slotTag += std::string("  " UTF8_STAR) + intToStr(save_.countCompletedWorlds(totalW)) + "/" + intToStr(totalW);
	ui_.text(slotTag, glm::vec2(14.0f, 11.0f), 1.0f, COL_GOLD);
	renderStatsRow(glm::vec2(154.0f, 10.0f), save_.lives, save_.maxBombs, save_.bombRange, save_.speedUps);

	// Island paths, world pedestals, and animated Bomberman avatar
	renderWorldPaths();
	renderWorldNodes();

	float bob = traveling_ ? 0.0f : std::floor(std::sin(time_ * 0.006f) * 1.5f + 0.5f);
	renderBomberman(cursorPos_ + glm::vec2(0.0f, -3.0f + bob), traveling_, travelFaceRight_, 1.25f);

	// Golden pointer triangle floating above Bomberman's head
	float py = cursorPos_.y - 31.0f + std::floor(std::sin(time_ * 0.008f) * 2.0f);
	ui_.rect(glm::vec2(cursorPos_.x - 3.0f, py),     glm::vec2(7.0f, 2.0f), COL_GOLD);
	ui_.rect(glm::vec2(cursorPos_.x - 2.0f, py + 2), glm::vec2(5.0f, 2.0f), COL_GOLD);
	ui_.rect(glm::vec2(cursorPos_.x - 1.0f, py + 4), glm::vec2(3.0f, 2.0f), COL_GOLD);

	renderWorldInfoPanel();
}

void MenuSystem::renderWorldPaths()
{
	const std::vector<WorldDef> &worlds = SaveSystem::instance().getWorlds();
	for(size_t i = 1; i < worlds.size(); i++)
	{
		bool unlocked = isWorldUnlocked(worlds[i].id);
		std::vector<glm::vec2> pts;
		pts.push_back(worlds[i - 1].mapNodePos);
		for(size_t k = 0; k < worlds[i].pathFromPrev.size(); k++)
			pts.push_back(worlds[i].pathFromPrev[k]);
		pts.push_back(worlds[i].mapNodePos);

		// Draw stepping-stone dots every 8 px along the polyline
		int dotIdx = 0;
		for(size_t s = 1; s < pts.size(); s++)
		{
			glm::vec2 diff = pts[s] - pts[s - 1];
			float len = glm::length(diff);
			int steps = std::max(1, int(len / 8.0f));
			for(int k = 1; k < steps; k++, dotIdx++)
			{
				glm::vec2 p = pts[s - 1] + diff * (float(k) / float(steps));
				bool shimmer = unlocked && ((int(time_ / 140.0f) + dotIdx) % 5 == 0);
				glm::vec4 col = !unlocked ? glm::vec4(0.45f, 0.48f, 0.55f, 0.9f)
				                          : (shimmer ? COL_GOLD : COL_CREAM);
				ui_.disc(p, 2.5f, glm::vec4(0.05f, 0.07f, 0.14f, 0.9f));
				ui_.disc(p, 1.6f, col);
			}
		}
	}
}

void MenuSystem::renderWorldNodes()
{
	const std::vector<WorldDef> &worlds = SaveSystem::instance().getWorlds();
	for(size_t i = 0; i < worlds.size(); i++)
	{
		const WorldDef &w = worlds[i];
		bool selected  = (w.id == worldSel_);
		bool unlocked  = isWorldUnlocked(w.id);
		bool completed = save_.isWorldCompleted(w.id);

		float pulse = selected ? (1.5f + 1.0f * std::sin(time_ * 0.008f)) : 0.0f;
		if(selected)
			ui_.disc(w.mapNodePos, 11.0f + pulse, glm::vec4(1.0f, 0.92f, 0.30f, 0.45f));

		ui_.disc(w.mapNodePos + glm::vec2(0.0f, 2.0f), 9.0f, glm::vec4(0.02f, 0.04f, 0.10f, 0.85f));
		glm::vec4 ringCol = !unlocked ? COL_MUTED : (completed ? COL_GOLD : w.accentColor);
		ui_.disc(w.mapNodePos, 8.5f, glm::vec4(0.05f, 0.08f, 0.16f, 1.0f));
		ui_.disc(w.mapNodePos, 7.0f, ringCol);
		ui_.disc(w.mapNodePos, 4.5f, unlocked ? glm::vec4(0.10f, 0.16f, 0.30f, 1.0f)
		                                      : glm::vec4(0.20f, 0.22f, 0.28f, 1.0f));

		// World number badge below the node
		ui_.rect(glm::vec2(w.mapNodePos.x - 10.0f, w.mapNodePos.y + 10.0f),
		         glm::vec2(20.0f, 10.0f), glm::vec4(0.04f, 0.07f, 0.16f, 0.88f));
		ui_.textCentered("M" + intToStr(w.id), w.mapNodePos.x, w.mapNodePos.y + 11.0f, 1.0f,
		                 selected ? COL_GOLD : COL_WHITE);

		if(completed)
			ui_.textCentered(UTF8_STAR, w.mapNodePos.x + 9.0f, w.mapNodePos.y - 12.0f, 1.0f, COL_GOLD);
	}
}

void MenuSystem::renderWorldInfoPanel()
{
	const WorldDef *w = SaveSystem::instance().getWorldById(worldSel_);
	if(w == NULL)
		return;

	bool unlocked  = isWorldUnlocked(w->id);
	bool completed = save_.isWorldCompleted(w->id);
	float shakeX   = (shakeTimer_ > 0.0f) ? (std::sin(time_ * 0.06f) * 3.0f) : 0.0f;

	glm::vec2 pPos(8.0f + shakeX, 182.0f), pSz(304.0f, 54.0f);
	ui_.panel(pPos, pSz, COL_PANEL_BG, unlocked ? w->accentColor : COL_MUTED);

	std::string title = w->name + ": " + w->themeName;
	ui_.text(title, glm::vec2(pPos.x + 10.0f, pPos.y + 7.0f), 1.0f, COL_GOLD);

	if(completed)
		ui_.text(UTF8_STAR " COMPLETADO", glm::vec2(pPos.x + 10.0f, pPos.y + 19.0f), 1.0f, COL_GREEN);
	else if(unlocked)
		ui_.text(UTF8_ARR_R " DISPONIBLE", glm::vec2(pPos.x + 10.0f, pPos.y + 19.0f), 1.0f, COL_SKY);
	else
		ui_.text("BLOQUEADO (SUPERA M" + intToStr(w->id - 1) + ")", glm::vec2(pPos.x + 10.0f, pPos.y + 19.0f), 1.0f, COL_RED);

	// Enemy preview sprites
	ui_.text("FAUNA:", glm::vec2(pPos.x + 132.0f, pPos.y + 19.0f), 1.0f, COL_MUTED);
	int step = (int(time_ / 180.0f) % 2);
	if(w->hasPinkEnemy)
		ui_.sheet(glm::vec2(pPos.x + 172.0f, pPos.y + 14.0f), glm::vec2(14.0f, 15.0f), step ? 30 : 45, 153, 15, 16);
	if(w->hasKirbyEnemy)
		ui_.sheet(glm::vec2(pPos.x + 190.0f, pPos.y + 14.0f), glm::vec2(15.0f, 15.0f), step ? 2 : 36, 171, 16, 16);

	// Clickable [JUGAR] button on the right of the card
	glm::vec2 playBtn(238.0f + shakeX, 191.0f), playSz(62.0f, 20.0f);
	bool btnHover = mouseInRect(playBtn, playSz);
	ui_.panel(playBtn, playSz,
	          unlocked ? (btnHover ? glm::vec4(0.20f, 0.42f, 0.24f, 1.0f) : glm::vec4(0.12f, 0.28f, 0.18f, 1.0f))
	                   : glm::vec4(0.20f, 0.20f, 0.26f, 1.0f),
	          unlocked ? COL_GOLD : COL_MUTED);
	ui_.textCentered(unlocked ? "JUGAR" : "CERRADO", playBtn.x + 31.0f, playBtn.y + 6.0f, 1.0f,
	                 unlocked ? COL_GOLD : COL_MUTED);

	ui_.rect(glm::vec2(pPos.x + 8.0f, pPos.y + 34.0f), glm::vec2(pSz.x - 16.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
	ui_.textCentered(UTF8_ARR_L "/" UTF8_ARR_R ": MOVER   ENTER: JUGAR   G: GUARDAR   ESC: MENU",
	                 160.0f, pPos.y + 39.0f, 1.0f, COL_CREAM);
}

void MenuSystem::renderHUD()
{
	if(scene_ == NULL)
		return;

	const Player &p = scene_->getPlayer();

	// Compact Game Boy Color top HUD bar
	ui_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(320.0f, 18.0f), glm::vec4(0.04f, 0.07f, 0.16f, 0.86f));
	ui_.rect(glm::vec2(0.0f, 18.0f), glm::vec2(320.0f, 1.0f), glm::vec4(1.0f, 0.82f, 0.25f, 0.65f));

	// Separate pixel-art red hearts (one icon per remaining life)
	int lives = std::max(0, p.getLives());
	float heartStartX = 5.0f;
	float heartStep = (lives > 5) ? float(UIRenderer::HEART_W + 1) : float(UIRenderer::HEART_W + 2);
	for(int i = 0; i < lives; i++)
	{
		ui_.heart(glm::vec2(heartStartX + float(i) * heartStep, 4.0f));
	}

	float heartsEnd = (lives > 0)
	                  ? (heartStartX + float(lives - 1) * heartStep + float(UIRenderer::HEART_W))
	                  : heartStartX;

	// Power-up stats (fixed anchor up to 3 hearts so icons stay steady when losing lives)
	float statStep = (lives > 5) ? 23.0f : 26.0f;
	float x = std::max(48.0f, heartsEnd + 6.0f);
	float y = 3.0f;

	glm::ivec2 bSrc = PowerUp::getSheetCoords(EXTRA_BOMB);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), bSrc.x, bSrc.y, 16, 16);
	ui_.text(intToStr(p.getMaxBombs()), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
	x += statStep;

	glm::ivec2 rSrc = PowerUp::getSheetCoords(BOMB_RANGE);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), rSrc.x, rSrc.y, 16, 16);
	ui_.text(intToStr(p.getBombRange()), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
	x += statStep;

	glm::ivec2 sSrc = PowerUp::getSheetCoords(SPEED_UP);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), sSrc.x, sSrc.y, 16, 16);
	ui_.text(intToStr(p.getSpeedUps()), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
	x += 23.0f;

	// Active Vest shield timer pill
	if(p.hasVest())
	{
		int secs = int(std::ceil(p.getVestTimer() / 1000.0f));
		glm::ivec2 vSrc = PowerUp::getSheetCoords(VEST);
		ui_.sheet(glm::vec2(x, 2.0f), glm::vec2(13.0f, 13.0f), vSrc.x, vSrc.y, 16, 16);
		std::string secStr = intToStr(secs) + "S";
		ui_.text(secStr, glm::vec2(x + 15.0f, 5.0f), 1.0f, COL_GOLD);
		x += 15.0f + ui_.textWidth(secStr, 1.0f) + 5.0f;
	}

	// Timer + clickable [II] Pause button
	std::string timeStr = formatTime(scene_->getElapsedTime());
	float timeRightX = 292.0f;
	float timeLeftX = timeRightX - ui_.textWidth(timeStr, 1.0f);
	ui_.textRight(timeStr, timeRightX, 5.0f, 1.0f, COL_WHITE);

	// Enemies left / Exit door alert
	int rem = scene_->getEnemiesRemaining();
	float availRight = timeLeftX - 5.0f;
	if(rem == 0 && scene_->levelHasExitDoor())
	{
		if((int(time_ / 260.0f) % 2) == 0)
		{
			std::string doorMsg = UTF8_INV_EX "VE A LA PUERTA!";
			if(availRight - x < ui_.textWidth(doorMsg, 1.0f))
				doorMsg = UTF8_INV_EX "PUERTA!";
			if(availRight - x < ui_.textWidth(doorMsg, 1.0f))
				doorMsg = "META";
			float cx = std::max(x + ui_.textWidth(doorMsg, 1.0f) * 0.5f,
			                    std::min(200.0f, (x + availRight) * 0.5f));
			ui_.textCentered(doorMsg, cx, 5.0f, 1.0f, COL_GOLD);
		}
	}
	else
	{
		std::string enemStr = "ENEM:" + intToStr(rem);
		if(availRight - x < ui_.textWidth(enemStr, 1.0f))
			enemStr = "E:" + intToStr(rem);
		float enemW = ui_.textWidth(enemStr, 1.0f);
		float enemX = std::min(std::max(176.0f, x), availRight - enemW);
		ui_.text(enemStr, glm::vec2(enemX, 5.0f), 1.0f, COL_SKY);
	}

	glm::vec2 pauseBtn(298.0f, 2.0f), pauseSz(19.0f, 14.0f);
	bool pHover = (state_ == STATE_PLAYING) && mouseInRect(pauseBtn, pauseSz);
	ui_.rect(pauseBtn, pauseSz, pHover ? glm::vec4(0.28f, 0.38f, 0.68f, 1.0f) : glm::vec4(0.14f, 0.20f, 0.38f, 0.95f));
	ui_.rectOutline(pauseBtn, pauseSz, COL_GOLD, 1.0f);
	ui_.textCentered("II", pauseBtn.x + 9.5f, pauseBtn.y + 3.0f, 1.0f, COL_GOLD, false);

	if(state_ == STATE_PLAYING)
	{
		renderIntroBanner();
		renderPickupBanner();
	}
}

void MenuSystem::renderIntroBanner()
{
	if(introTimer_ <= 0.0f)
		return;

	const WorldDef *w = SaveSystem::instance().getWorldById(scene_->getCurrentLevel());
	float alpha = glm::clamp(introTimer_ / 350.0f, 0.0f, 1.0f);
	glm::vec2 bPos(64.0f, 88.0f), bSz(192.0f, 48.0f);
	ui_.panel(bPos, bSz, glm::vec4(0.05f, 0.09f, 0.22f, 0.90f * alpha),
	          glm::vec4(1.0f, 0.84f, 0.25f, alpha));

	std::string header = (w != NULL) ? (w->name + " - " + w->themeName) : ("MUNDO " + intToStr(scene_->getCurrentLevel()));
	ui_.textCentered(header, 160.0f, 98.0f, 1.0f, glm::vec4(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, alpha));
	ui_.textCentered(UTF8_INV_EX "ADELANTE!", 160.0f, 114.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
}

void MenuSystem::renderPickupBanner()
{
	if(pickupTimer_ <= 0.0f)
		return;

	const char *msg = "+MEJORA!";
	switch(pickupType_)
	{
	case EXTRA_BOMB: msg = UTF8_INV_EX "BOMBA EXTRA! (+1 BOMBA)";    break;
	case BOMB_RANGE: msg = UTF8_INV_EX "MAS FUEGO! (+1 RANGO)";      break;
	case SPEED_UP:   msg = UTF8_INV_EX "PATINES! (+VELOCIDAD)";      break;
	case EXTRA_LIFE: msg = UTF8_INV_EX "VIDA EXTRA! (+1 " UTF8_HEART ")"; break;
	case FULL_FIRE:  msg = UTF8_INV_EX "FUEGO MAXIMO!";              break;
	case VEST:       msg = UTF8_INV_EX "CHALECO INVENCIBLE!";        break;
	default: break;
	}

	float w = ui_.textWidth(msg, 1.0f) + 32.0f;
	glm::vec2 bPos(160.0f - w * 0.5f, 24.0f);
	ui_.panel(bPos, glm::vec2(w, 22.0f), glm::vec4(0.06f, 0.12f, 0.28f, 0.92f), COL_GOLD);
	glm::ivec2 src = PowerUp::getSheetCoords(pickupType_);
	ui_.sheet(glm::vec2(bPos.x + 6.0f, bPos.y + 3.0f), glm::vec2(16.0f, 16.0f), src.x, src.y, 16, 16);
	ui_.text(msg, glm::vec2(bPos.x + 25.0f, bPos.y + 7.0f), 1.0f, COL_GOLD);
}

void MenuSystem::renderPause()
{
	ui_.fullscreen(glm::vec4(0.03f, 0.05f, 0.12f, 0.68f));

	glm::vec2 pPos(56.0f, 24.0f), pSz(208.0f, 184.0f);
	ui_.panel(pPos, pSz, COL_PANEL_BG, COL_PANEL_BORDER);

	ui_.textCentered("PAUSA", 160.0f, 33.0f, 2.0f, COL_GOLD);

	const WorldDef *w = SaveSystem::instance().getWorldById(scene_->getCurrentLevel());
	std::string stageStr = (w != NULL) ? (w->name + ": " + w->themeName) : ("MUNDO " + intToStr(scene_->getCurrentLevel()));
	ui_.textCentered(stageStr, 160.0f, 55.0f, 1.0f, COL_SKY);

	const Player &pl = scene_->getPlayer();
	renderStatsRow(glm::vec2(102.0f, 67.0f), pl.getLives(), pl.getMaxBombs(), pl.getBombRange(), pl.getSpeedUps());
	ui_.rect(glm::vec2(pPos.x + 12.0f, 82.0f), glm::vec2(pSz.x - 24.0f, 1.0f), glm::vec4(1.0f, 0.82f, 0.25f, 0.4f));

	std::vector<std::string> items;
	items.push_back("CONTINUAR");
	items.push_back("REINICIAR MUNDO");
	items.push_back("GUARDAR PARTIDA");
	items.push_back("CARGAR PARTIDA");
	items.push_back("SELECCIONAR MUNDO");
	items.push_back("MENU PRINCIPAL");
	std::vector<bool> enabled(items.size(), true);

	renderMenuList(items, enabled, pauseSel_, 160.0f, 89.0f, 16.0f, 168.0f);
	renderHint("ESC / P: CONTINUAR     ENTER: ACEPTAR");
}

void MenuSystem::renderWorldClear()
{
	ui_.fullscreen(glm::vec4(0.04f, 0.08f, 0.18f, 0.72f));

	glm::vec2 pPos(48.0f, 26.0f), pSz(224.0f, 180.0f);
	ui_.panel(pPos, pSz, COL_PANEL_BG, COL_GOLD);

	ui_.textCentered(clearGameComplete_ ? UTF8_INV_EX "JUEGO COMPLETADO!" : UTF8_INV_EX "MUNDO SUPERADO!",
	                 160.0f, 36.0f, 1.0f, COL_GOLD);
	ui_.textCentered("VICTORIA " UTF8_STAR, 160.0f, 50.0f, 2.0f, COL_GREEN);

	ui_.textCentered("PUNTOS: " + intToStr(clearScore_) + "   TIEMPO: " + formatTime(clearTime_),
	                 160.0f, 76.0f, 1.0f, COL_CREAM);

	if(clearUnlockedNew_)
		ui_.textCentered(UTF8_INV_EX "MUNDO " + intToStr(clearedWorld_ + 1) + " DESBLOQUEADO!", 160.0f, 90.0f, 1.0f, COL_SKY);
	else if(currentSlot_ >= 0)
		ui_.textCentered("PROGRESO GUARDADO EN RANURA " + intToStr(currentSlot_ + 1), 160.0f, 90.0f, 1.0f, COL_SKY);

	// Celebratory jumping Bomberman
	int jumpOff = int(std::fabs(std::sin(time_ * 0.008f)) * 6.0f);
	ui_.sheet(glm::vec2(151.0f, 101.0f - float(jumpOff)), glm::vec2(18.0f, 18.0f), 68, 1, -17, 17);

	std::vector<std::string> items;
	if(clearedWorld_ < SaveSystem::instance().getWorldCount())
		items.push_back("SIGUIENTE MUNDO");
	items.push_back("MAPA DE MUNDOS");
	items.push_back("REPETIR MUNDO");
	items.push_back("MENU PRINCIPAL");
	std::vector<bool> enabled(items.size(), true);

	renderMenuList(items, enabled, clearSel_, 160.0f, 124.0f, 16.0f, 172.0f);
}

void MenuSystem::renderGameOver()
{
	ui_.fullscreen(glm::vec4(0.10f, 0.02f, 0.05f, 0.76f));

	glm::vec2 pPos(52.0f, 38.0f), pSz(216.0f, 160.0f);
	ui_.panel(pPos, pSz, COL_PANEL_BG, COL_RED);

	ui_.textCentered("FIN DE PARTIDA", 160.0f, 50.0f, 2.0f, COL_RED);
	ui_.textCentered("TE HAS QUEDADO SIN VIDAS", 160.0f, 76.0f, 1.0f, COL_CREAM);
	ui_.textCentered("PUNTOS: " + intToStr(scene_->getScore()), 160.0f, 90.0f, 1.0f, COL_SKY);
	ui_.rect(glm::vec2(pPos.x + 14.0f, 106.0f), glm::vec2(pSz.x - 28.0f, 1.0f), glm::vec4(1.0f, 0.34f, 0.36f, 0.4f));

	std::vector<std::string> items;
	items.push_back("REINTENTAR MUNDO");
	items.push_back("MAPA DE MUNDOS");
	items.push_back("CARGAR PARTIDA");
	items.push_back("MENU PRINCIPAL");
	std::vector<bool> enabled(items.size(), true);

	renderMenuList(items, enabled, overSel_, 160.0f, 118.0f, 16.0f, 172.0f);
}

void MenuSystem::renderConfirm()
{
	ui_.fullscreen(glm::vec4(0.01f, 0.02f, 0.06f, 0.72f));

	glm::vec2 pPos(44.0f, 74.0f), pSz(232.0f, 88.0f);
	ui_.panel(pPos, pSz, glm::vec4(0.08f, 0.11f, 0.26f, 0.98f), COL_GOLD);

	ui_.textCentered(confirmTitle_, 160.0f, 88.0f, 1.0f, COL_GOLD);
	ui_.textCentered(confirmMsg_,   160.0f, 106.0f, 1.0f, COL_CREAM);

	glm::vec2 yesBox(96.0f, 134.0f), noBox(166.0f, 134.0f), btnSz(58.0f, 18.0f);
	ui_.panel(yesBox, btnSz,
	          confirmSel_ == 0 ? glm::vec4(0.18f, 0.42f, 0.24f, 1.0f) : COL_PANEL_BG,
	          confirmSel_ == 0 ? COL_GOLD : COL_MUTED);
	ui_.textCentered("S" UTF8_I_AC, yesBox.x + 29.0f, yesBox.y + 5.0f, 1.0f,
	                 confirmSel_ == 0 ? COL_GOLD : COL_WHITE);

	ui_.panel(noBox, btnSz,
	          confirmSel_ == 1 ? glm::vec4(0.44f, 0.16f, 0.20f, 1.0f) : COL_PANEL_BG,
	          confirmSel_ == 1 ? COL_GOLD : COL_MUTED);
	ui_.textCentered("NO", noBox.x + 29.0f, noBox.y + 5.0f, 1.0f,
	                 confirmSel_ == 1 ? COL_GOLD : COL_WHITE);
}

void MenuSystem::renderToast()
{
	if(toastTimer_ <= 0.0f || toast_.empty())
		return;

	float alpha = glm::clamp(toastTimer_ / 300.0f, 0.0f, 1.0f);
	float w = ui_.textWidth(toast_, 1.0f) + 20.0f;
	float y = (state_ == STATE_WORLD_SELECT) ? 28.0f : 6.0f;
	glm::vec2 pos(160.0f - w * 0.5f, y);
	ui_.panel(pos, glm::vec2(w, 18.0f),
	          glm::vec4(0.06f, 0.14f, 0.30f, 0.94f * alpha),
	          glm::vec4(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, alpha));
	ui_.textCentered(toast_, 160.0f, y + 5.0f, 1.0f, glm::vec4(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, alpha));
}

// ---------------------------------------------------------------------------
// Reusable UI widgets
// ---------------------------------------------------------------------------

void MenuSystem::renderMenuList(const std::vector<std::string> &items, const std::vector<bool> &enabled,
                                int selected, float centerX, float startY, float spacing, float rowWidth)
{
	for(size_t i = 0; i < items.size(); i++)
	{
		float ry = startY + float(i) * spacing;
		bool isSel = (int(i) == selected);
		bool isEn  = (i < enabled.size()) ? enabled[i] : true;

		if(isSel)
		{
			glm::vec2 barPos(centerX - rowWidth * 0.5f, ry - 3.0f);
			ui_.rect(barPos, glm::vec2(rowWidth, 13.0f), glm::vec4(1.0f, 0.84f, 0.25f, 0.20f));
			ui_.rectOutline(barPos, glm::vec2(rowWidth, 13.0f), glm::vec4(1.0f, 0.84f, 0.25f, 0.65f), 1.0f);
			renderBombCursor(glm::vec2(barPos.x + 4.0f, ry - 2.0f), 11.0f);
		}

		glm::vec4 col = !isEn ? COL_MUTED : (isSel ? COL_GOLD : COL_WHITE);
		ui_.textCentered(items[i], centerX + 4.0f, ry, 1.0f, col);
	}
}

int MenuSystem::hitTestMenuList(int itemCount, float centerX, float startY, float spacing, float rowWidth) const
{
	for(int i = 0; i < itemCount; i++)
	{
		glm::vec2 barPos(centerX - rowWidth * 0.5f, startY + float(i) * spacing - 3.0f);
		if(mouseInRect(barPos, glm::vec2(rowWidth, 14.0f)))
			return i;
	}
	return -1;
}

void MenuSystem::renderStatsRow(const glm::vec2 &pos, int lives, int bombs, int range, int speed)
{
	float x = pos.x;
	float y = pos.y;

	// Lives (Heart icon + count)
	glm::ivec2 hSrc = PowerUp::getSheetCoords(EXTRA_LIFE);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), hSrc.x, hSrc.y, 16, 16);
	ui_.text(intToStr(lives), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
	x += 28.0f;

	// Max Bombs
	glm::ivec2 bSrc = PowerUp::getSheetCoords(EXTRA_BOMB);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), bSrc.x, bSrc.y, 16, 16);
	ui_.text(intToStr(bombs), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
	x += 28.0f;

	// Flame Range
	glm::ivec2 rSrc = PowerUp::getSheetCoords(BOMB_RANGE);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), rSrc.x, rSrc.y, 16, 16);
	ui_.text(intToStr(range), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
	x += 28.0f;

	// Speed Ups
	glm::ivec2 sSrc = PowerUp::getSheetCoords(SPEED_UP);
	ui_.sheet(glm::vec2(x, y), glm::vec2(12.0f, 12.0f), sSrc.x, sSrc.y, 16, 16);
	ui_.text(intToStr(speed), glm::vec2(x + 14.0f, y + 2.0f), 1.0f, COL_WHITE);
}

void MenuSystem::renderBomberman(const glm::vec2 &feetCenter, bool walking, bool faceRight, float scale)
{
	static const int WALK_X[4] = { 3, 19, 3, 36 };
	int srcX = walking ? WALK_X[int(time_ / 120.0f) % 4] : 3;
	float sz = std::floor(17.0f * scale);
	glm::vec2 drawPos(std::floor(feetCenter.x - sz * 0.5f), std::floor(feetCenter.y - sz));
	ui_.sheet(drawPos, glm::vec2(sz, sz), srcX, 1, faceRight ? -17 : 17, 17);
}

void MenuSystem::renderBombCursor(const glm::vec2 &pos, float size)
{
	static const int BOMB_X[3] = { 57, 75, 92 };
	int stage = int(time_ / 220.0f) % 3;
	ui_.sheet(pos, glm::vec2(size, size), BOMB_X[stage], 51, 16, 16);
}

void MenuSystem::renderHint(const std::string &hint)
{
	ui_.rect(glm::vec2(0.0f, 224.0f), glm::vec2(320.0f, 16.0f), glm::vec4(0.03f, 0.05f, 0.14f, 0.86f));
	ui_.rect(glm::vec2(0.0f, 224.0f), glm::vec2(320.0f, 1.0f), glm::vec4(1.0f, 0.82f, 0.25f, 0.45f));
	ui_.textCentered(hint, 160.0f, 228.0f, 1.0f, COL_CREAM);
}

std::string MenuSystem::formatTime(float ms)
{
	int totalSec = std::max(0, int(ms / 1000.0f));
	int mins = totalSec / 60;
	int secs = totalSec % 60;
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
	return std::string(buf);
}

std::string MenuSystem::intToStr(int v)
{
	std::ostringstream oss;
	oss << v;
	return oss.str();
}
