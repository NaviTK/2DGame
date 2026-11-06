#ifndef _SAVE_SYSTEM_INCLUDE
#define _SAVE_SYSTEM_INCLUDE


#include <string>
#include <vector>
#include <glm/glm.hpp>


struct WorldDef
{
	int id;                             // 1-based world/level identifier
	std::string name;                   // Short header, e.g. "MUNDO 1"
	std::string themeName;              // Descriptive subtitle, e.g. "BOSQUE VERDE"
	std::string levelFile;              // Path to level file, e.g. "levels/level01.txt"
	glm::vec2 mapNodePos;               // Node coordinates in 320x240 World Select space
	std::vector<glm::vec2> pathFromPrev;// Optional waypoints from previous world node
	glm::vec4 accentColor;              // Theme UI accent color
	bool hasPinkEnemy;                  // Enemy preview flags
	bool hasKirbyEnemy;
};


struct SaveSlotData
{
	bool occupied;
	int currentWorld;
	int highestUnlockedWorld;
	unsigned int completedMask; // Bit (worldId - 1) set when that world has been cleared
	int lives;
	int maxBombs;
	int bombRange;
	int speedUps;
	std::string timestamp;

	SaveSlotData()
	{
		resetDefaults();
	}

	void resetDefaults()
	{
		occupied = false;
		currentWorld = 1;
		highestUnlockedWorld = 1;
		completedMask = 0;
		lives = 3;
		maxBombs = 1;
		bombRange = 3;
		speedUps = 0;
		timestamp = "";
	}

	bool isWorldCompleted(int worldId) const
	{
		if(worldId < 1 || worldId > 30)
			return false;
		return (completedMask & (1u << (worldId - 1))) != 0;
	}

	void markWorldCompleted(int worldId)
	{
		if(worldId >= 1 && worldId <= 30)
			completedMask |= (1u << (worldId - 1));
	}

	int countCompletedWorlds(int totalWorlds) const
	{
		int count = 0;
		for(int i = 1; i <= totalWorlds; i++)
		{
			if(isWorldCompleted(i))
				count++;
		}
		return count;
	}
};


class SaveSystem
{

public:
	static const int NUM_SLOTS = 3;

	static SaveSystem &instance()
	{
		static SaveSystem S;
		return S;
	}

	void init();

	// Save slot management (persisted in saves/slot1.sav .. slot3.sav)
	void loadAllSlots();
	bool saveSlot(int slotIndex, const SaveSlotData &data);
	bool deleteSlot(int slotIndex);
	const SaveSlotData &getSlot(int slotIndex) const;
	int findFirstFreeOrDefaultSlot() const;

	// World registry (currently level01 & level02; easily extensible by adding entries in initWorlds)
	const std::vector<WorldDef> &getWorlds() const { return worlds_; }
	int getWorldCount() const { return int(worlds_.size()); }
	const WorldDef *getWorldById(int worldId) const;

	static std::string buildCurrentTimestamp();

private:
	SaveSystem() {}
	void initWorlds();
	std::string getSlotFilePath(int slotIndex) const;

	SaveSlotData slots_[NUM_SLOTS];
	std::vector<WorldDef> worlds_;

};


#endif // _SAVE_SYSTEM_INCLUDE
