
#include "filtros.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <omp.h>

namespace {
    int fallos = 0;

    void comprobar(const char* nombre,
                   const Imagen& obtenida,
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

            std::cout << "\n  Obtenida: ";
            for (unsigned char pixel : obtenida.datos) {
                std::cout << static_cast<int>(pixel) << " ";
            }
            std::cout << '\n';
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

    void comprobar_paralelo(const Imagen& entrada,
                            int hilos,
                            const char* escenario) {
        omp_set_num_threads(hilos);

        const Imagen secuencial = otsu_secuencial(entrada);
        const Imagen paralelo = otsu_paralelo(entrada);

        const std::string nombre =
            std::string(escenario) +
            ": secuencial vs paralelo con " +
            std::to_string(hilos) + " hilo(s)";

        if (secuencial.datos == paralelo.datos) {
            std::cout << " OK " << nombre << '\n';
        } else {
            std::cout << " FAIL " << nombre << '\n';
            fallos++;
        }
    }
}

int main() {
    std::cout << "Pruebas de correctitud de Otsu:\n";

    // Prueba 1: Dos grupos de intensidades claramente separados.
    {
        Imagen entrada = crear_imagen(4, 2, {
            10, 10, 10, 10,
            200, 200, 200, 200
        });

        Imagen salida = otsu_secuencial(entrada);

        comprobar("Separacion de dos grupos", salida, {
            0, 0, 0, 0,
            255, 255, 255, 255
        });
    }

    // Prueba 2: Imagen con una sola intensidad.
    {
        Imagen entrada = crear_imagen(3, 2, {
            50, 50, 50,
            50, 50, 50
        });

        Imagen salida = otsu_secuencial(entrada);

        comprobar("Imagen de intensidad constante", salida, {
            255, 255, 255,
            255, 255, 255
        });
    }

    // Prueba 3: Todos los píxeles tienen intensidad cero.
    {
        Imagen entrada = crear_imagen(2, 2, {
            0, 0,
            0, 0
        });

        Imagen salida = otsu_secuencial(entrada);

        comprobar("Imagen completamente negra", salida, {
            0, 0,
            0, 0
        });
    }

    // Prueba 4: Otsu debe rechazar imágenes RGB.
    {
        Imagen entrada;
        entrada.ancho = 2;
        entrada.alto = 2;
        entrada.canales = 3;
        entrada.datos.resize(12, 100);

        try {
            (void)otsu_secuencial(entrada);
            std::cout << " FAIL Otsu acepta imagen RGB\n";
            fallos++;
        } catch (const std::runtime_error&) {
            std::cout << " OK Otsu rechaza imagen RGB\n";
        }
    }

    // Prueba 5: Otsu debe rechazar imágenes vacías.
    {
        Imagen entrada;
        entrada.ancho = 0;
        entrada.alto = 0;
        entrada.canales = 1;

        try {
            (void)otsu_secuencial(entrada);
            std::cout << " FAIL Otsu acepta imagen vacia\n";
            fallos++;
        } catch (const std::runtime_error&) {
            std::cout << " OK Otsu rechaza imagen vacia\n";
        }
    }

    // Prueba 6: Comparar diferentes escenarios con varios hilos.
    {
        const int cantidades[] = {1, 2, 4, 8};

        // Escenario 1: Dos grupos separados.
        Imagen grupos = crear_imagen(8, 4, {
            10, 10, 20, 20, 180, 180, 200, 200,
            15, 25, 30, 40, 170, 190, 210, 220,
            10, 20, 30, 40, 180, 190, 200, 210,
            15, 25, 35, 45, 175, 185, 195, 205
        });

        for (int hilos : cantidades) {
            comprobar_paralelo(grupos, hilos, "Dos grupos separados");
        }

        // Escenario 2: Imagen de intensidad constante.
        Imagen constante = crear_imagen(4, 3, {
            50, 50, 50, 50,
            50, 50, 50, 50,
            50, 50, 50, 50
        });

        for (int hilos : cantidades) {
            comprobar_paralelo(constante, hilos, "Imagen constante");
        }

        // Escenario 3: Imagen completamente negra.
        Imagen negra = crear_imagen(4, 3, {
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        });

        for (int hilos : cantidades) {
            comprobar_paralelo(negra, hilos, "Imagen negra");
        }
    }
}
