#include "../Scenes/TutorialScene.h"
#include "../App/SceneManager.h"
#include "../Scenes/StageSelectScene.h"
#include "base/TextureManager.h"
#include "input/Input.h"

using namespace KamataEngine;

TutorialScene::TutorialScene() {}

TutorialScene::~TutorialScene() { delete m_guideSprite; }

void TutorialScene::Initialize() {
	// 説明画像の読み込みとスプライト生成
	m_guideTexture = TextureManager::Load("Tutorial/Tutorial.png");
	m_guideSprite = Sprite::Create(m_guideTexture, {0.0f, 0.0f});
}

void TutorialScene::Update(float deltaTime) {
	(void)deltaTime; // 未使用警告防止

	// SPACEキーが押されたらステージセレクトへ移動
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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