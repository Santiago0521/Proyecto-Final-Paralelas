#include "filtros.hpp"

#include <cstddef>
#include <stdexcept>
#include <omp.h>

Imagen ecualizacion_secuencial(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("ecualizacion espera una imagen de un canal");
    }

    if (e.datos.empty()) {
        throw std::runtime_error("ecualizacion: imagen vacía");
    }

    // Obtener el histograma de la imagen
    const Histograma h = histograma_secuencial(e);

    // Calcular el histograma acumulado
    Histograma acumulado{};
    acumulado[0] = h[0];
    for (std::size_t i = 1; i < h.size(); ++i) {
        acumulado[i] = acumulado[i - 1] + h[i];
    }

    // Encontrar el primer acumulado no nulo
    std::uint64_t cmin = 0;

    for (std::size_t i = 0; i < acumulado.size(); ++i) {
        if (acumulado[i] != 0) {
            cmin = acumulado[i];
            break;
        }
    }

    const std::uint64_t total = e.datos.size();

    // Si todos los píxeles tienen la misma intensidad,
    // se conserva la imagen para evitar división por cero.
    if (total == cmin) {
        return e;
    }

    // Construir la imagen ecualizada
    Imagen salida;
    salida.ancho = e.ancho;
    salida.alto = e.alto;
    salida.canales = 1;
    salida.datos.resize(e.datos.size());

    for (std::size_t i = 0; i < e.datos.size(); ++i) {
        const unsigned char pixel = e.datos[i];

        const double valor = (static_cast<double>(acumulado[pixel] - cmin) / 
        static_cast<double>(total - cmin)) * 255.0;

        salida.datos[i] = redondear(valor);
    }

    return salida;
}

Imagen ecualizacion_paralela(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("ecualizacion espera una imagen de un canal");
    }

    if (e.datos.empty()) {
        throw std::runtime_error("ecualizacion: imagen vacía");
    }

    // Calcular el histograma en paralelo
    const int hilos = omp_get_max_threads();
    const Histograma h = histograma_paralelo_privado(e, hilos);

    // Calcular el histograma acumulado
    Histograma acumulado{};
    acumulado[0] = h[0];
    for (std::size_t i = 1; i < h.size(); ++i) {
        acumulado[i] = acumulado[i - 1] + h[i];
    }

    // Encontrar el primer acumulado no nulo
    std::uint64_t cmin = 0;
    for (std::size_t i = 0; i < acumulado.size(); ++i) {
        if (acumulado[i] != 0) {
            cmin = acumulado[i];
            break;
        }
    }

    const std::uint64_t total = e.datos.size();

    // Si todos los píxeles tienen la misma intensidad,
    // devolvemos la imagen original
    if (total == cmin) {
        return e;
    }

    // Preparar la imagen de salida
    Imagen salida;
    salida.ancho = e.ancho;
    salida.alto = e.alto;
    salida.canales = 1;
    salida.datos.resize(e.datos.size());

    // Ecualizar los píxeles en paralelo
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < e.datos.size(); ++i) {
        const unsigned char pixel = e.datos[i];

        const double valor = (static_cast<double>(acumulado[pixel] - cmin) /
        static_cast<double>(total - cmin)) * 255.0;
        salida.datos[i] = redondear(valor);
    }

    return salida;
}