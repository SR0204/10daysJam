#pragma once
#include "2d/Sprite.h"
#include "KamataEngine.h"
#include <DirectXMath.h>
#include <array>

class CountDownTimer {
public:
	CountDownTimer() = default;
	~CountDownTimer();

	// 初期化（引数で制限時間を秒単位で指定。デフォルト180秒＝3分）
	void Initialize(float limitTimeSeconds = 180.0f);

	// 更新処理
	void Update(float deltaTime);

	// 描画処理
	void Draw();

	// 終了判定
	bool IsFinished() const { return m_timeRemaining <= 0.0f; }

	// 残り時間（秒）の取得
	float GetTimeRemaining() const { return m_timeRemaining; }

private:
	float m_timeRemaining = 180.0f; // 残り時間（秒）
	bool m_isFinished = false;

	// テクスチャハンドル
	std::array<uint32_t, 10> m_texNumbers{};
	uint32_t m_texColon = 0;

	// スプライト
	KamataEngine::Sprite* m_spriteMin = nullptr;
	KamataEngine::Sprite* m_spriteColon = nullptr;
	KamataEngine::Sprite* m_spriteSec10 = nullptr;
	KamataEngine::Sprite* m_spriteSec1 = nullptr;
};