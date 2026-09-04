#include "Goal.h"
#include "base/TextureManager.h"
#include <cmath>

using namespace KamataEngine;

Goal::Goal(int x, int y) { Initialize(x, y); }

// ★ デストラクタで生成したスプライトを解放
Goal::~Goal() { delete m_sprite; }

void Goal::Initialize(int x, int y) {
	m_x = x;
	m_y = y;
	m_isReached = false;
	m_clearTimer = 0.0f;

	// テクスチャの読み込み
	m_textureOff = TextureManager::Load("Goal/Goal_Off.png");
	m_textureOn = TextureManager::Load("Goal/Goal_On.png");

	// ★ スプライトの生成（既存がある場合は解放してから再生成）
	if (m_sprite) {
		delete m_sprite;
		m_sprite = nullptr;
	}
	m_sprite = Sprite::Create(m_textureOff, {0.0f, 0.0f});
}

void Goal::Update(const Board& board, float deltaTime) {
	if (!board.IsValid(m_x, m_y))
		return;

	bool isPowered = board.GetTile(m_x, m_y).isPowered;

	if (isPowered) {
		m_isReached = true;
		m_clearTimer += deltaTime;
	} else {
		m_isReached = false;
		m_clearTimer = 0.0f;
	}
}

void Goal::Render(ID3D12GraphicsCommandList* commandList) {
	if (!m_sprite)
		return;

	// 1. 通電状態に合わせてテクスチャを切り替え
	if (m_isReached) {
		m_sprite->SetTextureHandle(m_textureOn);
	} else {
		m_sprite->SetTextureHandle(m_textureOff);
	}

	// 2. マス目（1セル）のピクセルサイズにフィットさせる
	m_sprite->SetSize({m_tileSizeX, m_tileSizeY});

	// ★ 3. 確定した回転角をスプライトに適用する（ここを追加！）
	m_sprite->SetRotation(m_rotation);

	// 4. 描画処理
	Sprite::PreDraw(commandList);
	m_sprite->Draw();
	Sprite::PostDraw();
}

void Goal::SetupRotation(const Board& board) {
	if (!board.IsValid(m_x, m_y))
		return;

	uint8_t mask = board.GetTile(m_x, m_y).mask;

	// ゴールの接続口（上側の突起）を正解の道の方向へ向ける
	if (mask & UP) {
		m_rotation = 0.0f; // 0度（上）
	} else if (mask & RIGHT) {
		m_rotation = 1.5707963f; // 90度（右）
	} else if (mask & DOWN) {
		m_rotation = 3.1415926f; // 180度（下）
	} else if (mask & LEFT) {
		m_rotation = 4.7123889f; // 270度（左）
	}

	if (m_sprite) {
		m_sprite->SetRotation(m_rotation);
	}
}

void Goal::UpdatePosition(int boardWidth, int boardHeight, float windowWidth, float windowHeight) {
	if (!m_sprite)
		return;

	m_tileSizeX = windowWidth / static_cast<float>(boardWidth);
	m_tileSizeY = windowHeight / static_cast<float>(boardHeight);

	// ★ 回転の軸を中心（0.5, 0.5）に設定
	m_sprite->SetAnchorPoint({0.5f, 0.5f});

	// ★ 中心基準の位置計算
	float posX = (m_x + 0.5f) * m_tileSizeX;
	float posY = (m_y + 0.5f) * m_tileSizeY;

	m_sprite->SetPosition({posX, posY});
	m_sprite->SetSize({m_tileSizeX, m_tileSizeY});

	// ★ 保持していた回転角をセット
	m_sprite->SetRotation(m_rotation);
}