#include "../Scenes/StageSelectScene.h"
#include "../App/SceneManager.h"
#include "../Scenes/GameScene.h"
#include "audio/Audio.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "input/Input.h"
#include <cassert>
#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;
using namespace KamataEngine;

StageSelectScene::StageSelectScene() : m_board(18, 10) {}

StageSelectScene::~StageSelectScene() {
	// ★ スプライトの解放
	delete m_spriteNum1;
	delete m_spriteNum2;
}

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
	D3DCompileFromFile(L"Graphics/PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	D3DCompileFromFile(L"Graphics/PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);

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

	// ★ ステージナンバー画像の読み込みとスプライト作成
	m_texNum1 = TextureManager::Load("SelectNumber/SelectNumber_1.png");
	m_texNum2 = TextureManager::Load("SelectNumber/SelectNumber_2.png");

	m_spriteNum1 = Sprite::Create(m_texNum1, {0.0f, 0.0f});
	m_spriteNum2 = Sprite::Create(m_texNum2, {0.0f, 0.0f});

	// ★ 画像のサイズと配置場所の調整
	if (m_spriteNum1 && m_spriteNum2) {
		float winWidth = static_cast<float>(WinApp::kWindowWidth);
		float winHeight = static_cast<float>(WinApp::kWindowHeight);

		// 中心軸を中央に設定
		m_spriteNum1->SetAnchorPoint({0.5f, 0.5f});
		m_spriteNum2->SetAnchorPoint({0.5f, 0.5f});

		// 左右に並べて表示（間隔も少し調整できます）
		m_spriteNum1->SetPosition({winWidth * 0.35f, winHeight * 0.5f});
		m_spriteNum2->SetPosition({winWidth * 0.65f, winHeight * 0.5f});

		// ★ サイズを大きく変更（例: 200 -> 360 に拡大）
		float spriteSize = 360.0f;
		m_spriteNum1->SetSize({spriteSize, spriteSize});
		m_spriteNum2->SetSize({spriteSize, spriteSize});
	}

	// ★ BGMの読み込みとループ再生（音源ファイルのパスを指定してください）
	m_bgmHandle = Audio::GetInstance()->LoadWave("BGM/StageSelectSceneBGM.wav");
	m_playHandle = Audio::GetInstance()->PlayWave(m_bgmHandle, true, 0.5f);
}

void StageSelectScene::Update(float /*deltaTime*/) {
	auto input = Input::GetInstance();

	// [1] キー：EASY（10x6 の小さめマップ）
	if (input->PushKey(DIK_1)) {
		// ★ ステージセレクトへ遷移するタイミングでBGMを停止
		Audio::GetInstance()->StopWave(m_playHandle);
		Audio::GetInstance()->StopWave(m_bgmHandle);
		m_sceneManager->ChangeScene(std::make_unique<GameScene>(10, 6));
	}
	// [2] キー：NORMAL（18x10 の画面ピッタリ全画面マップ）
	else if (input->PushKey(DIK_2)) {
		// ★ ステージセレクトへ遷移するタイミングでBGMを停止
		Audio::GetInstance()->StopWave(m_playHandle);
		Audio::GetInstance()->StopWave(m_bgmHandle);
		m_sceneManager->ChangeScene(std::make_unique<GameScene>(18, 10));
	}
}

void StageSelectScene::UpdateInstanceBuffers() {
	m_instanceData.clear();
	m_instanceData.reserve(m_board.GetWidth() * m_board.GetHeight());

	float winWidth = static_cast<float>(WinApp::kWindowWidth);   // 1280.0f
	float winHeight = static_cast<float>(WinApp::kWindowHeight); // 720.0f
	float aspectRatio = winWidth / winHeight;                    // 16:9 (約1.777f)

	// ★ 1. 縦幅いっぱいに収まるベースのマスサイズ（NDC座標系は高さ全域で 2.0f）
	float tileSizeY = 2.0f / static_cast<float>(m_board.GetHeight());
	// ★ 正方形にするため、X方向のスケールにはアスペクト比で割った値を適用
	float tileSizeX = tileSizeY / aspectRatio;

	// ★ 2. 盤面全体の幅と高さ（NDC座標）
	float totalWidthNDC = tileSizeX * static_cast<float>(m_board.GetWidth());

	// ★ 3. 盤面を画面中央に配置するための開始オフセット（左上座標）
	float startX = -totalWidthNDC * 0.5f + (tileSizeX * 0.5f);
	float startY = 1.0f - (tileSizeY * 0.5f);

	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			const Tile& tile = m_board.GetTile(x, y);

			PipeInstanceData inst;
			float posX = startX + (x * tileSizeX);
			float posY = startY - (y * tileSizeY);

			// スケールと並進の行列作成（マスが歪まないよう正方形描画）
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
	if (!commandList)
		return;

	// 1. パイプ盤面（背景）の描画
	if (!m_instanceData.empty()) {
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

	// ★ 2. ステージナンバー画像（スプライト）の描画処理
	Sprite::PreDraw(commandList);

	if (m_spriteNum1) {
		m_spriteNum1->Draw();
	}
	if (m_spriteNum2) {
		m_spriteNum2->Draw();
	}

	Sprite::PostDraw();
}