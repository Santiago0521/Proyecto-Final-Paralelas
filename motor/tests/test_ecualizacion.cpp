
#include "filtros.hpp"

#include <stdexcept>
#include <iostream>
#include <cassert>
#include <omp.h>
#include <vector>

// Compara la version paralela con la secuencial.
void comprobar_paralelo(const Imagen& entrada, int hilos) {
    omp_set_num_threads(hilos);

    const Imagen secuencial = ecualizacion_secuencial(entrada);
    const Imagen paralelo = ecualizacion_paralela(entrada);

    assert(secuencial.datos == paralelo.datos);

    std::cout << " OK Secuencial vs paralelo con "
              << hilos << " hilo(s)\n";
}

int main() {
    // Prueba 1: Intensidades diferentes
    {
        Imagen entrada;
        entrada.ancho = 4;
        entrada.alto = 1;
        entrada.canales = 1;
        entrada.datos = {10, 20, 30, 40};

        Imagen salida = ecualizacion_secuencial(entrada);

        assert(salida.datos == std::vector<unsigned char>({0, 85, 170, 255}));

        std::cout << " OK Intensidades diferentes\n";

        for (int hilos : {1, 2, 4, 8}) {
            comprobar_paralelo(entrada, hilos);
        }
    }

    // Prueba 2: Intensidades repetidas
    {
        Imagen entrada;
        entrada.ancho = 4;
        entrada.alto = 1;
        entrada.canales = 1;
        entrada.datos = {10, 10, 20, 30};

        Imagen salida = ecualizacion_secuencial(entrada);

        assert(salida.datos == std::vector<unsigned char>({0, 0, 128, 255}));

        std::cout << " OK Intensidades repetidas\n";

        for (int hilos : {1, 2, 4, 8}) {
            comprobar_paralelo(entrada, hilos);
        }
    }

    // Prueba 3: Imagen de intensidad constante
    {
        Imagen entrada;
        entrada.ancho = 3;
        entrada.alto = 1;
        entrada.canales = 1;
        entrada.datos = {42, 42, 42};

        Imagen salida = ecualizacion_secuencial(entrada);

        assert(salida.datos == entrada.datos);

        std::cout << " OK Imagen de intensidad constante\n";

        for (int hilos : {1, 2, 4, 8}) {
            comprobar_paralelo(entrada, hilos);
        }
    }

    // Prueba 4: Rechazar imagen RGB
    {
        Imagen entrada;
        entrada.ancho = 1;
        entrada.alto = 1;
        entrada.canales = 3;
        entrada.datos = {10, 20, 30};

        bool excepcion = false;

        try {
            ecualizacion_secuencial(entrada);
        } catch (const std::runtime_error&) {
            excepcion = true;
        }

        assert(excepcion);

        excepcion = false;

        try {
            ecualizacion_paralela(entrada);
        } catch (const std::runtime_error&) {
            excepcion = true;
        }

        assert(excepcion);

        std::cout << " OK Rechazo de imagen RGB\n";
    }

    // Prueba 5: Rechazar imagen vacia
    {
        Imagen entrada;
        entrada.ancho = 0;
        entrada.alto = 0;
        entrada.canales = 1;

        bool excepcion = false;

        try {
            ecualizacion_secuencial(entrada);
        } catch (const std::runtime_error&) {
            excepcion = true;
        }

        assert(excepcion);

        excepcion = false;

        try {
            ecualizacion_paralela(entrada);
        } catch (const std::runtime_error&) {
            excepcion = true;
        }

        assert(excepcion);

        std::cout << " OK Rechazo de imagen vacia\n";
    }

    std::cout << "\nTodas las pruebas de Ecualizacion pasaron.\n";

    return 0;
}
