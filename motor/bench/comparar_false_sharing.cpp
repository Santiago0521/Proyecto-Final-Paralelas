
#include "filtros.hpp"

#include <chrono>
#include <iostream>
#include <omp.h>
#include <vector>

Histograma histograma_privado_sin_alinear(
    const Imagen& e, int hilos
);

using Reloj = std::chrono::steady_clock;

int main() {
    std::cout << "Experimento: false sharing\n";

    Imagen imagen;
    imagen.ancho = 4096;
    imagen.alto = 4096;
    imagen.canales = 1;

    imagen.datos.resize(
        static_cast<std::size_t>(imagen.ancho) * imagen.alto
    );

    for (std::size_t i = 0; i < imagen.datos.size(); ++i) {
        imagen.datos[i] = static_cast<unsigned char>(
            (i * 37 + i / 17) % 256
        );
    }

    const int cantidades[] = {1, 2, 4, 8};
    const int repeticiones = 5;

    std::cout << "\nVariante,Hilos,Repeticion,Tiempo_ms\n";

    for (int hilos : cantidades) {

        // Calentamiento de ambas variantes.
        const Histograma calentamiento_alineado =
            histograma_paralelo_privado(imagen, hilos);

        const Histograma calentamiento_sin_alinear =
            histograma_privado_sin_alinear(imagen, hilos);

        if (calentamiento_alineado != calentamiento_sin_alinear) {
            std::cerr << "Error en el calentamiento\n";
            return 1;
        }

        for (int repeticion = 1; repeticion <= repeticiones; ++repeticion) {

            const auto inicio_alineado = Reloj::now();

            const Histograma alineado =
                histograma_paralelo_privado(imagen, hilos);

            const auto fin_alineado = Reloj::now();

            const double tiempo_alineado =
                std::chrono::duration<double, std::milli>(
                    fin_alineado - inicio_alineado
                ).count();

            const auto inicio_sin_alinear = Reloj::now();

            const Histograma sin_alinear =
                histograma_privado_sin_alinear(imagen, hilos);

            const auto fin_sin_alinear = Reloj::now();

            const double tiempo_sin_alinear =
                std::chrono::duration<double, std::milli>(
                    fin_sin_alinear - inicio_sin_alinear
                ).count();

            if (alineado != sin_alinear) {
                std::cerr << "Error: histogramas diferentes\n";
                return 1;
            }

            std::cout << "Alineado," << hilos << ","
                      << repeticion << "," << tiempo_alineado << '\n';

            std::cout << "Sin_alinear," << hilos << ","
                      << repeticion << "," << tiempo_sin_alinear << '\n';
        }
    }

    std::cout << "\nExperimento terminado correctamente.\n";

    return 0;
}
