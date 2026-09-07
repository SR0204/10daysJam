#pragma once
#include "../App/IScene.h"
#include "../Game/Board.h"
#include "2d/Sprite.h"

class TitleScene : public IScene {
public:
	TitleScene();
	~TitleScene() override;

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;

private:
	Board m_board;
	float m_animationTimer = 0.0f;

	uint32_t m_textureHandleOn = 0;
	KamataEngine::Sprite* m_titleSprite = nullptr;

	// ★ BGM用の変数
	uint32_t m_bgmHandle = 0;
	uint32_t m_playHandle = 0;
};