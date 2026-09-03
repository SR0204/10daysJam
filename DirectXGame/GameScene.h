#pragma once
#include "Board.h"
#include "IScene.h"
#include "PipeRenderer.h"
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

public:
	GameScene(int boardWidth = 18, int boardHeight = 10);
	~GameScene() override = default;

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;
	void OnMouseDown(int screenX, int screenY, int windowWidth, int windowHeight);

	bool IsCleared() const { return m_isCleared; }

private:
	void RefreshCircuit();
	void GenerateStage();
};