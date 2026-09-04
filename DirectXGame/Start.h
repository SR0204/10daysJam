#pragma once
#include "2d/Sprite.h"
#include "Board.h"
#include "KamataEngine.h"
#include <d3d12.h>

class Start {
public:
	Start() = default;
	Start(int x, int y);
	~Start();

	void Initialize(int x, int y);
	void UpdatePosition(int boardWidth, int boardHeight, float windowWidth, float windowHeight);
	void Render(ID3D12GraphicsCommandList* commandList);

private:
	int m_x = 0;
	int m_y = 0;
	uint32_t m_texture = 0;
	KamataEngine::Sprite* m_sprite = nullptr;

	float m_tileSizeX = 0.0f;
	float m_tileSizeY = 0.0f;
};