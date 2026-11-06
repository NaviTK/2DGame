#ifndef _PINK_ENEMY_INCLUDE
#define _PINK_ENEMY_INCLUDE


#include "Enemy.h"


// PinkEnemy: Row 10 enemy from images/caracters.png (y = 153..168).
// Features custom slim hitbox, full walk/turn/death animations from row 10,
// and platform-bound pathfinding (walks slowly back and forth and turns around
// at platform ledges and walls without ever jumping or falling off its platform).
class PinkEnemy : public Enemy
{

public:
	enum PinkEnemyAnim
	{
		STAND_LEFT = 0,
		STAND_RIGHT,
		MOVE_LEFT,
		MOVE_RIGHT,
		TURN_FRONT,
		DYING_ANIM,
		NUM_ANIMS
	};

	PinkEnemy();
	virtual ~PinkEnemy();

	virtual void init(const glm::vec2 &initialPos, float tileSize = 16.0f) override;
	virtual void initSprite(ShaderProgram &shaderProgram) override;
	virtual void update(int deltaTime, TileMap *map) override;
	virtual void kill() override;

private:
	float turnTimer; // Brief front-facing pause when turning at a ledge or wall

};


#endif // _PINK_ENEMY_INCLUDE

