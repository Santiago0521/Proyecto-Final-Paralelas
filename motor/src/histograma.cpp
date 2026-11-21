#include "filtros.hpp"

#include <stdexcept>
#include <omp.h>
#include <cstddef>
#include <vector>

Histograma histograma_secuencial(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("histograma espera una imagen de un canal");
    }

    Histograma h{};

    for (unsigned char pixel : e.datos) {
        ++h[pixel];
    }
    return h;
}

Histograma histograma_paralelo_compartido(const Imagen& e, int hilos) {
    if (e.canales != 1) {
        throw std::runtime_error("histograma espera una imagen de un canal");
    }

    if (hilos <= 0) {
        throw std::runtime_error("histograma: número de hilos debe ser positivo");
    }

    Histograma h{};

    #pragma omp parallel for num_threads(hilos)
    for (std::size_t i = 0; i < e.datos.size(); ++i) {
        const unsigned char pixel = e.datos[i];

        #pragma omp atomic update
        h[pixel] += 1;
    }
    return h;
}

Histograma histograma_paralelo_privado(const Imagen& e, int hilos) {
    if (e.canales != 1) {
        throw std::runtime_error("histograma espera una imagen de un canal");
    }

    if (hilos <= 0) {
        throw std::runtime_error("histograma: número de hilos debe ser positivo");
    }

    // Cada hilo tendrá su propio histograma.
    // alignas(64) ayuda a evitar false sharing entre hilos.
    struct alignas(64) HistogramaAliniado {
        Histograma valores{};
    };

    std::vector<HistogramaAliniado> locales(static_cast<std::size_t>(hilos));

    #pragma omp parallel num_threads(hilos)
    {
        const int id = omp_get_thread_num();
        
        #pragma omp for
        for (std::size_t i = 0; i < e.datos.size(); ++i) {
            const unsigned char pixel = e.datos[i];
            ++locales[id].valores[pixel];
        }
    }

    // Reducción final: combinar los histogramas privados.
    Histograma resultado{};
    for (int hilo = 0; hilo < hilos; ++hilo) {
        for (std::size_t i = 0; i < resultado.size(); ++i) {
            resultado[i] += locales[hilo].valores[i];
        }
    }
    return resultado;
}