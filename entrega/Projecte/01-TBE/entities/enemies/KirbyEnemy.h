#ifndef _KIRBY_ENEMY_INCLUDE
#define _KIRBY_ENEMY_INCLUDE


#include "Enemy.h"


// KirbyEnemy: Row 11 enemy from images/caracters.png (y = 171..186), right below PinkEnemy.
// Features a rounder custom hitbox (0.80 x 0.80), full walk/smile/alert/death animations
// from row 11, and platform-bound wandering AI that randomly charges towards the player
// whenever the player is on the same platform (without ever jumping or falling off ledges).
class KirbyEnemy : public Enemy
{

public:
	enum KirbyAnim
	{
		STAND_LEFT = 0,
		STAND_RIGHT,
		MOVE_LEFT,
		MOVE_RIGHT,
		SMILE_FRONT,
		ALERT_FRONT,
		DYING_ANIM,
		NUM_ANIMS
	};

	KirbyEnemy();
	virtual ~KirbyEnemy();

	virtual void init(const glm::vec2 &initialPos, float tileSize = 16.0f) override;
	virtual void initSprite(ShaderProgram &shaderProgram) override;
	virtual void update(int deltaTime, TileMap *map) override;
	virtual void kill() override;

private:
	float pauseTimer;    // Brief front-facing pause when turning or spotting the player
	float decisionTimer; // Periodic timer to decide whether to chase player on same platform
	bool chasingPlayer;  // True when actively charging toward the player on the same platform

};


#endif // _KIRBY_ENEMY_INCLUDE

