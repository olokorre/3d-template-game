#include "core/Engine.h"
#include <iostream>

int main() {
    try {
        Engine engine;
        engine.init();
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "Erro fatal: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
