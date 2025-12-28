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
- `src/assets/shaders`: Shaders GLSL (compilados automaticamente para SPIR-V).
- `src/assets/levels`: Arquivos de nível (.txt) e headers embutidos (.h).
- `tools`: Ferramentas Python para desenvolvimento.
- `external`: Dependências gerenciadas via CMake FetchContent.

## Pré-requisitos

- **CMake** (3.20+)
- **Compilador C++** compatível com C++20 (GCC 11+, Clang 12+, MSVC Latest)
- **Vulkan SDK** instalado e validado (`vulkaninfo`).
- **Python 3** com `tkinter` instalado (para o Editor de Níveis).

## Como Compilar e Rodar

1. Clone o repositório:
   ```bash
   git clone <URL>
   cd doido
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

### Editor de Níveis Visual (Level Manager)
O projeto inclui um editor visual completo para criação de fases.

```bash
python3 tools/level_manager.py
```

**Funcionalidades:**
- **Level Browser**: Interface para listar, criar novas fases e deletar fases antigas.
- **Grade Interativa**: Pintura de tiles com o mouse (Wall, Player Start, Exit).
- **Sistema de Build**: Ao salvar, a ferramenta gera um header C++ e atualiza o registro global (`AllLevels.h`), embutindo a fase diretamente no executável.
- **Resize**: Altera as dimensões da fase dinamicamente.

**Nota**: Após salvar no editor, é necessário recompilar o jogo (`cmake --build .`) para aplicar as mudanças.

## Status Atual

- [x] Renderização Vulkan (Triângulos, Cubos, Shaders).
- [x] Câmera Orbital Suave.
- [x] Sistema de Níveis Dinâmico (ASCII para Header Embedding).
- [x] Editor de Níveis GUI (Python/Tkinter).
- [x] Progressão de Níveis (Zonas de Saída e Transições).
- [x] Física MVP (Gravidade, Colisão AABB).
- [ ] Física Avançada (Integração Jolt Physics).
- [ ] Refatoração do Sistema de Input.
