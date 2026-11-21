
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <omp.h>

constexpr std::uint64_t ITERACIONES = 50000000;

// Contadores compactos.
struct ContadorCompacto {
    std::atomic<std::uint64_t> valor{0};
};

// Contadores separados en líneas de caché.
struct alignas(64) ContadorSeparado {
    std::atomic<std::uint64_t> valor{0};
};

using Reloj = std::chrono::steady_clock;

template <typename Contador>
double medir(int hilos) {
    Contador contadores[8];

    const auto inicio = Reloj::now();

    #pragma omp parallel num_threads(hilos)
    {
        const int id = omp_get_thread_num();

        for (std::uint64_t i = 0; i < ITERACIONES; ++i) {
            contadores[id].valor.fetch_add(
                1, std::memory_order_relaxed
            );
        }
    }

    const auto fin = Reloj::now();

    for (int i = 0; i < hilos; ++i) {
        if (contadores[i].valor.load() != ITERACIONES) {
            std::cerr << "Error en contador del hilo " << i << '\n';
            return -1.0;
        }
    }

    return std::chrono::duration<double, std::milli>(
        fin - inicio
    ).count();
}

int main() {
    const int cantidades[] = {1, 2, 4, 8};
    const int repeticiones = 5;

    std::cout << "Experimento controlado de false sharing\n";
    std::cout << "Variante,Hilos,Repeticion,Tiempo_ms\n";

    for (int hilos : cantidades) {
        for (int repeticion = 1; repeticion <= repeticiones; ++repeticion) {

            const double compacto = medir<ContadorCompacto>(hilos);
            const double separado = medir<ContadorSeparado>(hilos);

            if (compacto < 0 || separado < 0) {
                return 1;
            }

            std::cout << "Compacto," << hilos << ","
                      << repeticion << "," << compacto << '\n';

            std::cout << "Separado," << hilos << ","
                      << repeticion << "," << separado << '\n';
        }
    }

    std::cout << "Experimento terminado correctamente.\n";
    return 0;
}
