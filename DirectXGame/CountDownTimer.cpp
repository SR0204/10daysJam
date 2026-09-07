#include "CountDownTimer.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include <algorithm>
#include <cmath>
#include <string>

using namespace DirectX;
using namespace KamataEngine;

CountDownTimer::~CountDownTimer() {
	delete m_spriteMin;
	delete m_spriteColon;
	delete m_spriteSec10;
	delete m_spriteSec1;
}

void CountDownTimer::Initialize(float limitTimeSeconds) {
	m_timeRemaining = limitTimeSeconds;
	m_isFinished = false;

	// 数字テクスチャのロード (0~9)
	for (int i = 0; i < 10; ++i) {
		std::string path = "SelectNumber/SelectNumber_" + std::to_string(i) + ".png";
		m_texNumbers[i] = TextureManager::Load(path);
	}
	m_texColon = TextureManager::Load("SelectNumber/colon.png");

	// スプライト生成
	m_spriteMin = Sprite::Create(m_texNumbers[0], {0.0f, 0.0f});
	m_spriteColon = Sprite::Create(m_texColon, {0.0f, 0.0f});
	m_spriteSec10 = Sprite::Create(m_texNumbers[0], {0.0f, 0.0f});
	m_spriteSec1 = Sprite::Create(m_texNumbers[0], {0.0f, 0.0f});

	// アンカーポイントを中央に設定
	m_spriteMin->SetAnchorPoint({0.5f, 0.5f});
	m_spriteColon->SetAnchorPoint({0.5f, 0.5f});
	m_spriteSec10->SetAnchorPoint({0.5f, 0.5f});
	m_spriteSec1->SetAnchorPoint({0.5f, 0.5f});

	// ★ XMFLOAT2 から Vector2 に変更（または直接 {} で渡す）
	KamataEngine::Vector2 numSize = {40.0f, 50.0f};

	m_spriteMin->SetSize(numSize);
	m_spriteColon->SetSize({20.0f, 50.0f});
	m_spriteSec10->SetSize(numSize);
	m_spriteSec1->SetSize(numSize);

	// 画面中央上に配置
	float centerX = WinApp::kWindowWidth * 0.5f;
	float topY = 40.0f;
	float spacing = 35.0f;

	m_spriteMin->SetPosition({centerX - spacing * 1.5f, topY});
	m_spriteColon->SetPosition({centerX - spacing * 0.6f, topY});
	m_spriteSec10->SetPosition({centerX + spacing * 0.3f, topY});
	m_spriteSec1->SetPosition({centerX + spacing * 1.2f, topY});
}

void CountDownTimer::Update(float deltaTime) {
	if (m_isFinished)
		return;

	// カウントダウン処理
	m_timeRemaining -= deltaTime;
	if (m_timeRemaining <= 0.0f) {
		m_timeRemaining = 0.0f;
		m_isFinished = true;
	}

	// 分・秒の計算
	int totalSeconds = static_cast<int>(m_timeRemaining);
	int minutes = totalSeconds / 60;
	int seconds = totalSeconds % 60;

	int sec10 = seconds / 10;
	int sec1 = seconds % 10;

	// スプライトのテクスチャを該当する数字に切り替え
	m_spriteMin->SetTextureHandle(m_texNumbers[minutes]);
	m_spriteSec10->SetTextureHandle(m_texNumbers[sec10]);
	m_spriteSec1->SetTextureHandle(m_texNumbers[sec1]);
}

void CountDownTimer::Draw() {
	if (m_spriteMin)
		m_spriteMin->Draw();
	if (m_spriteColon)
		m_spriteColon->Draw();
	if (m_spriteSec10)
		m_spriteSec10->Draw();
	if (m_spriteSec1)
		m_spriteSec1->Draw();
}