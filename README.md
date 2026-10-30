# Tema 5 (i): Diccionarios y tablas hash

## Cómo usar esta guía

Igual que en las clases anteriores: cada bloque introduce una idea y termina
en un ejercicio con código ejecutable. Lee primero el planteamiento, intenta
anticipar el resultado y solo después despliega el código y ejecútalo para
comparar tu razonamiento con lo que mide la máquina.

Algunos ficheros tienen partes marcadas con `// TODO: implementar`: son el
ejercicio. Complétalas tú.

Para compilar y ejecutar cualquier fichero:

```bash
g++ -std=c++17 -Wall -o 01_dicc 01_tad_diccionario.cpp && ./01_dicc
```

En los ficheros que miden tiempos (`01`, `03`, `05`, `07`) conviene compilar
con optimización, que es como se compila el código de verdad:

```bash
g++ -std=c++17 -O2 -Wall -o 07_comp 07_comparativa_diccionarios.cpp && ./07_comp
```

**Ficheros de esta carpeta:**

| Fichero                                  | Contenido                                                                                     | ¿Hay que completarlo? |
| ---------------------------------------- | --------------------------------------------------------------------------------------------- | ---------------------- |
| `01_tad_diccionario.cpp`               | El TAD Diccionario, tres implementaciones con lo ya visto, y el direccionamiento directo      | No, está completo     |
| `02_funciones_hash.cpp`                | De la clave a la casilla: `mod m` con m primo, hash de cadenas, la paradoja del cumpleaños | **Sí**          |
| `03_hash_encadenamiento.cpp`           | Tabla hash con encadenamiento: una lista por cubeta, factor de carga y rehash                 | **Sí**          |
| `04_hash_direccionamiento_abierto.cpp` | Todo dentro del array: sondeo lineal, agrupamiento y lápidas                                 | **Sí**          |
| `05_unordered_stl.cpp`                 | `std::unordered_map` y `std::unordered_set`: la tabla del 03 hecha tipo                   | No, está completo     |
| `06_trie.cpp`                          | El trie: un árbol de letras, con los hijos en un array, un `map` o un `unordered_map`    | **Sí**          |
| `07_comparativa_diccionarios.cpp`      | Árbol, tabla hash y trie, frente a frente con las mismas claves                              | No, está completo     |
| `chuleta_hash_ejercicios.docx`         | Hoja para practicar en papel: hash, las dos tablas, el trie y tres usos de la STL             | Sí, a mano            |

**Lo que se da por sabido de las clases anteriores:** el tema 2 (la lista
enlazada y el **coste amortizado** del array dinámico, que reaparece tal cual
en el rehash), el tema 3 (Pila y Cola) y el tema 4 completo. En concreto, el
**árbol de búsqueda** y `std::map` son el punto de partida, que es lo que hay
que superar, y el **árbol general** del tema 4 vuelve en el bloque 6 como trie.

---

## 0. El hilo conductor del tema

El tema 4 terminó con un diccionario que busca en O(log n) **siempre**:
`std::map`. Este tema hace una sola pregunta: ***¿se puede bajar de log n?***
La respuesta es que sí, pero cada paso supone algo a cambio:

| # | Bloque                        | Lo que aporta                                                           | Lo que cuesta o deja abierto                                                         |
| - | ----------------------------- | ----------------------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| 1 | Diccionario sobre lo ya visto | Lista O(n), vector ordenado O(log n) al buscar, árbol O(log n) en todo | Sigue siendo log n                                                                   |
| 2 | Direccionamiento directo      | O(1) exacto: la clave **es** el índice                           | Solo con claves enteras pequeñas: la memoria es la del **universo** de claves |
| 3 | Función hash                 | Lleva cualquier clave a una de las m casillas                           | **Colisiones**, inevitables y mucho más pronto de lo que parece               |
| 4 | Encadenamiento                | Una lista por casilla; O(1 + α)                                        | Un nodo suelto por clave; α crece → **rehash**                               |
| 5 | Direccionamiento abierto      | Todo dentro del array, amigo de la caché                               | Agrupamiento, α < 1, y borrar con marcas o **lápidas (tombstones)**         |
| 6 | Trie                          | Claves de texto: O(longitud), y consultas por **prefijo**         | Mucha memoria por nodo                                                               |
| 7 | Comparativa                   | Elegir con datos                                                        | (cierra el tema)                                                                     |

Fíjate en lo que pierde la tabla hash para ganar velocidad: **el orden**. Un
árbol sabe cuál es la clave siguiente a otra; una tabla hash no tiene ni idea,
porque la función hash está pensada precisamente para desordenar. Todo lo que
dependa del orden (listar en orden, buscar un rango, buscar por prefijo) deja
de ser barato. El trie del bloque 6 recupera el orden para el caso concreto
de las cadenas de texto.

---

## 1. El TAD Diccionario

Un **Diccionario** (también *mapa* o *tabla asociativa*) guarda pares
**(clave, valor)** sin claves repetidas, y se usa **por clave**:

| Operación         | Pre | Post                                                                                     |
| ------------------ | --- | ---------------------------------------------------------------------------------------- |
| `insertar(c, v)` | —  | `c` tiene asociado `v`; si `c` ya estaba, su valor anterior se **sustituye** |
| `buscar(c, v)`   | —  | devuelve si `c` está; si está, deja en `v` su valor                                |
| `borrar(c)`      | —  | si `c` estaba, la quita y devuelve `true`                                            |
| `tamanio()`      | —  | número de claves distintas                                                              |

Si no hay valor, solo claves, el TAD es un **Conjunto** (`std::set`,
`std::unordered_set`). Ya los conoces: el BST del tema 4 era un Conjunto y
`std::map` un Diccionario. Lo nuevo es la pregunta de cuánto puede bajar el
coste.

En `01_tad_diccionario.cpp` el contrato es una clase abstracta, como
`ListaDeEnteros` en el tema 2 o `Arbol` en el tema 4, con tres
implementaciones hechas con lo que ya sabemos:

| Implementación                  | `buscar`                  | `insertar`                     | `borrar` |
| -------------------------------- | --------------------------- | -------------------------------- | ---------- |
| (A) vector **sin** ordenar | O(n)                        | O(n): hay que mirar si ya estaba | O(n)       |
| (B) vector **ordenado**    | O(log n): búsqueda binaria | O(n): desplazar para hacer hueco | O(n)       |
| (C) `std::map` (rojo-negro)    | O(log n)                    | O(log n)                         | O(log n)   |

### Ejercicio 1: el mismo contrato, tres costes (completo, para leer)

**Antes de ejecutar:** si n se multiplica por 4, ¿por cuánto se multiplica el
tiempo de insertar n claves y buscarlas todas en cada implementación?

<details>
<summary>Mostrar resultado</summary>

```text
=== El mismo contrato, tres implementaciones ===
  vector sin ordenar  tamanio=2  buscar(ana)=true valor=10  buscar(luis)=false
  vector ordenado     tamanio=2  buscar(ana)=true valor=10  buscar(luis)=false
  std::map (arbol)    tamanio=2  buscar(ana)=true valor=10  buscar(luis)=false

=== Cuanto cuesta: insertar n claves y buscarlas todas ===
  n        sin ordenar (ms)      ordenado (ms)         std::map (ms)
  1000     2.20                  0.46                  0.26
  4000     14.72                 4.95                  1.27
  16000    395.31                66.37                 5.92
```

</details>

**La respuesta:** son n operaciones de cada tipo. En (A) cada una es O(n), así
que el total es O(n²): al multiplicar n por 4 el tiempo se multiplica por 16, y
algo más cuando los datos dejan de caber en la caché. En (B) la búsqueda es
barata, pero cada inserción desplaza la mitad del vector de media, así que
también es cuadrático, con una constante menor. En (C) el total es
O(n log n): un poco más de 4.

### El direccionamiento directo: O(1) exacto… con letra pequeña

Si las claves son enteros pequeños (el número de lista de 0 a 99 de un grupo),
se puede usar la **propia clave como índice** de un array: `notaDe[42]`. No hay
comparaciones ni recorrido: buscar, insertar y borrar son una sola operación
de array.

El problema es que el array mide lo que mide el **universo** de claves posibles,
no lo que de verdad se guarda:

```text
  Y si la clave es un DNI (8 digitos) y hay 300 alumnos?
    casillas necesarias: 100000000 (el universo entero)
    memoria (4 bytes por casilla): 381 MB
    casillas USADAS: 300  ->  ocupacion del 0.0003 %
    Y si la clave es una cadena de 10 letras: 26^10 = 1.4e14 casillas.
    Imposible.
```

**Pregunta para pensar:** ¿qué tendría que hacer una función que llevara cada
DNI a una casilla de un array de, digamos, 600 posiciones? ¿Qué es inevitable
que pase alguna vez?

<details>
<summary>Mostrar respuesta</summary>

Tendría que ser **determinista** (el mismo DNI va siempre a la misma casilla,
o no se podría volver a encontrar) y **repartir** bien (DNIs distintos, a
casillas distintas en lo posible). Lo inevitable es que dos DNIs distintos
caigan alguna vez en la **misma** casilla: hay cien millones de DNIs y 600
casillas (el principio del palomar). Esa función es una **función hash**, y
esos choques son **colisiones**. Son los dos bloques siguientes.

</details>

---

## 2. Funciones hash

Una **función hash** lleva cada clave a un índice entre 0 y m − 1:

```text
        h : claves  ->  {0, 1, ..., m-1}        (m = número de cubetas)
```

Lo que se le pide:

1. **Determinista:** la misma clave da siempre la misma casilla.
2. **Rápida:** O(longitud de la clave), nada más.
3. **Que reparta:** claves parecidas (`casa`, `cosa`, `caso`) o con un patrón
   (todas múltiplos de 10) deben acabar en casillas distintas.

Lo que **no** se le puede pedir es que no haya colisiones.

### Claves enteras: h(k) = k mod m, con m primo

Con claves enteras lo natural es `k % m`. Pero si las claves siguen un patrón
que comparte un factor con m, la mayoría de las cubetas se quedan vacías:

```text
=== 1) Enteros: h(k) = k mod m, y por qué m debe ser primo ===
  1000 claves que son todas MÚLTIPLOS DE 10 (0, 10, 20, ...)

  m         cubetas usadas    cubeta más llena
  100       10 de 100         100
  101       101 de 101        10
  128       64 de 128         16
  127       127 de 127        8
```

Con m = 100, el resto de dividir un múltiplo de 10 entre 100 es otro múltiplo
de 10: solo se usan 10 cubetas. Un m **primo** no comparte factores con ningún
patrón de las claves.

### Claves cadena: tres candidatas

| Función              | Idea                                                                                         | Defecto                                                                                                                 |
| --------------------- | -------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| Longitud              | `s.size()`                                                                                 | Casi todas chocan: hay pocas longitudes distintas                                                                       |
| Suma                  | Suma de los códigos de los caracteres                                                       | Ignora el **orden** (los anagramas chocan), y su rango es pequeño: una palabra de 10 letras suma como mucho 1220 |
| **Polinómica** | Cada carácter pesa según su **posición**, como los dígitos de un número en base B | —                                                                                                                      |

### Ejercicio 2: el hash polinómico, con la regla de Horner

```text
        h("abc") = 'a'·B² + 'b'·B¹ + 'c'·B⁰
```

Calcular cada potencia por separado sería O(L²). La **regla de Horner** lo
hace en una pasada, multiplicando lo acumulado por B antes de sumar el
siguiente carácter:

```text
        h = 0
        para cada carácter c:   h = h * B + c
```

**Tu tarea:** en `02_funciones_hash.cpp`, completar `hashPolinomico()` con
B = 31 (es la base que usa `String.hashCode()` en Java).

**Antes de escribir código:** el resultado se sale de cualquier entero en
cuanto la palabra pasa de 13 letras. ¿Es un error? ¿Por qué se declara
`unsigned long long` y no `long long`?

<details>
<summary>Mostrar código (fragmento; completo en <code>02_funciones_hash.cpp</code>)</summary>

```cpp
// (b) La suma: ignora el orden, y su rango es pequenio.
unsigned long long hashSuma(const std::string& s) {
    unsigned long long h = 0;
    for (char c : s) h += static_cast<unsigned char>(c);
    return h;
}

// (c) TU TAREA: Horner con B = 31.
unsigned long long hashPolinomico(const std::string& s) {
    // TODO: implementar
    return 0;
}
```

</details>

<details>
<summary>Mostrar resultado (con la solución correcta)</summary>

```text
=== 2) Cadenas: tres funciones hash con las mismas claves ===
  50000 palabras distintas en m = 10007 cubetas (alfa = n/m = 5.00)
  ejemplos: quepla, quelo, fremenoti4, node

  funcion         usadas    %        max/cubeta  comparaciones medias
  longitud        11        0.1      11249       3676.05
  suma            933       9.3      234         50.89
  polinomico      9949      99.4     15          3.50
  (reparto perfecto: 3.50 comparaciones de media = 1 + alfa/2)

  Los anagramas, con cada funcion (m = 10007):
    amor    suma -> 431     polinomico -> 5938
    roma    suma -> 431     polinomico -> 3871
    mora    suma -> 431     polinomico -> 5176
    ramo    suma -> 431     polinomico -> 438
    omar    suma -> 431     polinomico -> 2284
```

</details>

**La respuesta:** desbordar no es un error. Con un tipo `unsigned`, C++
garantiza que la aritmética es **módulo 2⁶⁴**: el resultado da la vuelta, pero
siempre de la misma manera, así que el hash sigue siendo determinista. Con un
tipo **con signo**, desbordar es *comportamiento indefinido*: el compilador
puede suponer que nunca pasa y optimizar en consecuencia. El
`static_cast<unsigned char>(c)` existe por lo mismo: una `ñ` o una `á` son
`char` negativos y restarían.

Fíjate en la última columna: el polinómico consigue **exactamente** el coste
del reparto perfecto, 1 + α/2.

### La paradoja del cumpleaños: las colisiones llegan pronto

Si se van metiendo claves al azar en m cubetas, la intuición dice que la
primera colisión llega hacia la clave m/2. Llega mucho antes, hacia la clave
√(πm/2):

```text
  m           raiz(pi*m/2)        medido (media de 200)    m/2
  365         23.9                24.5                     182
  10007       125.4               129.5                    5003
  1000003     1253.3              1152.7                   500001
```

En un grupo de 23 personas, la probabilidad de que al menos dos cumplan años el mismo día es mayor del 50 %.
Si pensamos en si hay alguien que cumpla el mismo día que yo, 22/365=0.06 (6% de probabilidades) pero si preguntamos ¿Hay alguna pareja cualquiera que coincida? Aquí cuentan todas las parejas posibles, y hay muchas: C(23, 2) = 23·22/2 = 253 parejas. Cada una tiene una pequeña probabilidad de coincidir, pero al ser tantas, en conjunto pasan del 50%. $1-(365/365 * 364/365 *...*343/365)=50.7%$
Con un millón de cubetas, la primera colisión llega con la tabla ocupada al 0,1 %. **No se pueden evitar las colisiones: hay que resolverlas.**
Hay dos formas, y son los dos bloques siguientes.

---

## 3. Encadenamiento: una lista por cubeta

Cada cubeta guarda una **lista enlazada** (la del tema 2) con todas las claves
que han caído en ella:

```text
    cubetas_[0] -> nullptr
    cubetas_[1] -> [mesa|3] -> [pelo|8] -> nullptr     h(mesa) = h(pelo) = 1
    cubetas_[2] -> [sol|1]  -> nullptr
    ...
```

- **Insertar:** calcular `h(c)`, recorrer esa lista para ver si ya estaba y, si
  no, meter el nodo **al principio** (el `insertarPrincipio` del tema 2).
- **Buscar** y **borrar:** recorrer solo esa lista.

El coste depende de la longitud de la lista, y eso lo mide el **factor de
carga** α = n/m, que es el número medio de claves por cubeta. Con una buena
función hash, buscar una clave que está cuesta unas 1 + α/2 comparaciones, y
una que no está, unas α.

### Ejercicio 3: el rehash

Si m es fijo y n crece, α crece con n y buscar vuelve a ser O(n). La solución
es la misma que la del array dinámico del tema 2: cuando α pasa de un límite
(aquí, 1), se pide un array de cubetas **más grande**, el doble redondeado a
primo, y se recoloca cada clave.

**Tu tarea:** en `03_hash_encadenamiento.cpp`, completar `rehash(nuevoM)`.

**Antes de escribir código:** ¿por qué no basta con copiar el array viejo de
cubetas en las primeras posiciones del nuevo? ¿Hace falta algún `new` o
`delete` de `Nodo`?

<details>
<summary>Mostrar código (fragmento; completo en <code>03_hash_encadenamiento.cpp</code>)</summary>

```cpp
void insertar(const std::string& clave, int valor) {
    int i = indice(clave);
    for (Nodo* p = cubetas_[i]; p != nullptr; p = p->siguiente) {
        if (p->clave == clave) { p->valor = valor; return; }   // ya estaba
    }
    cubetas_[i] = new Nodo{clave, valor, cubetas_[i]};         // al principio: O(1)
    n_++;
    if (redimensionar_ && n_ > ALFA_MAX * cubetas()) {
        rehash(siguientePrimo(2 * cubetas()));
    }
}

// TU TAREA:
void rehash(int nuevoM) {
    // TODO: implementar
}
```

</details>

<details>
<summary>Mostrar resultado (con la solución correcta)</summary>

```text
=== Las mismas 10 claves, CON rehash al pasar de alfa = 1 ===
  al insertar 'pan' (n=8 > 7): rehash de 7 a 17 cubetas
    ...
  n=10  m=17  alfa=0.59  cadena mas larga=2

=== Sin rehash frente a con rehash, a gran escala ===
  (comparaciones = media por busqueda CON exito)

  n        | alfa    max     comp    ms        | m        alfa   max   comp   ms
           | sin rehash (m = 1009 fijo)        | con rehash (alfa <= 1)
  1000     | 0.99    4       1.48    0.1       | 1009     0.99   4     1.48   0.1
  10000    | 9.91    24      5.95    1.3       | 16249    0.62   6     1.30   1.0
  100000   | 99.11   130     50.55   78.7      | 130027   0.77   6     1.38   10.2
  400000   | 396.43  466     199.23  2040.0    | 520193   0.77   8     1.39   53.3

=== Cuanto cuesta el rehash, amortizado ===
  400000 inserciones partiendo de m = 7: m final = 701819
  nodos recolocados en TODOS los rehash juntos: 701662
  recolocados por insercion: 1.75
```

</details>

**La respuesta:** al cambiar m cambia `hash % m`, así que cada clave tiene que
ir a **su nueva** cubeta. Si se copiara el array tal cual, una clave que estaba
en la cubeta 3 de 7 se buscaría ahora en `hash % 17`, que casi nunca es 3, y
no se encontraría. No hace falta ningún `new` ni ningún `delete`: los nodos ya
existen y solo se **reenlazan**, cambiando su campo `siguiente`. Lo único que
hay que cuidar es guardar `p->siguiente` **antes** de reenlazar `p`, o se
pierde el resto de la lista vieja.

La última medida es la cuenta del tema 2: cada rehash es O(n), pero como el
tamaño se duplica, el total de recolocaciones es n·(1 + ½ + ¼ + …) < 2n.
**Insertar es O(1) amortizado.**

---

## 4. Direccionamiento abierto: todo dentro del array

El encadenamiento hace un `new` por clave, y cada salto de nodo en nodo es un
salto por memoria que la caché no predice. La alternativa es no usar listas:
si la casilla `h(c)` está ocupada, se prueba la siguiente, y la siguiente…,
hasta encontrar hueco. Es el **sondeo lineal**:

```text
        casilla que se prueba en el intento k = (h(c) + k) mod m,    k = 0, 1, 2...
```

```text
=== Sondeo lineal: m = 11 fijo, 7 claves ===
    [ 0] .
    [ 1] luz    h= 1
    [ 2] casa   h= 2
    [ 3] .
    [ 4] mar    h= 4
    [ 5] sol    h= 5
    [ 6] mesa   h= 4  (+2)
    [ 7] rio    h= 5  (+2)
    [ 8] pelo   h= 8
```

Ahora α tiene que ser menor que 1, porque no caben más claves que casillas, y
el coste se dispara al acercarse a 1, sobre todo al buscar lo que **no** está:

```text
=== Sondeos medios frente a alfa (m = 100003 fijo) ===
  alfa    exito     teoria        fracaso   teoria        encadenamiento (exito)
  0.25    1.16      1.17          1.39      1.39          1.12
  0.50    1.50      1.50          2.54      2.50          1.25
  0.70    2.28      2.17          7.28      6.06          1.35
  0.80    3.46      3.00          17.97     13.00         1.40
  0.90    7.96      5.50          90.10     50.50         1.45
  0.95    18.64     10.50         473.93    200.50        1.48
```

Por eso esta tabla se redimensiona en α = 0,7, no en 1. A partir de ahí la
medida se aparta de la teoría (Knuth, 1962) por el **agrupamiento primario**:
las claves que chocan forman bloques contiguos, y cualquier clave que caiga en
cualquier casilla de un bloque acaba al final de él y lo alarga. Los bloques
grandes crecen más deprisa que los pequeños:

```text
  ...#.#...####..###...#########...#.....#..######.#....##.#...
  bloque ocupado mas largo: 9 casillas seguidas
```

### Ejercicio 4: borrar con lápida

Buscar se para en la primera casilla **vacía**, con este razonamiento: "si la
clave estuviera, la habría encontrado antes de este hueco".

**Tu tarea:** en `04_hash_direccionamiento_abierto.cpp`, completar `borrar()`.
El fichero trae `borrarMal()`, que deja la casilla vacía, para que veas qué
falla.

**Antes de escribir código:** `sol` y `rio` empiezan en la misma casilla (5).
`rio` entró después, la encontró ocupada y se quedó en la 7. Si se borra
`sol` dejando la casilla 5 vacía, ¿qué pasa al buscar `rio`?

<details>
<summary>Mostrar código (fragmento; completo en <code>04_hash_direccionamiento_abierto.cpp</code>)</summary>

```cpp
enum class Estado { VACIA, OCUPADA, BORRADA };

// Devuelve la casilla de 'clave', o -1 si no esta.
int posicionDe(const std::string& clave) const {
    int m = cubetas();
    int i = inicio(clave);
    for (int k = 0; k < m; k++, i = (i + 1) % m) {
        const Casilla& c = casillas_[i];
        if (c.estado == Estado::VACIA) return -1;   // hueco: no puede estar mas alla
        if (c.estado == Estado::OCUPADA && c.clave == clave) return i;
        // BORRADA, u OCUPADA por otra clave: seguir sondeando
    }
    return -1;
}

// TU TAREA:
bool borrar(const std::string& clave) {
    // TODO: implementar
    return false;
}
```

</details>

<details>
<summary>Mostrar resultado (con la solución correcta)</summary>

```text
=== Por que borrar NO es vaciar la casilla ===
  'sol' y 'rio' empiezan en la misma casilla (h=5); 'rio' entro despues y salto por encima.
  borrarMal(sol) deja la casilla VACIA ->  buscar(rio) = false   <-- !perdida, y sigue en la tabla!
  borrar(sol) deja una LAPIDA      ->  buscar(rio) = true
    ...
    [ 5] x  (lapida)
    [ 6] mesa   h= 4  (+2)
    [ 7] rio    h= 5  (+2)
  insertar(quepa), que empieza en h=5: comprueba que no esta (sigue
  hasta el hueco) y se queda en la lapida. lapidas ahora: 0

=== Las lapidas se acumulan: por que cuentan en alfa ===
  n=100  lapidas=0      sondeos buscando algo que no esta: 1.11
  n=100  lapidas=434    sondeos buscando algo que no esta: 2.17
  n=100  lapidas=666    sondeos buscando algo que no esta: 4.36
  n=100  lapidas=785    sondeos buscando algo que no esta: 9.19
  n=100  lapidas=836    sondeos buscando algo que no esta: 15.09
```

</details>

**La respuesta:** buscar `rio` empieza en la casilla 5, la encuentra vacía y
se para: concluye que `rio` no está, y **sigue en la tabla**, en la casilla 7.
Vaciar la casilla corta el camino de sondeo de todas las claves que saltaron
por encima de ella. La solución es una **lápida** (`BORRADA`): para buscar
cuenta como ocupada (se salta y se sigue), y para insertar cuenta como libre
(se reutiliza, después de comprobar que la clave no está más adelante).

El precio se ve en la segunda medida: las lápidas alargan los caminos de
sondeo igual que las claves de verdad. Con solo 100 claves vivas (α real =
0,1), buscar algo que no está pasa de 1,1 a 15 sondeos. Por eso el rehash se
dispara con (n + lápidas)/m, y al reconstruir la tabla las lápidas
desaparecen.

### Encadenamiento frente a direccionamiento abierto

|                                   | Encadenamiento                                 | Direccionamiento abierto                         |
| --------------------------------- | ---------------------------------------------- | ------------------------------------------------ |
| Dónde van las colisiones         | En una lista, fuera del array                  | En otra casilla del mismo array                  |
| α                                | Puede pasar de 1 (se suele redimensionar en 1) | Siempre < 1 (se suele redimensionar en 0,5–0,8) |
| Memoria por clave                 | Clave + puntero + un bloque del heap           | Clave + estado, contiguo                         |
| Caché                            | Mala: un salto por nodo                        | Buena: casillas consecutivas                     |
| Borrar                            | Quitar un nodo de una lista                    | Lápidas                                         |
| Sensible a una mala función hash | Poco                                           | Mucho (agrupamiento)                             |

El sondeo lineal no es el único: el **sondeo cuadrático** prueba
h + 1, h + 4, h + 9…, y el **doble hash** usa una segunda función hash para el
tamaño del salto. Los dos reducen el agrupamiento primario a cambio de perder
parte de la ventaja de caché.

---

## 5. La STL: `std::unordered_map` y `std::unordered_set`

Igual que en el tema 4 `std::map` era un árbol rojo-negro convertido en tipo,
`std::unordered_map` es la **tabla con encadenamiento** del fichero 03
convertida en tipo. Hasta deja ver sus cubetas:

<details>
<summary>Mostrar resultado (<code>05_unordered_stl.cpp</code>)</summary>

```text
=== 1) Uso basico, y la trampa de operator[] ===
  edad["pepe"] = 0  ... y ahora tamanio: 4   <-- !se ha colado pepe!

=== 2) Las cubetas, a la vista: el rehash del fichero 03 ===
  max_load_factor() = 1  (el ALFA_MAX del fichero 03)

  al insertar n=  bucket_count()    load_factor()
  1               2                 0.500
  3               5                 0.600
  6               11                0.545
  12              23                0.522
  24              47                0.511
  ...
  1598            3203              0.499

  Y ojo: std::hash<int>(12345) = 12345. Para enteros, el hash de la STL
  es la IDENTIDAD: todo el reparto lo hace el 'mod m' con m primo

=== 3) reserve(n): pedir las cubetas de antemano ===
  sin reserve:    21   rehash, 65.3 ms
  con reserve(n): 0    rehash, 43.5 ms

=== 4) El orden de recorrido no significa nada ===
  unordered_set: seis cuatro tres cinco dos uno
  tras un rehash: uno dos seis cuatro tres cinco
  map (arbol):    cinco cuatro dos seis tres uno

=== 5) Una clave propia: hash + operator== ===
  puntos    HashPunto (ms)      HashPesimo (ms)
  2500      0.07                3.43
  10000     0.34                70.79
  19600     0.53                280.58
  cubeta mas llena: HashPunto -> 2 claves;  HashPesimo -> 19600 claves
```

</details>

Lo que hay que llevarse:

- **`operator[]` inserta** si la clave no está. Para consultar sin modificar,
  `find()` o `count()`.
- El rehash ocurre solo, cuando `load_factor()` pasaría de
  `max_load_factor()` (1 por defecto). Si se sabe cuántas claves van a entrar,
  `reserve(n)` se ahorra todos los rehash, como `reserve` en `std::vector`.
- El **orden de recorrido** depende de los hash y del número de cubetas, y
  cambia al crecer la tabla. No significa nada.
- Una clave propia necesita **dos cosas**: una función hash (un *functor*
  como `HashPunto`) y un `operator==`. Si la función hash reparte mal, la
  tabla degenera en una lista: el O(1) de una tabla hash es un O(1)
  **esperado**, y el peor caso es O(n) por operación.

**Nota:** los números de cubetas son los de la biblioteca de macOS (libc++).
Con GCC en Linux o Windows (libstdc++) la secuencia es otra (empieza en 1 y
crece por otros primos), pero la idea es la misma.

---

## 6. El trie: un diccionario hecho para cadenas

Las dos familias que tenemos tratan la clave como un bloque. El árbol compara
cadenas enteras, y la tabla hash convierte la cadena entera en un número. Un
**trie** (de *re**trie**val*, también "árbol de prefijos") la recorre **letra a
letra**. Es el árbol general del tema 4, en el que:

- cada arista lleva una **letra**;
- el camino desde la raíz hasta un nodo deletrea un **prefijo**;
- cada nodo tiene una marca `finDePalabra`: "aquí termina una clave".

```text
   insertar casa, caso, cama, pez:          (*) = finDePalabra

        (raiz)
        /    \
       c      p
       |      |
       a      e
      / \     |
     s   m    z(*)
    / \   \
   a(*) o(*) a(*)
```

Lo que aporta:

1. **Buscar cuesta O(L)**, con L la longitud de la clave, sea cual sea el
   número de claves guardadas.
2. Los **prefijos comunes se guardan una vez**: "ca" sirve a casa, caso y
   cama.
3. Lo que la tabla hash no sabe hacer: **todas las claves que empiezan por
   un prefijo** (autocompletar). Se baja hasta el nodo del prefijo y se
   recorre su subárbol.

### ¿Cómo guarda cada nodo a sus hijos?

Es la misma pregunta que en el bloque 1 del tema 4, y `06_trie.cpp` prueba
tres respuestas:

| Representación                     | Bajar una letra         | Memoria por nodo                                          | Orden de los hijos    |
| ----------------------------------- | ----------------------- | --------------------------------------------------------- | --------------------- |
| `Nodo* hijos[26]`                 | O(1): un acceso a array | 26 punteros **siempre** (216 bytes)                 | Alfabético           |
| `std::map<char, Nodo*>`           | O(log σ)               | Solo los hijos que existen, cada uno en un nodo de árbol | **Alfabético** |
| `std::unordered_map<char, Nodo*>` | O(1) esperado           | Solo los hijos que existen, más el array de cubetas      | **Ninguno**     |

(σ es el tamaño del alfabeto.) Las versiones con `map` y `unordered_map` son
**la misma clase**: el tipo de diccionario es un parámetro de plantilla.

```cpp
template <template <class...> class Mapa>
class Trie {
    struct Nodo {
        Mapa<char, Nodo*> hijos;     // Mapa = std::map o std::unordered_map
        bool finDePalabra = false;
    };
    // ...
};

Trie<std::map> b;                    // hijos en un árbol rojo-negro
Trie<std::unordered_map> c;          // hijos en una tabla hash
```

### Ejercicio 5: autocompletar

**Tu tarea:** en `06_trie.cpp`, completar `recoger()`, el recorrido que usa
`autocompletar(prefijo)` para reunir todas las claves del subárbol del
prefijo.

**Antes de escribir código:** el recorrido lleva un `std::string palabra` con
las letras del camino desde la raíz. ¿Qué hay que hacerle al bajar a un hijo,
y qué al volver? ¿Qué recorrido del tema 4 es? Y la pregunta interesante:
¿saldrán las palabras en el mismo orden con `Trie<std::map>` que con
`Trie<std::unordered_map>`?

<details>
<summary>Mostrar código (fragmento; completo en <code>06_trie.cpp</code>)</summary>

```cpp
std::vector<std::string> autocompletar(const std::string& prefijo) const {
    std::vector<std::string> salida;
    const Nodo* n = bajar(prefijo);
    if (n == nullptr) return salida;        // ninguna empieza asi
    std::string palabra = prefijo;
    recoger(n, palabra, salida);
    return salida;
}

// TU TAREA:
static void recoger(const Nodo* n, std::string& palabra, std::vector<std::string>& salida) {
    // TODO: implementar
}
```

</details>

<details>
<summary>Mostrar resultado (con la solución correcta)</summary>

```text
=== (A) Trie con un array de 26 hijos por nodo ===
    c
      a
        l  *
        m
          a  *
          i
            o
              n  *
        s
          a  *
          c
            o  *
          o  *
    p
      e  *
        r
          a  *
          r
            o  *
        z  *
  (* = finDePalabra)
  buscar(casa)=true  buscar(cas)=false  empiezaPor(cas)=true  buscar(pe)=true  buscar(gato)=false
  10 palabras, 40 letras, pero solo 21 nodos (raiz incluida):
  los prefijos comunes (ca, cas, cam, pe) se guardan una sola vez.

=== autocompletar: lo que una tabla hash no sabe hacer ===
  Trie<map>,           "ca": cal cama camion casa casco caso
  Trie<unordered_map>, "ca": cal camion cama casco caso casa
  Trie<map>,           "pe": pe pera perro pez
  Trie<map>,           "x":

=== Lo que ocupa UN nodo (sin contar sus hijos) ===
  TrieArray:              216 bytes (26 punteros + bool)
  Trie<map>:              32 bytes + un nodo de arbol POR CADA hijo
  Trie<unordered_map>:    48 bytes + un array de cubetas + un nodo POR CADA hijo
```

</details>

**La respuesta:** es un **preorden**. Primero se visita el nodo (si tiene
`finDePalabra`, `palabra` es una clave completa) y después cada hijo. Al bajar
se hace `palabra.push_back(letra)` y al volver `palabra.pop_back()`: la cadena
funciona como una **pila** con las letras del camino. Con `std::map` los hijos
se recorren en orden alfabético, así que el preorden devuelve las palabras
**ordenadas** sin ordenar nada. Con `std::unordered_map` salen las mismas,
pero en el orden que dicten las cubetas de cada nodo.

Fíjate también en que `buscar("cas")` es `false` y `empiezaPor("cas")` es
`true`: estar en el trie como **prefijo** no es lo mismo que estar como
**clave**. Para eso existe `finDePalabra`.

---

## 7. Árbol, tabla hash y trie, frente a frente

`07_comparativa_diccionarios.cpp` mide las tres familias con las mismas 100 000
palabras: construir, memoria (los bytes pedidos con `new`, contados
sustituyendo el `operator new` global), buscar 100 000 palabras que están y
100 000 que no, y 300 consultas de "¿cuántas palabras empiezan por…?".

```text
=== 100000 palabras (media 9.7 letras), 100000 busquedas con exito, 100000 sin exito,
    300 consultas por prefijo ===

                             construir   memoria    bytes/    buscar    prefijos
                                  (ms)      (MB)   palabra      (ms)        (ms)
  ARBOL  std::map                 24.7       6.1      64.0      42.2        7.69
  HASH   std::unordered_map        6.8       5.4      56.2       5.9      164.17
  TRIE   hijos[26]                25.1      64.1     672.6       9.9        0.00
  TRIE   hijos en map             42.9      23.8     249.1      36.1        0.01
  TRIE   hijos en unordered       51.1      28.0     293.1      35.7        0.01

  todos encuentran lo mismo: true  (514684 palabras contadas por prefijo)
  nodos del trie: 311402 para 973467 letras (3.11 nodos por palabra):
  el resto de las letras caen en prefijos que ya estaban
```

Cómo leerla:

- **Buscar:** gana la tabla hash. El trie de 26 punteros va cerca, porque hace
  un acceso a array por letra. Los tries con `map` o `unordered_map` pagan una
  búsqueda en un contenedor **por cada letra**, y acaban tan lentos como el
  árbol, que hace unas 17 comparaciones de cadenas enteras (log₂ 100 000).
- **Prefijos:** la tabla hash pierde por goleada: unas 20 veces más lenta que
  el árbol y miles de veces más que el trie, porque no tiene orden y tiene que
  mirar las n claves en cada consulta. El árbol recorre un tramo **contiguo** desde `lower_bound(p)`, en
  O(log n + k). El trie baja |p| letras y lee un contador. Los dos son rápidos
  porque **tienen orden**.
- **Memoria:** el trie **no guarda las cadenas** y aun así es el que más gasta.
  Compartir prefijos ahorra letras (3,1 nodos por palabra en vez de 9,7), pero
  cada nodo cuesta mucho más que una letra. En la versión de array son 26
  punteros casi todos a `nullptr`; en las otras, un contenedor entero por nodo.

### La tabla final del tema

| TAD                                | En la STL                                      | Por debajo                    | Buscar                                        | Orden                                     |
| ---------------------------------- | ---------------------------------------------- | ----------------------------- | --------------------------------------------- | ----------------------------------------- |
| Diccionario ordenado               | `std::map`, `std::set`                     | Árbol rojo-negro             | O(log n) **garantizado**                | Sí: recorrido, rangos y prefijos baratos |
| Diccionario sin orden              | `std::unordered_map`, `std::unordered_set` | Tabla hash con encadenamiento | O(1) **esperado**, O(n) en el peor caso | No                                        |
| Diccionario de cadenas por prefijo | (no está en la STL)                           | Trie                          | O(longitud de la clave)                       | Sí, si los hijos están ordenados        |

### Cómo elegir, en orden

1. ¿Solo necesito **buscar, insertar y borrar por clave exacta**? →
   **`unordered_map`**. Es lo más rápido, y si sé cuántas claves vienen,
   con `reserve()`.
2. ¿Necesito **recorrer en orden**, el **siguiente** o el **anterior** de una
   clave, o un **rango** (todas entre "b" y "d")? → **`map`**.
3. ¿Las claves son **cadenas** y necesito consultas por **prefijo** de forma
   intensiva (autocompletar, diccionarios de idiomas, tablas de rutas IP)? →
   un **trie**. Si hay pocas consultas por prefijo, un `map` con `lower_bound`
   suele bastar y gasta mucho menos.
4. ¿Las claves son **enteros pequeños** (0..n−1)? → un **array** o un
   `vector`: es el direccionamiento directo, y no hay nada más rápido.

### Y fuera de clase: cómo se hace en C++ real

1. **La STL se quedó con el encadenamiento por compatibilidad, no por
   velocidad.** El estándar exige que los punteros e iteradores a un elemento
   sigan siendo válidos tras un rehash, y ofrece una interfaz de cubetas
   (`bucket_size(i)`…). Eso obliga a tener un nodo por elemento. Las tablas
   más rápidas de hoy son de **direccionamiento abierto**:
   `absl::flat_hash_map` de Google (las *Swiss tables*),
   `boost::unordered_flat_map` o las tablas *Robin Hood*, y en muchas pruebas
   son varias veces más rápidas que `std::unordered_map`.
2. **`std::hash<std::string>`** no es el polinómico de Java: las bibliotecas
   usan funciones como MurmurHash o CityHash, que mezclan los bits mucho
   mejor y procesan la cadena de 8 en 8 bytes.
3. **Ataques de colisión.** Si un atacante conoce la función hash de un
   servidor, puede enviarle miles de claves que caigan en la misma cubeta y
   convertir cada inserción en O(n) (el `HashPesimo` del fichero 05, pero a
   propósito). Por eso Python y Rust calculan el hash de las cadenas con una
   **semilla aleatoria** distinta en cada ejecución, y el `HashMap` de Java
   convierte en un árbol de búsqueda cualquier cubeta que pase de 8 claves.
4. **Tries en la práctica:** casi nunca se usa el de 26 punteros. Los
   *radix trees* (o tries compactos) juntan en una sola arista las cadenas
   de nodos con un único hijo. Los usan los routers, para la búsqueda del
   prefijo más largo en las tablas de rutas IP, el núcleo de Linux y muchas
   bases de datos en memoria.

---

## Resumen

- El **TAD Diccionario** guarda pares (clave, valor) sin claves repetidas y se
  usa por clave. Con lo visto hasta el tema 4, lo mejor es O(log n): `std::map`.
- El **direccionamiento directo** (la clave es el índice) da O(1) exacto, pero
  solo con claves enteras pequeñas: la memoria es la del universo de claves.
- Una **función hash** lleva cualquier clave a una de m casillas. Tiene que ser
  determinista, rápida y repartir bien. Con enteros, `k mod m` con m primo; con
  cadenas, el hash polinómico con la regla de Horner.
- Las **colisiones** son inevitables y llegan pronto, hacia la clave √(πm/2)
  (la paradoja del cumpleaños). Hay que resolverlas.
- **Encadenamiento:** una lista por cubeta. Coste O(1 + α). Cuando α pasa de 1,
  **rehash** al doble: O(n) cada vez, pero O(1) amortizado, igual que el array
  dinámico del tema 2.
- **Direccionamiento abierto:** todo dentro del array, con sondeo lineal.
  Mejor caché, pero α < 1, agrupamiento primario, y borrar exige **lápidas**,
  que también cuentan en α.
- `std::unordered_map` es una tabla con encadenamiento. Su O(1) es
  **esperado**: con una mala función hash, O(n). Y **no tiene orden**.
- El **trie** recorre las claves de texto letra a letra: O(longitud) y
  consultas por prefijo baratas, a cambio de mucha memoria por nodo. Con los
  hijos en un `std::map`, el preorden da las claves ordenadas.
- **Elegir:** clave exacta → `unordered_map`; orden o rangos → `map`; prefijos
  de cadenas → trie; enteros pequeños → array.
