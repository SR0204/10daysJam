#include "../Scenes/TitleScene.h"
#include "../App/SceneManager.h"
#include "../Scenes/TutorialScene.h"
#include "audio/Audio.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include "input/Input.h"

using namespace KamataEngine;

TitleScene::TitleScene() : m_board(1, 1) {}

TitleScene::~TitleScene() { delete m_titleSprite; }

void TitleScene::Initialize() {
	// タイトル画像の読み込み
	m_textureHandleOn = TextureManager::Load("Title/Title.png");

	// 1280x720 の全画面スプライトを作成
	m_titleSprite = Sprite::Create(m_textureHandleOn, {0.0f, 0.0f});
	if (m_titleSprite) {
		float winWidth = static_cast<float>(WinApp::kWindowWidth);   // 1280
		float winHeight = static_cast<float>(WinApp::kWindowHeight); // 720
		m_titleSprite->SetSize({winWidth, winHeight});
	}

	// ★ BGMの読み込みとループ再生（音源ファイルのパスを指定してください）
	m_bgmHandle = Audio::GetInstance()->LoadWave("BGM/TitleBGM.wav");
	m_playHandle = Audio::GetInstance()->PlayWave(m_bgmHandle, true, 0.5f);
}

void TitleScene::Update(float deltaTime) {
	m_animationTimer += deltaTime;

	// SPACE キーでチュートリアルへ遷移
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		m_sceneManager->ChangeScene(std::make_unique<TutorialScene>());
	}
}

void TitleScene::Render(ID3D12GraphicsCommandList* commandList) {
	if (!commandList)
		return;

	// スプライト描画
	Sprite::PreDraw(commandList);
	if (m_titleSprite) {
		m_titleSprite->Draw();
	}
	Sprite::PostDraw();
}