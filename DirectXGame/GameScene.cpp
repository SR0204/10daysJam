#include "GameScene.h"
#include "CircuitSolver.h"
#include <cassert>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace KamataEngine;

// 頂点構造体
struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

GameScene::GameScene() : m_board(6, 5), m_startX(0), m_startY(2), m_goalX(5), m_goalY(2) {}

void GameScene::Initialize() {
	m_board.Clear();

	// 画面の見た目通りのパイプ配置
	m_board.GetTile(0, 2).mask = LEFT | RIGHT; // (0,2): 横直線
	m_board.GetTile(1, 2).mask = LEFT | DOWN;  // (1,2): 左から下へ曲がる
	m_board.GetTile(1, 3).mask = UP | RIGHT;   // (1,3): 上から右へ曲がる
	m_board.GetTile(2, 3).mask = LEFT | RIGHT; // (2,3): 横直線
	m_board.GetTile(3, 3).mask = LEFT | UP;    // (3,3): 左から上へ曲がる
	m_board.GetTile(3, 2).mask = DOWN | RIGHT; // (3,2): 下から右へ曲がる
	m_board.GetTile(4, 2).mask = LEFT | RIGHT; // (4,2): 横直線
	m_board.GetTile(5, 2).mask = LEFT | RIGHT; // (5,2): 横直線

	// デバイスの取得とNULLチェック
	ID3D12Device* device = KamataEngine::DirectXCommon::GetInstance()->GetDevice();
	assert(device != nullptr && "DirectX12 Device の取得に失敗しました！");

	// ----------------------------------------------------
	// 1. 板ポリゴン（正方形）の頂点＆インデックスバッファ生成
	// ----------------------------------------------------
	Vertex vertices[] = {
	    {{-0.08f, 0.08f, 0.0f},  {0.0f, 0.0f}}, // 左上
	    {{0.08f, 0.08f, 0.0f},   {1.0f, 0.0f}}, // 右上
	    {{-0.08f, -0.08f, 0.0f}, {0.0f, 1.0f}}, // 左下
	    {{0.08f, -0.08f, 0.0f},  {1.0f, 1.0f}}, // 右下
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

	// ----------------------------------------------------
	// 2. インスタンスデータ用GPUバッファ（StructuredBuffer）の生成
	// ----------------------------------------------------
	UINT instBufferSize = sizeof(PipeInstanceData) * 30;
	auto instResDesc = CD3DX12_RESOURCE_DESC::Buffer(instBufferSize);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
	m_instanceBufferGPUAddress = m_instanceBuffer->GetGPUVirtualAddress();

	// ----------------------------------------------------
	// 3. シェーダーのコンパイル (VS / PS)
	// ----------------------------------------------------
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

	HRESULT hrVS = D3DCompileFromFile(L"PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hrVS)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "PipeVS.hlsl の読み込み/コンパイルに失敗しました！");
	}

	HRESULT hrPS = D3DCompileFromFile(L"PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hrPS)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "PipePS.hlsl の読み込み/コンパイルに失敗しました！");
	}

	// ----------------------------------------------------
	// 4. ルートシグネチャ & PSO の作成
	// ----------------------------------------------------
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam.Descriptor.ShaderRegister = 0;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 1;
	rootSigDesc.pParameters = &rootParam;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	HRESULT hrSig = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hrSig)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "ルートシグネチャのシリアライズに失敗しました");
	}

	HRESULT hrRoot = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hrRoot)) {
		assert(false && "CreateRootSignature に失敗しました");
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 両面描画

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ★KamataEngineの標準フォーマットへ変更
	psoDesc.SampleDesc.Count = 1;

	HRESULT hrPSO = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hrPSO)) {
		assert(false && "CreateGraphicsPipelineState (PSO作成) に失敗しました");
	}

	RefreshCircuit();
}

void GameScene::Update(float /*deltaTime*/) {
	Input* input = Input::GetInstance();
	if (input->IsTriggerMouse(0)) { // 左クリック
		POINT mousePos;
		GetCursorPos(&mousePos);

		// ウィンドウハンドルを取得してクライアント領域座標に変換
		HWND hwnd = WinApp::GetInstance()->GetHwnd();
		ScreenToClient(hwnd, &mousePos);

		// ウィンドウのクライアントサイズを取得
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		OnMouseDown(mousePos.x, mousePos.y, width, height);
	}
}

void GameScene::OnMouseDown(int screenX, int screenY, int windowWidth, int windowHeight) {
	// 1. スクリーン座標を NDC 座標 (-1.0 ~ 1.0) に変換
	float ndcX = (2.0f * screenX / windowWidth) - 1.0f;
	float ndcY = -(2.0f * screenY / windowHeight) + 1.0f; // Y軸は上下反転

	// 2. タイル判定用の半サイズ
	float halfTile = m_tileSize * 0.5f;

	// 3. 画面上の全タイルの描画矩形と当たり判定
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			// タイルの中心座標（UpdateInstanceBuffersと同じ計算）
			float tileCenterX = m_boardOffset.x + (x * m_tileSize);
			float tileCenterY = m_boardOffset.y - (y * m_tileSize);

			// クリック座標がタイルの範囲内にあるかチェック
			if (ndcX >= (tileCenterX - halfTile) && ndcX <= (tileCenterX + halfTile) && ndcY >= (tileCenterY - halfTile) && ndcY <= (tileCenterY + halfTile)) {

				// 判定が一致したマスのみを回転
				m_board.RotateTile(x, y);
				RefreshCircuit();
				return; // 1つのマスが反応したら終了
			}
		}
	}
}

void GameScene::RefreshCircuit() {
	CircuitSolver::UpdatePower(m_board, m_startX, m_startY);
	m_isCleared = m_board.GetTile(m_goalX, m_goalY).isPowered;
	UpdateInstanceBuffers();
}

void GameScene::UpdateInstanceBuffers() {
	m_instanceData.clear();
	m_instanceData.reserve(m_board.GetWidth() * m_board.GetHeight());

	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			const Tile& tile = m_board.GetTile(x, y);

			PipeInstanceData inst;
			float posX = m_boardOffset.x + (x * m_tileSize);
			float posY = m_boardOffset.y - (y * m_tileSize);

			// ★ XMMatrixTranspose を除去してそのまま直接入れる
			XMMATRIX matWorld = XMMatrixTranslation(posX, posY, 0.0f);
			XMStoreFloat4x4(&inst.worldMatrix, matWorld);

			inst.isPowered = tile.isPowered ? 1 : 0;
			inst.mask = static_cast<int>(tile.mask);

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

void GameScene::Render(ID3D12GraphicsCommandList* commandList) {
	if (!commandList || m_instanceData.empty())
		return;
	if (!m_rootSignature || !m_pipelineState)
		return;

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->SetGraphicsRootShaderResourceView(0, m_instanceBufferGPUAddress);

	UINT indexCountPerInstance = 6;
	UINT instanceCount = static_cast<UINT>(m_instanceData.size()); // 30が入っているか
	commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, 0, 0, 0);
}