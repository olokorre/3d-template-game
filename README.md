# Platformer 3D Engine

Este projeto é um motor de jogo 3D simples escrito em **C++20** utilizando **Vulkan 1.3**. O objetivo é criar uma base performática e modular para um jogo de plataforma no estilo Mario 64.

## Tecnologias

- **Linguagem**: C++20
- **Gráficos**: Vulkan 1.3
- **Janela/Input**: GLFW
- **Matemática**: GLM
- **Auxiliares Vulkan**: vk-bootstrap, Vulkan Memory Allocator (VMA)

## Estrutura do Projeto

- `src/core`: Classes principais (Engine, Camera, lógica de loop).
- `src/renderer`: Abstrações do Vulkan (Context, Pipeline, Mesh, Swapchain).
- `assets/shaders`: Shaders GLSL (compilados automaticamente para SPIR-V pelo CMake).
- `external`: Dependências gerenciadas via CMake FetchContent.

## Pré-requisitos

- **CMake** (3.20+)
- **Compilador C++** compatível com C++20 (GCC 11+, Clang 12+, MSVC Latest)
- **Vulkan SDK** instalado e validado (`vulkaninfo`).
- **Drivers de GPU** atualizados.
- **Pacotes de desenvolvimento do sistema** (X11/Wayland libs no Linux).

## Como Compilar e Rodar

1. Clone o repositório:
   ```bash
   git clone <URL>
   cd <NOME_DA_PASTA>
   ```

2. Crie a pasta de build:
   ```bash
   mkdir build && cd build
   ```

3. Gere os arquivos de projeto e compile:
   ```bash
   cmake ..
   cmake --build .
   ```

4. Execute o jogo:
   ```bash
   ./Platformer3D
   ```

## Controles

- **Mouse**: Rotacionar Câmera (Orbital).
- **W / A / S / D**: Mover o Personagem (Relativo à Câmera).
- **Espaço**: Pular.

## Ferramentas de Desenvolvimento

### Editor de Níveis Visual (GUI)
Para abrir o editor visual e desenhar seu mapa:

```bash
python3 tools/level_manager.py level1
```

**Funcionalidades:**
- **Pintar**: Selecione uma ferramenta (Wall, Player, Empty) e clique/arraste na grade.
- **Save & Build**: Salva o arquivo e recompila o header automaticamente.
- **Resize**: Altera o tamanho do mapa.

Obs: Após clicar em "Save & Build", você ainda precisa recompilar o jogo (`cmake --build .`) para atualizar o binário.

## Status Atual

- [x] Renderização Vulkan básica (Triângulos, Shades).
- [x] Carregamento de Meshes (Cubos).
- [x] Câmera Orbital.
- [x] Física Simples (Gravidade, Colisão AABB).
- [ ] Física Avançada (Jolt Physics).
