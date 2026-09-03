#define NOMINMAX
#include "KamataEngine.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

using namespace KamataEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	KamataEngine::Initialize(L"4063_解路 ");

	// ★ SceneManager の生成と最初のシーン (TitleScene) の設定
	SceneManager sceneManager;
	sceneManager.ChangeScene(std::make_unique<TitleScene>());

	// メインループ
	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		// シーン更新
		sceneManager.Update(0.016f);

		DirectXCommon::GetInstance()->PreDraw();

		// シーン描画
		ID3D12GraphicsCommandList* commandList = DirectXCommon::GetInstance()->GetCommandList();
		sceneManager.Render(commandList);

		DirectXCommon::GetInstance()->PostDraw();
	}

	KamataEngine::Finalize();

	return 0;
}