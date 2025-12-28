# AI Agent Guide - Layout e Contexto do Projeto

Este arquivo serve como guia para agentes de IA que trabalharão neste projeto no futuro. Leia-o para obter contexto rápido sobre arquitetura, decisões técnicas e estado atual.

## Contexto do Projeto
- **Objetivo**: Criar um motor de "Platformer 3D Tight" (estilo Mario 64).
- **Fase Atual**: MVP de Física e Renderização concluído. Próximo passo: Integração Jolt Physics.

## Arquitetura Técnica

### 1. Sistema de Coordenadas
- **World Space**: **+Y é CIMA (UP)**.
- **Vulkan CLIP Space**: +Y é BAIXO.
- **Correção**: A matriz de projeção em `Camera.cpp` inverte o eixo Y (`projectionMatrix[1][1] = -1.0f/...`) para alinhar o World Space com o Vulkan.
- **Atenção**: Ao calcular posições no mundo, assuma Y positivo como altura.

### 2. Rendering Pipeline
- **API**: Vulkan 1.3.
- **Helpers**: `vk-bootstrap` (Instance/Device) e `VMA` (Vulkan Memory Allocator).
- **Shaders**: Compilados de `assets/shaders` (*.vert, *.frag) para SPIR-V no build time.
- **Push Constants**: Usados para passar matrizes MVP (`projection * view * model`) para o vertex shader.
- **Meshes**:
    - Gerados proceduralmente em `Engine::createScene` (atualmente cubos).
    - `Mesh.cpp` gerencia Vertex Buffers via VMA.

### 3. Física e Colisão (Implementação Atual - Engine.cpp)
- **Tipo**: AABB (Axis-Aligned Bounding Box) customizada.
- **Lógica**: Explicita em `Engine::processInput`.
    - Gravidade constante aplicada a `playerVelocityY`.
    - Colisão com chão (Ground Plane) hardcoded em `y < 0.5f`.
    - Colisão com obstáculos usa struct `AABB` e loop simples.
- **Roadmap**: Esta lógica deve ser removida e substituída pela **Jolt Physics** na Fase 2.

### 4. Input e Câmera
- **Input**: GLFW (polling em `Engine::run`). Lógica de controle misturada em `processInput` (Refactoring needed).
- **Câmera**: Orbital/Arcball.
    - `cameraYaw` / `cameraPitch`: Controle esférico.
    - Posição da câmera é calculada a partir de `playerPosition` (câmera segue o jogador).
    - Movimento do jogador é relativo à rotação da câmera (Vetores Forward/Right calculados com base no Yaw).

## Diretrizes de Código
1. **C++20**: Use `std::unique_ptr`, `auto`, lambdas e inicializadores de struct.
2. **Bibliotecas**:
    - `glm`: Matemática (vec3, mat4).
    - `vkb`: Inicialização Vulkan.
3. **Segurança**: Sempre verifique `VK_SUCCESS`. Use `std::runtime_error` para falhas fatais na inicialização.

## Arquivos Chave
- `src/core/Engine.cpp`: Coração do jogo. Loop principal, lógica de jogo (atualmente).
- `src/core/Camera.cpp`: Matrizes de View/Projection. Contém o fix de Y-flip.
- `src/renderer/Mesh.cpp`: Gerenciamento de buffers de geometria.
- `src/renderer/Pipeline.cpp`: Configuração do pipeline gráfico (Rasterization, Depth Test, etc).

## Próximos Passos Sugeridos
1. **Refatoração**: Mover lógica de Input de `Engine.cpp` para classe `InputManager`.
2. **Integração Física**: Adicionar Jolt Physics para substituir a física AABB simples.
3. **Assets**: Implementar carregador de GLTF para substituir cubos procedurais.
