#include "StageSelectScene.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "input/Input.h"
#include <cassert>
#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;
using namespace KamataEngine;

StageSelectScene::StageSelectScene() : m_board(18, 10) {}

void StageSelectScene::Initialize() {
	// 18x10 のグリッド上に「1」と「2」を描画するドット配列 (1: ON, 0: OFF)
	const int fontMap[10][18] = {
	    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
	    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
        {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	};

	// 盤面タイルへの反映
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			Tile& tile = m_board.GetTile(x, y);
			tile.mask = UP | RIGHT | DOWN | LEFT; // 背景は全方位パイプ

			// ドット絵データに合わせて通電色を切り替える
			tile.isPowered = (fontMap[y][x] == 1);
		}
	}

	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	assert(device != nullptr);

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

	UINT instBufferSize = sizeof(PipeInstanceData) * m_board.GetWidth() * m_board.GetHeight();
	auto instResDesc = CD3DX12_RESOURCE_DESC::Buffer(instBufferSize);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
	m_instanceBufferGPUAddress = m_instanceBuffer->GetGPUVirtualAddress();

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	D3DCompileFromFile(L"PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	D3DCompileFromFile(L"PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);

	// テクスチャ読み込み
	// m_textureHandleOff = TextureManager::Load("Pipe/I_Pipe/I_Pipe_Off.png");
	// m_textureHandleOn = TextureManager::Load("Pipe/I_Pipe/I_Pipe_On.png");

	// ルートパラメータの設定 (t0: インスタンス, t1: テクスチャOFF, t2: テクスチャON)
	D3D12_ROOT_PARAMETER rootParams[3] = {};

	// t0: インスタンスデータ (StructuredBuffer)
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// t1: テクスチャOFF (Descriptor Table)
	D3D12_DESCRIPTOR_RANGE rangeOff = {};
	rangeOff.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rangeOff.NumDescriptors = 1;
	rangeOff.BaseShaderRegister = 1;
	rangeOff.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &rangeOff;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// t2: テクスチャON (Descriptor Table)
	D3D12_DESCRIPTOR_RANGE rangeOn = {};
	rangeOn.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rangeOn.NumDescriptors = 1;
	rangeOn.BaseShaderRegister = 2;
	rangeOn.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &rangeOn;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// サンプラー設定 (s0)
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &sampler;
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

void StageSelectScene::Update(float /*deltaTime*/) {
	auto input = Input::GetInstance();

	// [1] キー：EASY（10x6 の小さめマップ）
	if (input->PushKey(DIK_1)) {
		m_sceneManager->ChangeScene(std::make_unique<GameScene>(10, 6));
	}
	// [2] キー：NORMAL（18x10 の画面ピッタリ全画面マップ）
	else if (input->PushKey(DIK_2)) {
		m_sceneManager->ChangeScene(std::make_unique<GameScene>(18, 10));
	}
}

void StageSelectScene::UpdateInstanceBuffers() {
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

void StageSelectScene::Render(ID3D12GraphicsCommandList* commandList) {
	if (!commandList || m_instanceData.empty())
		return;

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	// t0: インスタンスバッファ
	commandList->SetGraphicsRootShaderResourceView(0, m_instanceBufferGPUAddress);

	// t1, t2: テクスチャデスクリプタテーブルのセット
	// TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 1, m_textureHandleOff);
	// TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 2, m_textureHandleOn);

	commandList->DrawIndexedInstanced(6, static_cast<UINT>(m_instanceData.size()), 0, 0, 0);
}