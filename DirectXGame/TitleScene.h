#pragma once
#include "Board.h"
#include "GameScene.h"
#include "IScene.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

class TitleScene : public IScene {
private:
	Board m_board;
	int m_startX = 0;
	int m_startY = 0;
	int m_goalX = 17;
	int m_goalY = 9;

	// Direct3D 12 リソース（GameSceneと同じ）
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS m_instanceBufferGPUAddress{};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	std::vector<PipeInstanceData> m_instanceData;
	float m_animationTimer = 0.0f;

	void UpdateInstanceBuffers();

public:
	TitleScene();
	~TitleScene() override = default;

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;
};