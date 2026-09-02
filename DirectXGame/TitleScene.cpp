#include "TitleScene.h"
#include "base/DirectXCommon.h"
#include "input/Input.h"
#include "SceneManager.h"
#include "StageSelectScene.h"
#include "base/WinApp.h"
#include <cassert>
#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;
using namespace KamataEngine;

TitleScene::TitleScene() : m_board(18, 10) {}

void TitleScene::Initialize() {
	// 盤面初期化：タイトルロゴ風のグラフィック用にパターン配置 (背景・通電アニメーション付き)
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			Tile& tile = m_board.GetTile(x, y);
			tile.mask = UP | RIGHT | DOWN | LEFT; // 十字パイプで統一
			tile.isPowered = true;                // ★ すべて「通電（ON）」にして一色にする（消したい場合は false）
		}
	}

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	assert(device != nullptr);

	// GameScene と同じ頂点・インデックス定義
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
	Vertex vertices[] = {
	    {{-0.5f, 0.5f, 0.0f},  {0.0f, 0.0f}},
	    {{0.5f, 0.5f, 0.0f},   {1.0f, 0.0f}},
	    {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
	    {{0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}},
	};
	uint16_t indices[] = {0, 1, 2, 2, 1, 3};

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	auto vertResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

	void* vertData = nullptr;
	m_vertexBuffer->Map(0, nullptr, &vertData);
	memcpy(vertData, vertices, sizeof(vertices));
	m_vertexBuffer->Unmap(0, nullptr);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizeof(vertices);
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);

	auto idxResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices));
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &idxResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));

	void* idxData = nullptr;
	m_indexBuffer->Map(0, nullptr, &idxData);
	memcpy(idxData, indices, sizeof(indices));
	m_indexBuffer->Unmap(0, nullptr);

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = sizeof(indices);
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

	// インスタンスバッファ
	UINT instBufferSize = sizeof(PipeInstanceData) * m_board.GetWidth() * m_board.GetHeight();
	auto instResDesc = CD3DX12_RESOURCE_DESC::Buffer(instBufferSize);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
	m_instanceBufferGPUAddress = m_instanceBuffer->GetGPUVirtualAddress();

	// シェーダー・パイプライン読み込み (PipeVS.hlsl / PipePS.hlsl 流用)
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	D3DCompileFromFile(L"PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	D3DCompileFromFile(L"PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam.Descriptor.ShaderRegister = 0;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = &rootParam;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	UpdateInstanceBuffers();
}

void TitleScene::Update(float deltaTime) {
	m_animationTimer += deltaTime;

	// 点滅アニメーション（一定時間ごとに通電パターン変更）
	/*if (m_animationTimer >= 0.5f) {
		m_animationTimer = 0.0f;
		for (int y = 0; y < m_board.GetHeight(); ++y) {
			for (int x = 0; x < m_board.GetWidth(); ++x) {
				Tile& tile = m_board.GetTile(x, y);
				tile.isPowered = !tile.isPowered;
			}
		}
		UpdateInstanceBuffers();
	}*/

	// SPACE キーでステージセレクトへ遷移
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		m_sceneManager->ChangeScene(std::make_unique<StageSelectScene>());
	}
}

void TitleScene::UpdateInstanceBuffers() {
	m_instanceData.clear();
	m_instanceData.reserve(m_board.GetWidth() * m_board.GetHeight());

	float aspectRatio = 1280.0f / 720.0f;
	float tileSizeY = 2.0f / static_cast<float>(m_board.GetHeight());
	float tileSizeX = tileSizeY / aspectRatio;

	float totalWidthX = tileSizeX * static_cast<float>(m_board.GetWidth());
	float startX = -totalWidthX * 0.5f + (tileSizeX * 0.5f);
	float startY = 1.0f - (tileSizeY * 0.5f);

	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			const Tile& tile = m_board.GetTile(x, y);

			PipeInstanceData inst;
			float posX = startX + (x * tileSizeX);
			float posY = startY - (y * tileSizeY);

			XMMATRIX matScale = XMMatrixScaling(tileSizeX, tileSizeY, 1.0f);
			XMMATRIX matTrans = XMMatrixTranslation(posX, posY, 0.0f);
			XMMATRIX matWorld = matScale * matTrans;

			XMStoreFloat4x4(&inst.worldMatrix, XMMatrixTranspose(matWorld));

			inst.isPowered = tile.isPowered ? 1 : 0;
			inst.mask = static_cast<int>(tile.mask);
			inst.isGoal = 0;

			m_instanceData.push_back(inst);
		}
	}

	if (m_instanceBuffer && !m_instanceData.empty()) {
		void* mappedData = nullptr;
		m_instanceBuffer->Map(0, nullptr, &mappedData);
		memcpy(mappedData, m_instanceData.data(), sizeof(PipeInstanceData) * m_instanceData.size());
		m_instanceBuffer->Unmap(0, nullptr);
	}
}

void TitleScene::Render(ID3D12GraphicsCommandList* commandList) {
	if (!commandList || m_instanceData.empty())
		return;

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->SetGraphicsRootShaderResourceView(0, m_instanceBufferGPUAddress);

	commandList->DrawIndexedInstanced(6, static_cast<UINT>(m_instanceData.size()), 0, 0, 0);
}