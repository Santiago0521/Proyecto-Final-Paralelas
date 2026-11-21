#ifndef FILTROS_HPP
#define FILTROS_HPP

#include "imagen.hpp"
#include <array>
#include <cstdint>

// Escala de grises con la ponderación BT.601. Entra un PPM de color y sale
// un PGM de un canal. Cada píxel de salida depende solo del píxel de entrada
// en la misma posición.
Imagen gris_secuencial(const Imagen& e);
Imagen gris_paralelo(const Imagen& e);

// Desenfoque de caja: cada píxel pasa a ser el promedio de los nueve de su
// vecindad de 3x3. Trabaja igual sobre uno o tres canales.
Imagen caja_secuencial(const Imagen& e);
Imagen caja_paralelo(const Imagen& e);

// Desenfoque gaussiano separable con el núcleo binomial [1 4 6 4 1]/16, en
// una pasada horizontal y otra vertical sobre el resultado intermedio.
Imagen gaussiano_secuencial(const Imagen& e);
Imagen gaussiano_paralelo(const Imagen& e);

// Gaussiano separable por bloques de filas: misma aritmética que las otras
// versiones, pero la pasada vertical trabaja sobre un buffer pequeño por
// bloque en lugar de sobre toda la imagen intermedia T.
Imagen gaussiano_bloques(const Imagen& e);

// Magnitud del gradiente de Sobel sobre una imagen de un canal.
Imagen sobel_secuencial(const Imagen& e);
Imagen sobel_paralelo(const Imagen& e);

// Umbral de Otsu: busca el umbral que maximiza la varianza entre clases del
// histograma y binariza con él.
Imagen otsu_secuencial(const Imagen& e);
Imagen otsu_paralelo(const Imagen& e);

using Histograma = std::array<std::uint64_t, 256>;

Histograma histograma_secuencial(const Imagen& e);
Histograma histograma_paralelo_compartido(
    const Imagen& e, int hilos);

Histograma histograma_paralelo_privado(
    const Imagen& e, int hilos);

Imagen ecualizacion_secuencial(const Imagen& e);
Imagen ecualizacion_paralela(const Imagen& e);

#endif