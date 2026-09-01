#define NOMINMAX
#include "GameScene.h" // 解路のGameScene
#include "KamataEngine.h"
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// ★ DirectX12 デバッグレイヤーの有効化（エラーの理由を出力ウィンドウに表示する）
#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	// 1. エンジンの初期化
	KamataEngine::Initialize(L"解路 - KAIRO");

	// 2. GameScene の生成と初期化
	GameScene* gameScene = new GameScene();
	gameScene->Initialize();

	// メインループ
	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		gameScene->Update(0.016f);

		DirectXCommon::GetInstance()->PreDraw();

		ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();
		gameScene->Render(commandList);

		DirectXCommon::GetInstance()->PostDraw();
	}

	delete gameScene;
	gameScene = nullptr;

	KamataEngine::Finalize();

	return 0;
}