#include "PipeRenderer.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include <cassert>
#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;
using namespace KamataEngine;

struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

void PipeRenderer::Initialize(int boardWidth, int boardHeight) {
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();

	Vertex vertices[] = {
	    // pos                       uv
	    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}}, // 左上
	    {{0.5f, -0.5f, 0.0f},  {1.0f, 0.0f}}, // 右上
	    {{-0.5f, 0.5f, 0.0f},  {0.0f, 1.0f}}, // 左下
	    {{0.5f, 0.5f, 0.0f},   {1.0f, 1.0f}}, // 右下
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

	UINT instBufferSize = sizeof(PipeInstanceData) * boardWidth * boardHeight;
	auto instResDesc = CD3DX12_RESOURCE_DESC::Buffer(instBufferSize);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
	m_instanceBufferGPUAddress = m_instanceBuffer->GetGPUVirtualAddress();

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
	D3DCompileFromFile(L"PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	D3DCompileFromFile(L"PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);

	// 各テクスチャの読み込み
	m_texOff[(int)PipeType::I] = TextureManager::Load("Pipe/I_Pipe/I_Pipe_Off.png");
	m_texOn[(int)PipeType::I] = TextureManager::Load("Pipe/I_Pipe/I_Pipe_On.png");
	m_texOff[(int)PipeType::L] = TextureManager::Load("Pipe/L_Pipe/L_Pipe_Off.png");
	m_texOn[(int)PipeType::L] = TextureManager::Load("Pipe/L_Pipe/L_Pipe_On.png");
	m_texOff[(int)PipeType::T] = TextureManager::Load("Pipe/T_Pipe/T_Pipe_Off.png");
	m_texOn[(int)PipeType::T] = TextureManager::Load("Pipe/T_Pipe/T_Pipe_On.png");

	// CROSS用
	m_texOff[(int)PipeType::CROSS] = m_texOff[(int)PipeType::T];
	m_texOn[(int)PipeType::CROSS] = m_texOn[(int)PipeType::T];

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

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {_countof(rootParams), rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
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

	// アルファブレンディング設定
	D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
	blendDesc.BlendEnable = TRUE;
	blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.BlendState.RenderTarget[0] = blendDesc;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
}

void PipeRenderer::UpdateBuffers(const Board& board, const std::vector<std::vector<float>>& chargeProgress, int goalX, int goalY) {
	for (int i = 0; i < (int)PipeType::COUNT; ++i) {
		m_instanceDataGroup[i].clear();
	}
	m_allInstanceData.clear();

	float aspectRatio = 1280.0f / 720.0f;

	// 1マスのサイズ（画面Y方向基準）
	float tileSizeY = 2.0f / static_cast<float>(board.GetHeight());
	// 画面上で見た目を「完全な正方形」にするXサイズ
	float tileSizeX = tileSizeY / aspectRatio;

	float totalWidthX = tileSizeX * static_cast<float>(board.GetWidth());
	float startX = -totalWidthX * 0.5f + (tileSizeX * 0.5f);
	float startY = 1.0f - (tileSizeY * 0.5f);

	for (int y = 0; y < board.GetHeight(); ++y) {
		for (int x = 0; x < board.GetWidth(); ++x) {
			const Tile& tile = board.GetTile(x, y);
			PipeTransformInfo info = PipeTransformHelper::GetInfoFromMask(tile.mask);

			PipeInstanceData inst;
			float posX = startX + (x * tileSizeX);
			float posY = startY - (y * tileSizeY);

			// ★ 修正：正しく均等に回転させる行列の組み立て
			// 1. まず1マスの基準サイズ(tileSizeY)で等倍拡大（正方形を維持）
			XMMATRIX matScaleBase = XMMatrixScaling(tileSizeY, tileSizeY, 1.0f);

			// 2. 正方形のまま回転
			XMMATRIX matRot = XMMatrixRotationZ(info.rotationAngle);

			// 3. 画面のアスペクト比（16:9）に合わせてX方向のみ潰す補正
			XMMATRIX matAspect = XMMatrixScaling(1.0f / aspectRatio, 1.0f, 1.0f);

			// 4. マスへの移動
			XMMATRIX matTrans = XMMatrixTranslation(posX, posY, 0.0f);

			// 合成順序： 等倍スケール -> 回転 -> アスペクト比補正 -> 平行移動
			XMMATRIX matWorld = matScaleBase * matRot * matAspect * matTrans;

			XMStoreFloat4x4(&inst.worldMatrix, XMMatrixTranspose(matWorld));

			inst.isPowered = tile.isPowered ? 1 : 0;
			inst.mask = static_cast<int>(tile.mask);
			inst.isGoal = (x == goalX && y == goalY) ? 1 : 0;
			inst.chargeProgress = chargeProgress[y][x];

			m_instanceDataGroup[(int)info.type].push_back(inst);
		}
	}

	for (int i = 0; i < (int)PipeType::COUNT; ++i) {
		m_allInstanceData.insert(m_allInstanceData.end(), m_instanceDataGroup[i].begin(), m_instanceDataGroup[i].end());
	}

	if (m_instanceBuffer && !m_allInstanceData.empty()) {
		void* mappedData = nullptr;
		m_instanceBuffer->Map(0, nullptr, &mappedData);
		memcpy(mappedData, m_allInstanceData.data(), sizeof(PipeInstanceData) * m_allInstanceData.size());
		m_instanceBuffer->Unmap(0, nullptr);
	}
}

void PipeRenderer::Render(ID3D12GraphicsCommandList* commandList) {
	if (!commandList || m_allInstanceData.empty())
		return;

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	UINT startInstanceLocation = 0;

	for (int i = 0; i < (int)PipeType::COUNT; ++i) {
		UINT instanceCount = static_cast<UINT>(m_instanceDataGroup[i].size());
		if (instanceCount == 0)
			continue;

		D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = m_instanceBufferGPUAddress + (startInstanceLocation * sizeof(PipeInstanceData));
		commandList->SetGraphicsRootShaderResourceView(0, cbvGpuAddress);

		TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 1, m_texOff[i]);
		TextureManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, 2, m_texOn[i]);

		commandList->DrawIndexedInstanced(6, instanceCount, 0, 0, 0);

		startInstanceLocation += instanceCount;
	}
}