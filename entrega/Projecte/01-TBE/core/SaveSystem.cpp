#include "SaveSystem.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <algorithm>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif


void SaveSystem::init()
{
	initWorlds();
	loadAllSlots();
}

void SaveSystem::initWorlds()
{
	worlds_.clear();

	// Nivell 1 -> levels/level01.txt
	WorldDef w1;
	w1.id = 1;
	w1.name = "NIVELL 1";
	w1.themeName = "BOSC VERD";
	w1.levelFile = "levels/level01.txt";
	w1.mapNodePos = glm::vec2(92.0f, 154.0f);
	w1.accentColor = glm::vec4(0.18f, 0.78f, 0.30f, 1.0f);
	w1.hasPinkEnemy = true;
	w1.hasKirbyEnemy = true;
	w1.hasJumperEnemy = false;
	w1.hasBossEnemy = false;
	worlds_.push_back(w1);

	// Nivell 2 -> levels/level02.txt
	WorldDef w2;
	w2.id = 2;
	w2.name = "NIVELL 2";
	w2.themeName = "RUINES ROCOSES";
	w2.levelFile = "levels/level02.txt";
	w2.mapNodePos = glm::vec2(198.0f, 132.0f);
	w2.pathFromPrev.push_back(glm::vec2(124.0f, 154.0f));
	w2.pathFromPrev.push_back(glm::vec2(124.0f, 116.0f));
	w2.pathFromPrev.push_back(glm::vec2(196.0f, 116.0f));
	w2.accentColor = glm::vec4(0.88f, 0.56f, 0.20f, 1.0f);
	w2.hasPinkEnemy = true;
	w2.hasKirbyEnemy = true;
	w2.hasJumperEnemy = true;
	w2.hasBossEnemy = false;
	worlds_.push_back(w2);

	// Nivell 3 -> levels/level03.txt
	WorldDef w3;
	w3.id = 3;
	w3.name = "NIVELL 3";
	w3.themeName = "COVA DE CRISTALL";
	w3.levelFile = "levels/level03.txt";
	w3.mapNodePos = glm::vec2(250.0f, 100.0f);
	w3.pathFromPrev.push_back(glm::vec2(202.0f, 152.0f));
	w3.pathFromPrev.push_back(glm::vec2(220.0f, 152.0f));
	w3.pathFromPrev.push_back(glm::vec2(242.0f, 136.0f));
	w3.pathFromPrev.push_back(glm::vec2(244.0f, 112.0f));
	w3.accentColor = glm::vec4(0.25f, 0.72f, 0.96f, 1.0f);
	w3.hasPinkEnemy = true;
	w3.hasKirbyEnemy = true;
	w3.hasJumperEnemy = true;
	w3.hasBossEnemy = false;
	worlds_.push_back(w3);

	// Nivell 4 -> levels/level04.txt
	WorldDef w4;
	w4.id = 4;
	w4.name = "NIVELL 4";
	w4.themeName = "TEMPLE ANTIC";
	w4.levelFile = "levels/level04.txt";
	w4.mapNodePos = glm::vec2(260.0f, 66.0f);
	w4.pathFromPrev.push_back(glm::vec2(268.0f, 96.0f));
	w4.accentColor = glm::vec4(0.82f, 0.42f, 0.92f, 1.0f);
	w4.hasPinkEnemy = true;
	w4.hasKirbyEnemy = true;
	w4.hasJumperEnemy = true;
	w4.hasBossEnemy = false;
	worlds_.push_back(w4);

	// Nivell 5 -> levels/level05.txt (Final Boss Arena)
	WorldDef w5;
	w5.id = 5;
	w5.name = "NIVELL 5";
	w5.themeName = "FORTALESA DEL BOSS";
	w5.levelFile = "levels/level05.txt";
	w5.mapNodePos = glm::vec2(215.0f, 58.0f);
	w5.pathFromPrev.push_back(glm::vec2(242.0f, 60.0f));
	w5.accentColor = glm::vec4(1.0f, 0.28f, 0.32f, 1.0f);
	w5.hasPinkEnemy = true;
	w5.hasKirbyEnemy = false;
	w5.hasJumperEnemy = true;
	w5.hasBossEnemy = true;
	worlds_.push_back(w5);
}

const WorldDef *SaveSystem::getWorldById(int worldId) const
{
	for(size_t i = 0; i < worlds_.size(); i++)
	{
		if(worlds_[i].id == worldId)
			return &worlds_[i];
	}
	return worlds_.empty() ? NULL : &worlds_[0];
}

std::string SaveSystem::getSlotFilePath(int slotIndex) const
{
	std::ostringstream oss;
	oss << "saves/slot" << (slotIndex + 1) << ".sav";
	return oss.str();
}

void SaveSystem::loadAllSlots()
{
	int maxWorld = std::max(1, getWorldCount());

	for(int i = 0; i < NUM_SLOTS; i++)
	{
		slots_[i].resetDefaults();
		std::ifstream fin(getSlotFilePath(i).c_str());
		if(!fin.is_open())
			continue;

		std::string header;
		std::getline(fin, header);
		if(header.find("POCKET_BOMBERMAN_SAVE") == std::string::npos)
			continue;

		SaveSlotData loaded;
		loaded.occupied = true;

		std::string line;
		while(std::getline(fin, line))
		{
			if(line.empty())
				continue;
			std::istringstream iss(line);
			std::string key;
			iss >> key;
			if(key == "CURRENT_WORLD")
				iss >> loaded.currentWorld;
			else if(key == "HIGHEST_WORLD")
				iss >> loaded.highestUnlockedWorld;
			else if(key == "COMPLETED_MASK")
				iss >> loaded.completedMask;
			else if(key == "LIVES")
				iss >> loaded.lives;
			else if(key == "MAX_BOMBS")
				iss >> loaded.maxBombs;
			else if(key == "BOMB_RANGE")
				iss >> loaded.bombRange;
			else if(key == "SPEED_UPS")
				iss >> loaded.speedUps;
			else if(key == "TIMESTAMP")
			{
				std::string rest;
				std::getline(iss, rest);
				while(!rest.empty() && (rest[0] == ' ' || rest[0] == '\t'))
					rest.erase(0, 1);
				while(!rest.empty() && (rest.back() == '\r' || rest.back() == '\n'))
					rest.pop_back();
				loaded.timestamp = rest;
			}
		}

		for(int w = 1; w < maxWorld; w++)
		{
			if(loaded.isWorldCompleted(w) && loaded.highestUnlockedWorld < w + 1)
				loaded.highestUnlockedWorld = w + 1;
		}
		loaded.currentWorld = std::max(1, std::min(loaded.currentWorld, maxWorld));
		loaded.highestUnlockedWorld = std::max(1, std::min(loaded.highestUnlockedWorld, maxWorld));
		loaded.lives = std::max(1, std::min(loaded.lives, 9));
		loaded.maxBombs = std::max(1, std::min(loaded.maxBombs, 9));
		loaded.bombRange = std::max(1, std::min(loaded.bombRange, 9));
		loaded.speedUps = std::max(0, std::min(loaded.speedUps, 3));

		slots_[i] = loaded;
	}
}

bool SaveSystem::saveSlot(int slotIndex, const SaveSlotData &data)
{
	if(slotIndex < 0 || slotIndex >= NUM_SLOTS)
		return false;

#ifdef _WIN32
	_mkdir("saves");
#else
	mkdir("saves", 0755);
#endif

	SaveSlotData toSave = data;
	toSave.occupied = true;
	if(toSave.timestamp.empty())
		toSave.timestamp = buildCurrentTimestamp();

	std::ofstream fout(getSlotFilePath(slotIndex).c_str(), std::ios::trunc);
	if(!fout.is_open())
		return false;

	fout << "POCKET_BOMBERMAN_SAVE_V1\n";
	fout << "CURRENT_WORLD " << toSave.currentWorld << "\n";
	fout << "HIGHEST_WORLD " << toSave.highestUnlockedWorld << "\n";
	fout << "COMPLETED_MASK " << toSave.completedMask << "\n";
	fout << "LIVES " << toSave.lives << "\n";
	fout << "MAX_BOMBS " << toSave.maxBombs << "\n";
	fout << "BOMB_RANGE " << toSave.bombRange << "\n";
	fout << "SPEED_UPS " << toSave.speedUps << "\n";
	fout << "TIMESTAMP " << toSave.timestamp << "\n";
	fout.close();

	slots_[slotIndex] = toSave;
	return true;
}

bool SaveSystem::deleteSlot(int slotIndex)
{
	if(slotIndex < 0 || slotIndex >= NUM_SLOTS)
		return false;

	std::remove(getSlotFilePath(slotIndex).c_str());
	slots_[slotIndex].resetDefaults();
	return true;
}

const SaveSlotData &SaveSystem::getSlot(int slotIndex) const
{
	static SaveSlotData emptySlot;
	if(slotIndex < 0 || slotIndex >= NUM_SLOTS)
		return emptySlot;
	return slots_[slotIndex];
}

int SaveSystem::findFirstFreeOrDefaultSlot() const
{
	for(int i = 0; i < NUM_SLOTS; i++)
	{
		if(!slots_[i].occupied)
			return i;
	}
	return 0;
}

std::string SaveSystem::buildCurrentTimestamp()
{
	std::time_t now = std::time(NULL);
	std::tm localTm;
#ifdef _WIN32
	localtime_s(&localTm, &now);
#else
	localTm = *std::localtime(&now);
#endif
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%02d/%02d %02d:%02d",
	              localTm.tm_mday, localTm.tm_mon + 1,
	              localTm.tm_hour, localTm.tm_min);
	return std::string(buf);
}
