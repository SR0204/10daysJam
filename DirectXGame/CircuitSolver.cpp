#include "CircuitSolver.h"
#include <queue>
#include <utility>

// ----------------------------------------------------
// 逆方向を取得する関数
// ----------------------------------------------------
Dir CircuitSolver::GetOppositeDir(Dir dir) {
	switch (dir) {
	case UP:
		return DOWN;
	case DOWN:
		return UP;
	case LEFT:
		return RIGHT;
	case RIGHT:
		return LEFT;
	default:
		return NONE;
	}
}

// ----------------------------------------------------
// 2つのマスが繋がっているか判定
// ----------------------------------------------------
bool CircuitSolver::IsConnected(const Board& board, int x1, int y1, Dir dir, int x2, int y2) {
	// 盤面外チェック
	if (!board.IsValid(x1, y1) || !board.IsValid(x2, y2)) {
		return false;
	}

	uint8_t maskA = board.GetTile(x1, y1).mask;
	uint8_t maskB = board.GetTile(x2, y2).mask;
	Dir oppDir = GetOppositeDir(dir);

	// 【接続条件】
	// 1. マスAが dir 方向に開いている (maskA & dir)
	// 2. マスBが oppDir(反対) 方向に開いている (maskB & oppDir)
	return ((maskA & dir) != 0) && ((maskB & oppDir) != 0);
}

// ----------------------------------------------------
// 幅優先探索（BFS）による通電フラグの更新
// ----------------------------------------------------
void CircuitSolver::UpdatePower(Board& board, int startX, int startY) {
	// 1. まず盤面全体の通電フラグをリセット
	board.ResetPowerState();

	// スタート座標が無効なら処理終了
	if (!board.IsValid(startX, startY))
		return;

	// 2. スタート地点のマスが外部（左側）に向けて開いていない場合は通電不可
	if (!(board.GetTile(startX, startY).mask & LEFT)) {
		return;
	}

	// 3. BFS（幅優先探索）用のキューを用意
	std::queue<std::pair<int, int>> searchQueue;

	// スタート地点を通電状態にしてキューに追加
	board.GetTile(startX, startY).isPowered = true;
	searchQueue.push({startX, startY});

	// ★ 4方向の定義（上、下、左、右）の順番を完全に同期させる
	// UP(y-1), DOWN(y+1), LEFT(x-1), RIGHT(x+1)
	const Dir dirs[] = {UP, DOWN, LEFT, RIGHT};
	const int dx[] = {0, 0, -1, 1};
	const int dy[] = {-1, 1, 0, 0};

	// キューが空になるまで電波/電流を伝播させる
	while (!searchQueue.empty()) {
		auto [cx, cy] = searchQueue.front();
		searchQueue.pop();

		// 上・下・左・右 の4方向を順番にチェック
		for (int i = 0; i < 4; ++i) {
			int nx = cx + dx[i];
			int ny = cy + dy[i];
			Dir dir = dirs[i];

			// 隣のマスが存在し、まだ通電しておらず、かつ接続口が正しく繋がっている場合
			if (board.IsValid(nx, ny) && !board.GetTile(nx, ny).isPowered) {
				if (IsConnected(board, cx, cy, dir, nx, ny)) {
					// 隣のマスに通電フラグを立てて、さらに先を探索するためにキューへ積む
					board.GetTile(nx, ny).isPowered = true;
					searchQueue.push({nx, ny});
				}
			}
		}
	}
}