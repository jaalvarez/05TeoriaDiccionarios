// 02_funciones_hash.cpp
// Objetivo: la pieza que faltaba al final del fichero 01. Una FUNCION HASH
// lleva cada clave de un universo enorme (todos los enteros, todas las
// cadenas) a un indice de un array PEQUENIO de m casillas ("cubetas"):
//
//        h : claves  ->  {0, 1, ..., m-1}
//
// Con eso, el direccionamiento directo del fichero 01 vuelve a funcionar:
// la casilla de la clave c es h(c), y se llega a ella en O(1).
//
// LO QUE SE PIDE A UNA BUENA FUNCION HASH:
//   1. Determinista: la misma clave da SIEMPRE la misma casilla (si no,
//      no se podria volver a encontrar lo guardado).
//   2. Rapida: O(longitud de la clave), nada mas.
//   3. Que REPARTA: claves distintas deben caer en casillas distintas con
//      la mayor frecuencia posible, aunque las claves se parezcan entre si
//      ("casa", "cosa", "caso") o sigan un patron (multiplos de 10).
//
// LO QUE NO SE LE PUEDE PEDIR: que no haya nunca dos claves en la misma
// casilla (una COLISION). Hay mas claves posibles que casillas, asi que
// por el principio del palomar las colisiones son inevitables. La parte 3
// mide que llegan MUCHO antes de lo que la intuicion dice (la paradoja del
// cumpleanios). Que hacer con ellas es el tema de los ficheros 03 y 04.
//
// EJERCICIO: completar hashPolinomico() (parte 2) con la regla de Horner.
// Mientras no este hecho devuelve siempre 0: todas las claves caen en la
// cubeta 0 y la fila 'polinomico' de la tabla sale la peor de todas.
//
// Compilar:
//   g++ -std=c++17 -O2 -Wall -o 02_hash 02_funciones_hash.cpp && ./02_hash

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <cmath>
#include <algorithm>

// =========================================================================
// PARTE 1: claves enteras. h(k) = k mod m, y por que importa elegir m
// =========================================================================
int hashModulo(long long k, int m) { return static_cast<int>(k % m); }

const double PI = 3.14159265358979;   // M_PI no es estandar (falta en MinGW)

// =========================================================================
// PARTE 2: claves cadena. Tres candidatas, de peor a mejor
// =========================================================================

// (a) La longitud: determinista y rapida... y casi todo choca.
unsigned long long hashLongitud(const std::string& s) { return s.size(); }

// (b) La suma de los codigos de los caracteres. Mejor, pero con dos
//     defectos: (1) ignora el ORDEN, asi que todos los anagramas chocan
//     ("amor", "roma", "mora"...), y (2) su rango es pequenio: una palabra
//     de 10 letras suma como mucho 10 * 122 = 1220, asi que en una tabla
//     de 10000 cubetas solo se usan las primeras ~1200.
unsigned long long hashSuma(const std::string& s) {
    unsigned long long h = 0;
    for (char c : s) h += static_cast<unsigned char>(c);
    return h;
}

// (c) EL EJERCICIO: el hash POLINOMICO. Cada caracter pesa distinto segun
//     su POSICION, como los digitos de un numero en base B:
//
//         h("abc") = 'a'*B^2 + 'b'*B^1 + 'c'*B^0
//
//     Calcular las potencias una a una seria O(L^2). La regla de HORNER lo
//     hace en una sola pasada, multiplicando lo acumulado por B antes de
//     sumar el siguiente caracter:
//
//         h = 0
//         para cada caracter c:  h = h * B + c
//
//     ("abc": 0 -> 'a' -> 'a'*B + 'b' -> ('a'*B + 'b')*B + 'c'). Se usa
//     B = 31 (primo e impar; es la base de java.lang.String.hashCode). El
//     resultado DESBORDA enseguida: con unsigned eso no es un error, es
//     aritmetica modulo 2^64, y el hash sigue siendo determinista.
unsigned long long hashPolinomico(const std::string& s) {
    // TODO: implementar con la regla de Horner y base B = 31.
    // Pista: igual que hashSuma(), pero multiplicando lo acumulado por B
    // ANTES de sumar cada caracter. Usa static_cast<unsigned char>(c) para
    // que los caracteres con acento (negativos en char) no resten.
    (void)s;
    return 0;
}

// =========================================================================
// Herramientas de medida
// =========================================================================
struct Reparto {
    int usadas;             // cubetas con al menos una clave
    int maxima;             // claves en la cubeta mas llena
    double comparaciones;   // media, buscando una clave que SI esta
};

// Si una cubeta tiene c claves, encontrar la 1a cuesta 1 comparacion, la
// 2a cuesta 2... en total c(c+1)/2. Con un reparto perfecto, la media es
// 1 + alfa/2, donde alfa = n/m (las claves por cubeta: el FACTOR DE CARGA).
Reparto medir(const std::vector<int>& cuantas, int n) {
    Reparto r{0, 0, 0.0};
    for (int c : cuantas) {
        if (c > 0) r.usadas++;
        r.maxima = std::max(r.maxima, c);
        r.comparaciones += c * (c + 1.0) / 2.0;
    }
    r.comparaciones /= n;
    return r;
}

template <class FuncionHash>
Reparto repartir(const std::vector<std::string>& claves, int m, FuncionHash h) {
    std::vector<int> cuantas(m, 0);
    for (const auto& c : claves) cuantas[h(c) % m]++;
    return medir(cuantas, static_cast<int>(claves.size()));
}

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

void filaReparto(const char* nombre, const Reparto& r, int m) {
    std::cout << "  " << std::left << std::setw(16) << nombre
              << std::setw(10) << r.usadas
              << std::setw(9) << std::setprecision(1) << std::fixed
              << 100.0 * r.usadas / m
              << std::setw(12) << r.maxima
              << std::setprecision(2) << r.comparaciones << '\n';
}

int main() {
    std::cout << "=== 1) Enteros: h(k) = k mod m, y por que m debe ser primo ===\n";
    std::cout << "  1000 claves que son todas MULTIPLOS DE 10 (0, 10, 20, ...)\n\n";
    std::cout << "  " << std::left << std::setw(10) << "m"
              << std::setw(18) << "cubetas usadas" << "cubeta mas llena\n";
    for (int m : {100, 101, 128, 127}) {
        std::vector<int> cuantas(m, 0);
        for (int i = 0; i < 1000; i++) cuantas[hashModulo(10LL * i, m)]++;
        Reparto r = medir(cuantas, 1000);
        std::cout << "  " << std::left << std::setw(10) << m
                  << std::setw(18) << (std::to_string(r.usadas) + " de " + std::to_string(m))
                  << r.maxima << '\n';
    }
    std::cout << "\n  Con m = 100, k mod 100 de un multiplo de 10 es otro multiplo de\n"
              << "  10: solo se usan 10 cubetas, con 100 claves cada una. Con m = 128\n"
              << "  pasa lo mismo a menor escala (comparten el factor 2). Un m PRIMO\n"
              << "  no comparte factores con ningun patron de las claves: se usan todas.\n";

    std::cout << "\n=== 2) Cadenas: tres funciones hash con las mismas claves ===\n";
    const int n = 50000, m = 10007;   // m primo; alfa = n/m ~ 5 claves por cubeta
    auto palabras = generarPalabras(n, 7);
    std::cout << "  " << n << " palabras distintas en m = " << m
              << " cubetas (alfa = n/m = " << std::setprecision(2) << std::fixed
              << static_cast<double>(n) / m << ")\n";
    std::cout << "  ejemplos: " << palabras[0] << ", " << palabras[1] << ", "
              << palabras[2] << ", " << palabras[3] << "\n\n";
    std::cout << "  " << std::left << std::setw(16) << "funcion"
              << std::setw(10) << "usadas" << std::setw(9) << "%"
              << std::setw(12) << "max/cubeta" << "comparaciones medias\n";
    filaReparto("longitud", repartir(palabras, m, hashLongitud), m);
    filaReparto("suma", repartir(palabras, m, hashSuma), m);
    filaReparto("polinomico", repartir(palabras, m, hashPolinomico), m);
    double ideal = 1.0 + (static_cast<double>(n) / m) / 2.0;
    std::cout << "  (reparto perfecto: " << std::setprecision(2) << ideal
              << " comparaciones de media = 1 + alfa/2)\n";

    std::cout << "\n  Los anagramas, con cada funcion (m = " << m << "):\n";
    for (const char* p : {"amor", "roma", "mora", "ramo", "omar"}) {
        std::cout << "    " << std::left << std::setw(6) << p
                  << "  suma -> " << std::setw(6) << hashSuma(p) % m
                  << "  polinomico -> " << hashPolinomico(p) % m << '\n';
    }

    std::cout << "\n=== 3) Las colisiones llegan pronto: la paradoja del cumpleanios ===\n";
    std::cout << "  Insertando claves al azar en m cubetas, cuantas hasta la PRIMERA\n"
              << "  colision? La intuicion dice ~m/2. La realidad: ~raiz(pi*m/2).\n\n";
    std::cout << "  " << std::left << std::setw(12) << "m"
              << std::setw(20) << "raiz(pi*m/2)" << std::setw(25) << "medido (media de 200)"
              << "m/2\n";
    std::mt19937 gen(2026);
    for (int mm : {365, 10007, 1000003}) {
        std::uniform_int_distribution<int> casilla(0, mm - 1);
        long long total = 0;
        const int repeticiones = 200;
        for (int r = 0; r < repeticiones; r++) {
            std::vector<char> ocupada(mm, 0);
            int k = 0;
            while (true) {
                int c = casilla(gen);
                k++;
                if (ocupada[c]) break;
                ocupada[c] = 1;
            }
            total += k;
        }
        std::cout << "  " << std::left << std::setw(12) << mm
                  << std::setw(20) << std::setprecision(1) << std::sqrt(PI * mm / 2)
                  << std::setw(25) << static_cast<double>(total) / repeticiones
                  << mm / 2 << '\n';
    }
    std::cout << "\n  Con 365 dias, en un grupo de ~23 personas ya es mas probable que\n"
              << "  no que dos cumplan el mismo dia. Con un millon de cubetas, la\n"
              << "  primera colision llega hacia la clave 1200, con la tabla casi\n"
              << "  vacia (0,1 %). Conclusion: no se pueden EVITAR las colisiones;\n"
              << "  hay que saber RESOLVERLAS (ficheros 03 y 04).\n";
}
