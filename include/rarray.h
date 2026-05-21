#ifndef PROJEKT_RARRAY_H
#define PROJEKT_RARRAY_H

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief Optimálne zmeniteľné pole (Resizable Array).
 * * Táto trieda implementuje inteligentnú alternatívu k std::vector podľa vedeckých prác Tarjana a Zwicka.
 * Používa štruktúru s viacerými úrovňami blokov, čo zabezpečuje pamäťovú efektivitu N + O(N^(1/r)).
 * * @tparam T Typ prvkov uložených v poli.
 * @tparam R Parameter r (predvolene 3), ktorý určuje kompromis medzi pamäťou a rýchlosťou.
 */
template<typename T, size_t R = 3>
class ResizableArray {
public:
    // ==================== ZÁKLADNÉ VECI ====================

    /**
     * @brief Vytvorí prázdne ResizableArray.
     * Inicializuje základné parametre N a B.
     */
    ResizableArray();

    /**
     * @brief Deštruktor, korektne uvoľňuje všetky úrovne a bloky.
     */
    ~ResizableArray();

    /**
     * @brief Kopírovací konštruktor.
     * @param other Objekt, z ktorého sa kopíruje.
     */
    ResizableArray(const ResizableArray& other);

    /**
     * @brief Move konštruktor.
     * @param other Objekt, ktorého zdroje sa preberajú.
     */
    ResizableArray(ResizableArray&& other) noexcept;

    /**
     * @brief Operátor priradenia kópiou.
     * * Ak sa nejedná o samopriradenie, uvoľní aktuálne zdroje, nastaví parametre
     * podľa vzoru a vytvorí hĺbkovú kópiu všetkých prvkov.
     * * @param other Zdrojové pole, ktoré sa má skopírovať.
     * @return ResizableArray& Referencia na tento objekt (*this).
     */
    ResizableArray& operator=(const ResizableArray& other);

    /**
     * @brief Operátor priradenia presunutím (Move assignment).
     * * Efektívne preberie vlastníctvo vnútorných štruktúr a ukazovateľov z objektu other.
     * Pôvodný objekt ostane v prázdnom, platnom stave.
     * * @param other Zdrojové pole (r-value referencia).
     * @return ResizableArray& Referencia na tento objekt (*this) po presune.
     */
    ResizableArray& operator=(ResizableArray&& other) noexcept;

    // ==================== HLAVNÉ OPERÁCIE ====================

        /**
     * @brief Pridá prvok na koniec poľa.
     * * Ak je aktuálna úroveň zaplnená, pridá nový blok. Ak počet prvkov presiahne
     * matematický limit pre aktuálne B, vyvolá sa rebuild(B*2).
     * * @param item Hodnota, ktorá sa má pridať.
     */
    void push_back(const T& item);

        /**
     * @brief Odstráni posledný prvok z poľa.
     * * Zmenší logickú veľkosť poľa N. Ak N klesne pod kritickú hranicu (N < B^R / 4),
     * vyvolá sa metóda rebuild(B/2) na zmenšenie fyzickej kapacity a úsporu pamäte.
     * * @throw std::out_of_range Ak sa volá na prázdnom poli.
     */
    void shrink();

        /**
     * @brief Vráti referenciu na prvok na zadanom indexe.
     * * Metóda najprv skontroluje platnosť indexu, následne vypočíta, na ktorej
     * úrovni a v ktorom bloku sa prvok nachádza.
     * * @param index Logická pozícia v poli.
     * @return T& Referencia na hľadaný prvok.
     * @throw std::out_of_range Ak index >= N_.
     */
    T& get(size_t index);
    const T& get(size_t index) const;

    /**
     * @brief Nastaví hodnotu prvku na danom indexe.
     * @param index Logický index.
     * @param item Nová hodnota.
     */
    void set(size_t index, const T& item);

    // ==================== INE OPERÁCIE ====================

        /**
     * @brief Pridá všetky prvky z iného ResizableArray na koniec tohto poľa.
     * @param other Zdrojové pole typu ResizableArray.
     */
    void push_back_all(const ResizableArray& other) {
        for (size_t i = 0; i < other.length(); ++i) {
            push_back(other.get(i));
        }
    }

    /**
     * @brief Pridá prvky zo surového C-poľa so zadanou veľkosťou.
     * @param arr Ukazovateľ na začiatok poľa.
     * @param size Počet prvkov, ktoré sa majú pridať.
     */
    void push_back_all(const T* arr, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            push_back(arr[i]);
        }
    }

        /**
     * @brief Šablónový variant pre pridávanie z C-poľa so známou veľkosťou počas kompilácie.
     * @tparam N Veľkosť poľa odvodená kompilátorom.
     * @param arr Referencia na statické C-pole.
     */
    template<size_t N>
    void push_back_all(const T (&arr)[N]) {
        for (size_t i = 0; i < N; ++i) {
            push_back(arr[i]);
        }
    }

    /**
     * @brief Pridá všetky prvky zo štandardného kontajnera std::vector.
     * @param vec Zdrojový vektor.
     */
    void push_back_all(const std::vector<T>& vec) {
        for (const auto& x : vec) {
            push_back(x);
        }
    }

    /**
     * @brief Vytvorí pod-pole z daného rozsahu.
     * @param from Počiatočný index (vrátane).
     * @param to Koncový index (nie je zahrnutý).
     * @return ResizableArray Nové pole s vybranými prvkami.
     * @throw std::out_of_range Ak sú indexy mimo hraníc N_.
     */
    ResizableArray sub_rarray(size_t from, size_t to) const;

    /**
     * @brief Vyfiltruje prvky na základe predikátu.
     * @tparam Predicate Funkčný objekt alebo lambda.
     * @param pred Podmienka, ktorú musí prvok spĺňať.
     * @return ResizableArray Pole obsahujúce iba vyhovujúce prvky.
     */
    template<typename Predicate>
    ResizableArray<T, R> filter(Predicate pred) const;

    /**
     * @brief Sploští pole polí do jedného lineárneho poľa.
     * @tparam U Typ vnútorných prvkov.
     * @return ResizableArray<U, R> Nové sploštené pole.
     */
    template<typename U>
    ResizableArray<U, R> flatten() const;

        /**
     * @brief In-place Merge Sort prispôsobený pre hierarchickú blokovú štruktúru ResizableArray.
     *
     * Táto metóda realizuje triedenie prvkov bez alokácie dodatočnej dynamickej pamäte pre dáta (O(1) pomocná pamäť pre typ T).
     * Využíva princíp Bottom-up (zdola-nahor) Merge Sortu, pričom rešpektuje rozloženie blokov v pamäti:
     * * 1. Lokálne zoradenie: Každý fyzický dátový blok (DataBlock) sa najprv zoradí samostatne pomocou std::sort,
     * čo je vďaka sekvenčnému usporiadaniu v pamäti veľmi rýchle.
     * 2. Projekcia: Vytvorí sa virtuálna mapa indexov (začiatok a dĺžka) jednotlivých blokov.
     * 3. In-place zlúčenie: Bloky sa hierarchicky spájajú v cykle. Najprv susediace bloky rovnakej veľkosti
     * v rámci jednej úrovne, a následne bloky rôznych veľkostí na hraniciach jednotlivých úrovní.
     *
     * @note Algoritmus manipuluje s prvkami výhradne pomocou presúvacej sémantiky (std::move).
     */
    void sort();

    // ==================== UŽITOČNÉ INFO ====================

    /**
     * @brief Vráti aktuálny počet prvkov v poli.
     * @return Počet prvkov (size_t).
     */
    size_t length() const { return N_; }

    /**
     * @brief Skontroluje, či pole neobsahuje žiadne prvky.
     * @return true Ak je N_ rovné 0, inak false.
     */
    bool empty() const { return N_ == 0; }

    /**
     * @brief Vráti aktuálnu hodnotu parametra B (veľkosť bázy).
     * * Užitočné predovšetkým na účely testovania, ladenia a overovania,
     * či štruktúra správne vykonáva operácie rebuild.
     * * @return size_t Aktuálna hodnota vnútornej bázy B_.
     */
    size_t getParameterB() const { return B_; }

    // ==================== OPERÁTORY ====================

        /**
     * @brief Indexový operátor pre prístup k prvkom (nekonštantná verzia).
     * * Umožňuje prístup k prvkom pomocou syntaxe arr[i].
     * @param index Pozícia prvku v poli.
     * @return T& Referencia na prvok na danom indexe.
     */
    T& operator[](size_t index) { return get(index); }

        /**
     * @brief Indexový operátor pre prístup k prvkom (konštantná verzia).
     * @param index Pozícia prvku v poli.
     * @return const T& Konštantná referencia na prvok.
     */
    const T& operator[](size_t index) const { return get(index); }

    // ==================== ITERÁTORY ====================

        /**
     * @brief Trieda Iterator umožňujúca prechod prvkami poľa.
     * * Implementuje základné operácie pre spoluprácu s range-based for cyklami.
     */
    class Iterator {
    public:
        /** @brief Inicializuje iterátor pre konkrétne pole a index. */
        Iterator(ResizableArray* arr, size_t index)
            : arr_(arr), index_(index) {}
        /** @brief Dereferenčný operátor pre prístup k hodnote. */
        T& operator*() {
            return arr_->get(index_);
        }
        /** @brief Operátor inkrementácie (posun na ďalší prvok). */
        Iterator& operator++() {
            ++index_;
            return *this;
        }
        /** @brief Operátor porovnania nerovnosti. */
        bool operator!=(const Iterator& other) const {
            return index_ != other.index_;
        }

    private:
        ResizableArray* arr_;
        size_t index_;
    };

        /**
     * @brief Konštantná verzia iterátora pre prístup k dátam len na čítanie.
     */
    class ConstIterator {
    public:
        ConstIterator(const ResizableArray* arr, size_t index)
            : arr_(arr), index_(index) {}

        const T& operator*() const {
            return arr_->get(index_);
        }

        ConstIterator& operator++() {
            ++index_;
            return *this;
        }

        bool operator!=(const ConstIterator& other) const {
            return index_ != other.index_;
        }

    private:
        const ResizableArray* arr_;
        size_t index_;
    };

    /** @brief Vráti iterátor na začiatok poľa. */
    Iterator begin() {
        return Iterator(this, 0);
    }

    /** @brief Vráti iterátor na koniec poľa (za posledný prvok). */
    Iterator end() {
        return Iterator(this, N_);
    }

    /** @brief Vráti konštantný iterátor na začiatok poľa. */
    ConstIterator begin() const {
        return ConstIterator(this, 0);
    }

    /** @brief Vráti konštantný iterátor na koniec poľa. */
    ConstIterator end() const {
        return ConstIterator(this, N_);
    }



private:
    // ==================== VNÚTORNÉ ŠTRUKTÚRY ====================

    /**
     * @brief Štruktúra reprezentujúca jeden fyzický pamäťový blok.
     */
    struct DataBlock {
        T* data;           ///< Ukazovateľ na surové dáta.
        size_t capacity;   ///< Maximálna kapacita bloku.

        /**
         * @brief Konštruuje nový DataBlock s danou kapacitou.
         * @param cap Požadovaná kapacita bloku.
         */
        DataBlock(size_t cap);

        /**
         * @brief Deštruktor, uvoľňuje alokované pole dát.
         */
        ~DataBlock();

        // Zakázanie kopírovania pre bezpečnú správu pamäte
        DataBlock(const DataBlock&) = delete;
        DataBlock& operator=(const DataBlock&) = delete;

        /**
         * @brief Move konštruktor na efektívny presun zdrojov.
         * @param other Zdrojový blok.
         */
        DataBlock(DataBlock&& other) noexcept;

        /**
         * @brief Move operátor priradenia.
         * @param other Zdrojový blok.
         * @return Referencia na tento objekt.
         */
        DataBlock& operator=(DataBlock&& other) noexcept;
    };

        /**
     * @brief Bezpečná implementácia nízkoúrovňového dynamického poľa pre správu blokov.
     * * Táto pomocná štruktúra slúži na ukladanie ukazovateľov na objekty typu BlockType.
     * Je navrhnutá tak, aby automaticky spravovala alokáciu a uvoľňovanie pamäte
     * pre bloky používané v hierarchii ResizableArray.
     * * @tparam BlockType Typ elementu, ktorý pole spravuje (zvyčajne DataBlock).
     */
    template<typename BlockType>
    struct DynamicArray {
        BlockType** data;   ///< Dynamicky alokované pole ukazovateľov na prvky typu BlockType.
        size_t size;        ///< Aktuálny počet uložených ukazovateľov v poli.
        size_t capacity;    ///< Celková alokovaná kapacita poľa ukazovateľov.

            /**
         * @brief Konštruuje prázdne dynamické pole.
         * Inicializuje ukazovateľ na nullptr a veľkosti na 0.
         */
        DynamicArray();

            /**
         * @brief Deštruktor dynamického poľa.
         * Volá metódu clear(), čím zabezpečí uvoľnenie všetkých spravovaných blokov a samotného poľa.
         */
        ~DynamicArray();

    /**
     * @brief Zväčší kapacitu poľa na požadovanú hodnotu.
     * Ak je nová kapacita väčšia ako súčasná, realokuje pamäť pre pole ukazovateľov.
     * * @param newCap Požadovaná minimálna kapacita.
     */
    void reserve(size_t newCap);

    /**
     * @brief Pridá nový ukazovateľ na blok na koniec poľa.
     * V prípade potreby automaticky zväčší kapacitu poľa (zvyčajne zdvojnásobí).
     * * @param block Ukazovateľ na objekt typu BlockType, ktorý sa má pridať.
     */
    void push_back(BlockType* block);

    /**
     * @brief Odstráni posledný ukazovateľ z poľa a uvoľní prislúchajúcu pamäť.
     * @note Metóda fyzicky maže objekt (delete), na ktorý ukazovateľ smeroval.
     * @throw std::underflow_error Ak je pole prázdne.
     */
    void pop_back();

    /**
     * @brief Odstráni a uvoľní rozsah blokov z poľa.
     * Zmaže všetky objekty v rozsahu [start, end) a posunie zostávajúce prvky.
     * * @param start Počiatočný index (vrátane).
     * @param end Koncový index (nie je zahrnutý).
     * @throw std::out_of_range Ak sú indexy mimo platných hraníc.
     */
    void erase(size_t start, size_t end);

    /**
     * @brief Uvoľní všetky spravované bloky a vyčistí pole.
     * Po zavolaní tejto metódy bude veľkosť poľa 0.
     */
    void clear();

    /**
     * @brief Indexový operátor pre rýchly prístup k ukazovateľu bloku.
     * @param index Pozícia v poli.
     * @return BlockType* Ukazovateľ na blok na danej pozícii.
     */
    BlockType* operator[](size_t index);

    /**
     * @brief Konštantný indexový operátor pre prístup k ukazovateľu bloku.
     * @param index Pozícia v poli.
     * @return const BlockType* Konštantný ukazovateľ na blok.
     */
    const BlockType* operator[](size_t index) const;

    /**
     * @brief Bezpečný prístup k referencii na ukazovateľ bloku s kontrolou hraníc.
     * Umožňuje modifikáciu ukazovateľa uloženého na danej pozícii.
     * * @param index Pozícia v poli.
     * @return BlockType*& Referencia na ukazovateľ na objekt typu BlockType.
     * @throw std::out_of_range Ak je index mimo rozsahu [0, size-1].
     */
    BlockType*& at(size_t index);

    // Zakázanie kopírovania na zabránenie plytkým kópiám a dvojitému uvoľneniu pamäte.
        DynamicArray(const DynamicArray&) = delete;
        DynamicArray& operator=(const DynamicArray&) = delete;
    };

    // ==================== VNÚTORNÁ LOGIKA ====================

        /**
     * @brief Skombinuje B blokov z jednej úrovne do jedného väčšieho bloku na vyššej úrovni.
     * * Implementuje princíp "redundant base-B counter". Táto operácia je kľúčová pre
     * udržanie štruktúry poľa podľa Tarjanovho algoritmu pri raste poľa.
     */
    void combineBlocks();

        /**
     * @brief Rozdelí veľký blok z vyššej úrovne na B menších blokov na nižšej úrovni.
     * * Táto inverzná operácia k combineBlocks sa vykonáva pri zmenšovaní poľa,
     * keď potrebujeme uvoľniť kapacitu alebo reorganizovať dáta.
     */
    void splitBlocks();

        /**
     * @brief Reorganizuje fyzickú štruktúru blokov.
     * * Táto operácia sa volá, keď pole prerastie kapacitu aktuálnej bázy B.
     * Vytvorí novú konfiguráciu úrovní s novou bázou, prekopíruje dáta a staré bloky zmaže.
     * * @param newB Nová hodnota bázy (zvyčajne B*2 alebo B/2).
     */
    void rebuild(size_t newB);

        /**
     * @brief Pomocná metóda pre výpočet mocniny celých čísel.
     * @param base Základ.
     * @param exp Exponent.
     * @return Výsledok base^exp.
     */
    size_t power(size_t base, size_t exp) const;

    /**
     * @brief Určí presné umiestnenie indexu v hierarchii úrovní.
     */
    std::pair<size_t, size_t> locateItem(size_t index) const;

    /**
     * @brief Prvotná alokácia a nastavenie počítadiel pre jednotlivé úrovne.
     * * Pripraví vnútorné pole levels_ a vynuluje počty blokov n_ pre každú hladinu.
     */
    void initializeLevels();

    /**
     * @brief Uvoľní všetky fyzické DataBlocky na všetkých úrovniach hierarchie.
     * * Slúži na kompletnú dezalokáciu pamäte pred zánikom objektu alebo pred rebuildom.
     */
    void cleanupLevels();

    /**
     * @brief Pomocná funkcia na vytvorenie kópie obsahu iného poľa.
     * * @param other Zdroj, z ktorého sa majú skopírovať všetky prvky a nastavenia.
     */
    void copyFrom(const ResizableArray& other);

    // ==================== PREMENNÉ ====================

    static constexpr size_t r_ = R;  ///< Parameter r (2, 3, 4, ...) - nastavuje trade-off.

    size_t N_;   ///< Celkový počet prvkov.
    size_t B_;   ///< Aktuálna veľkosť bázy blokov.

    // Pole úrovní - každá úroveň má bloky rôznych veľkostí
    // úroveň 1: bloky veľkosti B
    // úroveň 2: bloky veľkosti B²
    // úroveň 3: bloky veľkosti B³
    // atď.
    DynamicArray<DataBlock>* levels_; ///< Polia blokov pre jednotlivé úrovne.


    size_t* n_; ///< Počty blokov na jednotlivých úrovniach.

    size_t n0_; ///< Koľko prvkov je v poslednom (čiastočne zaplnenom) bloku úrovne 1

    // ==================== KONŠTANTY ====================

    static constexpr size_t INITIAL_B = 4; ///< Začíname s B=4 (pre malé pole. Keď pole rastie, B sa zdvojnásobuje.
};
#include "rarray_impl.tpp"
#endif // PROJEKT_RARRAY_H