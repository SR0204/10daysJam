#pragma once
#include "2d/Sprite.h"
#include "Board.h"
#include "IScene.h"

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
};