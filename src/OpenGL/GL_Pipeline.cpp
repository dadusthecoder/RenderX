#include "GL_Common.h"

namespace Rx::RxGL {

namespace {
GLenum ToGLCompare(CompareOp op) {
    switch (op) {
    case CompareOp::NEVER:
        return GL_NEVER;
    case CompareOp::LESS:
        return GL_LESS;
    case CompareOp::EQUAL:
        return GL_EQUAL;
    case CompareOp::LESS_EQUAL:
        return GL_LEQUAL;
    case CompareOp::GREATER:
        return GL_GREATER;
    case CompareOp::NOT_EQUAL:
        return GL_NOTEQUAL;
    case CompareOp::GREATER_EQUAL:
        return GL_GEQUAL;
    case CompareOp::ALWAYS:
    default:
        return GL_ALWAYS;
    }
}

GLenum ToGLBlendFunc(BlendFunc fn) {
    switch (fn) {
    case BlendFunc::ZERO:
        return GL_ZERO;
    case BlendFunc::ONE:
        return GL_ONE;
    case BlendFunc::SRC_COLOR:
        return GL_SRC_COLOR;
    case BlendFunc::ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFunc::DST_COLOR:
        return GL_DST_COLOR;
    case BlendFunc::ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFunc::SRC_ALPHA:
        return GL_SRC_ALPHA;
    case BlendFunc::ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFunc::DST_ALPHA:
        return GL_DST_ALPHA;
    case BlendFunc::ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case BlendFunc::CONSTANT_COLOR:
        return GL_SRC_ALPHA;
    case BlendFunc::ONE_MINUS_CONSTANT_COLOR:
    default:
        return GL_ONE_MINUS_SRC_ALPHA;
    }
}

} // namespace

ShaderHandle GLCreateShader(const ShaderDesc& desc) {
    PROFILE_FUNCTION();
    ShaderHandle handle{GLNextHandle()};
    g_Shaders.emplace(handle.id, GLShaderResource{desc});
    return handle;
}

void GLDestroyShader(ShaderHandle& handle) {
    PROFILE_FUNCTION();
    g_Shaders.erase(handle.id);
    handle.id = 0;
}

PipelineLayoutHandle GLCreatePipelineLayout(const SetLayoutHandle*   layouts,
                                            uint32_t                 layoutCount,
                                            const PushConstantRange* pushRanges,
                                            uint32_t                 pushRangeCount) {
    PROFILE_FUNCTION();

    GLPipelineLayoutResource res{};
    if (layouts && layoutCount > 0) {
        res.layouts.assign(layouts, layouts + layoutCount);
    }
    if (pushRanges && pushRangeCount > 0) {
        res.pushRanges.assign(pushRanges, pushRanges + pushRangeCount);
    }

    PipelineLayoutHandle handle{GLNextHandle()};
    g_PipelineLayouts.emplace(handle.id, std::move(res));
    return handle;
}

PipelineHandle GLCreateGraphicsPipeline(PipelineDesc& desc) {
    PROFILE_FUNCTION();

    PipelineHandle handle{GLNextHandle()};
    g_Pipelines.emplace(handle.id, GLPipelineResource{desc});
    return handle;
}

void GLDestroyPipeline(PipelineHandle& handle) {
    PROFILE_FUNCTION();
    g_Pipelines.erase(handle.id);
    handle.id = 0;
}

void GLDestroyPipelineLayout(PipelineLayoutHandle& handle) {
    PROFILE_FUNCTION();
    g_PipelineLayouts.erase(handle.id);
    handle.id = 0;
}

void GLBindPipeline(const PipelineHandle handle) {
    auto it = g_Pipelines.find(handle.id);
    if (it == g_Pipelines.end()) {
        return;
    }

    const auto& p = it->second.desc;

    if (p.depthStencil.depthEnable) {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(p.depthStencil.depthWriteEnable ? GL_TRUE : GL_FALSE);
        glDepthFunc(ToGLCompare(p.depthStencil.depthFunc));
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    switch (p.rasterizer.cullMode) {
    case CullMode::NONE:
        glDisable(GL_CULL_FACE);
        break;
    case CullMode::FRONT:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    case CullMode::BACK:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    case CullMode::FRONT_AND_BACK:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT_AND_BACK);
        break;
    }

    glFrontFace(p.rasterizer.frontCounterClockwise ? GL_CCW : GL_CW);

    if (p.blend.enable) {
        glEnable(GL_BLEND);
        glBlendFunc(ToGLBlendFunc(p.blend.srcColor), ToGLBlendFunc(p.blend.dstColor));
    } else {
        glDisable(GL_BLEND);
    }
}

const PipelineDesc* GLGetPipelineDesc(const PipelineHandle pipeline) {
    auto it = g_Pipelines.find(pipeline.id);
    if (it == g_Pipelines.end()) {
        return nullptr;
    }
    return &it->second.desc;
}

void GLClearPipelineCache() {
    g_Pipelines.clear();
}

} // namespace Rx::RxGL
