#pragma once
#include <d3d12.h>

class SceneManager;

class IScene {
protected:
	SceneManager* m_sceneManager = nullptr;

public:
	virtual ~IScene() = default;
	void SetSceneManager(SceneManager* manager) { m_sceneManager = manager; }

	virtual void Initialize() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render(ID3D12GraphicsCommandList* commandList) = 0;
};