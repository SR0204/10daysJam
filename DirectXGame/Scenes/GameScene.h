#pragma once
#include "../App/IScene.h"
#include "../Game/Board.h"
#include "../Game/CountDownTimer.h"
#include "../Game/GameOver.h"
#include "../Game/Goal.h"
#include "../Game/Start.h"
#include "../Graphics/PipeRenderer.h"
#include <memory>
#include <vector>

class GameScene : public IScene {
private:
	Board m_board;
	PipeRenderer m_renderer;

	int m_startX = 0;
	int m_startY = 0;
	int m_goalX = 9;
	int m_goalY = 9;

	bool m_isCleared = false;
	std::vector<std::vector<float>> m_chargeProgress;
	Goal m_goal;
	Start m_start;

	uint32_t m_clearTexture = 0;
	KamataEngine::Sprite* m_clearSprite = nullptr;

	std::unique_ptr<CountDownTimer> m_timer;

	std::unique_ptr<GameOver> m_gameOver;
	bool m_isGameOver = false;

	int m_stageWidth = 0;
	int m_stageHeight = 0;

	// 背景画像用
	uint32_t m_bgTexture = 0;
	KamataEngine::Sprite* m_bgSprite = nullptr;

public:
	GameScene(int boardWidth = 18, int boardHeight = 10);
	~GameScene();

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;
	void OnMouseDown(int screenX, int screenY, int windowWidth, int windowHeight);

	bool IsCleared() const { return m_isCleared; }

private:
	void RefreshCircuit();
	void GenerateStage();
};