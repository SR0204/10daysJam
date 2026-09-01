#pragma once

#include "Board.h"
#include "CircuitSolver.h"
#include "KamataEngine.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl/client.h>

// DirectX 12に送るインスタンス構造体
struct PipeInstanceData {
	DirectX::XMFLOAT4X4 worldMatrix; // 描画位置・回転角度
	int isPowered;                   // 1: 通電（発光）, 0: 消灯
	int mask;                        // パイプの形状
	float padding[2];
};

// ----------------------------------------------------
// ゲームシーンクラス
// ----------------------------------------------------
class GameScene {
private:
	// 盤面データ（6x5 グリッド）
	Board m_board;

	// スタートとゴールの座標
	int m_startX = 0;
	int m_startY = 2;
	int m_goalX = 5;
	int m_goalY = 2;

	// ゲームクリア状態
	bool m_isCleared = false;

	// 描画設定（マスのサイズやオフセット）
	DirectX::XMFLOAT2 m_boardOffset = DirectX::XMFLOAT2(-0.5f, 0.4f);
	float m_tileSize = 0.2f;                        // タイルのサイズと間隔

	// インスタンシング描画用データ
	std::vector<PipeInstanceData> m_instanceData;

	// --- DirectX 12 描画用メンバー変数 ---
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	// 頂点・インデックス・インスタンスバッファのリソース本体
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;

	// バッファビューとGPUアドレス
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
	D3D12_GPU_VIRTUAL_ADDRESS m_instanceBufferGPUAddress = 0;

public:
	GameScene();
	~GameScene() = default;

	void Initialize();
	void Update(float deltaTime);
	void Render(ID3D12GraphicsCommandList* commandList);
	void OnMouseDown(int screenX, int screenY, int windowWidth, int windowHeight);

	bool IsCleared() const { return m_isCleared; }

private:
	void RefreshCircuit();
	void UpdateInstanceBuffers();
};