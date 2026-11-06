#ifndef _JUMPER_ENEMY_INCLUDE
#define _JUMPER_ENEMY_INCLUDE


#include "Enemy.h"


// Enemy Type 3 (Saltador / Impredictible):
// Uses row 15 (y = 243) of images/caracters.png (blue springy jumper creature).
// Performs periodic jumps over obstacles and alternates between short hops and high leaps
// with unpredictable direction changes to make bomb timing challenging.
class JumperEnemy : public Enemy
{

public:
	enum JumperAnim
	{
		WALK_LEFT = 0,
		WALK_RIGHT,
		JUMP_LEFT,
		JUMP_RIGHT,
		LAND_ANIM,
		DYING_ANIM,
		NUM_ANIMS
	};

	JumperEnemy();
	virtual ~JumperEnemy();

	virtual void init(const glm::vec2 &initialPos, float tileSize = 16.0f) override;
	virtual void initSprite(ShaderProgram &shaderProgram) override;
	virtual void update(int deltaTime, TileMap *map) override;
	virtual void kill() override;

private:
	float jumpTimer_;     // Countdown until next periodic jump (ms)
	float landTimer_;     // Brief squash pose timer upon landing (ms)
	int jumpCycle_;       // Alternates between short hop, high leap, and direction-flip hop
	bool wasOnGround_;

};


#endif // _JUMPER_ENEMY_INCLUDE

