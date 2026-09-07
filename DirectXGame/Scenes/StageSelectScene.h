#pragma once
#include "../App/IScene.h"
#include "../Game/Board.h"
#include "../Graphics/PipeRenderer.h"
#include "../Scenes/GameScene.h"
#include "2d/Sprite.h"
#include "KamataEngine.h"
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

	// ★ 数字表示用のスプライト変数
	uint32_t m_texNum1 = 0;
	uint32_t m_texNum2 = 0;
	KamataEngine::Sprite* m_spriteNum1 = nullptr;
	KamataEngine::Sprite* m_spriteNum2 = nullptr;

	// ★ BGM用の変数
	uint32_t m_bgmHandle = 0;
	uint32_t m_playHandle = 0;

public:
	StageSelectScene();
	~StageSelectScene();

	void Initialize() override;
	void Update(float deltaTime) override;
	void Render(ID3D12GraphicsCommandList* commandList) override;
};