#pragma once
#include <cstring>
#include <stdexcept>

#include "rarray.h"

// ===================== DataBlock =====================
//
// Reprezentuje jeden pamäťový blok, ktorý uchováva prvky typu T.
// V triede ResizableArray sú tieto bloky používané na uloženie dát
// rôznych veľkostí: B, B^2, B^3, ...
//
template<typename T, size_t R>
ResizableArray<T, R>::DataBlock::DataBlock(size_t cap)
    : data(new T[cap]), capacity(cap) {}

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
    for (size_t i = 0; i < size; ++i)
        newData[i] = data[i];
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
    size -= (end - start);
}

template<typename T, size_t R>
template<typename BlockType>
void ResizableArray<T, R>::DynamicArray<BlockType>::clear() {
    // Odstráni všetky bloky a nastaví size = 0
    for (size_t i = 0; i < size; ++i)
        delete data[i];
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
}

template<typename T, size_t R>
void ResizableArray<T, R>::initializeLevels() {
    // vytvoríme pole úrovní
    levels_ = new DynamicArray<DataBlock>[R - 1];

    // vytvoríme pole počtov blokov v každej úrovni
    n_ = new size_t[R - 1];

    // všetky úrovne sú prázdne
    for (size_t i = 0; i < R - 1; ++i) {
        n_[i] = 0;
    }

    // posledný blok prvej úrovne je prázdny
    n0_ = 0;
}

template<typename T, size_t R>
void ResizableArray<T, R>::cleanupLevels() {
    if (!levels_) return;

    // odstránime všetky bloky v každej úrovni
    for (size_t i = 0; i < R - 1; ++i) {
        levels_[i].clear();  // zmaže bloky v tejto úrovni
    }

    delete[] levels_;
    delete[] n_;

    levels_ = nullptr;
    n_ = nullptr;

    N_ = 0;
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

    size_t remaining = index;

    // úroveň i (0-based) má bloky veľkosti B_^(i+1)
    for (size_t level = 0; level < R - 1; ++level) {
        size_t blockSize  = power(B_, level + 1);        // B^(level+1)
        size_t levelItems = n_[level] * blockSize;

        if (remaining < levelItems) {
            // prvok je na tejto úrovni
            size_t blockIndex = remaining / blockSize;   // index bloku v levels_[level]
            size_t offset     = remaining % blockSize;   // offset v danom bloku

            // Tu ako "pozíciu v úrovni" vrátime (blockIndex * blockSize + offset).
            // Samotné get()/set() si už podľa toho vedia nájsť konkrétny blok a index.
            size_t positionInLevel = blockIndex * blockSize + offset;
            return { level, positionInLevel };
        }

        remaining -= levelItems;
    }

    // Ak sem dôjdeme, niekde je chyba v štruktúre
    throw std::runtime_error("locateItem: internal inconsistency");
}

template<typename T, size_t R>
void ResizableArray<T, R>::combineBlocks() {
    // nájsť najmenšie k, kde n_[k] < 2B_
    size_t k = R;   // "∞"
    for (size_t i = 0; i < R - 1; ++i) {
        if (n_[i] < 2 * B_) {
            k = i;
            break;
        }
    }
    if (k == R) {
        throw std::runtime_error("combineBlocks: no free level");
    }

    // postupne zdola nahor (k-1, k-2, ..., 0)
    for (size_t level = k; level > 0; --level) {
        size_t i = level - 1;

        // veľkosť malého a veľkého bloku
        size_t smallSize = power(B_, i + 1);       // B^(i+1)
        size_t bigSize   = smallSize * B_;         // B^(i+2)

        // nový veľký blok na úrovni i+1
        DataBlock* bigBlock = new DataBlock(bigSize);

        size_t dst = 0;

        // skombinujeme prvých B_ blokov úrovne i
        for (size_t j = 0; j < B_; ++j) {
            DataBlock* src = levels_[i].data[j];

            for (size_t p = 0; p < smallSize; ++p) {
                bigBlock->data[dst++] = src->data[p];
            }

            delete src;
        }

        // posunieme zvyšné ukazovatele na úrovni i doľava o B_
        for (size_t j = B_; j < n_[i]; ++j) {
            levels_[i].data[j - B_] = levels_[i].data[j];
        }

        n_[i]      = B_;              // po kombinácii ostáva presne B blokov
        levels_[i].size = n_[i];

        // pridáme nový veľký blok na úroveň i+1
        levels_[i + 1].push_back(bigBlock);
        ++n_[i + 1];
    }
}

template<typename T, size_t R>
void ResizableArray<T, R>::splitBlocks() {
    // nájsť najmenšie k >= 1 s n_[k] > 0
    size_t k = R;   // "∞"
    for (size_t i = 1; i < R - 1; ++i) {
        if (n_[i] > 0) {
            k = i;
            break;
        }
    }
    if (k == R) {
        throw std::runtime_error("splitBlocks: no block to split");
    }

    // vezmeme posledný blok na úrovni k
    DataBlock* big = levels_[k].data[n_[k] - 1];

    size_t bigSize   = big->capacity;             // B^(k+1)
    size_t smallSize = bigSize / B_;              // B^k

    size_t offset = 0;

    // rozbijeme na B_ menších blokov veľkosti smallSize
    for (size_t j = 0; j < B_; ++j) {
        DataBlock* small = new DataBlock(smallSize);

        for (size_t p = 0; p < smallSize; ++p) {
            small->data[p] = big->data[offset++];
        }

        levels_[k - 1].push_back(small);
        ++n_[k - 1];
    }

    // odstránime pôvodný veľký blok z úrovne k
    delete big;
    levels_[k].pop_back();
    --n_[k];
}

template<typename T, size_t R>
void ResizableArray<T, R>::rebuild(size_t newB) {
    // 1. záloha všetkých prvkov
    std::vector<T> items;
    items.reserve(N_);
    for (size_t i = 0; i < N_; ++i) {
        items.push_back(get(i));   // používa existujúcu implementáciu get()
    }

    // 2. vyčistiť starú štruktúru
    cleanupLevels();

    // 3. zmeniť parameter B a inicializovať úrovne
    B_ = newB;
    initializeLevels();

    // 4. znova vložiť všetky hodnoty
    for (const T& x : items) {
        grow(x);
    }
}

template<typename T, size_t R>
void ResizableArray<T, R>::copyFrom(const ResizableArray& other) {
    // najprv vyčistíme vlastné dáta (aby nedošlo k úniku pamäte)
    cleanupLevels();

    B_  = other.B_;
    N_  = other.N_;
    n0_ = other.n0_;

    for (size_t i = 0; i < R - 1; ++i) {
        n_[i] = other.n_[i];

        // pripravíme si miesto na rovnako veľa blokov
        for (size_t j = 0; j < other.levels_[i].size; ++j) {
            const DataBlock* src = other.levels_[i].data[j];

            DataBlock* dst = new DataBlock(src->capacity);

            for (size_t k = 0; k < src->capacity; ++k) {
                dst->data[k] = src->data[k];
            }

            levels_[i].push_back(dst);
        }
    }
}






template<typename T, size_t R>
ResizableArray<T, R>::ResizableArray(const ResizableArray& other)
{
    // TODO: implement later
}
template<typename T, size_t R>
ResizableArray<T, R>::ResizableArray(ResizableArray&& other) noexcept
{
    // TODO: implement later
}
template<typename T, size_t R>
ResizableArray<T, R>& ResizableArray<T, R>::operator=(const ResizableArray& other)
{
    // TODO: implement later
    return *this;
}
template<typename T, size_t R>
ResizableArray<T, R>& ResizableArray<T, R>::operator=(ResizableArray&& other) noexcept
{
    // TODO: implement later
    return *this;
}
template<typename T, size_t R>
void ResizableArray<T, R>::grow(const T& item)
{
    // TODO: implement later
}
template<typename T, size_t R>
void ResizableArray<T, R>::shrink()
{
    // TODO: implement later
}
template<typename T, size_t R>
T& ResizableArray<T, R>::get(size_t index)
{
    // TODO: implement later
    static T dummy{};
    return dummy;
}
template<typename T, size_t R>
const T& ResizableArray<T, R>::get(size_t index) const
{
    // TODO: implement later
    static T dummy{};
    return dummy;
}
template<typename T, size_t R>
void ResizableArray<T, R>::set(size_t index, const T& item)
{
    // TODO: implement later
}
