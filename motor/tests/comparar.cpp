// Compara dos archivos byte a byte. La salida de la versión paralela de un
// filtro tiene que ser idéntica a la de la versión secuencial, así que la
// comparación no admite tolerancia.
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iterator>

namespace {

std::vector<char> contenido(const std::string& ruta) {
    std::ifstream e(ruta, std::ios::binary);
    if (!e) throw std::runtime_error("no se puede abrir " + ruta);
    return std::vector<char>(std::istreambuf_iterator<char>(e),
                             std::istreambuf_iterator<char>());
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "uso: comparar <archivo-a> <archivo-b>\n";
        return 2;
    }
    try {
        const std::vector<char> a = contenido(argv[1]);
        const std::vector<char> b = contenido(argv[2]);
        if (a.size() != b.size()) {
            std::cout << "distinto tamaño: " << a.size() << " y " << b.size() << " bytes\n";
            return 1;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) {
                std::cout << "difieren en el byte " << i << ": "
                          << static_cast<int>(static_cast<unsigned char>(a[i])) << " y "
                          << static_cast<int>(static_cast<unsigned char>(b[i])) << "\n";
                return 1;
            }
        }
        std::cout << "iguales, " << a.size() << " bytes\n";
    } catch (const std::exception& error) {
        std::cerr << "comparar: " << error.what() << "\n";
        return 2;
    }
    return 0;
}
