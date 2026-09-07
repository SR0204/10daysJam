#include "Start.h"
#include "base/TextureManager.h"

using namespace KamataEngine;

Start::Start(int x, int y) { Initialize(x, y); }

Start::~Start() { delete m_sprite; }

void Start::Initialize(int x, int y) {
	m_x = x;
	m_y = y;

	// 画像の読み込み
	m_texture = TextureManager::Load("Start/Start.png");

	if (m_sprite) {
		delete m_sprite;
		m_sprite = nullptr;
	}
	m_sprite = Sprite::Create(m_texture, {0.0f, 0.0f});
}

void Start::UpdatePosition(int boardWidth, int boardHeight, float windowWidth, float windowHeight) {
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
	float spriteWidth = (tileSizeX_NDC / 2.0f) * windowWidth;
	float spriteHeight = (tileSizeY_NDC / 2.0f) * windowHeight;

	// メンバ変数にも保存しておく
	m_tileSizeX = spriteWidth;
	m_tileSizeY = spriteHeight;

	// マスの中心のスクリーン座標 (0,0 は画面左上)
	float screenX = (posX_NDC + 1.0f) * 0.5f * windowWidth;
	float screenY = (1.0f - posY_NDC) * 0.5f * windowHeight;

	// 3. スプライトに位置とサイズを設定
	if (m_sprite) {
		m_sprite->SetAnchorPoint({0.5f, 0.5f});
		m_sprite->SetPosition({screenX, screenY});
		m_sprite->SetSize({spriteWidth, spriteHeight});
	}
}

void Start::Render(ID3D12GraphicsCommandList* commandList) {
	if (!m_sprite)
		return;

	Sprite::PreDraw(commandList);
	m_sprite->Draw();
	Sprite::PostDraw();
}