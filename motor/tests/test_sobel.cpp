
#include "filtros.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <omp.h>

namespace {
    int fallos = 0;

    void comprobar(const char* nombre, const Imagen& obtenida,
                   const std::vector<unsigned char>& esperada) {
        if (obtenida.datos == esperada) {
            std::cout << " OK " << nombre << '\n';
        } else {
            std::cout << " FAIL " << nombre << '\n';
            fallos++;

            std::cout << "  Esperada: ";
            for (unsigned char pixel : esperada) {
                std::cout << static_cast<int>(pixel) << " ";
            }

            std::cout << "\n  Obtenido: ";
            for (unsigned char pixel : obtenida.datos) {
                std::cout << static_cast<int>(pixel) << " ";
            }
            std::cout << "\n";
        }
    }

    void comprobar_iguales(const char* nombre,
                           const Imagen& secuencial,
                           const Imagen& paralelo) {
        if (secuencial.datos == paralelo.datos) {
            std::cout << " OK " << nombre << '\n';
        } else {
            std::cout << " FAIL " << nombre << '\n';
            fallos++;
        }
    }

    Imagen crear_imagen(int ancho, int alto,
                        const std::vector<unsigned char>& datos) {
        Imagen imagen;
        imagen.ancho = ancho;
        imagen.alto = alto;
        imagen.canales = 1;
        imagen.datos = datos;
        return imagen;
    }
}

int main() {
    std::cout << "Pruebas de correctitud de Sobel:\n";

    // Prueba 1: Imagen constante.
    {
        Imagen entrada = crear_imagen(3, 3, {
            50, 50, 50,
            50, 50, 50,
            50, 50, 50
        });

        Imagen salida = sobel_secuencial(entrada);

        comprobar("Imagen constante produce gradiente cero", salida, {
            0, 0, 0,
            0, 0, 0,
            0, 0, 0
        });
    }

    // Prueba 2: Gradiente horizontal.
    {
        Imagen entrada = crear_imagen(3, 3, {
            0, 10, 20,
            0, 10, 20,
            0, 10, 20
        });

        Imagen salida = sobel_secuencial(entrada);

        comprobar("Gradiente horizontal", salida, {
            40, 80, 40,
            40, 80, 40,
            40, 80, 40
        });
    }

    // Prueba 3: Gradiente vertical.
    {
        Imagen entrada = crear_imagen(3, 3, {
            0, 0, 0,
            10, 10, 10,
            20, 20, 20
        });

        Imagen salida = sobel_secuencial(entrada);

        comprobar("Gradiente vertical", salida, {
            40, 40, 40,
            80, 80, 80,
            40, 40, 40
        });
    }

    // Prueba 4: Rechazar imagen RGB.
    {
        Imagen entrada;
        entrada.ancho = 2;
        entrada.alto = 2;
        entrada.canales = 3;
        entrada.datos.resize(12, 100);

        try {
            (void)sobel_secuencial(entrada);
            std::cout << " FAIL Sobel acepta imagen RGB\n";
            fallos++;
        } catch (const std::runtime_error&) {
            std::cout << " OK Sobel rechaza imagen RGB\n";
        }
    }

    // Prueba 5: Sobel paralelo sobre imagen constante.
    {
        Imagen entrada = crear_imagen(3, 3, {
            50, 50, 50,
            50, 50, 50,
            50, 50, 50
        });

        Imagen salida = sobel_paralelo(entrada);

        comprobar("Paralelo: imagen constante", salida, {
            0, 0, 0,
            0, 0, 0,
            0, 0, 0
        });
    }

    
    // Prueba 6: Comparar secuencial y paralelo con diferentes hilos.
    {
        // Escenario 1: Gradiente horizontal y vertical combinado.
        Imagen entrada = crear_imagen(5, 5, {
            0, 10, 20, 30, 40,
            5, 15, 25, 35, 45,
            10, 20, 30, 40, 50,
            15, 25, 35, 45, 55,
            20, 30, 40, 50, 60
        });

        Imagen referencia = sobel_secuencial(entrada);

        const int cantidades[] = {1, 2, 4, 8};

        for (int hilos : cantidades) {
            omp_set_num_threads(hilos);

            Imagen salida = sobel_paralelo(entrada);

            const std::string nombre =
                "Gradiente combinado: secuencial vs paralelo con " +
                std::to_string(hilos) + " hilo(s)";

            comprobar_iguales(nombre.c_str(), referencia, salida);
        }
    }

    // Prueba 7: Comparar imagen constante.
    {
        Imagen entrada = crear_imagen(5, 5, {
            42, 42, 42, 42, 42,
            42, 42, 42, 42, 42,
            42, 42, 42, 42, 42,
            42, 42, 42, 42, 42,
            42, 42, 42, 42, 42
        });

        Imagen referencia = sobel_secuencial(entrada);

        const int cantidades[] = {1, 2, 4, 8};

        for (int hilos : cantidades) {
            omp_set_num_threads(hilos);

            Imagen salida = sobel_paralelo(entrada);

            const std::string nombre =
                "Imagen constante: secuencial vs paralelo con " +
                std::to_string(hilos) + " hilo(s)";

            comprobar_iguales(nombre.c_str(), referencia, salida);
        }
    }

    // Prueba 8: Sobel paralelo debe rechazar imagen RGB.
    {
        Imagen entrada;
        entrada.ancho = 2;
        entrada.alto = 2;
        entrada.canales = 3;
        entrada.datos.resize(12, 100);

        try {
            (void)sobel_paralelo(entrada);
            std::cout << " FAIL Sobel paralelo acepta imagen RGB\n";
            fallos++;
        } catch (const std::runtime_error&) {
            std::cout << " OK Sobel paralelo rechaza imagen RGB\n";
        }
    }

    std::cout << '\n';

    if (fallos == 0) {
        std::cout << "Todas las pruebas pasaron correctamente.\n";
        return 0;
    }

    std::cout << fallos << " pruebas fallaron.\n";
    return 1;
}
