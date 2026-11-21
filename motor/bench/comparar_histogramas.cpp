
#include "filtros.hpp"

#include <chrono>
#include <iostream>
#include <vector>
#include <omp.h>

using Reloj = std::chrono::steady_clock;

int main() {
    std::cout << "Experimento: histogramas compartido vs privado\n";

    // 1. Crear una imagen RGB de 4096 x 4096.
    Imagen original;
    original.ancho = 4096;
    original.alto = 4096;
    original.canales = 3;
    original.datos.resize(
        static_cast<std::size_t>(original.ancho) *
        original.alto * 3
    );

    // 2. Generar datos deterministas.
    for (int y = 0; y < original.alto; ++y) {
        for (int x = 0; x < original.ancho; ++x) {
            const int valor = (x * 37 + y * 17 + (x * y) % 29) % 256;

            original.poner(x, y, 0, static_cast<unsigned char>(valor));
            original.poner(x, y, 1, static_cast<unsigned char>(valor));
            original.poner(x, y, 2, static_cast<unsigned char>(valor));
        }
    }

    // 3. Convertir a escala de grises.
    const Imagen gris = gris_secuencial(original);

    const int cantidades[] = {1, 2, 4, 8};
    const int repeticiones = 5;

    std::cout << "\nVariante,Hilos,Repeticion,Tiempo_ms\n";

    // 4. Medir las dos variantes.
    for (int hilos : cantidades) {
        for (int repeticion = 1; repeticion <= repeticiones; ++repeticion) {

            // Histograma compartido.
            const auto inicio_compartido = Reloj::now();

            const Histograma compartido =
                histograma_paralelo_compartido(gris, hilos);

            const auto fin_compartido = Reloj::now();

            const double tiempo_compartido =
                std::chrono::duration<double, std::milli>(
                    fin_compartido - inicio_compartido
                ).count();

            // Histograma privado.
            const auto inicio_privado = Reloj::now();

            const Histograma privado =
                histograma_paralelo_privado(gris, hilos);

            const auto fin_privado = Reloj::now();

            const double tiempo_privado =
                std::chrono::duration<double, std::milli>(
                    fin_privado - inicio_privado
                ).count();

            // 5. Comprobar que los resultados sean idénticos.
            if (compartido != privado) {
                std::cerr << "Error: los histogramas no coinciden\n";
                return 1;
            }

            std::cout << "Compartido," << hilos << ","
                      << repeticion << "," << tiempo_compartido << '\n';

            std::cout << "Privado," << hilos << ","
                      << repeticion << "," << tiempo_privado << '\n';
        }
    }

    std::cout << "\nExperimento terminado correctamente.\n";

    return 0;
}
