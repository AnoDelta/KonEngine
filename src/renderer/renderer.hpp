#pragma once

class IRenderer {
public:
	virtual ~IRenderer() = default;
	virtual void Init() = 0;
	virtual void Present() = 0;
	virtual void Clear(float r, float g, float b) = 0;
};

void InitRenderer();
void ClearBackground(float r, float g, float b);
