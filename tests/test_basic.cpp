#include <gtest/gtest.h>
#include "../include/rarray.h"
#include "../include/rarray_impl.tpp"

using TestArray = ResizableArray<int, 3>;

// === TEST 1: DataBlock allocation and move ===
TEST(DataBlockTest, AllocationAndMove) {
    TestArray::DataBlock block(5);
    for (size_t i = 0; i < 5; ++i)
        block.data[i] = static_cast<int>(i * 10);

    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(block.data[i], static_cast<int>(i * 10));

    // Move constructor
    TestArray::DataBlock moved(std::move(block));
    EXPECT_EQ(moved.capacity, 5);
    EXPECT_EQ(block.data, nullptr);
}

// === TEST 2: DynamicArray push_back, erase, clear ===
TEST(DynamicArrayTest, PushEraseClear) {
    TestArray::DynamicArray<TestArray::DataBlock> arr;
    EXPECT_EQ(arr.size, 0);

    for (int i = 0; i < 3; ++i)
        arr.push_back(new TestArray::DataBlock(4));

    EXPECT_EQ(arr.size, 3);
    EXPECT_GE(arr.capacity, 3);

    arr.erase(1, 2);
    EXPECT_EQ(arr.size, 2);

    arr.pop_back();
    EXPECT_EQ(arr.size, 1);

    TestArray::DataBlock*& ref = arr.at(0);
    EXPECT_EQ(ref->capacity, 4);

    arr.clear();
    EXPECT_EQ(arr.size, 0);
}

// =======================================================================
// =====================  TEST 3: Private Methods  =======================
// =======================================================================
//
// Predpokladáme, že všetky tieto metódy sú dočasne v 'public'.
// Každý test sa zameriava na jednu konkrétnu vnútornú funkciu.
//


// =======================================================================
//  power() — vypočíta base^exp
// =======================================================================
//
// Funkcia sa používa vo všetkých výpočtoch veľkostí blokov.
// Musí byť presná a rýchla.
//

TEST(PrivateMethodsTest, PowerComputesExponentCorrectly) {
    TestArray arr;

    EXPECT_EQ(arr.power(2, 0), 1);   // akákoľvek báza na 0 = 1
    EXPECT_EQ(arr.power(2, 3), 8);
    EXPECT_EQ(arr.power(5, 1), 5);
    EXPECT_EQ(arr.power(3, 4), 81);
}

// =======================================================================
//  initializeLevels()
// =======================================================================
//
// Inicializuje vnútorné štruktúry — pole úrovní a počítadlá blokov.
// Po inicializácii musí byť všetko prázdne.
//

TEST(PrivateMethodsTest, InitializeLevelsCreatesEmptyStructure) {
    TestArray arr;

    arr.initializeLevels();

    for (size_t i = 0; i < 3 - 1; i++) {
        EXPECT_EQ(arr.n_[i], 0);
        EXPECT_EQ(arr.levels_[i].size, 0);
    }

    EXPECT_EQ(arr.n0_, 0);  // žiadny prvok v poslednom blok-u
}

// =======================================================================
//  cleanupLevels()
// =======================================================================
//
// Musí korektne uvoľniť všetky bloky a resetovať všetky počítadlá.
// Slúži pri destrukcii a veľkých prebudovaniach.
//

TEST(PrivateMethodsTest, CleanupLevelsFreesAllMemory) {
    TestArray arr;

    // Naplníme pole, aby vznikli nejaké bloky
    for (int i = 0; i < 10; i++)
        arr.grow(i);

    // Teraz vyčistíme všetko
    arr.cleanupLevels();

    // Očakávame úplne prázdnu štruktúru
    for (size_t i = 0; i < 3 - 1; i++) {
        EXPECT_EQ(arr.n_[i], 0);
        EXPECT_EQ(arr.levels_[i].size, 0);
    }

    EXPECT_EQ(arr.n0_, 0);
}

// =======================================================================
//  locateItem()
// =======================================================================
//
// Funkcia nájde konkrétny prvok a vráti (úroveň, offset).
// Je to kľúčové pre rýchly prístup O(1).
//

TEST(PrivateMethodsTest, LocateItemReturnsCorrectLevelAndOffset) {
    TestArray arr;

    for (int i = 0; i < 25; i++)
        arr.grow(i);

    auto pos = arr.locateItem(0);
    EXPECT_EQ(arr.get(0), 0);

    pos = arr.locateItem(10);
    EXPECT_EQ(arr.get(10), 10);

    pos = arr.locateItem(24);
    EXPECT_EQ(arr.get(24), 24);

    EXPECT_THROW(arr.locateItem(1000), std::out_of_range);
}

// =======================================================================
//  combineBlocks()
// =======================================================================
//
// combineBlocks() sa spustí keď je úroveň plná (2B blokov).
// Skombinuje B blokov do jedného väčšieho.
// Toto testujeme nepriamo — cez grow().
//

TEST(PrivateMethodsTest, CombineBlocksProducesLargerBlocks) {
    TestArray arr;

    size_t B = arr.getParameterB();

    // Toto určite vyvolá combineBlocks()
    for (size_t i = 0; i < B * 4; i++)
        arr.grow(int(i));

    EXPECT_EQ(arr.length(), B * 4);
    EXPECT_EQ(arr.get(0), 0);
    EXPECT_EQ(arr.get(B), int(B));
    EXPECT_EQ(arr.get(B * 3 - 1), int(B * 3 - 1));
}

// =======================================================================
//  splitBlocks()
// =======================================================================
//
// splitBlocks() rozbíja veľký blok na malé,
// keď pri shrink() dôjde k nedostatku blokov nižšej úrovne.
//

TEST(PrivateMethodsTest, SplitBlocksCreatesSmallerBlocks) {
    TestArray arr;

    for (int i = 0; i < 100; i++)
        arr.grow(i);

// Vymažeme veľa prvkov aby sa úrovne "vyprázdnili"
    for (int i = 0; i < 90; i++)
        arr.shrink();

    EXPECT_EQ(arr.length(), 10);
    EXPECT_EQ(arr.get(0), 0);
    EXPECT_EQ(arr.get(9), 9);
}

// =======================================================================
//  rebuild(newB)
// =======================================================================
//
// rebuild() kompletne prebuduje celú štruktúru.
// Používa sa pri zmene veľkosti B (nárast aj pokles).
//

TEST(PrivateMethodsTest, RebuildChangesBlockSizeAndKeepsData) {
    TestArray arr;

    for (int i = 0; i < 30; i++)
        arr.grow(i);

    size_t oldB = arr.getParameterB();
    size_t newB = oldB * 2;

    arr.rebuild(newB);

    EXPECT_EQ(arr.getParameterB(), newB);
    EXPECT_EQ(arr.length(), 30);

    for (int i = 0; i < 30; i++)
        EXPECT_EQ(arr.get(i), i);  // dáta musia zostať
}


// =======================================================================
// =====================  TEST 4: Public Methods  ========================
// =======================================================================
//
// Nasledujú testy pre všetky verejné metódy ResizableArray.
// Každá funkcia má vlastný izolovaný test.
//


// =======================================================================
//  Constructor — vytvorí prázdne pole
// =======================================================================

TEST(PublicMethodsTest, ConstructorCreatesEmptyArray) {
    TestArray arr;
    EXPECT_TRUE(arr.empty());          // pole musí byť prázdne
    EXPECT_EQ(arr.length(), 0);        // počet prvkov = 0
}


// =======================================================================
//  grow() — pridá prvok na koniec
// =======================================================================

TEST(PublicMethodsTest, GrowAddsElementsCorrectly) {
    TestArray arr;

    arr.grow(10);
    arr.grow(20);
    arr.grow(30);

    EXPECT_EQ(arr.length(), 3);
    EXPECT_EQ(arr.get(0), 10);
    EXPECT_EQ(arr.get(1), 20);
    EXPECT_EQ(arr.get(2), 30);
}


// =======================================================================
//  shrink() — odstráni posledný prvok
// =======================================================================

TEST(PublicMethodsTest, ShrinkRemovesLastElement) {
    TestArray arr;

    arr.grow(5);
    arr.grow(10);

    arr.shrink();

    EXPECT_EQ(arr.length(), 1);
    EXPECT_EQ(arr.get(0), 5);
}

TEST(PublicMethodsTest, ShrinkThrowsWhenEmpty) {
    TestArray arr;
    EXPECT_THROW(arr.shrink(), std::out_of_range);
}


// =======================================================================
//  get() — vráti prvok na indexe
// =======================================================================

TEST(PublicMethodsTest, GetReturnsCorrectValue) {
    TestArray arr;

    arr.grow(7);
    arr.grow(14);

    EXPECT_EQ(arr.get(0), 7);
    EXPECT_EQ(arr.get(1), 14);
}

TEST(PublicMethodsTest, GetThrowsOnInvalidIndex) {
    TestArray arr;
    arr.grow(5);

    EXPECT_THROW(arr.get(10), std::out_of_range);
    EXPECT_THROW(arr.get(1), std::out_of_range);
}


// =======================================================================
//  set() — nastaví novú hodnotu na indexe
// =======================================================================

TEST(PublicMethodsTest, SetChangesValueCorrectly) {
    TestArray arr;

    arr.grow(50);
    arr.grow(60);

    arr.set(0, 777);
    arr.set(1, 888);

    EXPECT_EQ(arr.get(0), 777);
    EXPECT_EQ(arr.get(1), 888);
}

TEST(PublicMethodsTest, SetThrowsOnInvalidIndex) {
    TestArray arr;
    arr.grow(1);

    EXPECT_THROW(arr.set(5, 111), std::out_of_range);
}


// =======================================================================
//  operator[] — alias pre get() bez kontroly hraníc
// =======================================================================

TEST(PublicMethodsTest, OperatorBracketReturnsReference) {
    TestArray arr;

    arr.grow(3);
    arr.grow(6);

    EXPECT_EQ(arr[0], 3);
    EXPECT_EQ(arr[1], 6);

// test priamej zmeny cez referenciu
    arr[1] = 99;
    EXPECT_EQ(arr.get(1), 99);
}


// =======================================================================
//  length() — aktuálny počet prvkov
// =======================================================================

TEST(PublicMethodsTest, LengthReturnsCorrectValue) {
    TestArray arr;

    for (int i = 0; i < 10; i++)
        arr.grow(i);

    EXPECT_EQ(arr.length(), 10);
}


// =======================================================================
//  empty() — či je pole prázdne
// =======================================================================

TEST(PublicMethodsTest, EmptyReturnsCorrectState) {
    TestArray arr;

    EXPECT_TRUE(arr.empty());

    arr.grow(1);
    EXPECT_FALSE(arr.empty());
}


// =======================================================================
//  Copy constructor — hlboká kópia
// =======================================================================

TEST(PublicMethodsTest, CopyConstructorCreatesIndependentCopy) {
    TestArray arr;
    arr.grow(5);
    arr.grow(10);

    TestArray copy(arr);

    // hodnoty sa musia zhodovať
    EXPECT_EQ(copy.length(), 2);
    EXPECT_EQ(copy.get(0), 5);
    EXPECT_EQ(copy.get(1), 10);

    // zmena v jednom nesmie ovplyvniť druhý
    copy.set(0, 999);
    EXPECT_NE(copy.get(0), arr.get(0));
}


// =======================================================================
//  Copy assignment — hlboká kópia
// =======================================================================

TEST(PublicMethodsTest, CopyAssignmentCreatesIndependentCopy) {
    TestArray arr;
    arr.grow(1);
    arr.grow(2);

    TestArray copy;
    copy = arr;

    EXPECT_EQ(copy.length(), 2);
    EXPECT_EQ(copy.get(0), 1);

    copy.set(0, 777);
    EXPECT_NE(copy.get(0), arr.get(0));
}


// =======================================================================
//  Move constructor — presunie vnútorné dáta
// =======================================================================

TEST(PublicMethodsTest, MoveConstructorTransfersOwnership) {
    TestArray arr;
    arr.grow(123);

    TestArray moved(std::move(arr));

    EXPECT_EQ(moved.length(), 1);
    EXPECT_EQ(moved.get(0), 123);
}


// =======================================================================
//  Move assignment — presunie vnútorné dáta
// =======================================================================

TEST(PublicMethodsTest, MoveAssignmentTransfersOwnership) {
    TestArray arr;
    arr.grow(42);

    TestArray moved;
    moved = std::move(arr);

    EXPECT_EQ(moved.length(), 1);
    EXPECT_EQ(moved.get(0), 42);
}