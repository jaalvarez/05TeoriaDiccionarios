// 07_comparativa_diccionarios.cpp
// Objetivo: cerrar el tema poniendo frente a frente, con las MISMAS claves
// (cadenas), las tres familias de diccionario que hemos visto:
//
//   ARBOL    std::map<string,int>            arbol rojo-negro (tema 4)
//   HASH     std::unordered_map<string,int>  tabla hash con encadenamiento
//   TRIE     el del fichero 06, con los hijos de cada nodo en:
//              - un array de 26 punteros
//              - un std::map<char, Nodo*>
//              - un std::unordered_map<char, Nodo*>
//
// Se miden cuatro cosas:
//   1. Construir: insertar N palabras.
//   2. Memoria: cuantos bytes se han pedido con new para guardarlas.
//   3. Buscar: N palabras que estan y N que no estan.
//   4. Contar por prefijo: cuantas palabras empiezan por "ca", "pla"...
//      Es la pregunta que separa a las tres familias:
//        - el arbol las tiene ORDENADAS: las que empiezan por p son un
//          tramo contiguo, desde lower_bound(p): O(log n + k);
//        - el trie baja por las letras de p y lee un contador: O(|p|);
//        - la tabla hash NO tiene orden: hay que mirarlas TODAS, O(n).
//
// Este fichero no tiene nada que completar. Para que contar por prefijo sea
// O(|p|), cada nodo del trie guarda cuantas palabras pasan por el (pasan).
//
// Compilar (con -O2: mide tiempos):
//   g++ -std=c++17 -O2 -Wall -o 07_comp 07_comparativa_diccionarios.cpp && ./07_comp

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <random>
#include <chrono>
#include <cstdlib>
#include <new>
#include <type_traits>

// =========================================================================
// HERRAMIENTA DE MEDIDA: contar los bytes pedidos con new
// =========================================================================
// Se sustituye el operator new GLOBAL (el que usan todos los contenedores
// de la STL por debajo) por uno que apunta cuanto se pide. Cada bloque
// lleva delante una cabecera de 16 bytes con su tamanio, para poder
// restarlo al liberarlo. No cuenta lo que aniade malloc por su cuenta.
static long long bytesVivos = 0;

void* operator new(std::size_t n) {
    void* p = std::malloc(n + 16);
    if (p == nullptr) throw std::bad_alloc();
    *static_cast<std::size_t*>(p) = n;
    bytesVivos += static_cast<long long>(n);
    return static_cast<char*>(p) + 16;
}

void operator delete(void* p) noexcept {
    if (p == nullptr) return;
    char* bloque = static_cast<char*>(p) - 16;
    bytesVivos -= static_cast<long long>(*reinterpret_cast<std::size_t*>(bloque));
    std::free(bloque);
}

void operator delete(void* p, std::size_t) noexcept { operator delete(p); }

// =========================================================================
// Los contendientes, todos con la misma interfaz:
//   insertar(p)   buscar(p)   contarPrefijo(pre)
// =========================================================================
class Arbol {                                     // std::map
public:
    void insertar(const std::string& p) { datos_[p] = 1; }
    bool buscar(const std::string& p) const { return datos_.count(p) > 0; }
    int contarPrefijo(const std::string& pre) const {
        int k = 0;
        for (auto it = datos_.lower_bound(pre);   // la primera >= pre...
             it != datos_.end() && it->first.compare(0, pre.size(), pre) == 0; ++it)
            k++;                                  // ...y las siguientes, mientras empiecen por pre
        return k;
    }
private:
    std::map<std::string, int> datos_;
};

class Hash {                                      // std::unordered_map
public:
    void insertar(const std::string& p) { datos_[p] = 1; }
    bool buscar(const std::string& p) const { return datos_.count(p) > 0; }
    int contarPrefijo(const std::string& pre) const {
        int k = 0;
        for (const auto& par : datos_)            // sin orden: hay que mirarlas TODAS
            if (par.first.compare(0, pre.size(), pre) == 0) k++;
        return k;
    }
private:
    std::unordered_map<std::string, int> datos_;
};

class TrieArray {                                 // hijos[26]
public:
    TrieArray() : raiz_(new Nodo) {}
    ~TrieArray() { liberar(raiz_); }
    TrieArray(const TrieArray&) = delete;
    TrieArray& operator=(const TrieArray&) = delete;

    void insertar(const std::string& p) {
        if (buscar(p)) return;                    // ya estaba: no contarla dos veces
        Nodo* n = raiz_;
        n->pasan++;
        for (char c : p) {
            Nodo*& h = n->hijos[c - 'a'];
            if (h == nullptr) { h = new Nodo; nodos_++; }
            n = h;
            n->pasan++;
        }
        n->finDePalabra = true;
    }
    bool buscar(const std::string& p) const {
        const Nodo* n = bajar(p);
        return n != nullptr && n->finDePalabra;
    }
    int contarPrefijo(const std::string& pre) const {
        const Nodo* n = bajar(pre);
        return n == nullptr ? 0 : n->pasan;
    }
    int nodos() const { return nodos_; }

private:
    struct Nodo {
        Nodo* hijos[26] = {};
        int pasan = 0;                            // palabras en este subarbol
        bool finDePalabra = false;
    };
    Nodo* raiz_;
    int nodos_ = 1;

    const Nodo* bajar(const std::string& s) const {
        const Nodo* n = raiz_;
        for (char c : s) {
            n = n->hijos[c - 'a'];
            if (n == nullptr) return nullptr;
        }
        return n;
    }
    static void liberar(Nodo* n) {
        for (Nodo* h : n->hijos)
            if (h != nullptr) liberar(h);
        delete n;
    }
};

template <template <class...> class Mapa>
class Trie {                                      // hijos en map o unordered_map
public:
    Trie() : raiz_(new Nodo) {}
    ~Trie() { liberar(raiz_); }
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insertar(const std::string& p) {
        if (buscar(p)) return;
        Nodo* n = raiz_;
        n->pasan++;
        for (char c : p) {
            Nodo*& h = n->hijos[c];
            if (h == nullptr) { h = new Nodo; nodos_++; }
            n = h;
            n->pasan++;
        }
        n->finDePalabra = true;
    }
    bool buscar(const std::string& p) const {
        const Nodo* n = bajar(p);
        return n != nullptr && n->finDePalabra;
    }
    int contarPrefijo(const std::string& pre) const {
        const Nodo* n = bajar(pre);
        return n == nullptr ? 0 : n->pasan;
    }
    int nodos() const { return nodos_; }

private:
    struct Nodo {
        Mapa<char, Nodo*> hijos;
        int pasan = 0;
        bool finDePalabra = false;
    };
    Nodo* raiz_;
    int nodos_ = 1;

    const Nodo* bajar(const std::string& s) const {
        const Nodo* n = raiz_;
        for (char c : s) {
            auto it = n->hijos.find(c);
            if (it == n->hijos.end()) return nullptr;
            n = it->second;
        }
        return n;
    }
    static void liberar(Nodo* n) {
        for (auto& par : n->hijos) liberar(par.second);
        delete n;
    }
};

// =========================================================================
// Datos: palabras inventadas a partir de silabas (solo letras a-z)
// =========================================================================
std::vector<std::string> generarPalabras(int n, unsigned semilla) {
    static const char* silabas[] = {"ca", "sa", "ma", "pe", "ro", "la", "ti", "no", "me", "su",
                                    "co", "ra", "de", "mi", "lo", "pa", "ta", "ne", "ve", "bu",
                                    "cha", "tra", "pla", "gri", "llo", "que", "fre", "sol", "mar", "luz"};
    const int nSilabas = sizeof(silabas) / sizeof(silabas[0]);
    std::mt19937 gen(semilla);
    std::uniform_int_distribution<int> cuantas(2, 5), cual(0, nSilabas - 1);
    std::vector<std::string> salida;
    std::set<std::string> vistas;
    while (static_cast<int>(salida.size()) < n) {
        std::string p;
        int k = cuantas(gen);
        for (int i = 0; i < k; i++) p += silabas[cual(gen)];
        if (vistas.insert(p).second) salida.push_back(p);
    }
    return salida;
}

struct Resultado {
    double construir, buscar, prefijos;   // ms
    long long bytes;
    int encontradas, contadas;
};

using Reloj = std::chrono::steady_clock;
double ms(Reloj::time_point a, Reloj::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

template <class Dicc>
Resultado medir(const std::vector<std::string>& dentro, const std::vector<std::string>& fuera,
                const std::vector<std::string>& prefijos, int* nodos = nullptr) {
    Resultado r{};
    long long antes = bytesVivos;
    auto t0 = Reloj::now();
    Dicc* d = new Dicc;
    for (const auto& p : dentro) d->insertar(p);
    auto t1 = Reloj::now();
    r.bytes = bytesVivos - antes;
    r.construir = ms(t0, t1);

    t0 = Reloj::now();
    for (const auto& p : dentro) r.encontradas += d->buscar(p);
    for (const auto& p : fuera) r.encontradas += d->buscar(p);
    t1 = Reloj::now();
    r.buscar = ms(t0, t1);

    t0 = Reloj::now();
    for (const auto& pre : prefijos) r.contadas += d->contarPrefijo(pre);
    t1 = Reloj::now();
    r.prefijos = ms(t0, t1);

    if constexpr (!std::is_same_v<Dicc, Arbol> && !std::is_same_v<Dicc, Hash>) {
        if (nodos != nullptr) *nodos = d->nodos();
    }
    delete d;
    return r;
}

void fila(const char* nombre, const Resultado& r, int n) {
    std::cout << "  " << std::left << std::setw(26) << nombre << std::right << std::fixed
              << std::setprecision(1)
              << std::setw(10) << r.construir
              << std::setw(10) << r.bytes / (1024.0 * 1024.0)
              << std::setw(10) << static_cast<double>(r.bytes) / n
              << std::setw(10) << r.buscar
              << std::setw(12) << std::setprecision(2) << r.prefijos << '\n';
}

int main() {
    const int n = 100000;
    auto todas = generarPalabras(2 * n, 2026);
    std::vector<std::string> dentro(todas.begin(), todas.begin() + n);
    std::vector<std::string> fuera(todas.begin() + n, todas.end());   // nunca se insertan

    // 300 prefijos de 2 a 4 letras, sacados de las propias palabras
    std::vector<std::string> prefijos;
    std::mt19937 gen(1);
    for (int i = 0; i < 300; i++) {
        const std::string& p = dentro[gen() % n];
        prefijos.push_back(p.substr(0, 2 + i % 3));
    }

    long long letras = 0;
    for (const auto& p : dentro) letras += static_cast<long long>(p.size());
    std::cout << "=== " << n << " palabras (media " << std::fixed << std::setprecision(1)
              << static_cast<double>(letras) / n << " letras), " << n
              << " busquedas con exito, " << n << " sin exito,\n"
              << "    " << prefijos.size() << " consultas por prefijo ===\n\n";

    int nodosA = 0, nodosM = 0, nodosU = 0;
    Resultado rArbol = medir<Arbol>(dentro, fuera, prefijos);
    Resultado rHash = medir<Hash>(dentro, fuera, prefijos);
    Resultado rTA = medir<TrieArray>(dentro, fuera, prefijos, &nodosA);
    Resultado rTM = medir<Trie<std::map>>(dentro, fuera, prefijos, &nodosM);
    Resultado rTU = medir<Trie<std::unordered_map>>(dentro, fuera, prefijos, &nodosU);

    std::cout << "  " << std::left << std::setw(26) << "" << std::right
              << std::setw(10) << "construir" << std::setw(10) << "memoria"
              << std::setw(10) << "bytes/" << std::setw(10) << "buscar"
              << std::setw(12) << "prefijos" << '\n';
    std::cout << "  " << std::left << std::setw(26) << "" << std::right
              << std::setw(10) << "(ms)" << std::setw(10) << "(MB)"
              << std::setw(10) << "palabra" << std::setw(10) << "(ms)"
              << std::setw(12) << "(ms)" << '\n';
    fila("ARBOL  std::map", rArbol, n);
    fila("HASH   std::unordered_map", rHash, n);
    fila("TRIE   hijos[26]", rTA, n);
    fila("TRIE   hijos en map", rTM, n);
    fila("TRIE   hijos en unordered", rTU, n);

    bool iguales = rArbol.encontradas == n && rHash.encontradas == n && rTA.encontradas == n &&
                   rTM.encontradas == n && rTU.encontradas == n &&
                   rArbol.contadas == rHash.contadas && rHash.contadas == rTA.contadas &&
                   rTA.contadas == rTM.contadas && rTM.contadas == rTU.contadas;
    std::cout << "\n  todos encuentran lo mismo: " << std::boolalpha << iguales
              << "  (" << rArbol.contadas << " palabras contadas por prefijo)\n";
    std::cout << "  nodos del trie: " << nodosA << " para " << letras << " letras ("
              << std::setprecision(2) << static_cast<double>(nodosA) / n
              << " nodos por palabra):\n  el resto de las letras caen en prefijos que ya estaban\n";

    std::cout << "\n  Lo que hay que leer en la tabla:\n"
              << "  - buscar: gana la tabla hash (O(1) esperado). El trie de 26 punteros\n"
              << "    va cerca: O(L), un acceso a array por letra. Los tries con map o\n"
              << "    unordered_map pagan una busqueda en un contenedor POR LETRA. El\n"
              << "    arbol hace ~log2(n) = 17 comparaciones de cadenas enteras.\n"
              << "  - prefijos: la tabla hash pierde por goleada: mira las n claves en\n"
              << "    cada consulta. El arbol recorre un tramo ordenado (O(log n + k)) y\n"
              << "    el trie lee un contador (O(|p|)): rapidos porque TIENEN ORDEN.\n"
              << "  - memoria: el trie NO guarda las cadenas y aun asi es el que mas\n"
              << "    gasta. Compartir prefijos ahorra letras, pero cada nodo cuesta\n"
              << "    mucho mas que una letra: 26 punteros casi todos a nullptr, o un\n"
              << "    contenedor entero (map, unordered_map) por nodo.\n";
}
