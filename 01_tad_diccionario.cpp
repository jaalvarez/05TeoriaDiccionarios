// 01_tad_diccionario.cpp
// Objetivo: separar el QUE (el TAD Diccionario) del COMO (la estructura que
// lo implementa), igual que en el tema 2 con la Lista y en el tema 4 con el
// Arbol. Un Diccionario guarda pares (clave, valor) y se usa POR CLAVE:
// "dame el valor asociado a esta clave". Nadie navega por el: no hay
// posiciones, ni siguiente, ni hijos.
//
// Ya tenemos tres formas de implementarlo con lo visto hasta ahora:
//
//   (A) Un vector de pares SIN ordenar     -> buscar O(n), insertar O(n)
//                                            (hay que mirar si ya estaba)
//   (B) Un vector de pares ORDENADO        -> buscar O(log n) (busqueda
//                                            binaria), insertar O(n)
//                                            (desplazar para hacer hueco)
//   (C) Un arbol de busqueda equilibrado   -> todo O(log n). Es std::map,
//       (el rojo-negro del tema 4)            el arbol rojo-negro del tema 4.
//
// La pregunta de este tema: se puede bajar de O(log n)? La segunda parte
// del fichero muestra que SI, con el DIRECCIONAMIENTO DIRECTO (un array
// indexado por la propia clave): O(1) exacto... pero solo si las claves
// son enteros pequenios. Lo que queda sin resolver -y motiva todo el resto
// del tema- es que pasa cuando el universo de claves es enorme (un DNI, una
// cadena de texto): ahi hace falta una FUNCION HASH (fichero 02).
//
// Compilar:
//   g++ -std=c++17 -O2 -Wall -o 01_dicc 01_tad_diccionario.cpp && ./01_dicc

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>

// =========================================================================
// EL CONTRATO: el TAD Diccionario (clave string, valor int)
// =========================================================================
//
//  insertar(c, v)  pre: -   post: c tiene asociado el valor v. Si c ya
//                               estaba, su valor anterior se SUSTITUYE
//                               (no hay claves repetidas)
//  buscar(c, v)    pre: -   post: devuelve si c esta; si esta, deja en v
//                               su valor asociado
//  borrar(c)       pre: -   post: si c estaba, la quita y devuelve true
//  tamanio()       pre: -   post: numero de claves distintas guardadas
//
// Nada de esto habla de vectores, arboles ni arrays: es el mismo contrato
// para las tres implementaciones de abajo.

class Diccionario {
public:
    virtual ~Diccionario() = default;
    virtual void insertar(const std::string& clave, int valor) = 0;
    virtual bool buscar(const std::string& clave, int& valor) const = 0;
    virtual bool borrar(const std::string& clave) = 0;
    virtual int tamanio() const = 0;
};

// -------------------------------------------------------------------------
// (A) Vector de pares SIN ordenar
// -------------------------------------------------------------------------
class DiccionarioLista : public Diccionario {
public:
    void insertar(const std::string& clave, int valor) override {
        for (auto& par : datos_) {
            if (par.first == clave) { par.second = valor; return; }   // ya estaba y sobreescribe
        }
        datos_.push_back({clave, valor});    // O(1) amortizado... tras O(n) del recorrido
    }

    bool buscar(const std::string& clave, int& valor) const override {
        for (const auto& par : datos_) {
            if (par.first == clave) { valor = par.second; return true; }
        }
        return false;
    }

    bool borrar(const std::string& clave) override {
        for (std::size_t i = 0; i < datos_.size(); i++) {
            if (datos_[i].first == clave) {
                datos_[i] = datos_.back();   // el orden da igual: tapar el hueco
                datos_.pop_back();           // con el ultimo, O(1)
                return true;
            }
        }
        return false;
    }

    int tamanio() const override { return static_cast<int>(datos_.size()); }

private:
    // --- Invariante de la representacion ---------------------------------
    //  Cada clave aparece como mucho UNA vez en datos_. Sin ningun orden.
    // -----------------------------------------------------------------------
    // el vector de pares (clave, valor) se puede acceder a first (clave) y second (valor)
    std::vector<std::pair<std::string, int>> datos_; 
};

// -------------------------------------------------------------------------
// (B) Vector de pares ORDENADO por clave
// -------------------------------------------------------------------------
class DiccionarioOrdenado : public Diccionario {
public:
    void insertar(const std::string& clave, int valor) override {
        auto it = posicion(clave); // definimos un iterador (lo hemos llamado posición)
        if (it != datos_.end() && it->first == clave) { it->second = valor; return; }
        datos_.insert(it, {clave, valor});   // O(n): desplaza todo lo que va detras
    }

    bool buscar(const std::string& clave, int& valor) const override {
        auto it = std::lower_bound(datos_.begin(), datos_.end(), clave, menorClave);
        if (it == datos_.end() || it->first != clave) return false; // casos en los que no se ha encontrado
        valor = it->second; // si se ha encontrado, se devuelve el valor asociado
        return true;
    }

    bool borrar(const std::string& clave) override {
        auto it = posicion(clave); // auto se pone porque es un iterador, no un int
        if (it == datos_.end() || it->first != clave) return false;
        datos_.erase(it);                    // O(n): cerrar el hueco
        return true;
    }

    int tamanio() const override { return static_cast<int>(datos_.size()); }

private:
    // --- Invariante de la representacion ---------------------------------
    //  datos_ esta ORDENADO por clave, de menor a mayor, sin repetidas.
    //  Es lo que permite la busqueda binaria (lower_bound): O(log n).
    // -----------------------------------------------------------------------
    std::vector<std::pair<std::string, int>> datos_;

    static bool menorClave(const std::pair<std::string, int>& par, const std::string& clave) {
        //Aqui definimos un comparador para que lower_bound sepa como comparar un par con una clave
        return par.first < clave;
    }

    std::vector<std::pair<std::string, int>>::iterator posicion(const std::string& clave) {
        //lower_bound es una plantilla que busca el primer elemento en un rango ordenado con un comparador. 
        //Devuelve un iterador al primer elemento que no es menor que la clave, o datos_.end() si todos son menores.
        return std::lower_bound(datos_.begin(), datos_.end(), clave, menorClave); 
    }
};

// -------------------------------------------------------------------------
// (C) Arbol de busqueda equilibrado: std::map (rojo-negro, tema 4)
// -------------------------------------------------------------------------
class DiccionarioArbol : public Diccionario {
public:
    void insertar(const std::string& clave, int valor) override { datos_[clave] = valor; }

    bool buscar(const std::string& clave, int& valor) const override {
        auto it = datos_.find(clave);
        if (it == datos_.end()) return false;
        valor = it->second;
        return true;
    }

    bool borrar(const std::string& clave) override { return datos_.erase(clave) > 0; }

    int tamanio() const override { return static_cast<int>(datos_.size()); }

private:
    std::map<std::string, int> datos_;       // el invariante lo mantiene std::map
};

// =========================================================================
// Datos de prueba: palabras inventadas, distintas, a partir de silabas
// =========================================================================
std::vector<std::string> generarPalabras(int n, unsigned semilla) {
    static const char* silabas[] = {"ca", "sa", "ma", "pe", "ro", "la", "ti", "no", "me", "su",
                                    "co", "ra", "de", "mi", "lo", "pa", "ta", "ne", "ve", "bu",
                                    "cha", "tra", "pla", "gri", "llo", "que", "fre", "sol", "mar", "luz"};
    const int nSilabas = sizeof(silabas) / sizeof(silabas[0]);
    std::mt19937 gen(semilla); //mt19937 es un generador de numeros aleatorios de 32 bits, con periodo 2^19937-1
    std::uniform_int_distribution<int> cuantas(2, 4), cual(0, nSilabas - 1); //cuantas genera un numero aleatorio entre 2 y 4, cual genera un numero aleatorio entre 0 y nSilabas-1
    std::vector<std::string> salida;
    std::map<std::string, bool> vistas;      // para no repetir
    while (static_cast<int>(salida.size()) < n) {
        std::string p;
        int k = cuantas(gen);
        for (int i = 0; i < k; i++) p += silabas[cual(gen)]; // concatenar silabas para formar palabras en p
        if (k == 4) p += std::to_string(cual(gen));   // para que haya de sobra
        if (!vistas[p]) { vistas[p] = true; salida.push_back(p); }
    }
    return salida;
}

double medirMs(Diccionario& d, const std::vector<std::string>& palabras) {
    auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < palabras.size(); i++) d.insertar(palabras[i], static_cast<int>(i));
    int v = 0, encontradas = 0;
    for (const auto& p : palabras) encontradas += d.buscar(p, v);
    auto t1 = std::chrono::steady_clock::now();
    if (encontradas != static_cast<int>(palabras.size())) std::cout << "  (!) faltan claves\n";
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== El mismo contrato, tres implementaciones ===\n";
    DiccionarioLista a;
    DiccionarioOrdenado b;
    DiccionarioArbol c;
    Diccionario* todos[] = {&a, &b, &c};
    const char* nombres[] = {"vector sin ordenar", "vector ordenado   ", "std::map (arbol)  "};
    for (int k = 0; k < 3; k++) {
        Diccionario& d = *todos[k];
        d.insertar("ana", 7);
        d.insertar("luis", 5);
        d.insertar("eva", 9);
        d.insertar("ana", 10);              // ya estaba: SUSTITUYE, no duplica
        d.borrar("luis");
        int v = -1;
        bool esta = d.buscar("ana", v);
        std::cout << "  " << nombres[k] << "  tamanio=" << d.tamanio()
                  << "  buscar(ana)=" << esta << " valor=" << v
                  << "  buscar(luis)=" << d.buscar("luis", v) << '\n';
    }

    std::cout << "\n=== Cuanto cuesta: insertar n claves y buscarlas todas ===\n";
    std::cout << "  " << std::left << std::setw(9) << "n"
              << std::setw(22) << "sin ordenar (ms)"
              << std::setw(22) << "ordenado (ms)"
              << "std::map (ms)\n";
    for (int n : {1000, 4000, 16000}) {
        auto palabras = generarPalabras(n, 42);
        DiccionarioLista la; DiccionarioOrdenado lo; DiccionarioArbol ar;
        std::cout << "  " << std::left << std::setw(9) << n << std::fixed << std::setprecision(2)
                  << std::setw(22) << medirMs(la, palabras)
                  << std::setw(22) << medirMs(lo, palabras)
                  << medirMs(ar, palabras) << '\n';
    }
    std::cout << "  Al multiplicar n por 4: la primera columna se multiplica por 16 o\n"
              << "  mas (n operaciones de O(n): cuadratico), la segunda algo menos\n"
              << "  (buscar es log n, pero insertar desplaza), la tercera por poco\n"
              << "  mas de 4 (n operaciones de O(log n)).\n";

    std::cout << "\n=== Bajar de O(log n): el direccionamiento directo ===\n";
    {
        // Las claves son enteros PEQUENIOS: el numero de lista de 0 a 99 de
        // un grupo de clase. El array se indexa con la propia clave:
        // ni comparaciones ni recorrido. O(1) exacto.
        std::vector<int> notaDe(100, -1);   // -1 = "no hay nadie con esa clave"
        notaDe[17] = 8;
        notaDe[42] = 6;
        notaDe[3]  = 10;
        std::cout << "  notaDe[42] = " << notaDe[42] << "   notaDe[50] = " << notaDe[50]
                  << "  (-1: no esta)\n";
        std::cout << "  buscar, insertar y borrar: UNA operacion de array, O(1)\n";
    }
    {
        // El precio: el array mide lo que mide el UNIVERSO de claves, no lo
        // que mide el conjunto que realmente se guarda.
        const long long universoDNI = 100000000LL;   // 8 digitos
        const int alumnos = 300;
        std::cout << "\n  Y si la clave es un DNI (8 digitos) y hay " << alumnos << " alumnos?\n";
        std::cout << "    casillas necesarias: " << universoDNI << " (el universo entero)\n";
        std::cout << "    memoria (4 bytes por casilla): " << universoDNI * 4 / (1024 * 1024) << " MB\n";
        std::cout << "    casillas USADAS: " << alumnos << "  ->  ocupacion del "
                  << std::setprecision(4) << 100.0 * alumnos / universoDNI << " %\n";
        std::cout << "    Y si la clave es una cadena de 10 letras: 26^10 = 1.4e14 casillas.\n"
                  << "    Imposible.\n";
    }
    std::cout << "\n  La idea que falta: una FUNCION que lleve cada clave del universo\n"
              << "  enorme a una casilla de un array PEQUENIO (del tamanio de lo que\n"
              << "  de verdad se guarda). Eso es una funcion hash: fichero 02.\n";
}
