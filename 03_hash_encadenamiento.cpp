// 03_hash_encadenamiento.cpp
// Objetivo: la primera forma de RESOLVER las colisiones del fichero 02.
// Cada cubeta del array no guarda una clave, sino una LISTA ENLAZADA (la
// del tema 2) con todas las claves que han caido en ella. Es el
// ENCADENAMIENTO (separate chaining):
//
//    cubetas_[0] -> nullptr
//    cubetas_[1] -> [mesa|3] -> [pelo|8] -> nullptr     (h(mesa) = h(pelo) = 1)
//    cubetas_[2] -> [sol|1] -> nullptr
//    ...
//
// Insertar: calcular h(c) y meter el nodo AL PRINCIPIO de esa lista (O(1),
// insertarPrincipio del tema 2... tras comprobar que no estaba). Buscar y
// borrar: recorrer solo ESA lista.
//
// EL COSTE depende de lo larga que sea la lista, y eso lo mide el FACTOR DE
// CARGA alfa = n / m (claves por cubeta). Con una buena funcion hash:
//     buscar con exito:  ~ 1 + alfa/2 comparaciones
//     buscar sin exito:  ~ alfa
// Si m es FIJO y n crece, alfa crece con n y buscar vuelve a ser O(n): una
// tabla de 1000 cubetas con un millon de claves es un array de 1000 listas
// de 1000 elementos.
//
// LA SOLUCION es la misma idea que el array dinamico del tema 2: cuando
// alfa pasa de un limite (aqui 1), se pide un array de cubetas MAS GRANDE
// (el doble, redondeado a primo) y se RECOLOCA cada clave en su nueva
// cubeta. Eso es el REHASH. Cada rehash cuesta O(n), pero como el tamanio
// se duplica, el coste AMORTIZADO por insercion sigue siendo O(1): la
// misma cuenta que en 06_coste_amortizado.cpp del tema 2.
//
// QUE SIGUE SIN RESOLVER: cada clave es un nodo suelto en el heap (un new
// por insercion, 8 bytes de puntero por nodo y saltos por memoria que la
// cache no predice). El fichero 04 lo guarda todo DENTRO del array.
//
// EJERCICIO: completar rehash(). Mientras no este hecho no hace nada: la
// tabla sigue funcionando, pero m no crece nunca y las dos mitades de la
// tabla grande del main salen iguales.
//
// Compilar:
//   g++ -std=c++17 -O2 -Wall -o 03_encad 03_hash_encadenamiento.cpp && ./03_encad

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <chrono>
#include <algorithm>

// La funcion hash del fichero 02 (ya resuelta).
unsigned long long hashPolinomico(const std::string& s) {
    unsigned long long h = 0;
    for (char c : s) h = h * 31 + static_cast<unsigned char>(c);
    return h;
}

bool esPrimo(int x) {
    if (x < 2) return false;
    for (int d = 2; static_cast<long long>(d) * d <= x; d++)
        if (x % d == 0) return false;
    return true;
}

int siguientePrimo(int x) {
    while (!esPrimo(x)) x++;
    return x;
}

// =========================================================================
// LA ESPECIFICACION: la del TAD Diccionario del fichero 01
// =========================================================================
//  insertar(c, v)  post: c tiene asociado v (si ya estaba, se sustituye)
//  buscar(c, v)    post: devuelve si c esta; si esta, deja su valor en v
//  borrar(c)       post: si c estaba, la quita y devuelve true
//  tamanio()       post: numero de claves guardadas

class TablaHashEncadenada {
public:
    // redimensionar = false deja m FIJO para siempre: sirve para medir lo
    // que pasa sin rehash.
    explicit TablaHashEncadenada(int m = 7, bool redimensionar = true)
        : cubetas_(m, nullptr), redimensionar_(redimensionar) {}

    ~TablaHashEncadenada() {
        for (Nodo* p : cubetas_) {
            while (p != nullptr) {
                Nodo* aBorrar = p;
                p = p->siguiente;
                delete aBorrar;
            }
        }
    }

    // Copiar una tabla entera casi nunca hace falta: se prohibe (como en el
    // BST del tema 4).
    TablaHashEncadenada(const TablaHashEncadenada&) = delete;
    TablaHashEncadenada& operator=(const TablaHashEncadenada&) = delete;

    void insertar(const std::string& clave, int valor) {
        int i = indice(clave);
        for (Nodo* p = cubetas_[i]; p != nullptr; p = p->siguiente) {
            comparaciones_++;
            if (p->clave == clave) { p->valor = valor; return; }   // ya estaba
        }
        cubetas_[i] = new Nodo{clave, valor, cubetas_[i]};         // al principio: O(1)
        n_++;
        if (redimensionar_ && n_ > ALFA_MAX * cubetas()) {
            rehash(siguientePrimo(2 * cubetas()));
        }
    }

    bool buscar(const std::string& clave, int& valor) const {
        for (Nodo* p = cubetas_[indice(clave)]; p != nullptr; p = p->siguiente) {
            comparaciones_++;
            if (p->clave == clave) { valor = p->valor; return true; }
        }
        return false;
    }

    bool borrar(const std::string& clave) {
        int i = indice(clave);
        Nodo* anterior = nullptr;
        for (Nodo* p = cubetas_[i]; p != nullptr; anterior = p, p = p->siguiente) {
            comparaciones_++;
            if (p->clave == clave) {
                if (anterior == nullptr) cubetas_[i] = p->siguiente;   // era el primero
                else anterior->siguiente = p->siguiente;
                delete p;
                n_--;
                return true;
            }
        }
        return false;
    }

    int tamanio() const { return n_; }
    int cubetas() const { return static_cast<int>(cubetas_.size()); }
    double factorCarga() const { return static_cast<double>(n_) / cubetas(); }
    long long comparaciones() const { return comparaciones_; }
    void reiniciarComparaciones() { comparaciones_ = 0; }
    long long nodosRecolocados() const { return recolocados_; }

    int cadenaMasLarga() const {
        int maxima = 0;
        for (Nodo* p : cubetas_) {
            int largo = 0;
            for (; p != nullptr; p = p->siguiente) largo++;
            maxima = std::max(maxima, largo);
        }
        return maxima;
    }

    void imprimir() const {
        for (int i = 0; i < cubetas(); i++) {
            std::cout << "    [" << std::setw(2) << i << "] ";
            for (Nodo* p = cubetas_[i]; p != nullptr; p = p->siguiente)
                std::cout << "-> " << p->clave << ' ';
            std::cout << '\n';
        }
    }

private:
    struct Nodo {
        std::string clave;
        int valor;
        Nodo* siguiente;
    };

    // --- Invariante de la representacion ---------------------------------
    //  1. Todo nodo de la lista cubetas_[i] cumple
    //         hashPolinomico(nodo->clave) % cubetas() == i
    //     (cada clave esta en LA cubeta que le toca, y solo en esa).
    //  2. No hay dos nodos con la misma clave en toda la tabla.
    //  3. n_ es el numero total de nodos.
    //  4. Si redimensionar_: n_ <= ALFA_MAX * cubetas() tras cada insertar.
    // -----------------------------------------------------------------------
    std::vector<Nodo*> cubetas_;
    int n_ = 0;
    bool redimensionar_;
    mutable long long comparaciones_ = 0;
    long long recolocados_ = 0;
    static constexpr double ALFA_MAX = 1.0;

    int indice(const std::string& clave) const {
        return static_cast<int>(hashPolinomico(clave) % cubetas_.size());
    }

    // --- Ejercicio: el rehash ---------------------------------------------
    //
    // Pasar de cubetas() cubetas a nuevoM cubetas SIN perder ninguna clave.
    // Lo importante: al cambiar m cambia hash % m, asi que cada clave tiene
    // que ir a su NUEVA cubeta (el punto 1 del invariante). No vale copiar
    // el array viejo en las primeras posiciones del nuevo.
    //
    // No hace falta ni un new ni un delete de Nodo: los nodos se REENLAZAN
    // en las nuevas listas (solo cambia su campo siguiente).
    void rehash(int nuevoM) {
        // TODO: implementar.
        // Pasos:
        //   1. Crear std::vector<Nodo*> nuevas(nuevoM, nullptr).
        //   2. Recorrer cada lista de cubetas_; para cada nodo p:
        //        - guardar p->siguiente ANTES de tocar nada (si no, se
        //          pierde el resto de la lista vieja),
        //        - calcular su nueva cubeta j = hashPolinomico(p->clave) % nuevoM,
        //        - meterlo al principio de nuevas[j],
        //        - sumar 1 a recolocados_ (para la medida del main).
        //   3. cubetas_.swap(nuevas).
        (void)nuevoM;
    }
};

std::vector<std::string> generarPalabras(int n, unsigned semilla) {
    static const char* silabas[] = {"ca", "sa", "ma", "pe", "ro", "la", "ti", "no", "me", "su",
                                    "co", "ra", "de", "mi", "lo", "pa", "ta", "ne", "ve", "bu",
                                    "cha", "tra", "pla", "gri", "llo", "que", "fre", "sol", "mar", "luz"};
    const int nSilabas = sizeof(silabas) / sizeof(silabas[0]);
    std::mt19937 gen(semilla);
    std::uniform_int_distribution<int> cuantas(2, 4), cual(0, nSilabas - 1);
    std::vector<std::string> salida;
    std::set<std::string> vistas;
    while (static_cast<int>(salida.size()) < n) {
        std::string p;
        int k = cuantas(gen);
        for (int i = 0; i < k; i++) p += silabas[cual(gen)];
        if (k == 4) p += std::to_string(cual(gen));
        if (vistas.insert(p).second) salida.push_back(p);
    }
    return salida;
}

int main() {
    std::cout << std::boolalpha;
    const std::vector<std::string> diez = {"sol", "mar", "luz", "casa", "mesa",
                                           "pelo", "rio", "pan", "tren", "nube"};

    std::cout << "=== Una tabla pequenia, m = 7 FIJO (sin rehash) ===\n";
    {
        TablaHashEncadenada t(7, false);
        for (std::size_t i = 0; i < diez.size(); i++) t.insertar(diez[i], static_cast<int>(i));
        t.imprimir();
        std::cout << "  n=" << t.tamanio() << "  m=" << t.cubetas()
                  << "  alfa=" << std::fixed << std::setprecision(2) << t.factorCarga()
                  << "  cadena mas larga=" << t.cadenaMasLarga() << '\n';
        int v = -1;
        std::cout << "  buscar(pan)=" << t.buscar("pan", v) << " (valor " << v << ")"
                  << "  borrar(mar)=" << t.borrar("mar")
                  << "  buscar(mar)=" << t.buscar("mar", v) << '\n';
    }

    std::cout << "\n=== Las mismas 10 claves, CON rehash al pasar de alfa = 1 ===\n";
    {
        TablaHashEncadenada t(7, true);
        for (std::size_t i = 0; i < diez.size(); i++) {
            int antes = t.cubetas();
            t.insertar(diez[i], static_cast<int>(i));
            if (t.cubetas() != antes)
                std::cout << "  al insertar '" << diez[i] << "' (n=" << t.tamanio()
                          << " > " << antes << "): rehash de " << antes << " a "
                          << t.cubetas() << " cubetas\n";
        }
        t.imprimir();
        std::cout << "  n=" << t.tamanio() << "  m=" << t.cubetas()
                  << "  alfa=" << t.factorCarga()
                  << "  cadena mas larga=" << t.cadenaMasLarga() << '\n';
    }

    std::cout << "\n=== Sin rehash frente a con rehash, a gran escala ===\n";
    std::cout << "  (comparaciones = media por busqueda CON exito)\n\n";
    std::cout << "  " << std::left << std::setw(9) << "n"
              << "| " << std::setw(8) << "alfa" << std::setw(8) << "max" << std::setw(8) << "comp"
              << std::setw(10) << "ms"
              << "| " << std::setw(9) << "m" << std::setw(7) << "alfa" << std::setw(6) << "max"
              << std::setw(7) << "comp" << "ms\n";
    std::cout << "  " << std::setw(9) << "" << "| sin rehash (m = 1009 fijo)        "
              << "| con rehash (alfa <= 1)\n";
    auto todas = generarPalabras(400000, 3);
    for (int n : {1000, 10000, 100000, 400000}) {
        std::vector<std::string> palabras(todas.begin(), todas.begin() + n);
        TablaHashEncadenada* tablas[2] = {new TablaHashEncadenada(1009, false),
                                          new TablaHashEncadenada(1009, true)};
        std::cout << "  " << std::setw(9) << n;
        for (TablaHashEncadenada* t : tablas) {
            auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < n; i++) t->insertar(palabras[i], i);
            t->reiniciarComparaciones();
            int v = 0;
            for (const auto& p : palabras) t->buscar(p, v);
            auto t1 = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            double comp = static_cast<double>(t->comparaciones()) / n;
            std::cout << "| ";
            if (t == tablas[1]) std::cout << std::setw(9) << t->cubetas();
            std::cout << std::setprecision(2) << std::setw(t == tablas[0] ? 8 : 7) << t->factorCarga()
                      << std::setw(t == tablas[0] ? 8 : 6) << t->cadenaMasLarga()
                      << std::setw(t == tablas[0] ? 8 : 7) << comp
                      << std::setprecision(1) << std::setw(t == tablas[0] ? 10 : 0) << ms;
        }
        std::cout << '\n';
        delete tablas[0];
        delete tablas[1];
    }
    std::cout << "\n  Sin rehash, alfa crece con n y las comparaciones con el: con\n"
              << "  400000 claves, cada busqueda recorre de media ~200 nodos. Con\n"
              << "  rehash, alfa se queda entre 0,5 y 1 y las comparaciones en ~1,4,\n"
              << "  sea cual sea n: O(1) de verdad.\n";

    std::cout << "\n=== Cuanto cuesta el rehash, amortizado ===\n";
    {
        const int n = 400000;
        TablaHashEncadenada t(7, true);
        for (int i = 0; i < n; i++) t.insertar(todas[i], i);
        std::cout << "  " << n << " inserciones partiendo de m = 7: m final = " << t.cubetas() << '\n'
                  << "  nodos recolocados en TODOS los rehash juntos: " << t.nodosRecolocados() << '\n'
                  << "  recolocados por insercion: " << std::setprecision(2)
                  << static_cast<double>(t.nodosRecolocados()) / n << '\n';
    }
    std::cout << "  Menos de 2 por insercion, igual que las copias del array dinamico\n"
              << "  del tema 2: 1 + 1/2 + 1/4 + ... < 2. Cada rehash es O(n), pero\n"
              << "  amortizado sobre las inserciones que lo provocan, insertar sigue\n"
              << "  siendo O(1).\n";
}
