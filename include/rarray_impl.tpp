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
