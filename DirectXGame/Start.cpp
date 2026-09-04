#include "Start.h"
#include "base/TextureManager.h"

using namespace KamataEngine;

Start::Start(int x, int y) { Initialize(x, y); }

Start::~Start() { delete m_sprite; }

void Start::Initialize(int x, int y) {
	m_x = x;
	m_y = y;

	// 画像の読み込み（保存先のパスを指定してください）
	m_texture = TextureManager::Load("Start/Start.png");

	if (m_sprite) {
		delete m_sprite;
		m_sprite = nullptr;
	}
	m_sprite = Sprite::Create(m_texture, {0.0f, 0.0f});
}

void Start::UpdatePosition(int boardWidth, int boardHeight, float windowWidth, float windowHeight) {
	if (!m_sprite)
		return;

	m_tileSizeX = windowWidth / static_cast<float>(boardWidth);
	m_tileSizeY = windowHeight / static_cast<float>(boardHeight);

	// アンカーポイントを中心（0.5, 0.5）に設定
	m_sprite->SetAnchorPoint({0.5f, 0.5f});

	float posX = (m_x + 0.5f) * m_tileSizeX;
	float posY = (m_y + 0.5f) * m_tileSizeY;

	m_sprite->SetPosition({posX, posY});
	m_sprite->SetSize({m_tileSizeX, m_tileSizeY});
}

void Start::Render(ID3D12GraphicsCommandList* commandList) {
	if (!m_sprite)
		return;

	m_sprite->SetSize({m_tileSizeX, m_tileSizeY});

	Sprite::PreDraw(commandList);
	m_sprite->Draw();
	Sprite::PostDraw();
}