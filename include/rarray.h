#ifndef PROJEKT_RARRAY_H
#define PROJEKT_RARRAY_H

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

// Optimálne zmeniteľné pole - inteligentná alternatíva k std::vector
// Používa menej pamäte (N + O(N^1/r) namiesto až 2N) ale za cenu trochu pomalších push_back/shrink operácií
// Parameter R určuje trade-off: väčšie R = menej pamäte, ale pomalšie operácie
template<typename T, size_t R = 3>
class ResizableArray {
public:
    // ==================== ZÁKLADNÉ VECI ====================

    // Vytvorí prázdne pole
    ResizableArray();

    // Uprace všetko
    ~ResizableArray();

    // Skopíruje celé pole (pomalá operácia!)
    ResizableArray(const ResizableArray& other);

    // Presunie pole (rýchle, bez kopírovania)
    ResizableArray(ResizableArray&& other) noexcept;

    // Priradenie kópiou
    ResizableArray& operator=(const ResizableArray& other);

    // Priradenie presunutím
    ResizableArray& operator=(ResizableArray&& other) noexcept;

    // ==================== HLAVNÉ OPERÁCIE ====================

    // Pridá prvok na koniec (ako push_back)
    // Trvá O(r) v priemere, niekedy môže trvať dlhšie keď sa musia presúvať bloky
    void push_back(const T& item);

    // Odstráni posledný prvok (ako pop_back)
    // Hodí výnimku ak je pole prázdne
    void shrink();

    // Vráti prvok na danej pozícii (rýchle - O(1))
    // Hodí výnimku ak index mimo rozsahu
    T& get(size_t index);

    // To isté ale pre konštantné objekty
    const T& get(size_t index) const;

    // Zmení hodnotu prvku na danej pozícii
    void set(size_t index, const T& item);

    // ==================== INE OPERÁCIE ====================

    // Pre iný ResizableArray
    void push_back_all(const ResizableArray& other) {
        for (size_t i = 0; i < other.length(); ++i) {
            push_back(other.get(i));
        }
    }

    // Pre obyčajné pole
    void push_back_all(const T* arr, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            push_back(arr[i]);
        }
    }
    // Iny variant
    template<size_t N>
    void push_back_all(const T (&arr)[N]) {
        for (size_t i = 0; i < N; ++i) {
            push_back(arr[i]);
        }
    }

    // Pre vector
    void push_back_all(const std::vector<T>& vec) {
        for (const auto& x : vec) {
            push_back(x);
        }
    }

    ResizableArray sub_rarray(size_t from, size_t to) const;

    template<typename Predicate>
    ResizableArray<T, R> filter(Predicate pred) const;

    template<typename U>
    ResizableArray<U, R> flatten() const;

    // ==================== UŽITOČNÉ INFO ====================

    // Koľko prvkov je v poli
    size_t length() const { return N_; }

    // Je pole prázdne?
    bool empty() const { return N_ == 0; }

    // Parameter B (veľkosť najmenších blokov)
    // Užitočné na debugovanie a testovanie
    size_t getParameterB() const { return B_; }

    // ==================== OPERÁTORY ====================

    // arr[5] = 10; - funguje rovnako, ako get/set
    T& operator[](size_t index) { return get(index); }
    const T& operator[](size_t index) const { return get(index); }

    // ==================== ITERÁTORY ====================

    class Iterator {
    public:
        Iterator(ResizableArray* arr, size_t index)
            : arr_(arr), index_(index) {}

        T& operator*() {
            return arr_->get(index_);
        }

        Iterator& operator++() {
            ++index_;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return index_ != other.index_;
        }

    private:
        ResizableArray* arr_;
        size_t index_;
    };

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

    // begin / end
    Iterator begin() {
        return Iterator(this, 0);
    }

    Iterator end() {
        return Iterator(this, N_);
    }

    ConstIterator begin() const {
        return ConstIterator(this, 0);
    }

    ConstIterator end() const {
        return ConstIterator(this, N_);
    }



//private:
    // ==================== VNÚTORNÉ ŠTRUKTÚRY ====================

    // Jeden blok pamäte, ktorý drží prvky
    // Bloky sú rôznych veľkostí: B, B², B³, ...
    struct DataBlock {
        T* data;           // Tu sú uložené prvky
        size_t capacity;   // Koľko prvkov sa sem zmestí

        // Vytvorí nový blok danej veľkosti
        DataBlock(size_t cap);

        // Zmaže blok a uvoľní pamäť
        ~DataBlock();

        // Bloky sa nekopírujú, len presúvajú (kvôli výkonu)
        DataBlock(const DataBlock&) = delete;
        DataBlock& operator=(const DataBlock&) = delete;

        // Presunie blok (rýchle)
        DataBlock(DataBlock&& other) noexcept;
        DataBlock& operator=(DataBlock&& other) noexcept;
    };

    // Vlastná implementácia dynamického poľa (lebo nie je mozne používať std::vector)
    // Ukladá ukazovatele na bloky
    template<typename BlockType>
    struct DynamicArray {
        BlockType** data;   // Pole ukazovateľov
        size_t size;        // Koľko blokov tu máme
        size_t capacity;    // Koľko blokov sa zmestí

        // Začne prázdne
        DynamicArray();

        // Uprace všetky bloky
        ~DynamicArray();

        // Zabezpečí, že sa zmestí aspoň newCap blokov
        void reserve(size_t newCap);

        // Pridá blok na koniec
        void push_back(BlockType* block);

        // Odstráni a zmaže posledný blok
        void pop_back();

        // Odstráni bloky od start po end (nie vrátane end)
        void erase(size_t start, size_t end);

        // Zmaže všetky bloky
        void clear();

        // Prístup k blokom
        BlockType* operator[](size_t index);
        const BlockType* operator[](size_t index) const;
        BlockType*& at(size_t index);  // S kontrolou hraníc

        // Nekopíruje sa
        DynamicArray(const DynamicArray&) = delete;
        DynamicArray& operator=(const DynamicArray&) = delete;
    };

    // ==================== VNÚTORNÁ LOGIKA ====================

    // Keď sa naplní úroveň, skombinuj B blokov do jedného väčšieho
    // Toto je kľúčová operácia - implementuje "redundant base-B counter"
    void combineBlocks();

    // Opak combineBlocks - rozdelí veľký blok na malé
    // Volá sa keď už nie sú malé bloky a potrebujeme ich
    void splitBlocks();

    // Keď pole príliš narástlo alebo sa zmenšilo, musíme prebudovať všetko
    // s novým parametrom B (zdvojnásobí sa alebo zmenší na polovicu)
    void rebuild(size_t newB);

    // Vypočíta base^exp (napr. B^3)
    // Potrebujeme to často na výpočet veľkostí blokov
    size_t power(size_t base, size_t exp) const;

    // Nájde kde je prvok s daným indexom
    // Vráti (úroveň, pozícia_v_tej_úrovni)
    std::pair<size_t, size_t> locateItem(size_t index) const;

    // Nastaví pole úrovní a počítadlá
    void initializeLevels();

    // Uprace všetko
    void cleanupLevels();

    // Skopíruje obsah z iného poľa (pomocná funkcia)
    void copyFrom(const ResizableArray& other);

    // ==================== PREMENNÉ ====================

    static constexpr size_t r_ = R;  // Parameter r (2, 3, 4, ...) - nastavuje trade-off

    size_t N_;   // Koľko prvkov je celkovo v poli
    size_t B_;   // Veľkosť základného bloku - mení sa keď pole rastie/klesá

    // Pole úrovní - každá úroveň má bloky rôznych veľkostí
    // úroveň 1: bloky veľkosti B
    // úroveň 2: bloky veľkosti B²
    // úroveň 3: bloky veľkosti B³
    // atď.
    DynamicArray<DataBlock>* levels_;

    // Koľko blokov je na každej úrovni
    size_t* n_;

    // Koľko prvkov je v poslednom (čiastočne zaplnenom) bloku úrovne 1
    size_t n0_;

    // ==================== KONŠTANTY ====================

    // Začíname s B=4 (pre malé pole)
    // Keď pole rastie, B sa zdvojnásobuje
    static constexpr size_t INITIAL_B = 4;
};
#include "rarray_impl.tpp"
#endif // PROJEKT_RARRAY_H