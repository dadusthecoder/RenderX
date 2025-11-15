#include "RenderX/RenderX.h"

int main() {
	Lgt::LoadAPI(Lgt::RenderXAPI::OpenGL);
	Lgt::CreatePipelineGFX("shaders/basic.vert", "shaders/basic.frag");
	while (true) {
		/* code */
	}
	return 0;
}