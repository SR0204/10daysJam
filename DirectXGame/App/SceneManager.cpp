#include "SceneManager.h"

void SceneManager::ChangeScene(std::unique_ptr<IScene> nextScene) { m_nextScene = std::move(nextScene); }

void SceneManager::Update(float deltaTime) {
	// シーン切り替え処理
	if (m_nextScene) {
		m_currentScene = std::move(m_nextScene);
		m_currentScene->SetSceneManager(this);
		m_currentScene->Initialize();
	}

	if (m_currentScene) {
		m_currentScene->Update(deltaTime);
	}
}

void SceneManager::Render(ID3D12GraphicsCommandList* commandList) {
	if (m_currentScene) {
		m_currentScene->Render(commandList);
	}
}