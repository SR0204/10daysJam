#pragma once
#include "2d/Sprite.h"
#include <cstdint>

// ゲームオーバー後のプレイヤーの選択状態
enum class GameOverResult {
	None,        // 継続中
	Retry,       // Rキー：リトライ
	StageSelect, // SPACEキー：ステージセレクトへ戻る
	Title        // Tキー：タイトル画面へ戻る
};

class GameOver {
public:
	GameOver() = default;
	~GameOver();

	// 初期化（画像読み込み・配置）
	void Initialize();

	// 更新処理（入力判定）
	GameOverResult Update();

	// 描画処理
	void Draw();

	void PlayBGM();

	void StopBGM();

private:
	uint32_t m_texGameOver = 0;
	KamataEngine::Sprite* m_spriteGameOver = nullptr;

	// ★ BGM用
	uint32_t m_bgmHandle = 0;
	uint32_t m_playHandle = 0;
};