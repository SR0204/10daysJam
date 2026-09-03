#include "Board.h"
#include <utility> // std::swap 用

// コンストラクタ
Board::Board(int width, int height) : m_width(width), m_height(height), m_tiles(height, std::vector<Tile>(width)) {}

// 座標が盤面内にあるかチェック
bool Board::IsValid(int x, int y) const { return x >= 0 && x < m_width && y >= 0 && y < m_height; }

// タイル取得（参照）
Tile& Board::GetTile(int x, int y) { return m_tiles[y][x]; }

// タイル取得（const参照）
const Tile& Board::GetTile(int x, int y) const { return m_tiles[y][x]; }

// パイプを90度時計回りに回転
void Board::RotateTile(int x, int y) {
	if (!IsValid(x, y)) return;
	
	uint8_t oldMask = m_tiles[y][x].mask;
	uint8_t newMask = 0;

	// 時計回りシフト: UP->RIGHT, RIGHT->DOWN, DOWN->LEFT, LEFT->UP
	if (oldMask & UP)    newMask |= RIGHT;
	if (oldMask & RIGHT) newMask |= DOWN;
	if (oldMask & DOWN)  newMask |= LEFT;
	if (oldMask & LEFT)  newMask |= UP;

	m_tiles[y][x].mask = newMask;
}

// マス同士の入れ替え（パズドラ風操作用）
void Board::SwapTiles(int x1, int y1, int x2, int y2) {
	if (IsValid(x1, y1) && IsValid(x2, y2)) {
		std::swap(m_tiles[y1][x1], m_tiles[y2][x2]);
	}
}

// 通電フラグのリセット
void Board::ResetPowerState() {
	for (auto& row : m_tiles) {
		for (auto& tile : row) {
			tile.isPowered = false;
		}
	}
}

// 盤面全体のクリア
void Board::Clear() {
	for (auto& row : m_tiles) {
		for (auto& tile : row) {
			tile.mask = NONE;
			tile.isPowered = false;
		}
	}
}