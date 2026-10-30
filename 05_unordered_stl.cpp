// 05_unordered_stl.cpp
// Objetivo: ver como la biblioteca estandar implementa de verdad la tabla
// hash que hemos construido a mano en los ficheros 03 y 04. Igual que en el
// tema 4 std::set/std::map eran un arbol rojo-negro convertido en tipo,
// aqui std::unordered_set/std::unordered_map son una TABLA HASH CON
// ENCADENAMIENTO (la del fichero 03) convertida en tipo.
//
// Lo que se ve en este fichero:
//   1. El uso basico, y la trampa de operator[] (inserta si no esta).
//   2. La interfaz de cubetas: bucket_count(), load_factor(),
//      max_load_factor(). Se ve el rehash del fichero 03 ocurriendo solo.
//   3. reserve(n): pedir las cubetas de antemano evita todos los rehash.
//   4. El orden de recorrido no significa nada, y cambia tras un rehash.
//   5. Una clave propia (un struct) necesita DOS cosas: una funcion hash y
//      un operator==. Y que pasa si la funcion hash es mala.
//
// Compilar:
//   g++ -std=c++17 -O2 -Wall -o 05_stl 05_unordered_stl.cpp && ./05_stl

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <chrono>
#include <functional>

struct Punto {
    int x, y;
    bool operator==(const Punto& otro) const { return x == otro.x && y == otro.y; }
};

// Una buena funcion hash para Punto: combinar las dos coordenadas de forma
// que (1,2) y (2,1) NO choquen (la suma x+y los juntaria, como los
// anagramas del fichero 02). Mismo truco que el hash polinomico: multiplicar
// lo acumulado antes de sumar el siguiente campo.
struct HashPunto {
    std::size_t operator()(const Punto& p) const {
        std::size_t h = std::hash<int>{}(p.x); // std::hash<int> es la identidad para enteros eso significa que el hash es el propio valor
        return h * 1000003u + std::hash<int>{}(p.y);
    }
};

// Una funcion hash PESIMA, pero valida: es determinista y claves iguales
// dan el mismo hash. Todo cae en la misma cubeta.
struct HashPesimo {
    std::size_t operator()(const Punto&) const { return 42; }
};

template <class Tabla>
double insertarPuntos(Tabla& t, int lado) {
    auto t0 = std::chrono::steady_clock::now();
    for (int x = 0; x < lado; x++)
        for (int y = 0; y < lado; y++) t.insert(Punto{x, y});
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 1) Uso basico, y la trampa de operator[] ===\n";
    {
        std::unordered_map<std::string, int> edad;
        edad["ana"] = 20;
        edad["luis"] = 19;
        edad.insert({"eva", 22});
        std::cout << "  tamanio: " << edad.size() << '\n';

        // find: devuelve end() si no esta. NO modifica la tabla.
        auto it = edad.find("eva");
        if (it != edad.end()) std::cout << "  find(eva): " << it->second << '\n';
        std::cout << "  count(pepe): " << edad.count("pepe") << "  (0 o 1: no hay repetidas)\n";

        // operator[] con una clave que NO esta la INSERTA con valor 0.
        int e = edad["pepe"];
        std::cout << "  edad[\"pepe\"] = " << e << "  ... y ahora tamanio: " << edad.size()
                  << "   <-- !se ha colado pepe!\n";
        std::cout << "  Para consultar sin modificar: find() o count(). at() lanza una\n"
                  << "  excepcion si no esta, en vez de insertar.\n";
    }

    std::cout << "\n=== 2) Las cubetas, a la vista: el rehash del fichero 03 ===\n";
    {
        std::unordered_map<int, int> t;
        std::cout << "  max_load_factor() = " << t.max_load_factor()
                  << "  (el ALFA_MAX del fichero 03)\n\n";
        std::cout << "  " << std::left << std::setw(16) << "al insertar n="
                  << std::setw(18) << "bucket_count()" << "load_factor()\n";
        std::size_t cubetas = t.bucket_count();
        for (int i = 0; i < 2000; i++) {
            t[i] = i;
            if (t.bucket_count() != cubetas) {
                cubetas = t.bucket_count();
                std::cout << "  " << std::setw(16) << t.size() << std::setw(18) << cubetas
                          << std::fixed << std::setprecision(3) << t.load_factor() << '\n';
            }
        }
        std::cout << "  Cada linea es un rehash: el numero de cubetas mas o menos se\n"
                  << "  duplica (redondeado a un primo) en cuanto load_factor pasaria\n"
                  << "  de 1. Exactamente lo del fichero 03.\n";
        std::cout << "\n  Y ojo: std::hash<int>(12345) = " << std::hash<int>{}(12345)
                  << ". Para enteros, el hash de la STL\n"
                  << "  es la IDENTIDAD: todo el reparto lo hace el 'mod m' con m primo\n"
                  << "  (la parte 1 del fichero 02).\n";
    }

    std::cout << "\n=== 3) reserve(n): pedir las cubetas de antemano ===\n";
    {
        const int n = 2000000;
        for (bool reservar : {false, true}) {
            std::unordered_map<int, int> t;
            if (reservar) t.reserve(n);
            int rehashes = 0;
            std::size_t cubetas = t.bucket_count();
            auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < n; i++) {
                t[i * 7] = i;
                if (t.bucket_count() != cubetas) { rehashes++; cubetas = t.bucket_count(); }
            }
            auto t1 = std::chrono::steady_clock::now();
            std::cout << "  " << (reservar ? "con reserve(n): " : "sin reserve:    ")
                      << std::setw(4) << rehashes << " rehash, "
                      << std::setprecision(1) << std::chrono::duration<double, std::milli>(t1 - t0).count()
                      << " ms\n";
        }
        std::cout << "  Si se sabe cuantas claves van a entrar, reserve() se ahorra todos\n"
                  << "  los rehash. Como reserve() en el vector del tema 2.\n";
    }

    std::cout << "\n=== 4) El orden de recorrido no significa nada ===\n";
    {
        std::unordered_set<std::string> s = {"uno", "dos", "tres", "cuatro", "cinco", "seis"};
        std::cout << "  unordered_set: ";
        for (const auto& x : s) std::cout << x << ' ';
        for (int i = 0; i < 100; i++) s.insert("relleno" + std::to_string(i));   // provoca rehash
        std::cout << "\n  tras un rehash: ";
        for (const auto& x : s)
            if (x.rfind("relleno", 0) != 0) std::cout << x << ' ';
        std::map<std::string, int> m = {{"uno", 1}, {"dos", 2}, {"tres", 3},
                                        {"cuatro", 4}, {"cinco", 5}, {"seis", 6}};
        std::cout << "\n  map (arbol):    ";
        for (const auto& par : m) std::cout << par.first << ' ';
        std::cout << "\n  El orden de un unordered_* depende de los hash y del numero de\n"
                  << "  cubetas: cambia cuando la tabla crece. Si se necesita orden,\n"
                  << "  map/set (o copiar y ordenar).\n";
    }

    std::cout << "\n=== 5) Una clave propia: hash + operator== ===\n";
    {
        // std::unordered_set<Punto> s;   // NO compila: no hay std::hash<Punto>
        std::unordered_set<Punto, HashPunto> bueno;
        std::unordered_set<Punto, HashPesimo> pesimo;
        std::cout << "  " << std::left << std::setw(10) << "puntos"
                  << std::setw(20) << "HashPunto (ms)" << "HashPesimo (ms)\n";
        for (int lado : {50, 100, 140}) {
            std::unordered_set<Punto, HashPunto> b;
            std::unordered_set<Punto, HashPesimo> p;
            double tb = insertarPuntos(b, lado);
            double tp = insertarPuntos(p, lado);
            std::cout << "  " << std::setw(10) << lado * lado << std::setprecision(2)
                      << std::setw(20) << tb << tp << '\n';
            if (lado == 140) {
                bueno = std::move(b);
                pesimo = std::move(p);
            }
        }
        std::cout << "  cubeta mas llena: HashPunto -> ";
        std::size_t maxB = 0, maxP = 0;
        for (std::size_t i = 0; i < bueno.bucket_count(); i++) maxB = std::max(maxB, bueno.bucket_size(i));
        for (std::size_t i = 0; i < pesimo.bucket_count(); i++) maxP = std::max(maxP, pesimo.bucket_size(i));
        std::cout << maxB << " claves;  HashPesimo -> " << maxP << " claves\n";
        std::cout << "  Con el hash pesimo todo cae en UNA cubeta: la tabla es una lista\n"
                  << "  enlazada, cada insercion la recorre entera y el total es O(n^2).\n"
                  << "  El O(1) de una tabla hash es un O(1) ESPERADO: depende de que la\n"
                  << "  funcion hash reparta. El peor caso es O(n) por operacion.\n";
    }
}
