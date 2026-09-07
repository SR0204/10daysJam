#include "GameClear.h"
#include "audio/Audio.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include "input/Input.h"

using namespace KamataEngine;

GameClear::~GameClear() {
	delete m_spriteClear;
	StopBGM();
}

void GameClear::Initialize() {
	// クリア画像の読み込み
	m_texClear = TextureManager::Load("Clear/Clear.png");
	m_spriteClear = Sprite::Create(m_texClear, {0.0f, 0.0f});

	if (m_spriteClear) {
		float winWidth = static_cast<float>(WinApp::kWindowWidth);
		float winHeight = static_cast<float>(WinApp::kWindowHeight);

		// 画面中央に配置
		m_spriteClear->SetAnchorPoint({0.5f, 0.5f});
		m_spriteClear->SetPosition({winWidth * 0.5f, winHeight * 0.5f});
		m_spriteClear->SetSize({1280.0f, 720.0f});
	}

	// BGMの読み込み（再生は到達時に呼び出し）
	m_bgmHandle = Audio::GetInstance()->LoadWave("BGM/GameClearBGM.wav");
}

GameClearResult GameClear::Update() {
	auto input = Input::GetInstance();

	// Rキーでリトライ
	if (input->TriggerKey(DIK_R)) {
		// ★ ステージセレクトへ遷移するタイミングでBGMを停止
		Audio::GetInstance()->StopWave(m_playHandle);
		Audio::GetInstance()->StopWave(m_bgmHandle);
		return GameClearResult::Retry;
	}

	// SPACEキーでステージセレクトへ戻る
	if (input->TriggerKey(DIK_SPACE)) {
		// ★ ステージセレクトへ遷移するタイミングでBGMを停止
		Audio::GetInstance()->StopWave(m_playHandle);
		Audio::GetInstance()->StopWave(m_bgmHandle);
		return GameClearResult::StageSelect;
	}

	// Tキーでタイトルへ戻る
	if (input->TriggerKey(DIK_T)) {
		// ★ ステージセレクトへ遷移するタイミングでBGMを停止
		Audio::GetInstance()->StopWave(m_playHandle);
		Audio::GetInstance()->StopWave(m_bgmHandle);
		return GameClearResult::Title;
	}

	return GameClearResult::None;
}

void GameClear::Draw() {
	if (m_spriteClear) {
		m_spriteClear->Draw();
	}
}

void GameClear::PlayBGM() {
	// まだ再生されていなければ再生
	if (m_playHandle == 0 && m_bgmHandle != 0) {
		m_playHandle = Audio::GetInstance()->PlayWave(m_bgmHandle, true, 0.5f);
	}
}

void GameClear::StopBGM() {
	if (m_playHandle != 0) {
		Audio::GetInstance()->StopWave(m_playHandle);
		m_playHandle = 0;
	}
}