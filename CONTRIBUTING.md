# Contributing to RenderX

Thank you for your interest in contributing to RenderX! This document provides guidelines for contributing to the project.

---

## Project Status

RenderX is currently in **active development** and is not yet ready for production use. The Vulkan backend is the primary focus, while the OpenGL backend is non-functional.

---

## How to Contribute

### Reporting Issues

- **Bug Reports:** Open an issue with a clear description, steps to reproduce, and your environment details (OS, compiler, Vulkan SDK version).
- **Feature Requests:** Describe the feature, its use case, and how it fits with RenderX's design principles.
- **Questions:** For general questions, open a discussion or issue tagged appropriately.

### Code Contributions

We welcome pull requests for bug fixes, features, and improvements. Please follow these guidelines:

#### Before You Start

1. **Check existing issues** to see if your contribution is already being discussed or worked on.
2. **Open an issue** to discuss significant changes before starting work.
3. **Fork the repository** and create a feature branch from `main`.

#### Development Setup
```bash
git clone https://github.com/yourusername/RenderX.git
cd RenderX
git submodule update --init --recursive

mkdir Build && cd Build
cmake ..
cmake --build . --config Debug
```

#### Code Style

- **C++ Standard:** C++20
- **Naming Conventions:**
  - Types: `PascalCase` (e.g., `BufferHandle`, `CommandQueue`)
  - Functions: `PascalCase` (e.g., `CreateBuffer`, `Submit`)
  - Variables: `camelCase` (e.g., `vertexBuffer`, `queueFamily`)
  - Private members: `m_` prefix (e.g., `m_Device`, `m_CurrentFrame`)
- **Formatting:** Use consistent indentation (tabs or 4 spaces). Follow existing code style.
- **Comments:** Document complex logic, architectural decisions, and public APIs.

#### Commit Messages

- Use clear, descriptive commit messages.
- Format: `[Component] Brief description`
- Examples:
  - `[Vulkan] Fix barrier emission for depth/stencil textures`
  - `[Docs] Update build instructions for Linux`
  - `[Memory] Add timeline-based resource retirement`

#### Pull Request Process

1. **Create a feature branch:** `git checkout -b feature/your-feature-name`
2. **Make your changes** following the code style guidelines.
3. **Test your changes** with the provided examples (e.g., `HelloTriangle`).
4. **Commit your changes** with clear messages.
5. **Push to your fork:** `git push origin feature/your-feature-name`
6. **Open a pull request** against the `main` branch with:
   - A clear description of the changes
   - Reference to related issues (if any)
   - Screenshots or examples (if applicable)

#### Pull Request Guidelines

- Keep changes focused on a single feature or fix.
- Ensure code compiles without warnings on major compilers (MSVC, GCC, Clang).
- Update documentation if your changes affect the public API.
- Add comments for complex or non-obvious code.
- Test on both Debug and Release builds if possible.

---

## Areas Needing Contribution

Current development priorities:

### High Priority
- **Barrier System:** Complete barrier batching, merging, and per-queue emission
- **Resource Lifetime:** Implement timeline-based retirement system
- **Queue Ownership:** Finish automatic ownership transfer logic
- **Bindless Support:** Complete bindless descriptor implementation

### Medium Priority
- **Documentation:** Improve API documentation and usage examples
- **Testing:** Add unit tests and validation layers
- **Examples:** Create more example applications demonstrating features
- **Performance:** Profiling and optimization passes

### Low Priority
- **Platform Support:** Linux/macOS testing and fixes
- **D3D12 Backend:** Initial implementation planning
- **OpenGL Backend:** Restore functionality (if there's interest)

---

## Architectural Decisions

When contributing, keep these architectural principles in mind:

1. **Explicit Over Implicit:** Avoid hidden state changes or automatic behavior that obscures what's happening.
2. **Multi-Queue First:** Design with independent queue families and timeline semaphores in mind.
3. **Safety Without Cost:** Use compile-time checks where possible; runtime validation should be debug-only.
4. **Backend Agnostic:** Keep backend-specific code isolated; the public API should work across all backends.

---

## Testing

- Test your changes with the provided examples (especially `HelloTriangle`).
- Ensure Vulkan validation layers are enabled during development.
- Test on different hardware/drivers if possible.
- Check for memory leaks using tools like Valgrind or Visual Studio's leak detector.

---

## Documentation

- Update relevant documentation in `README.md` or other docs.
- Document public API changes in header files.
- Add inline comments for complex logic.
- Update examples if your changes affect usage patterns.

---

## Questions?

If you have questions about contributing:
- Open a GitHub issue tagged `question`
- Check existing issues and discussions
- Review the codebase for patterns and conventions

---

## License

By contributing to RenderX, you agree that your contributions will be licensed under the MIT License.

---

Thank you for helping improve RenderX!