#pragma once
#include "../Game/Board.h"
#include "KamataEngine.h"
#include <cstdint>
#include <d3d12.h>

class Goal {
private:
	int m_x = 0;
	int m_y = 0;
	bool m_isReached = false;
	float m_clearTimer = 0.0f;

	KamataEngine::Sprite* m_sprite = nullptr;

	uint32_t m_textureOff = 0;
	uint32_t m_textureOn = 0;

	// ★ マス目のピクセルサイズ保持用
	float m_tileSizeX = 0.0f;
	float m_tileSizeY = 0.0f;

	float m_rotation = 0.0f; // 回転角を保持する変数

public:
	Goal() = default;
	Goal(int x, int y);
	~Goal(); // ★ デストラクタを追加

	void Initialize(int x, int y);
	void Update(const Board& board, float deltaTime);
	void Render(ID3D12GraphicsCommandList* commandList);
	void UpdatePosition(int boardWidth, int boardHeight, float windowWidth = 1280.0f, float windowHeight = 720.0f);

	// ★ ステージ生成後にゴールの向きを確定する関数
	void SetupRotation(const Board& board);

	// ゲッター
	int GetX() const { return m_x; }
	int GetY() const { return m_y; }
	bool IsReached() const { return m_isReached; }
	float GetClearTimer() const { return m_clearTimer; }
};