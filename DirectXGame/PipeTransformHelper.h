#pragma once
#include "Board.h"
#include <DirectXMath.h>
#include <cstdint>

enum class PipeType { I, L, T, CROSS, COUNT };

struct PipeTransformInfo {
	PipeType type;
	float rotationAngle; // ラジアン
};

class PipeTransformHelper {
public:
	static PipeTransformInfo GetInfoFromMask(uint8_t mask) {
		bool u = (mask & UP) != 0;
		bool r = (mask & RIGHT) != 0;
		bool d = (mask & DOWN) != 0;
		bool l = (mask & LEFT) != 0;

		// 1. 十字 (CROSS)
		if (u && r && d && l) {
			return {PipeType::CROSS, 0.0f};
		}

		// 2. T字 (★最初にうまくいっていた設定)
		if (u && l && r && !d)
			return {PipeType::T, 0.0f}; // ┴ (上・左・右)
		if (r && u && d && !l)
			return {PipeType::T, -DirectX::XM_PIDIV2}; // ├ (右・上・下)
		if (d && l && r && !u)
			return {PipeType::T, -DirectX::XM_PI}; // ┬ (下・左・右)
		if (l && u && d && !r)
			return {PipeType::T, -DirectX::XM_PIDIV2 * 3.0f}; // ┤ (左・上・下)

		// 3. L字 (★最初にうまくいっていた設定)
		if (d && r && !u && !l)
			return {PipeType::L, 0.0f}; // ┌ (下・右)
		if (d && l && !u && !r)
			return {PipeType::L, -DirectX::XM_PIDIV2}; // ┐ (下・左)
		if (u && l && !d && !r)
			return {PipeType::L, -DirectX::XM_PI}; // ┘ (上・左)
		if (u && r && !d && !l)
			return {PipeType::L, -DirectX::XM_PIDIV2 * 3.0f}; // └ (上・右)

		// 4. I字 (★判定が確実に通るようシンプル化)
		if (u && d)
			return {PipeType::I, 0.0f}; // ｜ (縦)
		if (l && r)
			return {PipeType::I, -DirectX::XM_PIDIV2}; // ― (横)

		// フォールバック
		if (u || d)
			return {PipeType::I, 0.0f};
		if (l || r)
			return {PipeType::I, -DirectX::XM_PIDIV2};

		return {PipeType::I, 0.0f};
	}
};