#pragma once
#include "../App/IScene.h"
#include "2d/Sprite.h"

class TutorialScene : public IScene {
public:
	TutorialScene();
	~TutorialScene() override;

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;

private:
	uint32_t m_guideTexture = 0;
	KamataEngine::Sprite* m_guideSprite = nullptr;

	// ★ BGM用の変数
	uint32_t m_bgmHandle = 0;
	uint32_t m_playHandle = 0;
};