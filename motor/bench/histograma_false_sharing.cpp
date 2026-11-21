
#include "filtros.hpp"

#include <stdexcept>
#include <omp.h>
#include <cstddef>
#include <vector>

Histograma histograma_privado_sin_alinear(
    const Imagen& e, int hilos
) {
    if (e.canales != 1) {
        throw std::runtime_error(
            "histograma espera una imagen de un canal"
        );
    }

    if (hilos <= 0) {
        throw std::runtime_error(
            "histograma: número de hilos debe ser positivo"
        );
    }

    // Variante experimental: sin alineación explícita.
    struct HistogramaSinAlinear {
        Histograma valores{};
    };

    std::vector<HistogramaSinAlinear> locales(
        static_cast<std::size_t>(hilos)
    );

    #pragma omp parallel num_threads(hilos)
    {
        const int id = omp_get_thread_num();

        #pragma omp for schedule(static)
        for (std::size_t i = 0; i < e.datos.size(); ++i) {
            const unsigned char pixel = e.datos[i];
            ++locales[id].valores[pixel];
        }
    }

    Histograma resultado{};

    for (int hilo = 0; hilo < hilos; ++hilo) {
        for (std::size_t i = 0; i < resultado.size(); ++i) {
            resultado[i] += locales[hilo].valores[i];
        }
    }

    return resultado;
}
