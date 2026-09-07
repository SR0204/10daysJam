#include "GameOver.h"
#include "audio/Audio.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include "input/Input.h"

using namespace KamataEngine;

GameOver::~GameOver() {
	delete m_spriteGameOver;
	StopBGM();
}

void GameOver::Initialize() {

	// 画像の読み込み ※画像パスは実際の格納場所に合わせて変更してください
	m_texGameOver = TextureManager::Load("GameOver/GameOver.png");

	m_spriteGameOver = Sprite::Create(m_texGameOver, {0.0f, 0.0f});

	if (m_spriteGameOver) {
		float winWidth = static_cast<float>(WinApp::kWindowWidth);
		float winHeight = static_cast<float>(WinApp::kWindowHeight);

		// 画面中央に配置
		m_spriteGameOver->SetAnchorPoint({0.5f, 0.5f});
		m_spriteGameOver->SetPosition({winWidth * 0.5f, winHeight * 0.5f});
		m_spriteGameOver->SetSize({1280.0f, 720.0f});
	}

	// ★ BGMの読み込みとループ再生（音源ファイルのパスを指定してください）
	m_bgmHandle = Audio::GetInstance()->LoadWave("BGM/GameOverBGM.wav");
}

GameOverResult GameOver::Update() {
	auto input = Input::GetInstance();

	// Rキーでリトライ
	if (input->TriggerKey(DIK_R)) {
		return GameOverResult::Retry;
	}

	// SPACEキーでステージセレクトへ戻る
	if (input->TriggerKey(DIK_SPACE)) {
		return GameOverResult::StageSelect;
	}

	// ★ Tキーでタイトルへ戻る
	if (input->TriggerKey(DIK_T)) {
		return GameOverResult::Title;
	}

	return GameOverResult::None;
}

void GameOver::Draw() {
	if (m_spriteGameOver) {
		m_spriteGameOver->Draw();
	}
}

void GameOver::PlayBGM() {
	// まだ再生されていなければ再生
	if (m_playHandle == 0 && m_bgmHandle != 0) {
		m_playHandle = Audio::GetInstance()->PlayWave(m_bgmHandle, true, 0.5f);
	}
}

void GameOver::StopBGM() {
	if (m_playHandle != 0) {
		Audio::GetInstance()->StopWave(m_playHandle);
		m_playHandle = 0;
	}
}