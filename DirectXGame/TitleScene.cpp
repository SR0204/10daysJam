#include "TitleScene.h"
#include "SceneManager.h"
#include "StageSelectScene.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "base/WinApp.h"
#include "input/Input.h"
#include <cassert>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "TutorialScene.h"

using namespace DirectX;
using namespace KamataEngine;

TitleScene::TitleScene() : m_board(1, 1) {}

void TitleScene::Initialize() {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	assert(device != nullptr);

	// 1. 頂点バッファ・インデックスバッファ作成
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

	UINT instBufferSize = sizeof(PipeInstanceData);
	auto instResDesc = CD3DX12_RESOURCE_DESC::Buffer(instBufferSize);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
	m_instanceBufferGPUAddress = m_instanceBuffer->GetGPUVirtualAddress();

	// 2. ★ここで先にシェーダーをコンパイルする！
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

	HRESULT hrVS = D3DCompileFromFile(L"PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hrVS)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "PipeVS.hlsl のコンパイルに失敗しました。出力ウィンドウを確認してください。");
	}

	HRESULT hrPS = D3DCompileFromFile(L"PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hrPS)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "PipePS.hlsl のコンパイルに失敗しました。出力ウィンドウを確認してください。");
	}

	// 3. テクスチャ＆ルートシグネチャ作成
	m_textureHandleOn = TextureManager::Load("Title/Title.png");

	D3D12_ROOT_PARAMETER rootParams[3] = {};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_DESCRIPTOR_RANGE rangeOff = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable = {1, &rangeOff};
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_DESCRIPTOR_RANGE rangeOn = {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable = {1, &rangeOn};
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ShaderRegister = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {_countof(rootParams), rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// 4. ★コンパイル済みの vsBlob / psBlob を使って PSO を作成する
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

	// SPACE キーでステージセレクトへ遷移
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		m_sceneManager->ChangeScene(std::make_unique<TutorialScene>());
	}
}

void TitleScene::UpdateInstanceBuffers() {
	m_instanceData.clear();

	// 画面全体 (-1.0 ~ 1.0) を覆うように拡大行列を作成 (幅2.0, 高さ2.0)
	PipeInstanceData inst;
	XMMATRIX matScale = XMMatrixScaling(2.0f, 2.0f, 1.0f);
	XMMATRIX matTrans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	XMMATRIX matWorld = matScale * matTrans;

	XMStoreFloat4x4(&inst.worldMatrix, XMMatrixTranspose(matWorld));

	inst.isPowered = 1;
	inst.mask = 0;
	inst.isGoal = 0;
	inst.chargeProgress = 1.0f; // 100% 表示

	m_instanceData.push_back(inst);

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

	// t0: インスタンスバッファ
	commandList->SetGraphicsRootShaderResourceView(0, m_instanceBufferGPUAddress);

	// t1, t2: テクスチャのバインド
	// TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 1, m_textureHandleOff);
	TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 2, m_textureHandleOn);

	// 1つの大判スプライトとして描画
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}