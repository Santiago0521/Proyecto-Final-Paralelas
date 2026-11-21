
#include "filtros.hpp"
#include "imagen.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <omp.h>

using Reloj = std::chrono::steady_clock;

constexpr int REPETICIONES = 5;

struct Tiempos {
    double histograma;
    double barrido;
    double binarizacion;
    double total;
};

// Calcula el umbral óptimo de Otsu.
// Esta etapa es secuencial, igual que en filtros.cpp.
int calcular_umbral(const Histograma& h, std::uint64_t total) {
    double suma_total = 0.0;

    for (std::size_t i = 0; i < h.size(); ++i) {
        suma_total += static_cast<double>(i) *
                      static_cast<double>(h[i]);
    }

    std::uint64_t peso_fondo = 0;
    double suma_fondo = 0.0;
    double varianza_maxima = 0.0;
    int umbral_optimo = 0;

    for (int t = 0; t < 256; ++t) {
        peso_fondo += h[t];
        suma_fondo += static_cast<double>(t) *
                      static_cast<double>(h[t]);

        const std::uint64_t peso_objeto = total - peso_fondo;

        if (peso_fondo == 0 || peso_objeto == 0) {
            continue;
        }

        const double media_fondo = suma_fondo / peso_fondo;
        const double media_objeto =
            (suma_total - suma_fondo) / peso_objeto;

        const double diferencia = media_fondo - media_objeto;

        const double varianza =
            static_cast<double>(peso_fondo) *
            static_cast<double>(peso_objeto) *
            diferencia * diferencia;

        if (varianza > varianza_maxima) {
            varianza_maxima = varianza;
            umbral_optimo = t;
        }
    }

    return umbral_optimo;
}

Tiempos medir(const Imagen& e, int hilos) {
    omp_set_dynamic(0);
    omp_set_num_threads(hilos);

    const std::uint64_t total = e.datos.size();

    // Medición del histograma paralelo.
    auto inicio = Reloj::now();

    Histograma h = histograma_paralelo_privado(e, hilos);

    auto fin = Reloj::now();

    const double tiempo_histograma =
        std::chrono::duration<double, std::milli>(
            fin - inicio).count();

    // Medición del barrido secuencial de Otsu.
    inicio = Reloj::now();

    const int umbral = calcular_umbral(h, total);

    fin = Reloj::now();

    const double tiempo_barrido =
        std::chrono::duration<double, std::milli>(
            fin - inicio).count();

    // Preparación de la imagen de salida, fuera de la medición.
    Imagen salida;
    salida.ancho = e.ancho;
    salida.alto = e.alto;
    salida.canales = 1;
    salida.datos.resize(e.datos.size());

    // Medición de la binarización paralela.
    inicio = Reloj::now();

    #pragma omp parallel for num_threads(hilos) schedule(static)
    for (std::size_t i = 0; i < e.datos.size(); ++i) {
        salida.datos[i] =
            (e.datos[i] > umbral) ? 255 : 0;
    }

    fin = Reloj::now();

    const double tiempo_binarizacion =
        std::chrono::duration<double, std::milli>(
            fin - inicio).count();

    // Medición completa de Otsu.
    inicio = Reloj::now();

    const Imagen resultado = otsu_paralelo(e);

    fin = Reloj::now();

    const double tiempo_total =
        std::chrono::duration<double, std::milli>(
            fin - inicio).count();

    // Comprobación de correctitud.
    if (resultado.datos != salida.datos) {
        throw std::runtime_error(
            "El resultado medido no coincide con Otsu completo");
    }

    return {
        tiempo_histograma,
        tiempo_barrido,
        tiempo_binarizacion,
        tiempo_total
    };
}

double mediana(std::vector<double> valores) {
    std::sort(valores.begin(), valores.end());

    const std::size_t n = valores.size();

    if (n % 2 == 0) {
        return (valores[n / 2 - 1] + valores[n / 2]) / 2.0;
    }

    return valores[n / 2];
}

int main() {
    try {
        std::cout << "Ley de Amdahl para Otsu\n";

        Imagen entrada = leer("imagen_otsu.ppm");
        Imagen gris = gris_secuencial(entrada);

        const int cantidades[] = {1, 2, 4, 8};

        // Referencia secuencial de Otsu.
        std::vector<double> tiempos_secuenciales;

        for (int r = 0; r < REPETICIONES; ++r) {
            auto inicio = Reloj::now();

            Imagen resultado = otsu_secuencial(gris);

            auto fin = Reloj::now();

            if (r > 0) {
                tiempos_secuenciales.push_back(
                    std::chrono::duration<double, std::milli>(
                        fin - inicio).count()
                );
            }
        }

        const double tiempo_secuencial = mediana(tiempos_secuenciales);

        std::cout << "Otsu secuencial: "
          << tiempo_secuencial << " ms\n";

        std::cout << "Imagen: "
                  << gris.ancho << "x" << gris.alto << '\n';

        std::cout << std::fixed << std::setprecision(4);

        std::cout << "\nHilos|Histograma_ms|Barrido_ms|"
                     "Binarizacion_ms|Total_ms\n";

        for (int hilos : cantidades) {
            std::vector<double> histogramas;
            std::vector<double> barridos;
            std::vector<double> binarizaciones;
            std::vector<double> totales;

            for (int r = 0; r < REPETICIONES; ++r) {
                Tiempos t = medir(gris, hilos);

                // Se descarta la primera repetición.
                if (r == 0) {
                    continue;
                }

                histogramas.push_back(t.histograma);
                barridos.push_back(t.barrido);
                binarizaciones.push_back(t.binarizacion);
                totales.push_back(t.total);
            }

            std::cout << hilos << '|'
                      << mediana(histogramas) << '|'
                      << mediana(barridos) << '|'
                      << mediana(binarizaciones) << '|'
                      << mediana(totales) << '\n';
        }

        std::cout << "\nMediciones finalizadas correctamente.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
