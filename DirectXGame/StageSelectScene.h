#pragma once
#include "Board.h"
#include "GameScene.h"
#include "IScene.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

class StageSelectScene : public IScene {
private:
	Board m_board;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS m_instanceBufferGPUAddress{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	std::vector<PipeInstanceData> m_instanceData;

	void UpdateInstanceBuffers();

public:
	StageSelectScene();
	~StageSelectScene() override = default;

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;
};