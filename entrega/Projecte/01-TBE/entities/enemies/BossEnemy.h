#ifndef _BOSS_ENEMY_INCLUDE
#define _BOSS_ENEMY_INCLUDE


#include "Enemy.h"


// Final Boss (Level 5):
// Classic Pocket Bomberman Gargoyle Demon Boss from row 25 (y = 597, 32x32 px) of images/caracters.png.
// Features its own multi-hit health bar (5 HP), damage invulnerability flashing,
// and 3 differentiated attack patterns:
//   1. Stalking Patrol (tracks Bomberman horizontally)
//   2. High Jump Slam (telegraphs with arms raised, leaps across platforms toward Bomberman)
//   3. Enraged Dash Charge (high-speed horizontal rush across the arena)
class BossEnemy : public Enemy
{

public:
	static const int BOSS_MAX_HP = 5;

	enum BossAnim
	{
		WALK_ANIM = 0,
		ROAR_ATTACK_ANIM,
		HURT_DAZED_ANIM,
		DYING_ANIM,
		NUM_ANIMS
	};

	enum AttackState
	{
		STATE_PATROL = 0,
		STATE_TELEGRAPH,
		STATE_JUMP_SLAM,
		STATE_DASH_CHARGE
	};

	BossEnemy();
	virtual ~BossEnemy();

	virtual void init(const glm::vec2 &initialPos, float tileSize = 16.0f) override;
	virtual void initSprite(ShaderProgram &shaderProgram) override;
	virtual void update(int deltaTime, TileMap *map) override;
	virtual void render() const override;

	virtual void kill() override;
	virtual void forceKill() override;
	virtual bool isBoss() const override { return true; }
	virtual int getHp() const override { return hp_; }
	virtual int getMaxHp() const override { return maxHp_; }
	virtual bool isDamageFlashing() const override { return hitInvulnTimer_ > 0.0f; }

private:
	int hp_;
	int maxHp_;
	float hitInvulnTimer_; // Invulnerability window after taking 1 bomb hit (ms)
	AttackState state_;
	float stateTimer_;     // Timer for current attack state (ms)
	int nextAttackType_;   // Alternates between Jump Slam and Dash Charge

};


#endif // _BOSS_ENEMY_INCLUDE

