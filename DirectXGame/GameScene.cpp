#include "GameScene.h"
#include "CircuitSolver.h"
#include "SceneManager.h"
#include "StageSelectScene.h"
#include "base/WinApp.h"
#include "input/Input.h"
#include <algorithm>
#include <random>
#include <stack>

using namespace KamataEngine;

GameScene::GameScene(int boardWidth, int boardHeight) : m_board(boardWidth, boardHeight), m_startX(0), m_startY(0), m_goalX(boardWidth - 1), m_goalY(boardHeight - 1) {}

void GameScene::Initialize() {
	GenerateStage();
	m_chargeProgress.assign(m_board.GetHeight(), std::vector<float>(m_board.GetWidth(), 0.0f));

	std::random_device rd;
	std::mt19937 g(rd());
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			// ★ スタート地点とゴール地点（ロックマス）はシャッフルしない
			if ((x == m_startX && y == m_startY) || (x == m_goalX && y == m_goalY))
				continue;

			if (m_board.GetTile(x, y).isLocked)
				continue;

			int rotations = g() % 4;
			for (int r = 0; r < rotations; ++r)
				m_board.RotateTile(x, y);
		}
	}

	// 描画クラスの初期化
	m_renderer.Initialize(m_board.GetWidth(), m_board.GetHeight());
	RefreshCircuit();
}

void GameScene::Update(float deltaTime) {
	float chargeSpeed = 3.0f;
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			const Tile& tile = m_board.GetTile(x, y);
			if (tile.isPowered) {
				m_chargeProgress[y][x] = (std::min)(1.0f, m_chargeProgress[y][x] + deltaTime * chargeSpeed);
			} else {
				m_chargeProgress[y][x] = (std::max)(0.0f, m_chargeProgress[y][x] - deltaTime * chargeSpeed);
			}
		}
	}

	m_renderer.UpdateBuffers(m_board, m_chargeProgress, m_goalX, m_goalY);

	if (m_isCleared && Input::GetInstance()->PushKey(DIK_SPACE)) {
		m_sceneManager->ChangeScene(std::make_unique<StageSelectScene>());
		return;
	}

	Input* input = Input::GetInstance();
	if (input->IsTriggerMouse(0)) {
		POINT mousePos;
		GetCursorPos(&mousePos);
		HWND hwnd = WinApp::GetInstance()->GetHwnd();
		ScreenToClient(hwnd, &mousePos);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		OnMouseDown(mousePos.x, mousePos.y, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	}
}
void GameScene::OnMouseDown(int screenX, int screenY, int windowWidth, int windowHeight) {
	// クライアント領域のピクセルサイズから直接 NDC 座標 (-1.0 ～ 1.0) を算出
	float ndcX = (2.0f * static_cast<float>(screenX) / static_cast<float>(windowWidth)) - 1.0f;
	float ndcY = 1.0f - (2.0f * static_cast<float>(screenY) / static_cast<float>(windowHeight));

	float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
	float tileSizeY = 2.0f / static_cast<float>(m_board.GetHeight());
	float tileSizeX = tileSizeY / aspectRatio;

	float totalWidthX = tileSizeX * static_cast<float>(m_board.GetWidth());
	float startX = -totalWidthX * 0.5f + (tileSizeX * 0.5f);
	float startY = 1.0f - (tileSizeY * 0.5f);

	float halfX = tileSizeX * 0.5f;
	float halfY = tileSizeY * 0.5f;

	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			// ロック（ゴール等）されている場合はスキップ
			if (m_board.GetTile(x, y).isLocked)
				continue;

			float tileCenterX = startX + (x * tileSizeX);
			float tileCenterY = startY - (y * tileSizeY);

			// 当たり判定の範囲チェック
			if (ndcX >= (tileCenterX - halfX) && ndcX <= (tileCenterX + halfX) && ndcY >= (tileCenterY - halfY) && ndcY <= (tileCenterY + halfY)) {

				m_board.RotateTile(x, y);
				RefreshCircuit();
				return;
			}
		}
	}
}

void GameScene::RefreshCircuit() {
	CircuitSolver::UpdatePower(m_board, m_startX, m_startY);
	m_isCleared = m_board.GetTile(m_goalX, m_goalY).isPowered;
	m_renderer.UpdateBuffers(m_board, m_chargeProgress, m_goalX, m_goalY);
}

void GameScene::Render(ID3D12GraphicsCommandList* commandList) { m_renderer.Render(commandList); }

void GameScene::GenerateStage() {
	int width = m_board.GetWidth();
	int height = m_board.GetHeight();

	std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));

	struct Point {
		int x, y;
	};
	std::stack<Point> pathStack;

	std::random_device rd;
	std::mt19937 g(rd());

	bool success = false;

	while (!success) {
		m_board.Clear();
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				visited[y][x] = false;
			}
		}
		while (!pathStack.empty()) {
			pathStack.pop();
		}

		int currentX = m_startX;
		int currentY = m_startY;
		visited[currentY][currentX] = true;
		pathStack.push({currentX, currentY});

		while (!pathStack.empty()) {
			Point p = pathStack.top();

			struct Neighbor {
				int x, y;
				Dir dir;
				Dir oppositeDir;
			};
			std::vector<Neighbor> neighbors;

			if (p.y > 0 && !visited[p.y - 1][p.x])
				neighbors.push_back({p.x, p.y - 1, UP, DOWN});
			if (p.x < width - 1 && !visited[p.y][p.x + 1])
				neighbors.push_back({p.x + 1, p.y, RIGHT, LEFT});
			if (p.y < height - 1 && !visited[p.y + 1][p.x])
				neighbors.push_back({p.x, p.y + 1, DOWN, UP});
			if (p.x > 0 && !visited[p.y][p.x - 1])
				neighbors.push_back({p.x - 1, p.y, LEFT, RIGHT});

			if (!neighbors.empty()) {
				std::shuffle(neighbors.begin(), neighbors.end(), g);
				Neighbor next = neighbors[0];

				m_board.GetTile(p.x, p.y).mask |= next.dir;
				m_board.GetTile(next.x, next.y).mask |= next.oppositeDir;

				visited[next.y][next.x] = true;
				pathStack.push({next.x, next.y});
			} else {
				pathStack.pop();
			}
		}

		m_board.GetTile(m_startX, m_startY).isLocked = true;
		m_board.GetTile(m_goalX, m_goalY).isLocked = true;

		int visitedCount = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if (visited[y][x])
					visitedCount++;
			}
		}

		if (visitedCount == width * height && m_board.GetTile(m_goalX, m_goalY).mask != NONE) {
			success = true;
		}
	}

	//m_board.GetTile(m_startX, m_startY).mask |= LEFT;
	m_board.GetTile(m_startX, m_startY).isLocked = true;
}