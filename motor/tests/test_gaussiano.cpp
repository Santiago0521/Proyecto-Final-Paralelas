// Prueba del gaussiano separable: compara la versión secuencial y la paralela
// contra la forma directa 5x5 sobre imágenes aleatorias y sobre la imagen de
// prueba del repositorio. La forma directa usa enteros, así que la comparación
// es exacta.
#include "filtros.hpp"
#include "imagen.hpp"

#include <cstddef>
#include <iostream>
#include <random>
#include <string>

namespace {

// Núcleo binomial multiplicado por 16: [1 4 6 4 1] / 16.
constexpr int A[5] = {1, 4, 6, 4, 1};

// Forma directa 5x5: 25 productos A_i * A_j * I, sumados en enteros y
// divididos entre 256 (16 * 16). Redondeo con empates hacia arriba.
Imagen gaussiano_directo(const Imagen& e) {
    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = e.canales;
    s.datos.resize(static_cast<std::size_t>(e.ancho) * e.alto * e.canales);
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                long long suma = 0;
                for (int j = -2; j <= 2; ++j) {
                    for (int i = -2; i <= 2; ++i) {
                        suma += static_cast<long long>(A[i + 2]) * A[j + 2] *
                                e.en(recortar(x + i, e.ancho), recortar(y + j, e.alto), c);
                    }
                }
                s.poner(x, y, c, static_cast<unsigned char>((suma + 128) / 256));
            }
        }
    }
    return s;
}

Imagen aleatoria(int ancho, int alto, int canales, unsigned semilla) {
    Imagen img;
    img.ancho = ancho;
    img.alto = alto;
    img.canales = canales;
    img.datos.resize(static_cast<std::size_t>(ancho) * alto * canales);
    std::mt19937 generador(semilla);
    std::uniform_int_distribution<int> valor(0, 255);
    for (unsigned char& v : img.datos) v = static_cast<unsigned char>(valor(generador));
    return img;
}

// Cuenta los bytes distintos entre dos imágenes; -1 si no tienen la misma forma.
long long diferencias(const Imagen& a, const Imagen& b) {
    if (a.ancho != b.ancho || a.alto != b.alto || a.canales != b.canales) return -1;
    long long n = 0;
    for (std::size_t i = 0; i < a.datos.size(); ++i) {
        if (a.datos[i] != b.datos[i]) ++n;
    }
    return n;
}

int revisar(const std::string& nombre, const Imagen& referencia, const Imagen& obtenida) {
    const long long n = diferencias(referencia, obtenida);
    if (n == 0) {
        std::cout << "  ok    " << nombre << "\n";
        return 0;
    }
    if (n < 0) {
        std::cout << "  FALLA " << nombre << ": distinta forma\n";
    } else {
        std::cout << "  FALLA " << nombre << ": " << n << " byte(s) distintos\n";
    }
    return 1;
}

}  // namespace

int main() {
    int fallos = 0;

    struct Caso {
        int ancho;
        int alto;
        int canales;
        unsigned semilla;
    };
    const Caso casos[] = {
        {1, 1, 1, 1},   // un solo píxel: todos los vecinos son el mismo
        {2, 3, 1, 2},   // más angosta que la vecindad
        {7, 5, 1, 3},
        {37, 23, 1, 4},
        {16, 9, 3, 5},  // color
        {64, 48, 3, 6},
    };

    std::cout << "Gaussiano separable contra forma directa 5x5, imágenes aleatorias:\n";
    for (const Caso& k : casos) {
        const std::string etiqueta = "gaussiano " + std::to_string(k.ancho) + "x" +
                                     std::to_string(k.alto) + " con " +
                                     std::to_string(k.canales) + " canal(es)";
        const Imagen entrada = aleatoria(k.ancho, k.alto, k.canales, k.semilla);
        const Imagen referencia = gaussiano_directo(entrada);
        fallos += revisar(etiqueta + ", secuencial", referencia, gaussiano_secuencial(entrada));
        fallos += revisar(etiqueta + ", paralelo", referencia, gaussiano_paralelo(entrada));
        fallos += revisar(etiqueta + ", bloques", referencia, gaussiano_bloques(entrada));
    }

    std::cout << "Gaussiano separable contra forma directa 5x5, imagen de prueba:\n";
    const Imagen gris = leer("tests/datos/gris.pgm");
    const Imagen referencia = gaussiano_directo(gris);
    fallos += revisar("gris.pgm, secuencial", referencia, gaussiano_secuencial(gris));
    fallos += revisar("gris.pgm, paralelo", referencia, gaussiano_paralelo(gris));
    fallos += revisar("gris.pgm, bloques", referencia, gaussiano_bloques(gris));

    if (fallos == 0) {
        std::cout << "gaussiano: todas las pruebas pasaron\n";
        return 0;
    }
    std::cout << "gaussiano: " << fallos << " prueba(s) fallaron\n";
    return 1;
}