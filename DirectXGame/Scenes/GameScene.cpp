#include "GameScene.h"
#include "../App/SceneManager.h"
#include "../Game/CircuitSolver.h"
#include "../Scenes/StageSelectScene.h"
#include "../Scenes/TitleScene.h"
#include "audio/Audio.h"
#include "base/WinApp.h"
#include "input/Input.h"
#include <algorithm>
#include <random>
#include <stack>

using namespace KamataEngine;

GameScene::GameScene(int boardWidth, int boardHeight)
    : m_board(boardWidth, boardHeight), m_stageWidth(boardWidth), m_stageHeight(boardHeight), m_startX(0), m_startY(0), m_goalX(boardWidth - 1), m_goalY(boardHeight - 1) {}

GameScene::~GameScene() {
	delete m_bgSprite;
}

void GameScene::Initialize() {
	// ★ BGMの読み込みとループ再生
	m_bgmHandle = Audio::GetInstance()->LoadWave("BGM/PlayBGM.wav");
	m_playHandle = Audio::GetInstance()->PlayWave(m_bgmHandle, true, 0.5f);

	// ★ 背景スプライトの生成
	m_bgTexture = TextureManager::Load("BuckStage/BuckStage.png");
	m_bgSprite = Sprite::Create(m_bgTexture, {0.0f, 0.0f});
	if (m_bgSprite) {
		float winWidth = static_cast<float>(WinApp::kWindowWidth);
		float winHeight = static_cast<float>(WinApp::kWindowHeight);
		m_bgSprite->SetSize({winWidth, winHeight});
	}

	// 1. スタートとゴールの初期化
	m_start.Initialize(m_startX, m_startY);
	m_goal.Initialize(m_goalX, m_goalY);

	// 2. ステージ生成
	GenerateStage();

	// 3. ゴールの向き決定と位置計算
	m_goal.SetupRotation(m_board);

	float winWidth = static_cast<float>(WinApp::kWindowWidth);
	float winHeight = static_cast<float>(WinApp::kWindowHeight);

	m_start.UpdatePosition(m_board.GetWidth(), m_board.GetHeight(), winWidth, winHeight);
	m_goal.UpdatePosition(m_board.GetWidth(), m_board.GetHeight(), winWidth, winHeight);

	// 4. チャージ率配列の初期化
	m_chargeProgress.assign(m_board.GetHeight(), std::vector<float>(m_board.GetWidth(), 0.0f));

	// 5. 盤面のシャッフル
	std::random_device rd;
	std::mt19937 g(rd());
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			if ((x == m_startX && y == m_startY) || (x == m_goalX && y == m_goalY))
				continue;

			if (m_board.GetTile(x, y).isLocked)
				continue;

			int rotations = g() % 4;
			for (int r = 0; r < rotations; ++r)
				m_board.RotateTile(x, y);
		}
	}

	// 6. 描画クラスの初期化
	m_renderer.Initialize(m_board.GetWidth(), m_board.GetHeight());

	// 7. 通電状態を更新
	RefreshCircuit();

	m_board.GetTile(m_startX, m_startY).isPowered = true;
	m_chargeProgress[m_startY][m_startX] = 1.0f;

	m_renderer.UpdateBuffers(m_board, m_chargeProgress, m_goalX, m_goalY);

	// 3分（180秒）でタイマーを初期化
	m_timer = std::make_unique<CountDownTimer>();
	m_timer->Initialize(60.0f);

	// ゲームオーバー初期化+作成
	m_gameOver = std::make_unique<GameOver>();
	m_gameOver->Initialize();

	// ★ ゲームクリア初期化+作成
	m_gameClear = std::make_unique<GameClear>();
	m_gameClear->Initialize();
}

void GameScene::Update(float deltaTime) {
	// 1. 通電アニメーション（チャージ）の更新
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

	// 2. ゴール状態の更新
	m_goal.Update(m_board, deltaTime);

	// 3. 描画バッファの更新
	m_renderer.UpdateBuffers(m_board, m_chargeProgress, m_goalX, m_goalY);

	// ★ 4. クリア時のリザルト処理（GameClearクラスに任せる）
	if (m_goal.IsReached()) {
		// クリアした最初の1フレームだけPlayBGMを止めてクリアBGMを再生
		if (!m_isClearBgmPlayed) {
			Audio::GetInstance()->StopWave(m_playHandle);
			m_gameClear->PlayBGM();
			m_isClearBgmPlayed = true;
		}

		GameClearResult result = m_gameClear->Update();
		if (result == GameClearResult::Retry) {
			m_sceneManager->ChangeScene(std::make_unique<GameScene>(m_board.GetWidth(), m_board.GetHeight()));
		} else if (result == GameClearResult::StageSelect) {
			m_sceneManager->ChangeScene(std::make_unique<StageSelectScene>());
		} else if (result == GameClearResult::Title) {
			m_sceneManager->ChangeScene(std::make_unique<TitleScene>());
		}
		return; // クリア時は以降の回転やタイマー更新をストップ
	}

	// 5. 通常時の入力処理（クリックでパイプ回転）
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

	// 6. ゲームオーバー中の処理
	if (m_isGameOver) {
		GameOverResult result = m_gameOver->Update();

		if (result == GameOverResult::Retry) {
			m_sceneManager->ChangeScene(std::make_unique<GameScene>(m_stageWidth, m_stageHeight));
		} else if (result == GameOverResult::StageSelect) {
			m_sceneManager->ChangeScene(std::make_unique<StageSelectScene>());
		} else if (result == GameOverResult::Title) {
			m_sceneManager->ChangeScene(std::make_unique<TitleScene>());
		}
		return;
	}

	// タイマー更新
	m_timer->Update(deltaTime);
	if (m_timer->IsFinished() && !m_isGameOver) {
		m_isGameOver = true; // タイムアップ

		// GameScene の BGM を停止して GameOver の BGM を再生
		Audio::GetInstance()->StopWave(m_playHandle);
		m_gameOver->PlayBGM();
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
	// 回路の通電状態を最新化
	CircuitSolver::UpdatePower(m_board, m_startX, m_startY);

	// パイプが回転して通電が変わった直後にゴール状態も同期更新
	m_goal.Update(m_board, 0.0f);

	m_renderer.UpdateBuffers(m_board, m_chargeProgress, m_goalX, m_goalY);
}

void GameScene::Render(ID3D12GraphicsCommandList* commandList) {
	// 1. 背景の画像描画
	if (m_bgSprite) {
		Sprite::PreDraw(commandList);
		m_bgSprite->Draw();
		Sprite::PostDraw();
	}

	// 2. パイプ・スタート・ゴールの描画（3D）
	m_renderer.Render(commandList);
	m_start.Render(commandList);
	m_goal.Render(commandList);

	// 3. UI・スプライト描画
	Sprite::PreDraw(commandList);

	// ★ クリア演出描画（GameClearクラスに統一）
	if (m_goal.IsReached() && m_gameClear) {
		m_gameClear->Draw();
	}

	// タイマー
	if (m_timer) {
		m_timer->Draw();
	}

	// ゲームオーバー
	if (m_isGameOver && m_gameOver) {
		m_gameOver->Draw();
	}

	Sprite::PostDraw();
}

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

		// ★ スタートマス（0,0）の接続口を「右」と「下」の両方に開けておく
		m_board.GetTile(m_startX, m_startY).mask = (RIGHT | DOWN);

		// ★ ゴールマス・スタートマス以外の行き止まりマスを補正する
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if ((x == m_startX && y == m_startY) || (x == m_goalX && y == m_goalY)) {
					continue;
				}

				uint8_t m = m_board.GetTile(x, y).mask;

				// 開口が1つしかない場合（例: LEFTだけ）
				if (m == UP || m == DOWN) {
					m_board.GetTile(x, y).mask = (UP | DOWN); // 縦I字にする
				} else if (m == LEFT || m == RIGHT) {
					m_board.GetTile(x, y).mask = (LEFT | RIGHT); // 横I字にする
				}
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

	m_board.GetTile(m_startX, m_startY).isLocked = true;
}