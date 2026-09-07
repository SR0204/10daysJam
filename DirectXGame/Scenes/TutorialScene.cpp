#include "../Scenes/TutorialScene.h"
#include "../App/SceneManager.h"
#include "../Scenes/StageSelectScene.h"
#include "audio/Audio.h"
#include "base/TextureManager.h"
#include "input/Input.h"

using namespace KamataEngine;

TutorialScene::TutorialScene() {}

TutorialScene::~TutorialScene() {
	delete m_guideSprite;

	// ★ 変数の値に関係なく TitleBGM を直接狙い撃ちで強制停止！
	uint32_t titleBgm = Audio::GetInstance()->LoadWave("BGM/TitleBGM.wav");
	Audio::GetInstance()->StopWave(titleBgm);

	if (m_playHandle != 0) {
		Audio::GetInstance()->StopWave(m_playHandle);
		m_playHandle = 0;
	}
}

void TutorialScene::Initialize() {
	// 説明画像の読み込みとスプライト生成
	m_guideTexture = TextureManager::Load("Tutorial/Tutorial.png");
	m_guideSprite = Sprite::Create(m_guideTexture, {0.0f, 0.0f});
}

void TutorialScene::Update(float deltaTime) {
	(void)deltaTime; // 未使用警告防止

	// SPACEキーが押されたらステージセレクトへ移動
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		// ★ ここでも TitleBGM を直接指定して確実に止める！
		uint32_t titleBgm = Audio::GetInstance()->LoadWave("BGM/TitleBGM.wav");
		Audio::GetInstance()->StopWave(titleBgm);

		if (m_playHandle != 0) {
			Audio::GetInstance()->StopWave(m_playHandle);
			m_playHandle = 0;
		}

		m_sceneManager->ChangeScene(std::make_unique<StageSelectScene>());
	}
}

void TutorialScene::Render(ID3D12GraphicsCommandList* commandList) {
	// 画像（スプライト）のみを描画
	if (m_guideSprite) {
		Sprite::PreDraw(commandList);
		m_guideSprite->Draw();
		Sprite::PostDraw();
	}
}