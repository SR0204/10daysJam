#pragma once
#include "2d/Sprite.h"
#include "KamataEngine.h"
#include <cstdint>

enum class GameClearResult {
	None,
	Retry,       // Rキー：同じステージでリトライ
	StageSelect, // SPACEキー：ステージセレクト
	Title        // Tキー：タイトル
};

class GameClear {
public:
	GameClear() = default;
	~GameClear();

	void Initialize();
	GameClearResult Update();
	void Draw();
	void PlayBGM();
	void StopBGM();

private:
	uint32_t m_texClear = 0;
	KamataEngine::Sprite* m_spriteClear = nullptr;

	uint32_t m_bgmHandle = 0;
	uint32_t m_playHandle = 0;
};