#pragma once

#include "../renderer.hpp"

class OpenGLRenderer : public IRenderer {
public:
	void Init() override;
	void Clear(float r, float g, float b) override;
	void Present() override;
};
