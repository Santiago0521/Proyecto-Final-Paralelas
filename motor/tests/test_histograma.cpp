
#include "filtros.hpp"

#include <stdexcept>
#include <iostream>
#include <cassert>

void comprobar_paralelo(const Imagen& img, int hilos) {
    const Histograma secuencial = histograma_secuencial(img);

    const Histograma compartido =
        histograma_paralelo_compartido(img, hilos);

    const Histograma privado =
        histograma_paralelo_privado(img, hilos);

    assert(compartido == secuencial);
    assert(privado == secuencial);

    std::cout << " OK Histogramas compartido y privado con "
              << hilos << " hilo(s)\n";
}

int main() {
    // Prueba 1: Frecuencias conocidas
    {
        Imagen img;
        img.ancho = 3;
        img.alto = 2;
        img.canales = 1;
        img.datos = {
            10, 20, 10,
            30, 20, 10
        };

        const Histograma secuencial = histograma_secuencial(img);

        assert(secuencial[10] == 3);
        assert(secuencial[20] == 2);
        assert(secuencial[30] == 1);

        std::cout << " OK Frecuencias conocidas\n";

        for (int hilos : {1, 2, 4, 8}) {
            comprobar_paralelo(img, hilos);
        }
    }

    // Prueba 2: Todas las intensidades posibles
    {
        Imagen img;
        img.ancho = 256;
        img.alto = 1;
        img.canales = 1;

        for (int i = 0; i < 256; ++i) {
            img.datos.push_back(static_cast<unsigned char>(i));
        }

        const Histograma secuencial = histograma_secuencial(img);

        for (int i = 0; i < 256; ++i) {
            assert(secuencial[i] == 1);
        }

        std::cout << " OK Todas las intensidades posibles\n";

        for (int hilos : {1, 2, 4, 8}) {
            comprobar_paralelo(img, hilos);
        }
    }

    // Prueba 3: Imagen de una sola intensidad
    {
        Imagen img;
        img.ancho = 5;
        img.alto = 2;
        img.canales = 1;
        img.datos.assign(10, 42);

        const Histograma secuencial = histograma_secuencial(img);

        assert(secuencial[42] == 10);

        std::cout << " OK Imagen de intensidad unica\n";

        for (int hilos : {1, 2, 4, 8}) {
            comprobar_paralelo(img, hilos);
        }
    }

    std::cout << "\nTodas las pruebas de Histograma pasaron.\n";

    return 0;
}
