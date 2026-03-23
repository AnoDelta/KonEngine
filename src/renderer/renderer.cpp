#include "renderer.hpp"
#include "opengl/opengl_renderer.hpp"
#include <memory>

static std::unique_ptr<IRenderer> renderer = nullptr;

void InitRenderer() {
	renderer = std::make_unique<OpenGLRenderer>();
	renderer->Init();
}
