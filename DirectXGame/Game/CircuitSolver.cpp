#include "CircuitSolver.h"
#include <queue>
#include <utility>

// 反対方向を返す
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

// 隣り合う2マスが「互いに開口しているか」をチェック
bool CircuitSolver::IsConnected(const Board& board, int x1, int y1, Dir dir, int x2, int y2) {
	if (!board.IsValid(x1, y1) || !board.IsValid(x2, y2)) {
		return false;
	}

	uint8_t maskA = board.GetTile(x1, y1).mask;
	uint8_t maskB = board.GetTile(x2, y2).mask;
	Dir oppDir = GetOppositeDir(dir);

	// マスAが相手の方向(dir)に開いており、かつマスBがこちら側(oppDir)に開いている場合のみ true
	return ((maskA & dir) != 0) && ((maskB & oppDir) != 0);
}

void CircuitSolver::UpdatePower(Board& board, int startX, int startY) {
	board.ResetPowerState();

	if (!board.IsValid(startX, startY))
		return;

	// ★ 修正：LEFTフラグのチェックを削除し、無条件でスタートマスを通電開始点にする
	std::queue<std::pair<int, int>> searchQueue;

	// スタート地点を通電状態にする
	board.GetTile(startX, startY).isPowered = true;
	searchQueue.push({startX, startY});

	// 上:y-1, 下:y+1, 左:x-1, 右:x+1
	const Dir dirs[] = {UP, DOWN, LEFT, RIGHT};
	const int dx[] = {0, 0, -1, 1};
	const int dy[] = {-1, 1, 0, 0};

	while (!searchQueue.empty()) {
		auto [cx, cy] = searchQueue.front();
		searchQueue.pop();

		for (int i = 0; i < 4; ++i) {
			int nx = cx + dx[i];
			int ny = cy + dy[i];
			Dir dir = dirs[i];

			if (board.IsValid(nx, ny) && !board.GetTile(nx, ny).isPowered) {
				if (IsConnected(board, cx, cy, dir, nx, ny)) {
					board.GetTile(nx, ny).isPowered = true;
					searchQueue.push({nx, ny});
				}
			}
		}
	}
}