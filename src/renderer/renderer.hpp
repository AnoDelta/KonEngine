#pragma once

class IRenderer {
public:
	virtual void Init() = 0;
	virtual void Draw() = 0;
	virtual void Clear() = 0;
};
