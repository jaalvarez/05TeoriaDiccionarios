// 06_trie.cpp
// Objetivo: un diccionario ESPECIALIZADO en claves que son cadenas. Ni
// compara claves enteras (como el arbol de busqueda) ni las convierte en un
// numero (como la tabla hash): las recorre LETRA A LETRA por un arbol.
//
// Un TRIE (de reTRIEval; tambien "arbol de prefijos") es un arbol n-ario
// -el arbol general del tema 4- en el que:
//   - cada arista lleva una LETRA,
//   - el camino desde la raiz hasta un nodo deletrea un PREFIJO,
//   - cada nodo tiene una marca finDePalabra: "aqui termina una clave".
//
//   insertar casa, caso, cama, pez:          (*) = finDePalabra
//
//        (raiz)
//        /    \
//       c      p
//       |      |
//       a      e
//      / \     |
//     s   m    z(*)
//    / \   \
//   a(*) o(*) a(*)
//
// LO QUE APORTA:
//   1. Buscar una clave de longitud L cuesta O(L): ni log n (arbol) ni el
//      calculo del hash + comparar la cadena entera (tabla). No depende de
//      cuantas claves haya.
//   2. Los prefijos comunes se guardan UNA vez ("ca" sirve a casa, caso y
//      cama).
//   3. Lo que la tabla hash NO puede hacer: "todas las claves que empiezan
//      por ..." (autocompletar). Se baja hasta el nodo del prefijo y se
//      recorre su subarbol.
//
// EL PRECIO: muchos nodos, y la pregunta del tema 4 sobre el arbol general:
// como guarda cada nodo a sus HIJOS? Este fichero prueba tres respuestas:
//   (A) un array de 26 punteros (uno por letra): bajar es O(1), pero cada
//       nodo paga 26 punteros aunque tenga un solo hijo;
//   (B) un std::map<char, Nodo*>: solo los hijos que existen, EN ORDEN
//       alfabetico; bajar es O(log 26);
//   (C) un std::unordered_map<char, Nodo*>: solo los que existen, bajar es
//       O(1) esperado, pero SIN orden.
// El fichero 07 mide cuanto cuesta cada una frente a std::map y
// std::unordered_map con las palabras enteras como clave.
//
// EJERCICIO: completar recoger(), el recorrido de autocompletar().
// Mientras no este hecho, autocompletar() devuelve siempre una lista vacia.
//
// Compilar:
//   g++ -std=c++17 -O2 -Wall -o 06_trie 06_trie.cpp && ./06_trie

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// =========================================================================
// LA ESPECIFICACION (un Conjunto de cadenas, con consultas por prefijo)
// =========================================================================
//  insertar(p)       pre: -  post: p esta en el conjunto
//  buscar(p)         pre: -  post: devuelve si p es una de las claves
//                                  (no basta con que sea un prefijo)
//  empiezaPor(pre)   pre: -  post: devuelve si alguna clave empieza por pre
//  autocompletar(pre) pre: - post: todas las claves que empiezan por pre

// -------------------------------------------------------------------------
// (A) Hijos en un array de 26 punteros. Solo letras 'a'..'z'.
// -------------------------------------------------------------------------
class TrieArray {
public:
    TrieArray() : raiz_(new Nodo) {}
    ~TrieArray() { liberar(raiz_); }
    TrieArray(const TrieArray&) = delete;
    TrieArray& operator=(const TrieArray&) = delete;

    // pre: palabra solo contiene letras de la 'a' a la 'z'
    void insertar(const std::string& palabra) {
        Nodo* n = raiz_;
        for (char c : palabra) {
            int i = c - 'a';
            if (n->hijos[i] == nullptr) { n->hijos[i] = new Nodo; nodos_++; }
            n = n->hijos[i];
        }
        n->finDePalabra = true;
    }

    bool buscar(const std::string& palabra) const {
        const Nodo* n = bajar(palabra);
        return n != nullptr && n->finDePalabra;
    }

    bool empiezaPor(const std::string& prefijo) const { return bajar(prefijo) != nullptr; }

    int nodos() const { return nodos_; }
    static std::size_t tamanioNodo() { return sizeof(Nodo); }

    // El trie dibujado en PREORDEN, una letra por linea con sangria.
    void imprimir() const { imprimirEn(raiz_, 0); }

private:
    struct Nodo {
        Nodo* hijos[26] = {};          // hijos[c - 'a'], nullptr si no hay
        bool finDePalabra = false;
    };

    // --- Invariante de la representacion ---------------------------------
    //  1. La raiz no representa ninguna letra (es el prefijo vacio).
    //  2. Una cadena s esta en el conjunto si y solo si el camino desde la
    //     raiz siguiendo sus letras existe y acaba en un nodo con
    //     finDePalabra == true.
    //  3. nodos_ = numero de nodos, raiz incluida.
    // -----------------------------------------------------------------------
    Nodo* raiz_;
    int nodos_ = 1;

    // El nodo al que se llega siguiendo las letras de s, o nullptr.
    const Nodo* bajar(const std::string& s) const {
        const Nodo* n = raiz_;
        for (char c : s) {
            if (c < 'a' || c > 'z') return nullptr;
            n = n->hijos[c - 'a'];
            if (n == nullptr) return nullptr;
        }
        return n;
    }

    static void imprimirEn(const Nodo* n, int profundidad) {
        for (int i = 0; i < 26; i++) {
            if (n->hijos[i] == nullptr) continue;
            std::cout << "    " << std::string(2 * profundidad, ' ') << static_cast<char>('a' + i)
                      << (n->hijos[i]->finDePalabra ? "  *" : "") << '\n';
            imprimirEn(n->hijos[i], profundidad + 1);
        }
    }

    static void liberar(Nodo* n) {                // postorden, como en el tema 4
        for (Nodo* h : n->hijos)
            if (h != nullptr) liberar(h);
        delete n;
    }
};

// -------------------------------------------------------------------------
// (B) y (C): los hijos en un DICCIONARIO de letra a nodo.
// La misma clase sirve para los dos: el tipo de diccionario es un
// parametro de plantilla.
//
//    Trie<std::map>             hijos en un arbol rojo-negro (con orden)
//    Trie<std::unordered_map>   hijos en una tabla hash (sin orden)
//
// "template <class...> class Mapa" quiere decir: Mapa no es un tipo, es una
// PLANTILLA a la que todavia le faltan sus parametros. Dentro de la clase
// se completa como Mapa<char, Nodo*>.
// -------------------------------------------------------------------------
template <template <class...> class Mapa>
class Trie {
public:
    Trie() : raiz_(new Nodo) {}
    ~Trie() { liberar(raiz_); }
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insertar(const std::string& palabra) {
        Nodo* n = raiz_;
        for (char c : palabra) {
            Nodo*& hijo = n->hijos[c];          // si no estaba, se crea a nullptr
            if (hijo == nullptr) { hijo = new Nodo; nodos_++; }
            n = hijo;
        }
        n->finDePalabra = true;
    }

    bool buscar(const std::string& palabra) const {
        const Nodo* n = bajar(palabra);
        return n != nullptr && n->finDePalabra;
    }

    bool empiezaPor(const std::string& prefijo) const { return bajar(prefijo) != nullptr; }

    // Todas las claves que empiezan por 'prefijo'.
    std::vector<std::string> autocompletar(const std::string& prefijo) const {
        std::vector<std::string> salida;
        const Nodo* n = bajar(prefijo);
        if (n == nullptr) return salida;        // ninguna empieza asi
        std::string palabra = prefijo;
        recoger(n, palabra, salida);
        return salida;
    }

    int nodos() const { return nodos_; }
    static std::size_t tamanioNodo() { return sizeof(Nodo); }

private:
    struct Nodo {
        Mapa<char, Nodo*> hijos;                // solo las letras que existen
        bool finDePalabra = false;
    };

    // --- Invariante de la representacion: el mismo que TrieArray ---------
    Nodo* raiz_;
    int nodos_ = 1;

    const Nodo* bajar(const std::string& s) const {
        const Nodo* n = raiz_;
        for (char c : s) {
            auto it = n->hijos.find(c);         // find, NO operator[]: no hay que crear nada
            if (it == n->hijos.end()) return nullptr;
            n = it->second;
        }
        return n;
    }

    // --- Ejercicio: el recorrido de autocompletar ---------------------------
    //
    // Recorrer en PREORDEN el subarbol de n. 'palabra' lleva las letras del
    // camino desde la raiz hasta n: al bajar a un hijo se le aniade su
    // letra, y al volver se le quita (como una pila). Cada nodo con
    // finDePalabra aporta 'palabra' a la salida.
    static void recoger(const Nodo* n, std::string& palabra, std::vector<std::string>& salida) {
        // TODO: implementar.
        // Pistas:
        //   - Visitar: si n->finDePalabra, 'palabra' es una clave completa.
        //   - Los hijos se recorren con un for sobre el diccionario:
        //         for (const auto& par : n->hijos)   // par.first: la letra
        //                                            // par.second: el Nodo*
        //   - Antes de bajar, palabra.push_back(letra); al volver,
        //     palabra.pop_back(). Mira imprimirEn() de TrieArray: es el
        //     mismo recorrido, imprimiendo en vez de recoger.
        (void)n; (void)palabra; (void)salida;
    }

    static void liberar(Nodo* n) {
        for (auto& par : n->hijos) liberar(par.second);
        delete n;
    }
};

void imprimir(const char* etiqueta, const std::vector<std::string>& v) {
    std::cout << "  " << etiqueta;
    for (const auto& s : v) std::cout << s << ' ';
    std::cout << '\n';
}

int main() {
    std::cout << std::boolalpha;
    const std::vector<std::string> palabras = {"casa", "caso", "casco", "cama", "camion",
                                               "cal", "perro", "pera", "pez", "pe"};
    int letras = 0;
    for (const auto& p : palabras) letras += static_cast<int>(p.size());

    std::cout << "=== (A) Trie con un array de 26 hijos por nodo ===\n";
    TrieArray a;
    for (const auto& p : palabras) a.insertar(p);
    a.imprimir();
    std::cout << "  (* = finDePalabra)\n";
    std::cout << "  buscar(casa)=" << a.buscar("casa") << "  buscar(cas)=" << a.buscar("cas")
              << "  empiezaPor(cas)=" << a.empiezaPor("cas") << "  buscar(pe)=" << a.buscar("pe")
              << "  buscar(gato)=" << a.buscar("gato") << '\n';
    std::cout << "  " << palabras.size() << " palabras, " << letras << " letras, pero solo "
              << a.nodos() << " nodos (raiz incluida):\n"
              << "  los prefijos comunes (ca, cas, cam, pe) se guardan una sola vez.\n";

    std::cout << "\n=== (B) y (C): los hijos en un map o en un unordered_map ===\n";
    Trie<std::map> b;
    Trie<std::unordered_map> c;
    for (const auto& p : palabras) { b.insertar(p); c.insertar(p); }
    std::cout << "  mismos nodos: " << b.nodos() << " y " << c.nodos()
              << "   buscar(cas): " << b.buscar("cas") << ' ' << c.buscar("cas")
              << "   empiezaPor(cas): " << b.empiezaPor("cas") << ' ' << c.empiezaPor("cas") << '\n';

    std::cout << "\n=== autocompletar: lo que una tabla hash no sabe hacer ===\n";
    imprimir("Trie<map>,           \"ca\": ", b.autocompletar("ca"));
    imprimir("Trie<unordered_map>, \"ca\": ", c.autocompletar("ca"));
    imprimir("Trie<map>,           \"pe\": ", b.autocompletar("pe"));
    imprimir("Trie<map>,           \"x\":  ", b.autocompletar("x"));
    std::cout << "  Con map los hijos se recorren en orden alfabetico, y el preorden\n"
              << "  devuelve las palabras ORDENADAS sin ordenar nada. Con unordered_map\n"
              << "  salen las mismas, en el orden que dicten las cubetas de cada nodo.\n";

    std::cout << "\n=== Lo que ocupa UN nodo (sin contar sus hijos) ===\n";
    std::cout << "  TrieArray:              " << TrieArray::tamanioNodo() << " bytes (26 punteros + bool)\n"
              << "  Trie<map>:              " << Trie<std::map>::tamanioNodo()
              << " bytes + un nodo de arbol POR CADA hijo\n"
              << "  Trie<unordered_map>:    " << Trie<std::unordered_map>::tamanioNodo()
              << " bytes + un array de cubetas + un nodo POR CADA hijo\n"
              << "  El array paga 26 punteros en TODOS los nodos, aunque la mayoria\n"
              << "  tienen uno o ningun hijo. Cuanto pesa cada opcion de verdad, con\n"
              << "  miles de palabras, se mide en el fichero 07.\n";
}
