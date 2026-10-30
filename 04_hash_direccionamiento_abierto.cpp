// 04_hash_direccionamiento_abierto.cpp
// Objetivo: la segunda forma de resolver colisiones. En vez de colgar una
// lista de cada cubeta (fichero 03), TODO se guarda DENTRO del array: si la
// casilla h(c) esta ocupada, se prueba la siguiente, y la siguiente... hasta
// encontrar un hueco. Es el DIRECCIONAMIENTO ABIERTO con SONDEO LINEAL:
//
//      casilla k-esima que se prueba = (h(c) + k) mod m,   k = 0, 1, 2...
//
// QUE MEJORA respecto al encadenamiento: ni un new por clave, ni punteros;
// las casillas son contiguas y recorrer las siguientes es lo que mejor sabe
// hacer la cache. Por eso es lo que usan las tablas hash rapidas modernas.
//
// LO QUE CUESTA (y se mide en el main):
//   1. alfa = n/m tiene que ser < 1 (no caben mas claves que casillas), y
//      el coste se dispara al acercarse a 1: no basta con alfa <= 1 como
//      en el fichero 03, aqui se redimensiona en alfa = 0,7.
//   2. AGRUPAMIENTO PRIMARIO: las claves que chocan forman bloques
//      contiguos, y un bloque largo "atrae" mas claves (cualquier clave que
//      caiga en cualquier casilla del bloque acaba al final de el).
//   3. BORRAR NO ES VACIAR. Buscar se para en la primera casilla VACIA:
//      "si estuviera, estaria antes de este hueco". Si al borrar se deja la
//      casilla vacia, se rompe el camino de sondeo de las claves que
//      saltaron por encima de ella, y dejan de encontrarse. La solucion es
//      una LAPIDA (BORRADA): buscar la salta como si estuviera ocupada,
//      insertar puede reutilizarla.
//
// EJERCICIO: completar borrar() con lapida. Mientras no este hecho no
// borra nada (devuelve false), y las demostraciones de lapidas del main
// no muestran ninguna.
//
// Compilar:
//   g++ -std=c++17 -O2 -Wall -o 04_abierto 04_hash_direccionamiento_abierto.cpp && ./04_abierto

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <algorithm>

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

// Misma especificacion que el TAD Diccionario del fichero 01.
class TablaHashAbierta {
public:
    explicit TablaHashAbierta(int m = 11, bool redimensionar = true)
        : casillas_(m), redimensionar_(redimensionar) {}

    void insertar(const std::string& clave, int valor) {
        int m = cubetas();
        int i = inicio(clave);
        int primeraLapida = -1, hueco = -1;
        for (int k = 0; k < m; k++, i = (i + 1) % m) {
            sondeos_++;
            Casilla& c = casillas_[i];
            if (c.estado == Estado::VACIA) { hueco = i; break; }   // no esta: aqui se acaba
            if (c.estado == Estado::BORRADA) {
                if (primeraLapida == -1) primeraLapida = i;  // reutilizable... si no esta
            } else if (c.clave == clave) {
                c.valor = valor;                             // ya estaba: sustituir
                return;
            }
        }
        // No estaba. Va a la primera lapida del camino, o al hueco vacio.
        if (primeraLapida != -1) {
            i = primeraLapida;
            lapidas_--;
        } else if (hueco != -1) {
            i = hueco;
        } else {
            return;   // tabla LLENA: solo puede pasar con redimensionar_ == false
        }
        casillas_[i] = Casilla{clave, valor, Estado::OCUPADA};
        n_++;
        if (redimensionar_ && n_ + lapidas_ > ALFA_MAX * m) rehash(siguientePrimo(2 * m));
    }

    bool buscar(const std::string& clave, int& valor) const {
        int pos = posicionDe(clave);
        if (pos == -1) return false;
        valor = casillas_[pos].valor;
        return true;
    }

    // --- Ejercicio: borrar con lapida --------------------------------------
    //
    // Localizar la clave (posicionDe() ya lo hace) y, si esta, NO dejar la
    // casilla VACIA: marcarla BORRADA. Mantener los contadores del
    // invariante: n_ baja uno y lapidas_ sube uno.
    bool borrar(const std::string& clave) {
        // TODO: implementar.
        // Pistas:
        //   - int pos = posicionDe(clave); si es -1, no estaba.
        //   - Mira borrarMal() justo debajo: hace casi lo mismo, pero deja
        //     la casilla VACIA. Tu version la marca Estado::BORRADA.
        //   - Puedes vaciar el texto de la clave (clave.clear()): ya no
        //     hace falta, y buscar no mira la clave de una BORRADA.
        (void)clave;
        return false;
    }

    // El error clasico, para verlo fallar en el main: deja la casilla VACIA.
    bool borrarMal(const std::string& clave) {
        int pos = posicionDe(clave);
        if (pos == -1) return false;
        casillas_[pos] = Casilla{};
        n_--;
        return true;
    }

    int tamanio() const { return n_; }
    int cubetas() const { return static_cast<int>(casillas_.size()); }
    int lapidas() const { return lapidas_; }
    double factorCarga() const { return static_cast<double>(n_ + lapidas_) / cubetas(); }
    long long sondeos() const { return sondeos_; }
    void reiniciarSondeos() { sondeos_ = 0; }
    int inicio(const std::string& clave) const {
        return static_cast<int>(hashPolinomico(clave) % casillas_.size());
    }

    // Una linea por casilla: la clave, su casilla de inicio h y cuanto se desplazo.
    void imprimir() const {
        int m = cubetas();
        for (int i = 0; i < m; i++) {
            const Casilla& c = casillas_[i];
            std::cout << "    [" << std::setw(2) << i << "] ";
            if (c.estado == Estado::VACIA) std::cout << ".\n";
            else if (c.estado == Estado::BORRADA) std::cout << "x  (lapida)\n";
            else {
                int h = inicio(c.clave);
                int desplazada = (i - h + m) % m;
                std::cout << std::left << std::setw(6) << c.clave << std::right << " h=" << std::setw(2) << h;
                if (desplazada > 0) std::cout << "  (+" << desplazada << ")";
                std::cout << '\n';
            }
        }
    }

    // Un caracter por casilla: '#' ocupada, '.' vacia, 'x' lapida.
    std::string mapa() const {
        std::string s;
        for (const Casilla& c : casillas_)
            s += c.estado == Estado::OCUPADA ? '#' : (c.estado == Estado::VACIA ? '.' : 'x');
        return s;
    }

private:
    enum class Estado { VACIA, OCUPADA, BORRADA };
    struct Casilla {
        std::string clave;
        int valor = 0;
        Estado estado = Estado::VACIA;
    };

    // --- Invariante de la representacion ---------------------------------
    //  1. Toda clave OCUPADA en la casilla i es alcanzable desde su inicio
    //     h = hash % m: ninguna casilla entre h e i (avanzando y dando la
    //     vuelta) esta VACIA. Las BORRADAS no cortan el camino.
    //  2. No hay dos casillas OCUPADAS con la misma clave.
    //  3. n_ = casillas OCUPADAS, lapidas_ = casillas BORRADAS.
    //  4. Si redimensionar_: (n_ + lapidas_) <= ALFA_MAX * m tras insertar.
    //     Las lapidas cuentan: tambien alargan los caminos de sondeo.
    // -----------------------------------------------------------------------
    std::vector<Casilla> casillas_;
    int n_ = 0;
    int lapidas_ = 0;
    bool redimensionar_;
    mutable long long sondeos_ = 0;
    static constexpr double ALFA_MAX = 0.7;

    // Devuelve la casilla de 'clave', o -1 si no esta.
    int posicionDe(const std::string& clave) const {
        int m = cubetas();
        int i = inicio(clave);
        for (int k = 0; k < m; k++, i = (i + 1) % m) {
            sondeos_++;
            const Casilla& c = casillas_[i];
            if (c.estado == Estado::VACIA) return -1;   // hueco: no puede estar mas alla
            if (c.estado == Estado::OCUPADA && c.clave == clave) return i;
            // BORRADA, u OCUPADA por otra clave: seguir sondeando
        }
        return -1;                                      // vuelta completa
    }

    // Reinsertar solo las OCUPADAS en un array nuevo: las lapidas desaparecen.
    void rehash(int nuevoM) {
        std::vector<Casilla> viejas(nuevoM);
        viejas.swap(casillas_);
        n_ = 0;
        lapidas_ = 0;
        for (Casilla& c : viejas) {
            if (c.estado != Estado::OCUPADA) continue;
            int i = inicio(c.clave);
            while (casillas_[i].estado != Estado::VACIA) i = (i + 1) % nuevoM;
            casillas_[i] = std::move(c);
            n_++;
        }
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
    const std::vector<std::string> claves = {"sol", "mar", "luz", "casa", "mesa", "pelo", "rio"};

    std::cout << "=== Sondeo lineal: m = 11 fijo, 7 claves ===\n";
    TablaHashAbierta t(11, false);
    for (std::size_t i = 0; i < claves.size(); i++) t.insertar(claves[i], static_cast<int>(i));
    t.imprimir();
    std::cout << "  (+k: la clave esta k casillas despues de su inicio h)\n";

    // Buscar dos claves que se pisen: misma casilla de inicio.
    std::string a, b;
    for (std::size_t i = 0; i < claves.size() && a.empty(); i++)
        for (std::size_t j = i + 1; j < claves.size(); j++)
            if (t.inicio(claves[i]) == t.inicio(claves[j])) { a = claves[i]; b = claves[j]; break; }

    std::cout << "\n=== Por que borrar NO es vaciar la casilla ===\n";
    std::cout << "  '" << a << "' y '" << b << "' empiezan en la misma casilla (h="
              << t.inicio(a) << "); '" << b << "' entro despues y salto por encima.\n";
    int v = 0;
    {
        TablaHashAbierta mal(11, false);
        for (std::size_t i = 0; i < claves.size(); i++) mal.insertar(claves[i], static_cast<int>(i));
        mal.borrarMal(a);
        std::cout << "  borrarMal(" << a << ") deja la casilla VACIA ->  buscar(" << b
                  << ") = " << mal.buscar(b, v) << "   <-- !perdida, y sigue en la tabla!\n";
    }
    t.borrar(a);
    std::cout << "  borrar(" << a << ") deja una LAPIDA      ->  buscar(" << b
              << ") = " << t.buscar(b, v) << '\n';
    t.imprimir();
    // Una clave nueva que empiece en la casilla de la lapida: la reutiliza.
    std::string nueva;
    for (const auto& p : generarPalabras(200, 1))
        if (t.inicio(p) == t.inicio(a) && !t.buscar(p, v)) { nueva = p; break; }
    t.insertar(nueva, 99);
    std::cout << "  insertar(" << nueva << "), que empieza en h=" << t.inicio(nueva)
              << ": comprueba que no esta (sigue\n  hasta el hueco) y se queda en la"
              << " lapida. lapidas ahora: " << t.lapidas() << '\n';
    t.imprimir();

    std::cout << "\n=== Agrupamiento primario: m = 61, alfa = 0.5 ===\n";
    {
        auto pal = generarPalabras(30, 11);
        TablaHashAbierta g(61, false);
        for (int i = 0; i < 30; i++) g.insertar(pal[i], i);
        std::string s = g.mapa();
        int maximo = 0, actual = 0;
        for (char c : s) { actual = (c == '#') ? actual + 1 : 0; maximo = std::max(maximo, actual); }
        std::cout << "  " << s << '\n'
                  << "  bloque ocupado mas largo: " << maximo << " casillas seguidas\n"
                  << "  Una clave que caiga en CUALQUIER casilla de un bloque acaba al\n"
                  << "  final de el y lo alarga: los bloques grandes crecen mas deprisa.\n";
    }

    std::cout << "\n=== Sondeos medios frente a alfa (m = 100003 fijo) ===\n";
    std::cout << "  " << std::left << std::setw(8) << "alfa"
              << std::setw(10) << "exito" << std::setw(14) << "teoria"
              << std::setw(10) << "fracaso" << std::setw(14) << "teoria"
              << "encadenamiento (exito)\n";
    {
        const int m = 100003;
        auto pal = generarPalabras(2 * m, 5);   // la 2a mitad, para buscar sin exito
        for (double alfa : {0.25, 0.5, 0.7, 0.8, 0.9, 0.95}) {
            TablaHashAbierta h(m, false);
            int n = static_cast<int>(alfa * m);
            for (int i = 0; i < n; i++) h.insertar(pal[i], i);
            h.reiniciarSondeos();
            for (int i = 0; i < n; i++) h.buscar(pal[i], v);
            double exito = static_cast<double>(h.sondeos()) / n;
            h.reiniciarSondeos();
            const int pruebas = 20000;
            for (int i = 0; i < pruebas; i++) h.buscar(pal[m + i], v);
            double fracaso = static_cast<double>(h.sondeos()) / pruebas;
            double teoE = 0.5 * (1 + 1 / (1 - alfa));
            double teoF = 0.5 * (1 + 1 / ((1 - alfa) * (1 - alfa)));
            std::cout << "  " << std::fixed << std::setprecision(2) << std::setw(8) << alfa
                      << std::setw(10) << exito << std::setw(14) << teoE
                      << std::setw(10) << fracaso << std::setw(14) << teoF
                      << 1 + alfa / 2 << '\n';
        }
    }
    std::cout << "  (teoria: Knuth, 1962: exito ~ (1 + 1/(1-a))/2, fracaso ~ (1 + 1/(1-a)^2)/2;\n"
              << "   supone un hash perfectamente uniforme: el real se aparta al final)\n"
              << "  Hasta alfa = 0,7 sale barato; a partir de ahi se dispara, sobre todo\n"
              << "  buscando lo que NO esta. Por eso se redimensiona en 0,7.\n";

    std::cout << "\n=== Las lapidas se acumulan: por que cuentan en alfa ===\n";
    {
        const int m = 1009;
        auto pal = generarPalabras(20000, 9);   // 19000.. no se insertan nunca
        TablaHashAbierta h(m, false);              // sin rehash: nada las limpia
        for (int i = 0; i < 100; i++) h.insertar(pal[i], i);
        auto sinExito = [&]() {
            h.reiniciarSondeos();
            for (int i = 0; i < 1000; i++) h.buscar(pal[19000 + i], v);
            return static_cast<double>(h.sondeos()) / 1000;
        };
        std::cout << "  n=" << h.tamanio() << "  lapidas=" << std::setw(4) << h.lapidas()
                  << "   sondeos buscando algo que no esta: " << sinExito() << '\n';
        int siguiente = 100;
        for (int ronda = 0; ronda < 24; ronda++) {   // 100 vivas siempre; cada ronda
            for (int i = 0; i < 100; i++) {         // borra 100 y mete 100 nuevas
                h.borrar(pal[siguiente - 100]);
                h.insertar(pal[siguiente], siguiente);
                siguiente++;
            }
            if (ronda % 6 == 5)
                std::cout << "  n=" << h.tamanio() << "  lapidas=" << std::setw(4) << h.lapidas()
                          << "   sondeos buscando algo que no esta: " << sinExito() << '\n';
        }
        std::cout << "  Siempre hay 100 claves vivas (alfa REAL = 0,1), pero las lapidas\n"
                  << "  alargan cada camino de sondeo. Por eso el rehash se dispara con\n"
                  << "  (n + lapidas) / m, y al reconstruir la tabla las lapidas se van.\n";
    }

    std::cout << "\n=== Con rehash en alfa = 0,7, partiendo de m = 11 ===\n";
    {
        auto pal = generarPalabras(220000, 21);
        TablaHashAbierta h;                        // m = 11, redimensionar = true
        for (int i = 0; i < 200000; i++) h.insertar(pal[i], i);
        for (int i = 0; i < 200000; i += 2) h.borrar(pal[i]);      // la mitad fuera
        for (int i = 0; i < 200000; i += 2) h.insertar(pal[i], i); // y otra vez dentro
        h.reiniciarSondeos();
        int encontradas = 0;
        for (int i = 0; i < 200000; i++) encontradas += h.buscar(pal[i], v);
        double exito = static_cast<double>(h.sondeos()) / 200000;
        h.reiniciarSondeos();
        for (int i = 200000; i < 220000; i++) h.buscar(pal[i], v);
        double fracaso = static_cast<double>(h.sondeos()) / 20000;
        std::cout << "  n=" << h.tamanio() << " (encontradas " << encontradas << ")  m=" << h.cubetas()
                  << "  alfa=" << std::setprecision(2) << h.factorCarga()
                  << "  lapidas=" << h.lapidas() << '\n'
                  << "  sondeos medios: exito " << exito << ", fracaso " << fracaso << '\n';
    }
}
