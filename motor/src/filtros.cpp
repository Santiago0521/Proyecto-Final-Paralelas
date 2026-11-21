#include "filtros.hpp"
#include <cstddef>
#include <stdexcept>
#include <cmath>
#include <omp.h>
#include <vector>
#include <memory>
#include <algorithm>

Imagen gris_secuencial(const Imagen& e) {
    if (e.canales != 3) throw std::runtime_error("gris espera una imagen de tres canales");
    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = 1;
    s.datos.resize(static_cast<std::size_t>(e.ancho) * e.alto);
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            const double v = 0.299 * e.en(x, y, 0) + 0.587 * e.en(x, y, 1) + 0.114 * e.en(x, y, 2);
            s.poner(x, y, 0, redondear(v));
        }
    }
    return s;
}

Imagen gris_paralelo(const Imagen& e) {
    if (e.canales != 3) throw std::runtime_error("gris espera una imagen de tres canales");
    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = 1;
    s.datos.resize(static_cast<std::size_t>(e.ancho) * e.alto);
    // Cada fila es independiente de las demás: no hay nada que compartir
    // entre hilos ni dependencias entre iteraciones.
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            const double v = 0.299 * e.en(x, y, 0) + 0.587 * e.en(x, y, 1) + 0.114 * e.en(x, y, 2);
            s.poner(x, y, 0, redondear(v));
        }
    }
    return s;
}

namespace {

// El promedio de la vecindad de 3x3 de un canal, con las coordenadas
// recortadas al rectángulo de la imagen.
double promedio_vecindad(const Imagen& e, int x, int y, int c) {
    double suma = 0.0;
    for (int j = -1; j <= 1; ++j) {
        const int fila = recortar(y + j, e.alto);
        for (int i = -1; i <= 1; ++i) {
            suma += e.en(recortar(x + i, e.ancho), fila, c);
        }
    }
    return suma / 9.0;
}

}  // namespace

Imagen caja_secuencial(const Imagen& e) {
    Imagen s = e;
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                s.poner(x, y, c, redondear(promedio_vecindad(e, x, y, c)));
            }
        }
    }
    return s;
}

Imagen caja_paralelo(const Imagen& e) {
    Imagen s = e;
    // El ciclo externo recorre filas y el interno columnas, que es el orden
    // en que la imagen está en memoria. Repartir por filas deja a cada hilo
    // con una banda contigua.
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                s.poner(x, y, c, redondear(promedio_vecindad(e, x, y, c)));
            }
        }
    }
    return s;
}

namespace {

// Núcleo binomial [1 4 6 4 1] / 16, indexado de -2 a 2 con desplazamiento +2.
constexpr double KERNEL[5] = {1.0 / 16, 4.0 / 16, 6.0 / 16, 4.0 / 16, 1.0 / 16};

// Pasada horizontal: T(x, y) = sum_i k_i * I(x + i, y), en doble precisión.
std::vector<double> pasada_horizontal(const Imagen& e) {
    const std::size_t n = static_cast<std::size_t>(e.ancho) * e.alto * e.canales;
    std::vector<double> T(n);
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                double suma = 0.0;
                for (int i = -2; i <= 2; ++i) {
                    suma += KERNEL[i + 2] * e.en(recortar(x + i, e.ancho), y, c);
                }
                T[(static_cast<std::size_t>(y) * e.ancho + x) * e.canales + c] = suma;
            }
        }
    }
    return T;
}

void pasada_vertical(const double* T, int ancho, int alto, int canales,
                     Imagen& s, int y, int x, int c) {
    double suma = 0.0;
    for (int j = -2; j <= 2; ++j) {
        const int fila = recortar(y + j, alto);
        suma += KERNEL[j + 2] * T[(static_cast<std::size_t>(fila) * ancho + x) * canales + c];
    }
    s.poner(x, y, c, redondear(suma));
}

Imagen crear_salida(const Imagen& e) {
    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = e.canales;
    s.datos.resize(static_cast<std::size_t>(e.ancho) * e.alto * e.canales);
    return s;
}

}  // namespace

Imagen gaussiano_secuencial(const Imagen& e) {
    const std::vector<double> T = pasada_horizontal(e);
    Imagen s = crear_salida(e);
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                pasada_vertical(T.data(), e.ancho, e.alto, e.canales, s, y, x, c);
            }
        }
    }
    return s;
}

Imagen gaussiano_paralelo(const Imagen& e) {
    const std::size_t n = static_cast<std::size_t>(e.ancho) * e.alto * e.canales;
    // Sin inicializar: la primera escritura la hacen los hilos, así cada
    // hilo toca las páginas de su banda y no un solo hilo todas.
    std::unique_ptr<double[]> T(new double[n]);
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                double suma = 0.0;
                for (int i = -2; i <= 2; ++i) {
                    suma += KERNEL[i + 2] * e.en(recortar(x + i, e.ancho), y, c);
                }
                T[(static_cast<std::size_t>(y) * e.ancho + x) * e.canales + c] = suma;
            }
        }
    }
    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = e.canales;
    s.datos.resize(n);  // la salida se pone a cero aquí, en un hilo
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            for (int c = 0; c < e.canales; ++c) {
                pasada_vertical(T.get(), e.ancho, e.alto, e.canales, s, y, x, c);
            }
        }
    }
    return s;
}

Imagen gaussiano_bloques(const Imagen& e) {
    constexpr int B = 16;  // filas de salida por bloque
    const std::size_t w = static_cast<std::size_t>(e.ancho) * e.canales;  // doubles por fila de T
    Imagen s = crear_salida(e);
    #pragma omp parallel
    {
        // Buffer privado de cada hilo: las filas de T que necesita un bloque,
        // incluidas dos filas arriba y dos abajo.
        std::vector<double> tb(static_cast<std::size_t>(B + 4) * w);
        #pragma omp for schedule(static)
        for (int y0 = 0; y0 < e.alto; y0 += B) {
            const int y1 = std::min(y0 + B, e.alto);
            // t = 0 corresponde a la fila y0 - 2 de T.
            for (int t = 0; t < (y1 - y0) + 4; ++t) {
                const int fila = recortar(y0 - 2 + t, e.alto);
                for (int x = 0; x < e.ancho; ++x) {
                    for (int c = 0; c < e.canales; ++c) {
                        double suma = 0.0;
                        for (int i = -2; i <= 2; ++i) {
                            suma += KERNEL[i + 2] * e.en(recortar(x + i, e.ancho), fila, c);
                        }
                        tb[static_cast<std::size_t>(t) * w +
                           static_cast<std::size_t>(x * e.canales + c)] = suma;
                    }
                }
            }
            // Pasada vertical sobre el buffer del bloque.
            for (int y = y0; y < y1; ++y) {
                for (int x = 0; x < e.ancho; ++x) {
                    for (int c = 0; c < e.canales; ++c) {
                        double suma = 0.0;
                        for (int j = -2; j <= 2; ++j) {
                            const int t = y - y0 + j + 2;
                            suma += KERNEL[j + 2] *
                                    tb[static_cast<std::size_t>(t) * w +
                                       static_cast<std::size_t>(x * e.canales + c)];
                        }
                        s.poner(x, y, c, redondear(suma));
                    }
                }
            }
        }
    }
    return s;
}

Imagen sobel_secuencial(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("sobel espera una imagen de un canal");
    };

    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = 1;
    s.datos.resize(static_cast<std::size_t>(e.ancho) * e.alto);
    
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            double gx = 0.0;
            double gy = 0.0;

            // Calcular la convolución con el kernel de Sobel
            for (int j = -1; j <= 1; ++j) {
                const int fila = recortar(y + j, e.alto);
                for (int i = -1; i <= 1; ++i) {
                    const int col = recortar(x + i, e.ancho);
                    const double pixel = e.en(col, fila, 0);

                    // Mascara Sobel Horizontal
                    const int peso_x = (i == -1 ? -1 : (i == 1 ? 1 : 0)) * (j == 0 ? 2 : 1);
                    // Mascara Sobel Vertical
                    const int peso_y = (j == -1 ? -1 : (j == 1 ? 1 : 0)) * (i == 0 ? 2 : 1);

                    gx += pixel * peso_x;
                    gy += pixel * peso_y;
                }
            }
            const double magnitude = std::sqrt(gx * gx + gy * gy);

            s.poner(x, y, 0, redondear(magnitude));
        }
    }
    return s;
}

Imagen sobel_paralelo(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("sobel espera una imagen de un canal");
    }

    Imagen s;
    s.ancho = e.ancho;
    s.alto = e.alto;
    s.canales = 1;
    s.datos.resize(static_cast<std::size_t>(e.ancho) * e.alto);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < e.alto; ++y) {
        for (int x = 0; x < e.ancho; ++x) {
            double gx = 0.0;
            double gy = 0.0;

            // Calcular la convolución con el kernel de Sobel
            for (int j = -1; j <= 1; ++j) {
                const int fila = recortar(y + j, e.alto);
                for (int i = -1; i <= 1; ++i) {
                    const int col = recortar(x + i, e.ancho);
                    const double pixel = e.en(col, fila, 0);

                    // Mascara Sobel Horizontal
                    const int peso_x = (i == -1 ? -1 : (i == 1 ? 1 : 0)) * (j == 0 ? 2 : 1);
                    
                    // Mascara Sobel Vertical
                    const int peso_y = (j == -1 ? -1 : (j == 1 ? 1 : 0)) * (i == 0 ? 2 : 1);

                    gx += pixel * peso_x;
                    gy += pixel * peso_y;
                }
            }
            const double magnitude = std::sqrt(gx * gx + gy * gy);

            s.poner(x, y, 0, redondear(magnitude));
        }
    }
    return s;
}

Imagen otsu_secuencial(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("otsu espera una imagen de un canal");
    }

    if (e.datos.empty()) {
        throw std::runtime_error("otsu: imagen vacía");
    }

    // Obtener el histograma de la imagen
    const Histograma h = histograma_secuencial(e);

    const std::uint64_t total = e.datos.size();

    // Calcular la suma ponderada de todos las intensidades
    double suma_total = 0.0;
    for (std::size_t i = 0; i < h.size(); ++i) {
        suma_total += static_cast<double>(i) * static_cast<double>(h[i]);
    }

    std::uint64_t peso_fondo = 0;
    double suma_fondo = 0.0;

    double varianza_maxima = 0.0;
    int umbral_optimo = 0;

    for (int t = 0; t < 256; ++t) {
        peso_fondo += h[static_cast<std::size_t>(t)];

        suma_fondo += static_cast<double>(t) * static_cast<double>(h[static_cast<std::size_t>(t)]);

        const std::uint64_t peso_objeto = total - peso_fondo;

        // Ignorar umbrales que dejan una de las clases vacía
        if (peso_fondo == 0 || peso_objeto == 0) {
            continue;
        }

        const double media_fondo = suma_fondo / static_cast<double>(peso_fondo);
        const double media_objeto = (suma_total - suma_fondo) / static_cast<double>(peso_objeto);

        const double diferencia = media_fondo - media_objeto;
        const double varianza = static_cast<double>(peso_fondo) * static_cast<double>(peso_objeto) * diferencia * diferencia;

        if (varianza > varianza_maxima) {
            varianza_maxima = varianza;
            umbral_optimo = t;
        }
    }

    // Contruir la imagen binaria
    Imagen salida;
    salida.ancho = e.ancho;
    salida.alto = e.alto;
    salida.canales = 1;
    salida.datos.resize(e.datos.size());

    for (std::size_t i = 0; i < e.datos.size(); ++i) {
        salida.datos[i] = (e.datos[i] > umbral_optimo) ? 255 : 0;
    }
    return salida;
}

Imagen otsu_paralelo(const Imagen& e) {
    if (e.canales != 1) {
        throw std::runtime_error("otsu espera una imagen de un canal");
    }

    if (e.datos.empty()) {
        throw std::runtime_error("otsu: imagen vacía");
    }

    // Calcular el histograma usando acumuladores privados por hilo
    const int hilos = omp_get_max_threads();
    const Histograma h = histograma_paralelo_privado(e, hilos);

    const std::uint64_t total = e.datos.size();

    // Calcular la suma ponderada de todos las intensidades
    double suma_total = 0.0;
    for (std::size_t i = 0; i < h.size(); ++i) {
        suma_total += static_cast<double>(i) * static_cast<double>(h[i]);
    }

    // Buscar secuencialmente el umbral óptimo de Otsu
    std::uint64_t peso_fondo = 0;
    double suma_fondo = 0.0;
    
    double varianza_maxima = 0.0;
    int umbral_optimo = 0;

    for (int t = 0; t < 256; ++t) {
        peso_fondo += h[static_cast<std::size_t>(t)];
        suma_fondo += static_cast<double>(t) * static_cast<double>(h[static_cast<std::size_t>(t)]);

        const std::uint64_t peso_objeto = total - peso_fondo;

        if (peso_fondo == 0 || peso_objeto == 0) {
            continue;
        }

        const double media_fondo = suma_fondo / static_cast<double>(peso_fondo);
        const double media_objeto = (suma_total - suma_fondo) / static_cast<double>(peso_objeto);

        const double diferencia = media_fondo - media_objeto;
        const double varianza = static_cast<double>(peso_fondo) * static_cast<double>(peso_objeto) * diferencia * diferencia;

        if (varianza > varianza_maxima) {
            varianza_maxima = varianza;
            umbral_optimo = t;
        }
    }

    // Contruir la imagen binaria
    Imagen salida;
    salida.ancho = e.ancho;
    salida.alto = e.alto;
    salida.canales = 1;
    salida.datos.resize(e.datos.size());

    // Cada pixel es independiente de los demás, así que se puede paralelizar
    #pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < e.datos.size(); ++i) {
        salida.datos[i] = (e.datos[i] > umbral_optimo) ? 255 : 0;
    }
    return salida;
}
