#pragma once
#include "../Game/Board.h"
#include "../Graphics/PipeTransformHelper.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>
#include <wrl/client.h>

struct PipeInstanceData {
	DirectX::XMFLOAT4X4 worldMatrix;
	int isPowered;
	int mask;
	int isGoal;
	float chargeProgress;
};

class PipeRenderer {
private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
	D3D12_GPU_VIRTUAL_ADDRESS m_instanceBufferGPUAddress = 0;

	uint32_t m_texOff[(int)PipeType::COUNT] = {};
	uint32_t m_texOn[(int)PipeType::COUNT] = {};

	std::vector<PipeInstanceData> m_instanceDataGroup[(int)PipeType::COUNT];
	std::vector<PipeInstanceData> m_allInstanceData;

public:
	PipeRenderer() = default;
	~PipeRenderer() = default;

	void Initialize(int boardWidth, int boardHeight);
	void UpdateBuffers(const Board& board, const std::vector<std::vector<float>>& chargeProgress, int goalX, int goalY);
	void Render(ID3D12GraphicsCommandList* commandList);
};