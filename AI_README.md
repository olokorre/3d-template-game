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
- **Saída de Nível (Exit)**:
    - Blocos marcados com `E` são armazenados no vetor `exits`.
    - Colisão com `E` dispara `currentLevelIndex++` e recarrega o nível.
- **Resets**: `loadLevel` reseta `playerVelocityY` e `isGrounded` para evitar bugs de transição física.

### 4. Level Design System (Embedded ASCII)
- **Fluxo**: `.txt` (Editor) -> `.h` (String Literal) -> `AllLevels.h` (Registry/Vector).
- **Tool**: `tools/level_manager.py` (GUI Tkinter).
    - Gera headers individuais para cada nível.
    - Mantém `AllLevels.h` atualizado com um `std::vector<std::string>` contendo todas as fases.
- **Engine**: A classe `Engine` lê o `ALL_LEVEL_VECTOR` e parseia os caracteres para instanciar obstáculos e o spawn point do player.

### 5. Input e Câmera
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
4. **Alocação**: Utilize VMA via `Mesh.h`. Lembre-se de dar `reset()` nos Smart Pointers em `cleanup()` para evitar assertions do VMA.

### Sistemas de Gameplay e UI

1. **Saúde e Dano**:
   - `playerHealth` (100.0f) gerenciado na `Engine`. 
   - Dano por contato com inimigos (`takeDamage`).
   - Morte transiciona para `GameState::GAME_OVER`.

2. **Inimigos (X)**:
   - Símbolo `X` no `.txt` é convertido em AABB na lista `enemies`.
   - Renderizados via `enemyMesh` (Magenta).
   - Colisão por frame enquanto sobreposto ao player.

3. **World Boundaries**:
   - Calculados em `loadLevel` baseados nas dimensões do grid ASCII.
   - Clamp de `playerPosition` em `processInput` para impedir saída do mapa.

4. **Máquina de Estados (Menus)**:
   - Estados: `MAIN_MENU`, `PLAYING`, `GAME_OVER`, `VICTORY`.
   - Transições via `Enter`.
   - Feedback visual via Clear Color do swapchain (Azul/Cinza/Vermelho/Ouro).

## Arquivos Chave
- `src/core/Engine.cpp`: Coração do jogo. Loop principal e lógica de colisão.
- `src/assets/levels/AllLevels.h`: Registro central de todos os níveis embutidos.
- `tools/level_manager.py`: Ferramenta principal para design de assets/fases.
- `src/core/Camera.cpp`: Matrizes de View/Projection. Contém o fix de Y-flip.

## Próximos Passos Sugeridos
1. **Refatoração de Input**: Criar `InputManager` para limpar o `processInput` da `Engine`.
2. **Física Jolt**: Substituir o sistema AABB manual por um Character Controller real.
3. **UI Real**: Implementar renderização de texto e botões via ImGui ou sistema customizado.
3. **Assets**: Implementar carregador de GLTF para substituir cubos procedurais.
