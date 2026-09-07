#pragma once
#include "Board.h"
#include "CountDownTimer.h"
#include "GameOver.h"
#include "Goal.h"
#include "IScene.h"
#include "PipeRenderer.h"
#include "Start.h"
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