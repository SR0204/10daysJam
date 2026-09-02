#include "GameScene.h"
#include "CircuitSolver.h"
#include "SceneManager.h"
#include "StageSelectScene.h"
#include <algorithm>
#include <cassert>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <random>
#include <stack>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace KamataEngine;

// 頂点構造体
struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

// 画面全体を埋める 10x10 グリッドで初期化
// 左上 (0,0) をスタート、右下 (9,9) をゴールに指定
GameScene::GameScene(int boardWidth, int boardHeight) : m_board(boardWidth, boardHeight), m_startX(0), m_startY(0), m_goalX(boardWidth - 1), m_goalY(boardHeight - 1) {}

// ----------------------------------------------------
// 全マスを巡回する唯一の正解ルート（迷路）を生成
// ----------------------------------------------------
void GameScene::GenerateStage() {
	int width = m_board.GetWidth();
	int height = m_board.GetHeight();

	std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));

	struct Point {
		int x, y;
	};
	std::stack<Point> pathStack;

	std::random_device rd;
	std::mt19937 g(rd());

	bool success = false;

	// 全マス(100マス)を通るまでリトライして生成
	while (!success) {
		m_board.Clear();
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				visited[y][x] = false;
			}
		}
		while (!pathStack.empty()) {
			pathStack.pop();
		}

		int currentX = m_startX;
		int currentY = m_startY;
		visited[currentY][currentX] = true;
		pathStack.push({currentX, currentY});

		while (!pathStack.empty()) {
			Point p = pathStack.top();

			struct Neighbor {
				int x, y;
				Dir dir;
				Dir oppositeDir;
			};
			std::vector<Neighbor> neighbors;

			if (p.y > 0 && !visited[p.y - 1][p.x])
				neighbors.push_back({p.x, p.y - 1, UP, DOWN});
			if (p.x < width - 1 && !visited[p.y][p.x + 1])
				neighbors.push_back({p.x + 1, p.y, RIGHT, LEFT});
			if (p.y < height - 1 && !visited[p.y + 1][p.x])
				neighbors.push_back({p.x, p.y + 1, DOWN, UP});
			if (p.x > 0 && !visited[p.y][p.x - 1])
				neighbors.push_back({p.x - 1, p.y, LEFT, RIGHT});

			if (!neighbors.empty()) {
				std::shuffle(neighbors.begin(), neighbors.end(), g);
				Neighbor next = neighbors[0];

				m_board.GetTile(p.x, p.y).mask |= next.dir;
				m_board.GetTile(next.x, next.y).mask |= next.oppositeDir;

				visited[next.y][next.x] = true;
				pathStack.push({next.x, next.y});
			} else {
				pathStack.pop();
			}
		}

		// ★ 1. スタート地点 (0,0) を十字（全方位）パイプに変更
		// ※ DFSで生成された元々の向きを残しつつ、全方位（UP|RIGHT|DOWN|LEFT）を設定する
		m_board.GetTile(m_startX, m_startY).mask = UP | RIGHT | DOWN | LEFT;

		// ★ 2. ゴール地点 (9,9) の回転を固定（ロック）
		m_board.GetTile(m_goalX, m_goalY).isLocked = true;

		// 全マスを通過できたか判定
		int visitedCount = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				if (visited[y][x])
					visitedCount++;
			}
		}

		if (visitedCount == width * height && m_board.GetTile(m_goalX, m_goalY).mask != NONE) {
			success = true;
		}
	}

	// スタート位置 (0,0) を画面左端枠外に接続
	m_board.GetTile(m_startX, m_startY).mask |= LEFT;
}

void GameScene::Initialize() {
	// 1. 全マス通過の正解ルートを生成
	GenerateStage();

	// 2. タイルのランダム回転（ロックされていないマスのみ回転）
	std::random_device rd;
	std::mt19937 g(rd());
	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			// ロックされているマス（ゴールなど）はシャッフルしない
			if (m_board.GetTile(x, y).isLocked)
				continue;

			int rotations = g() % 4;
			for (int r = 0; r < rotations; ++r) {
				m_board.RotateTile(x, y);
			}
		}
	}

	// デバイスの取得
	ID3D12Device* device = KamataEngine::DirectXCommon::GetInstance()->GetDevice();
	assert(device != nullptr && "DirectX12 Device の取得に失敗しました！");

	// 板ポリゴンを基準サイズ (1.0 x 1.0) で定義
	Vertex vertices[] = {
	    {{-0.5f, 0.5f, 0.0f},  {0.0f, 0.0f}}, // 左上
	    {{0.5f, 0.5f, 0.0f},   {1.0f, 0.0f}}, // 右上
	    {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}}, // 左下
	    {{0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}}, // 右下
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

	// インスタンスバッファ (100マス分)
	UINT instBufferSize = sizeof(PipeInstanceData) * m_board.GetWidth() * m_board.GetHeight();
	auto instResDesc = CD3DX12_RESOURCE_DESC::Buffer(instBufferSize);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_instanceBuffer));
	m_instanceBufferGPUAddress = m_instanceBuffer->GetGPUVirtualAddress();

	// シェーダーのコンパイル
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

	HRESULT hrVS = D3DCompileFromFile(L"PipeVS.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
	if (FAILED(hrVS)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "PipeVS.hlsl のコンパイルに失敗しました！");
	}

	HRESULT hrPS = D3DCompileFromFile(L"PipePS.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
	if (FAILED(hrPS)) {
		if (errorBlob)
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		assert(false && "PipePS.hlsl のコンパイルに失敗しました！");
	}

	// ルートシグネチャ作成
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
		assert(false && "ルートシグネチャのシリアライズに失敗しました");
	}

	HRESULT hrRoot = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hrRoot)) {
		assert(false && "CreateRootSignature に失敗しました");
	}

	// PSO 作成
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

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
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hrPSO = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hrPSO)) {
		assert(false && "CreateGraphicsPipelineState に失敗しました");
	}

	RefreshCircuit();
}

void GameScene::Update(float /*deltaTime*/) {

	// ★ クリア時に SPACE キーでステージセレクトへ戻る
	if (m_isCleared && Input::GetInstance()->PushKey(DIK_SPACE)) {
		m_sceneManager->ChangeScene(std::make_unique<StageSelectScene>());
		return;
	}

	Input* input = Input::GetInstance();
	if (input->IsTriggerMouse(0)) {
		POINT mousePos;
		GetCursorPos(&mousePos);

		HWND hwnd = WinApp::GetInstance()->GetHwnd();
		ScreenToClient(hwnd, &mousePos);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		OnMouseDown(mousePos.x, mousePos.y, width, height);
	}
}

void GameScene::OnMouseDown(int screenX, int screenY, int windowWidth, int windowHeight) {
	float ndcX = (2.0f * screenX / windowWidth) - 1.0f;
	float ndcY = -(2.0f * screenY / windowHeight) + 1.0f;

	float aspectRatio = 1280.0f / 720.0f;
	float tileSizeY = 2.0f / static_cast<float>(m_board.GetHeight());
	float tileSizeX = tileSizeY / aspectRatio;

	float totalWidthX = tileSizeX * static_cast<float>(m_board.GetWidth());
	float startX = -totalWidthX * 0.5f + (tileSizeX * 0.5f);
	float startY = 1.0f - (tileSizeY * 0.5f);

	float halfX = tileSizeX * 0.5f;
	float halfY = tileSizeY * 0.5f;

	for (int y = 0; y < m_board.GetHeight(); ++y) {
		for (int x = 0; x < m_board.GetWidth(); ++x) {
			float tileCenterX = startX + (x * tileSizeX);
			float tileCenterY = startY - (y * tileSizeY);

			if (ndcX >= (tileCenterX - halfX) && ndcX <= (tileCenterX + halfX) && ndcY >= (tileCenterY - halfY) && ndcY <= (tileCenterY + halfY)) {

				m_board.RotateTile(x, y);
				RefreshCircuit();
				return;
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

	// 画面アスペクト比（16:9）から必要な横マス数を算出 (10 * (16/9) = 17.77... -> 18マス)
	// float aspectRatio = 1280.0f / 720.0f;
	// int boardHeight = 10;
	// int boardWidth = static_cast<int>(std::ceil(boardHeight * aspectRatio));

	//// もし盤面サイズが変わっていたらリサイズ（ゴール位置も右下に再設定）
	// if (m_board.GetWidth() != boardWidth || m_board.GetHeight() != boardHeight) {
	//	m_board = Board(boardWidth, boardHeight);
	//	m_goalX = boardWidth - 1;
	//	m_goalY = boardHeight - 1;
	//	GenerateStage();
	// }

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
			inst.isGoal = (x == m_goalX && y == m_goalY) ? 1 : 0;

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
	UINT instanceCount = static_cast<UINT>(m_instanceData.size());
	commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, 0, 0, 0);
}