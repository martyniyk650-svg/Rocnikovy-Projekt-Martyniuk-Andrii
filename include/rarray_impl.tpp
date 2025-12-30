#pragma once
#include <stdexcept>
#include <new>
#include <utility>

#include "rarray.h"

// ===================== DataBlock =====================
//
// Reprezentuje jeden pamäťový blok, ktorý uchováva prvky typu T.
// V triede ResizableArray sú tieto bloky používané na uloženie dát
// rôznych veľkostí: B, B^2, B^3, ...
//
template<typename T, size_t R>
ResizableArray<T, R>::DataBlock::DataBlock(size_t cap)
    : data(cap ? new T[cap] : nullptr), capacity(cap) {}

template<typename T, size_t R>
ResizableArray<T, R>::DataBlock::~DataBlock() {
    // Uvoľní dynamicky alokovanú pamäť
    delete[] data;
}

template<typename T, size_t R>
ResizableArray<T, R>::DataBlock::DataBlock(DataBlock&& other) noexcept
    : data(other.data), capacity(other.capacity) {
    // "Move" – presunie ukazovateľ a vyčistí pôvodný blok
    other.data = nullptr;
    other.capacity = 0;
}

template<typename T, size_t R>
typename ResizableArray<T, R>::DataBlock&
ResizableArray<T, R>::DataBlock::operator=(DataBlock&& other) noexcept {
    if (this != &other) {
        delete[] data; // odstráni pôvodné dáta
        data = other.data;
        capacity = other.capacity;
        other.data = nullptr;
        other.capacity = 0;
    }
    return *this;
}

// ===================== DynamicArray =====================
//
// Vlastná implementácia dynamického poľa ukazovateľov na bloky.
// Funguje podobne ako std::vector, ale nepoužíva STL.
// Používa sa v ResizableArray na uloženie blokov každej úrovne.
//
template<typename T, size_t R>
template<typename BlockType>
ResizableArray<T, R>::DynamicArray<BlockType>::DynamicArray()
    : data(nullptr), size(0), capacity(0) {}

template<typename T, size_t R>
template<typename BlockType>
ResizableArray<T, R>::DynamicArray<BlockType>::~DynamicArray() {
    // Vyčistí všetky bloky a uvoľní pamäť
    clear();
    delete[] data;
}

template<typename T, size_t R>
template<typename BlockType>
void ResizableArray<T, R>::DynamicArray<BlockType>::reserve(size_t newCap) {
    // Ak je potrebné viac miesta, vytvorí nové pole ukazovateľov
    if (newCap <= capacity) return;
    BlockType** newData = new BlockType*[newCap];
    // Dôležité: inicializuj nové sloty na nullptr, aby náhodný "trash" pointer
    // nikdy nemohol spôsobiť double-delete pri chybe/nekonzistencii.
    for (size_t i = 0; i < newCap; ++i) newData[i] = nullptr;
    for (size_t i = 0; i < size; ++i) newData[i] = data[i];
    delete[] data;
    data = newData;
    capacity = newCap;
}

template<typename T, size_t R>
template<typename BlockType>
void ResizableArray<T, R>::DynamicArray<BlockType>::push_back(BlockType* block) {
    // Pridá nový ukazovateľ na blok na koniec
    if (size >= capacity)
        reserve(capacity ? capacity * 2 : 4);
    data[size++] = block;
}

template<typename T, size_t R>
template<typename BlockType>
void ResizableArray<T, R>::DynamicArray<BlockType>::pop_back() {
    // Odstráni a zmaže posledný blok
    if (size == 0)
        throw std::out_of_range("pop_back() on empty DynamicArray");
    delete data[--size];
    data[size] = nullptr; // defensive: clear dangling pointer slot
}

template<typename T, size_t R>
template<typename BlockType>
void ResizableArray<T, R>::DynamicArray<BlockType>::erase(size_t start, size_t end) {
    // Vymaže bloky v rozsahu [start, end)
    if (start >= size || end > size || start >= end)
        throw std::out_of_range("Invalid erase range");
    for (size_t i = start; i < end; ++i)
        delete data[i];
    for (size_t i = end; i < size; ++i)
        data[start + i - end] = data[i];
    // Vymaž "tail" sloty, aby tam nezostali duplicitné/dangling pointers
    const size_t removed = (end - start);
    for (size_t i = size - removed; i < size; ++i) {
        data[i] = nullptr;
    }
    size -= removed;
}

template<typename T, size_t R>
template<typename BlockType>
void ResizableArray<T, R>::DynamicArray<BlockType>::clear() {
    // Odstráni všetky bloky a nastaví size = 0
    for (size_t i = 0; i < size; ++i) {
        delete data[i];
        data[i] = nullptr;
    }
    size = 0;
}

template<typename T, size_t R>
template<typename BlockType>
BlockType* ResizableArray<T, R>::DynamicArray<BlockType>::operator[](size_t index) {
    // Priamy prístup bez kontroly hraníc
    return data[index];
}

template<typename T, size_t R>
template<typename BlockType>
const BlockType* ResizableArray<T, R>::DynamicArray<BlockType>::operator[](size_t index) const {
    return data[index];
}

template<typename T, size_t R>
template<typename BlockType>
BlockType*& ResizableArray<T, R>::DynamicArray<BlockType>::at(size_t index) {
    // Bezpečný prístup – kontroluje hranice
    if (index >= size)
        throw std::out_of_range("DynamicArray index out of range");
    return data[index];
}


// ===============================================
// ResizableArray – constructor
// ===============================================
template<typename T, size_t R>
ResizableArray<T, R>::ResizableArray()
    : N_(0), B_(INITIAL_B), levels_(nullptr), n_(nullptr), n0_(0)
{
    initializeLevels();
}

// ===============================================
// ResizableArray – destructor
// ===============================================
template<typename T, size_t R>
ResizableArray<T, R>::~ResizableArray() {
    cleanupLevels();
    delete[] levels_;
    delete[] n_;
    levels_ = nullptr;
    n_ = nullptr;
}

template<typename T, size_t R>
void ResizableArray<T, R>::initializeLevels() {
    // Pozn.: levels_[0] je síce "nepoužitý" v algoritme, ale pre bezpečnosť
    // ho vždy držíme v konzistentnom stave (aby sa tam nikdy nehromadili bloky).
    if (!levels_) levels_ = new DynamicArray<DataBlock>[R];
    if (!n_)      n_      = new size_t[R];

    for (size_t i = 0; i < R; ++i) {
        n_[i] = 0;
        // Vycisti existujúce bloky (ak nejaké boli).
        levels_[i].clear();
        // Rezervujeme len pre reálne používané úrovne 1..R-1.
        if (i > 0) {
            levels_[i].reserve(2 * B_);
        }
    }

    N_  = 0;
    n0_ = 0;
}

template<typename T, size_t R>
void ResizableArray<T, R>::cleanupLevels() {
    if (!levels_ || !n_) {
        N_ = 0;
        n0_ = 0;
        return;
    }

    // Vycisti VŠETKY úrovne, vrátane úrovne 0 (defensive).
    for (size_t i = 0; i < R; ++i) {
        levels_[i].clear();
        n_[i] = 0;
    }
    N_  = 0;
    n0_ = 0;
}


// ==================== VNÚTORNÁ LOGIKA ====================

template<typename T, size_t R>
size_t ResizableArray<T, R>::power(size_t base, size_t exp) const {
    size_t result = 1;

    while (exp > 0) {
        if (exp % 2 == 1)          // exp je nepárny
            result *= base;

        exp /= 2;                  // posunieme exponent
        base *= base;              // umocníme bázu
    }

    return result;
}

template<typename T, size_t R>
std::pair<size_t, size_t> ResizableArray<T, R>::locateItem(size_t index) const {
    if (index >= N_) {
        throw std::out_of_range("locateItem: index out of range");
    }

    // locateItem musí reflektovať rovnaké poradie, aké používa get():
    // najprv úrovne R-1..2 ("väčšie" bloky), potom úroveň 1 (B-bloky).
    size_t remaining = index;

    for (size_t lvl = R - 1; lvl >= 2; --lvl) {
        const size_t blockSize  = power(B_, lvl); // B^lvl
        const size_t levelItems = n_[lvl] * blockSize;
        if (remaining < levelItems) {
            // offset v rámci spojenia blokov na tejto úrovni
            return {lvl, remaining};
        }
        remaining -= levelItems;
        if (lvl == 2) break; // underflow guard
    }

    // Zvyšok patrí do úrovne 1.
    return {1u, remaining};
}

template<typename T, size_t R>
void ResizableArray<T, R>::combineBlocks() {
    // k = min{i in [r-1] | n_i < 2B}, tu i=1..R-1
    size_t k = 0;
    for (size_t i = 1; i < R; ++i) {
        if (n_[i] < 2 * B_) { k = i; break; }
    }
    if (k == 0) throw std::runtime_error("combineBlocks: no k found");

    // for i = k-1 down to 1
    for (size_t i = k - 1; i >= 1; --i) {
        const size_t smallSize = power(B_, i);     // B^i
        const size_t bigSize   = power(B_, i + 1); // B^(i+1)

        auto* big = new DataBlock(bigSize);

        // skopíruj prvých B blokov A[i][0..B-1] do big (v poradí)
        for (size_t j = 0; j < B_; ++j) {
            DataBlock* src = levels_[i].at(j);
            for (size_t p = 0; p < smallSize; ++p) {
                big->data[j * smallSize + p] = src->data[p];
            }
            // dealokuj A[i][j]
            delete src;
        }

        // shift: A[i][j] = A[i][j+B] pre j=0..B-1
        for (size_t j = 0; j < B_; ++j) {
            levels_[i].at(j) = levels_[i].at(j + B_);
            levels_[i].at(j + B_) = nullptr; // aby clear() neskôr nič nedvoj-zmazal
        }

        // n_i = B, a veľkosť containeru nastav na B
        n_[i] = B_;
        levels_[i].size = B_; // (ak size je public, ako v tvojich testoch)

        // A[i+1][n_{i+1}] = big; n_{i+1}++
        levels_[i + 1].push_back(big);
        n_[i + 1] += 1;

        if (i == 1) break; // size_t underflow ochrana
    }
}

template<typename T, size_t R>
void ResizableArray<T, R>::splitBlocks() {
    // Implementácia podľa PDF (Figure 5):
    // nájdi najmenšie k >= 2 s n_k > 0 a rozbi jeden blok veľkosti B^k na:
    //  - (B-1) blokov na každej medz úrovni (k-1..2)
    //  - B blokov na úrovni 1
    size_t k = 0;
    for (size_t i = 2; i < R; ++i) {
        if (n_[i] > 0) { k = i; break; }
    }
    if (k == 0) throw std::runtime_error("splitBlocks: nothing to split");

    // Vyberieme posledný (najnovší) veľký blok z úrovne k.
    // "pop" posledného pointera z levels_[k] bez delete (blok budeme splitovať)
    DataBlock*& slotK = levels_[k].at(n_[k] - 1);
    DataBlock* big = slotK;
    slotK = nullptr;
    levels_[k].size--;
    n_[k]--;

    // Postupne delíme "big" smerom nadol.
    for (size_t i = k - 1; i >= 1; --i) {
        const size_t smallSize = power(B_, i);

        // Vytvor B menších blokov (B_ je runtime, nechceme VLA).
        DataBlock** tmp = new DataBlock*[B_];
        for (size_t j = 0; j < B_; ++j) tmp[j] = nullptr;

        for (size_t j = 0; j < B_; ++j) {
            tmp[j] = new DataBlock(smallSize);
            for (size_t p = 0; p < smallSize; ++p) {
                tmp[j]->data[p] = big->data[j * smallSize + p];
            }
        }
        delete big;

        if (i == 1) {
            // Na úrovni 1 uložíme všetkých B blokov.
            for (size_t j = 0; j < B_; ++j) {
                levels_[1].push_back(tmp[j]);
            }
            n_[1] += B_;
            delete[] tmp;
            break; // hotovo
        } else {
            // Na úrovni i uložíme prvých B-1 blokov, posledný delíme ďalej.
            for (size_t j = 0; j + 1 < B_; ++j) {
                levels_[i].push_back(tmp[j]);
            }
            n_[i] += (B_ - 1);
            big = tmp[B_ - 1];
            delete[] tmp;
        }

        if (i == 1) break; // underflow guard
    }

    // Po split-e sa predpokladá, že posledný B-blok je plný.
    n0_ = B_;
}

template<typename T, size_t R>
void ResizableArray<T, R>::rebuild(size_t newB) {
    // Bez std::vector (podľa zadania). Zálohujeme prvky do raw bufferu.
    const size_t oldN = N_;
    T* buffer = nullptr;
    size_t constructed = 0;
    if (oldN > 0) {
        buffer = static_cast<T*>(::operator new(sizeof(T) * oldN));
        try {
            for (size_t i = 0; i < oldN; ++i) {
                new (buffer + i) T(get(i));
                ++constructed;
            }
        } catch (...) {
            for (size_t j = 0; j < constructed; ++j) buffer[j].~T();
            ::operator delete(buffer);
            throw;
        }
    }

    // Vyčistiť starú štruktúru a zmeniť parameter B
    cleanupLevels();
    B_ = newB;
    initializeLevels();

    // Znovu vložiť všetky hodnoty
    for (size_t i = 0; i < constructed; ++i) {
        grow(buffer[i]);
        buffer[i].~T();
    }
    ::operator delete(buffer);
}

template<typename T, size_t R>
void ResizableArray<T, R>::copyFrom(const ResizableArray& other) {
    // Bezpečná (hoci pomalšia) implementácia: skopíruj obsah cez grow(get(i)).
    // Táto cesta sa vyhne problémom s kopírovaním neinitializovaných prvkov
    // v poslednom B-bloku a zároveň automaticky zachová konzistenciu počítadiel.

    // Ak sme "moved-from", musíme znovu alokovať interné polia.
    if (!levels_ || !n_) {
        delete[] levels_;
        delete[] n_;
        levels_ = nullptr;
        n_ = nullptr;
    }

    B_ = other.B_;
    initializeLevels(); // vymaže a pripraví štruktúru s novým B_

    for (size_t i = 0; i < other.N_; ++i) {
        grow(other.get(i));
    }
}





// ==================== PUBLIC METHODS (FULL IMPLEMENTATION) ====================

template<typename T, size_t R>
ResizableArray<T, R>::ResizableArray(const ResizableArray& other)
    : B_(other.B_), N_(0), n0_(0), levels_(nullptr), n_(nullptr) {
    initializeLevels();
    for (size_t i = 0; i < other.N_; ++i) {
        grow(other.get(i));
    }
}

template<typename T, size_t R>
ResizableArray<T, R>::ResizableArray(ResizableArray&& other) noexcept
    : N_(other.N_), B_(other.B_), levels_(other.levels_), n_(other.n_), n0_(other.n0_) {

    other.levels_ = nullptr;
    other.n_      = nullptr;
    other.N_      = 0;
    other.n0_     = 0;
}


template<typename T, size_t R>
ResizableArray<T, R>& ResizableArray<T, R>::operator=(const ResizableArray& other) {
    if (this == &other) return *this;

    // Ak sme moved-from (levels_/n_ == nullptr), musíme najprv znovu vytvoriť interné polia.
    B_ = other.B_;
    if (!levels_ || !n_) {
        delete[] levels_;
        delete[] n_;
        levels_ = nullptr;
        n_ = nullptr;
        initializeLevels();
    } else {
        cleanupLevels();
        // initializeLevels() by tiež fungovalo, ale cleanup + reserve je lacnejšie.
        for (size_t i = 1; i < R; ++i) {
            levels_[i].reserve(2 * B_);
        }
    }

    for (size_t i = 0; i < other.N_; ++i) {
        grow(other.get(i));
    }
    return *this;
}

template<typename T, size_t R>
ResizableArray<T, R>& ResizableArray<T, R>::operator=(ResizableArray&& other) noexcept {
    if (this == &other) return *this;

    cleanupLevels();
    delete[] levels_;
    delete[] n_;

    N_      = other.N_;
    B_      = other.B_;
    n0_     = other.n0_;
    levels_ = other.levels_;
    n_      = other.n_;

    other.levels_ = nullptr;
    other.n_      = nullptr;
    other.N_      = 0;
    other.n0_     = 0;

    return *this;
}


// ==================== HLAVNÉ OPERÁCIE ====================

template<typename T, size_t R>
void ResizableArray<T, R>::grow(const T& item) {
    // Dôležité: tieto kroky NESMÚ byť else-if reťazec.
    // Po combineBlocks() je posledný B-blok stále plný (n0_==B_), takže musíme vedieť
    // následne alokovať nový B-blok pred zápisom.

    if (N_ == power(B_, R)) {
        rebuild(2 * B_);
    }
    if (n_[1] == 2 * B_ && n0_ == B_) {
        combineBlocks();
    }
    if (n_[1] == 0 || n0_ == B_) {
        levels_[1].push_back(new DataBlock(B_));
        n_[1] += 1;
        n0_ = 0;
    }

    levels_[1].at(n_[1] - 1)->data[n0_] = item;
    n0_ += 1;
    N_  += 1;
}

template<typename T, size_t R>
void ResizableArray<T, R>::shrink() {
    if (N_ == 0) {
        throw std::out_of_range("shrink on empty array");
    }

    // Rebuild(B/2) keď N = (B/4)^r (len ak B>=4*2, aby B/4 >= 2)
    if (B_ >= 8 && N_ == power(B_ / 4, R)) {
        // Podľa PDF: shrink musí stále odstrániť 1 prvok aj keď došlo k rebuild.
        rebuild(B_ / 2);
        // pokračujeme ďalej a odstránime jeden prvok
    }

    if (n_[1] == 0) {
        splitBlocks(); // musí vyrobiť B-bloky
    }

    // odstráň posledný prvok
    n0_ -= 1;
    N_  -= 1;

    // ak sa posledný B-blok vyprázdnil, dealokuj ho
    if (n0_ == 0) {
        // aby pop_back nezmazal blok skôr než chceme: zmažeme ho cez pop_back owner logikou
        levels_[1].pop_back();
        n_[1] -= 1;

        if (N_ == 0 || n_[1] == 0) {
            n0_ = 0;
        } else {
            n0_ = B_; // nový posledný blok je plný
        }
    }
}

template<typename T, size_t R>
T& ResizableArray<T, R>::get(size_t index) {
    if (index >= N_) throw std::out_of_range("get: index out of range");

    size_t x = index;

    // Najprv veľké bloky (úrovne r-1 ... 2) — tie majú nižšie indexy
    for (size_t lvl = R - 1; lvl >= 2; --lvl) {
        const size_t blockSize = power(B_, lvl);          // B^lvl
        const size_t levelItems = n_[lvl] * blockSize;

        if (x < levelItems) {
            const size_t b = x / blockSize;
            const size_t off = x % blockSize;
            return levels_[lvl].at(b)->data[off];
        }
        x -= levelItems;

        if (lvl == 2) break; // ochrana pred underflow pre size_t
    }

    // Teraz úroveň 1 (bloky veľkosti B), posledný môže byť čiastočný
    if (n_[1] == 0) throw std::runtime_error("get: internal n1==0");

    const size_t fullPart = (n_[1] > 0 ? (n_[1] - 1) * B_ : 0);

    if (x < fullPart) {
        const size_t b = x / B_;
        const size_t off = x % B_;
        return levels_[1].at(b)->data[off];
    } else {
        x -= fullPart;                  // v poslednom (čiastočnom) bloku
        if (x >= n0_) throw std::runtime_error("get: past n0");
        return levels_[1].at(n_[1] - 1)->data[x];
    }
}

template<typename T, size_t R>
const T& ResizableArray<T, R>::get(size_t index) const {
    return const_cast<ResizableArray*>(this)->get(index);
}

template<typename T, size_t R>
void ResizableArray<T, R>::set(size_t index, const T& item) {
    get(index) = item;
}
