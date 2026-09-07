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

	// ★ 修正：ここで m_sprite->SetSize({m_tileSizeX, m_tileSizeY}); を行っていたのを削除！
	// (サイズ設定は UpdatePosition 内で行います)

	// 2. 確定した回転角をスプライトに適用する
	m_sprite->SetRotation(m_rotation);

	// 3. 描画処理
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
	float aspectRatio = windowWidth / windowHeight; // 16:9

	// 1. NDC空間（-1.0～1.0）でのマスサイズを算出
	float tileSizeY_NDC = 2.0f / static_cast<float>(boardHeight);
	float tileSizeX_NDC = tileSizeY_NDC / aspectRatio;

	// 盤面全体の幅と、左上マス（0,0）の中心座標（NDC）
	float totalWidthNDC = tileSizeX_NDC * static_cast<float>(boardWidth);
	float startX_NDC = -totalWidthNDC * 0.5f + (tileSizeX_NDC * 0.5f);
	float startY_NDC = 1.0f - (tileSizeY_NDC * 0.5f);

	// 指定マス (m_x, m_y) の中心位置（NDC）
	float posX_NDC = startX_NDC + (m_x * tileSizeX_NDC);
	float posY_NDC = startY_NDC - (m_y * tileSizeY_NDC);

	// 2. NDC座標をスクリーン座標（ピクセル）に変換
	// スプライトのサイズ（ピクセル）
	float spriteWidth = (tileSizeX_NDC / 2.0f) * windowWidth;
	float spriteHeight = (tileSizeY_NDC / 2.0f) * windowHeight;

	// ★ メンバ変数にも正しいサイズを保存
	m_tileSizeX = spriteWidth;
	m_tileSizeY = spriteHeight;

	// マスの中心のスクリーン座標 (0,0 は画面左上)
	float screenX = (posX_NDC + 1.0f) * 0.5f * windowWidth;
	float screenY = (1.0f - posY_NDC) * 0.5f * windowHeight;

	// 3. スプライトに位置とサイズを設定
	if (m_sprite) {
		// アンカーポイントを中心（0.5, 0.5）に設定して中央に配置
		m_sprite->SetAnchorPoint({0.5f, 0.5f});
		m_sprite->SetPosition({screenX, screenY});
		m_sprite->SetSize({spriteWidth, spriteHeight});
	}
}