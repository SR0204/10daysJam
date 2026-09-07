#pragma once
#include "../App/IScene.h"
#include <memory>

class SceneManager {
private:
	std::unique_ptr<IScene> m_currentScene;
	std::unique_ptr<IScene> m_nextScene;

public:
	void ChangeScene(std::unique_ptr<IScene> nextScene);
	void Update(float deltaTime);
	void Render(ID3D12GraphicsCommandList* commandList);
};