#pragma once

#include <cstdint>
#include <vector>

// ----------------------------------------------------
// パイプの開口方向（ビットフラグ）
// ----------------------------------------------------
enum Dir : uint8_t {
	NONE = 0,
	UP = 1 << 0,    // 0001 (1)
	RIGHT = 1 << 1, // 0010 (2)
	DOWN = 1 << 2,  // 0100 (4)
	LEFT = 1 << 3   // 1000 (8)
};

// ----------------------------------------------------
// 1つのマスのデータ構造
// ----------------------------------------------------
struct Tile {
	uint8_t mask = NONE;    // 開口方向の論理和 (例: UP | RIGHT)
	bool isPowered = false; // 通電状態（発光エフェクト用）
	bool isLocked = false;  // ★ 追加: trueなら回転不可
};

// ----------------------------------------------------
// 盤面データモデルクラス
// ----------------------------------------------------
class Board {
private:
	int m_width;
	int m_height;
	std::vector<std::vector<Tile>> m_tiles;

public:
	// コンストラクタ / デストラクタ
	Board(int width, int height);
	~Board() = default;

	// ゲッター
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	bool IsValid(int x, int y) const;

	// タイルデータへのアクセス
	Tile& GetTile(int x, int y);
	const Tile& GetTile(int x, int y) const;

	// グリッド操作
	void RotateTile(int x, int y);                  // 時計回りに90度回転
	void SwapTiles(int x1, int y1, int x2, int y2); // 2つのマスを入れ替え
	void ResetPowerState();                         // 通電フラグのリセット
	void Clear();                                   // 全マスをリセット
};