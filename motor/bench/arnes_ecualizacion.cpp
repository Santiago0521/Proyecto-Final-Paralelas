
#include "filtros.hpp"
#include "imagen.hpp"

#include <algorithm>
#include <omp.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <chrono>

using Reloj = std::chrono::steady_clock;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "uso: arnes_ecualizacion <imagen>\n";
        return 2;
    }

    try {
        const Imagen entrada = leer(argv[1]);

        if (entrada.canales != 1) {
            throw std::runtime_error(
                "la ecualizacion necesita una imagen de un canal"
            );
        }

        const int repeticiones = 5;

        // Referencia secuencial para comprobar la correctitud.
        const Imagen referencia = ecualizacion_secuencial(entrada);

        std::cout << "Variante,Hilos,Repeticion,Tiempo_ms\n";

        // Medición secuencial.
        std::vector<double> secuenciales;

        for (int r = 1; r <= repeticiones; ++r) {
            const auto inicio = Reloj::now();
            const Imagen salida = ecualizacion_secuencial(entrada);
            const auto fin = Reloj::now();

            if (salida.datos != referencia.datos) {
                throw std::runtime_error(
                    "resultado secuencial incorrecto"
                );
            }

            const double ms =
                std::chrono::duration<double, std::milli>(
                    fin - inicio
                ).count();

            secuenciales.push_back(ms);
            std::cout << "Secuencial,1," << r << "," << ms << '\n';
        }

        // Medición paralela con 1, 2, 4 y 8 hilos.
        const int cantidades[] = {1, 2, 4, 8};

        omp_set_dynamic(0);

        for (int hilos : cantidades) {
            omp_set_num_threads(hilos);

            std::vector<double> medidas;

            for (int r = 1; r <= repeticiones; ++r) {
                const auto inicio = Reloj::now();
                const Imagen salida = ecualizacion_paralela(entrada);
                const auto fin = Reloj::now();

                if (salida.datos != referencia.datos) {
                    throw std::runtime_error(
                        "resultado paralelo diferente con " +
                        std::to_string(hilos) + " hilos"
                    );
                }

                const double ms =
                    std::chrono::duration<double, std::milli>(
                        fin - inicio
                    ).count();

                medidas.push_back(ms);
                std::cout << "Paralela," << hilos << ","
                          << r << "," << ms << '\n';
            }
        }

        std::cout << "Experimento terminado correctamente.\n";

    } catch (const std::exception& error) {
        std::cerr << "arnes_ecualizacion: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;
}
