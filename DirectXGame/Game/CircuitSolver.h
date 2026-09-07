#pragma once

#include "../Game/Board.h"

// ----------------------------------------------------
// 回路の通電・接続状態を判定するクラス
// ----------------------------------------------------
class CircuitSolver {
public:
	// スタート地点から幅優先探索（BFS）を実行し、盤面全体の通電フラグ（isPowered）を更新する
	static void UpdatePower(Board& board, int startX, int startY);

	// 2つの隣り合うマスが正しく繋がっているかを単体判定するヘルパー関数
	static bool IsConnected(const Board& board, int x1, int y1, Dir dir, int x2, int y2);

	// 逆方向のビットフラグを取得するヘルパー関数（例: UP -> DOWN）
	static Dir GetOppositeDir(Dir dir);
};