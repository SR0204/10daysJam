#pragma once

#include "Board.h"
#include "CircuitSolver.h"
#include "KamataEngine.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl/client.h>

// DirectX 12に送るインスタンス構造体 (計80バイトに統一)
struct PipeInstanceData {
	DirectX::XMFLOAT4X4 worldMatrix; // 64 bytes
	int isPowered;                   // 4 bytes
	int mask;                        // 4 bytes
	int isGoal;                      // 4 bytes
	float padding[1];                // 4 bytes (パディングを1個に修正)
};

class GameScene {
private:
	Board m_board;

	int m_startX = 0;
	int m_startY = 0;
	int m_goalX = 9;
	int m_goalY = 9;

	bool m_isCleared = false;

	// インスタンシング描画用データ
	std::vector<PipeInstanceData> m_instanceData;

	// --- DirectX 12 描画用メンバー変数 ---
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;

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
	void GenerateStage();
};