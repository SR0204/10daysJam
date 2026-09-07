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
	// 1. タイトル画像の読み込みとスプライト生成
	m_textureHandleOn = TextureManager::Load("Title/Title.png");

	m_titleSprite = Sprite::Create(m_textureHandleOn, {0.0f, 0.0f});
	if (m_titleSprite) {
		float winWidth = static_cast<float>(WinApp::kWindowWidth);   // 1280
		float winHeight = static_cast<float>(WinApp::kWindowHeight); // 720
		m_titleSprite->SetSize({winWidth, winHeight});
	}

	// ★ 2. 前回の再生が残っている場合は確実に一度止めてから新規再生する
	if (m_playHandle != 0) {
		Audio::GetInstance()->StopWave(m_playHandle);
		m_playHandle = 0;
	}

	m_bgmHandle = Audio::GetInstance()->LoadWave("BGM/TitleBGM.wav");
	m_playHandle = Audio::GetInstance()->PlayWave(m_bgmHandle, true, 0.5f);
}

void TitleScene::Update(float deltaTime) {
	m_animationTimer += deltaTime;

	// SPACE キーでチュートリアルへ遷移
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		// 1. チュートリアルシーンのインスタンスを生成
		auto tutorialScene = std::make_unique<TutorialScene>();

		// 2. ★ 再生中のBGMハンドルを渡す（ここが抜けていると止まりません）
		tutorialScene->SetBgmHandle(m_playHandle);

		// 3. タイトル側の二重停止を防ぐため0クリア
		m_playHandle = 0;

		// 4. シーン切り替え
		m_sceneManager->ChangeScene(std::move(tutorialScene));
	}
}

void TitleScene::Render(ID3D12GraphicsCommandList* commandList) {
	if (!commandList)
		return;

	Sprite::PreDraw(commandList);
	if (m_titleSprite) {
		m_titleSprite->Draw();
	}
	Sprite::PostDraw();
}