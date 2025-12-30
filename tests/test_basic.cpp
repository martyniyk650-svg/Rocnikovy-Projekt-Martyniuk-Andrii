#include <gtest/gtest.h>
#include "../include/rarray.h"
#include "../include/rarray_impl.tpp"
#include <new>
#include <utility>


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

    // Core expected identities
    EXPECT_EQ(arr.power(2, 0), 1u) << "Any base^0 must be 1";
    EXPECT_EQ(arr.power(2, 3), 8u);
    EXPECT_EQ(arr.power(5, 1), 5u);
    EXPECT_EQ(arr.power(3, 4), 81u);

    // Additional edge-ish cases: base 0 and base 1
    EXPECT_EQ(arr.power(0, 1), 0u) << "0^exp (exp>0) should be 0";
    EXPECT_EQ(arr.power(1, 50), 1u) << "1^exp must be 1";

    // Consistency check: power(b, e+1) == power(b,e)*b for small exponents
    for (size_t e = 0; e < 8; ++e) {
        const size_t p  = arr.power(7, e);
        const size_t p1 = arr.power(7, e + 1);
        EXPECT_EQ(p1, p * 7u) << "Exponentiation must be consistent across steps";
    }
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

    // Pre-fill to ensure initializeLevels truly resets a non-empty structure.
    for (int i = 0; i < 25; ++i) arr.grow(i);
    ASSERT_EQ(arr.length(), 25u) << "Precondition: array must be non-empty before init reset";

    arr.initializeLevels();

    // Original structure checks (kept), plus expanded checks below.
    for (size_t i = 0; i < 3 - 1; i++) {
        EXPECT_EQ(arr.n_[i], 0u) << "All counters must be reset to 0 after initializeLevels()";
        EXPECT_EQ(arr.levels_[i].size, 0u) << "All levels must be empty after initializeLevels()";
    }
    EXPECT_EQ(arr.n0_, 0u) << "No elements should remain in the last B-block after init";

    // Expanded: check all meaningful levels [1..R-1] as well (R=3 => levels 1 and 2).
    for (size_t level = 1; level < 3; ++level) {
        EXPECT_EQ(arr.n_[level], 0u) << "Level counter must be 0 after init";
        EXPECT_EQ(arr.levels_[level].size, 0u) << "Level DynamicArray size must be 0 after init";
    }

    // After initializeLevels, array must behave as fresh (reusable).
    EXPECT_TRUE(arr.empty()) << "After initializeLevels, array must be empty";
    EXPECT_EQ(arr.length(), 0u) << "After initializeLevels, length must be 0";

    arr.grow(123);
    EXPECT_EQ(arr.length(), 1u) << "Array must be reusable after initializeLevels";
    EXPECT_EQ(arr.get(0), 123) << "Inserted element must be accessible";
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

    // Fill to allocate multiple blocks (not just one).
    for (int i = 0; i < 200; i++)
        arr.grow(i);
    ASSERT_EQ(arr.length(), 200u) << "Precondition: array must contain data before cleanup";

    // Now cleanup everything.
    arr.cleanupLevels();

    // Original expectations (kept), expanded with stronger checks.
    for (size_t i = 1; i < 3; i++) {
        EXPECT_EQ(arr.n_[i], 0u) << "All block counters must be zero after cleanupLevels()";
        EXPECT_EQ(arr.levels_[i].size, 0u) << "All levels must be empty after cleanupLevels()";
    }
    EXPECT_EQ(arr.n0_, 0u) << "n0 must be zero after cleanupLevels()";
    EXPECT_TRUE(arr.empty()) << "Array must be empty after cleanupLevels()";
    EXPECT_EQ(arr.length(), 0u) << "Length must be 0 after cleanupLevels()";

    // cleanupLevels should be safe to call multiple times (idempotent safety)
    EXPECT_NO_THROW(arr.cleanupLevels()) << "cleanupLevels() should be safe to call multiple times";

    // Reusability after cleanup
    for (int i = 0; i < 10; ++i) arr.grow(1000 + i);
    EXPECT_EQ(arr.length(), 10u) << "Array must be reusable after cleanupLevels()";
    EXPECT_EQ(arr.get(0), 1000);
    EXPECT_EQ(arr.get(9), 1009);
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

    // Fill enough elements to span multiple B-blocks and likely higher structures.
    for (int i = 0; i < 100; i++)
        arr.grow(i);
    ASSERT_EQ(arr.length(), 100u);

    // Basic correctness: get(index) must match inserted value.
    // Also: locateItem(index) must not throw for valid indices.
    auto pos = arr.locateItem(0);
    EXPECT_EQ(arr.get(0), 0);

    pos = arr.locateItem(10);
    EXPECT_EQ(arr.get(10), 10);

    pos = arr.locateItem(99);
    EXPECT_EQ(arr.get(99), 99);

    // Expanded: probe a handful of indices across the range
    for (size_t idx : {1u, 2u, 3u, 7u, 15u, 31u, 63u, 64u, 80u}) {
        auto p = arr.locateItem(idx);
        (void)p; // We mainly ensure it doesn't throw and data matches.
        EXPECT_EQ(arr.get(idx), static_cast<int>(idx))
            << "locateItem/get must agree on the value stored";
    }

    // Negative paths: out of bounds
    EXPECT_THROW(arr.locateItem(1000), std::out_of_range) << "locateItem must throw on huge index";
    EXPECT_THROW(arr.locateItem(arr.length()), std::out_of_range) << "locateItem must throw at index==length";
    EXPECT_THROW(arr.locateItem(static_cast<size_t>(-1)), std::out_of_range)
        << "locateItem must throw for very large size_t index (wraparound)";

    // Optional sanity about returned pair (without assuming exact mapping):
    // level should be within [1, R-1] i.e. [1,2] for R=3
    auto p0 = arr.locateItem(0);
    EXPECT_GE(p0.first, 1u);
    EXPECT_LE(p0.first, 2u);
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

    // Keep original “small fill” but extend to actually reach the PDF combine boundary for R=3:
    // combine triggers when n1==2B and n0==B, which corresponds to N == 2*B*B elements.
    for (size_t i = 0; i < B * 4; i++)
        arr.grow(int(i));

    EXPECT_EQ(arr.length(), B * 4) << "Basic growth should work for small counts";
    EXPECT_EQ(arr.get(0), 0);
    EXPECT_EQ(arr.get(B), int(B));
    EXPECT_EQ(arr.get(B * 3 - 1), int(B * 3 - 1));

    // Extended: grow up to combine boundary and then add one more element.
    // This is the most crash-prone edge case in buggy implementations (OOB after combine).
    const size_t target = 2 * B * B;
    for (size_t i = arr.length(); i < target; ++i)
        arr.grow(int(i));

    ASSERT_EQ(arr.length(), target) << "We must reach the combine trigger boundary";

    // Add one more element to force “combine + append” path to be safe.
    arr.grow(123456);
    EXPECT_EQ(arr.length(), target + 1) << "After grow beyond boundary, length must increase by 1";

    // Data integrity: prefix must be preserved, last must be the new value.
    EXPECT_EQ(arr.get(0), 0) << "First element must remain correct after combine";
    EXPECT_EQ(arr.get(target - 1), int(target - 1)) << "Last element of old prefix must remain correct";
    EXPECT_EQ(arr.get(target), 123456) << "Newly appended element must be stored correctly";

    // Spot-check a couple of internal fields if exposed (helps detect counter corruption)
    EXPECT_LE(arr.n0_, B) << "n0 must never exceed B";
    EXPECT_GE(arr.n_[1], 1u) << "After growth, level 1 must contain at least one block";
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

    // Use a larger fill to ensure we likely create higher-level blocks.
    for (int i = 0; i < 300; i++)
        arr.grow(i);
    ASSERT_EQ(arr.length(), 300u);

    // Shrink a lot to force internal restructuring and potential split paths.
    for (int i = 0; i < 290; i++)
        arr.shrink();

    EXPECT_EQ(arr.length(), 10u) << "After shrinking 290 elements from 300, 10 must remain";
    EXPECT_EQ(arr.get(0), 0) << "First element must remain correct after many shrinks";
    EXPECT_EQ(arr.get(9), 9) << "Last remaining element must be correct";

    // Expanded: verify all remaining elements are intact and ordered
    for (size_t i = 0; i < arr.length(); ++i)
        EXPECT_EQ(arr.get(i), static_cast<int>(i))
            << "Remaining prefix must stay in order after heavy shrink";

    // Negative path: shrink down to empty then one more shrink must throw
    for (int i = 0; i < 10; ++i) arr.shrink();
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.length(), 0u);

    EXPECT_THROW(arr.shrink(), std::out_of_range)
        << "shrink() on empty array must throw";

    // Reusability after reaching empty via shrinks
    arr.grow(777);
    arr.grow(888);
    EXPECT_EQ(arr.length(), 2u);
    EXPECT_EQ(arr.get(0), 777);
    EXPECT_EQ(arr.get(1), 888);
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

    EXPECT_EQ(arr.getParameterB(), newB) << "Block parameter B must be updated after rebuild";
    EXPECT_EQ(arr.length(), 30u) << "Rebuild must preserve array length";

    for (int i = 0; i < 30; i++)
        EXPECT_EQ(arr.get(i), i) << "All values must remain intact after rebuild";

    // Expanded: verify rebuild doesn't break future operations (grow/shrink)
    arr.grow(999);
    EXPECT_EQ(arr.length(), 31u);
    EXPECT_EQ(arr.get(30), 999) << "After rebuild, grow() must still append correctly";

    arr.shrink();
    EXPECT_EQ(arr.length(), 30u) << "shrink() after rebuild must remove last element";
    EXPECT_EQ(arr.get(29), 29) << "After removing appended element, original tail must be intact";

    // Expanded: rebuild back down (if allowed) and verify data still preserved
    // (We assume oldB is still valid; if your spec forbids shrinking B manually, remove this part.)
    arr.rebuild(oldB);
    EXPECT_EQ(arr.getParameterB(), oldB) << "Rebuild back should restore B";
    EXPECT_EQ(arr.length(), 30u);
    for (int i = 0; i < 30; i++)
        EXPECT_EQ(arr.get(i), i) << "Data must remain intact after rebuild back";
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
    EXPECT_TRUE(arr.empty()) << "New array must be empty";
    EXPECT_EQ(arr.length(), 0u) << "New array length must be 0";

    // Expanded: operations on empty must throw (negative paths)
    EXPECT_THROW(arr.get(0), std::out_of_range) << "get(0) on empty must throw";
    EXPECT_THROW(arr.set(0, 1), std::out_of_range) << "set(0) on empty must throw";
    EXPECT_THROW((void)arr[0], std::out_of_range) << "operator[] on empty must throw if it is bounds-checked";
}


// =======================================================================
//  grow() — pridá prvok na koniec
// =======================================================================

TEST(PublicMethodsTest, GrowAddsElementsCorrectly) {
    TestArray arr;

    arr.grow(10);
    arr.grow(20);
    arr.grow(30);

    EXPECT_EQ(arr.length(), 3u) << "Length must reflect number of inserted items";
    EXPECT_EQ(arr.get(0), 10);
    EXPECT_EQ(arr.get(1), 20);
    EXPECT_EQ(arr.get(2), 30);

    // Expanded: diverse valid inputs (duplicates, negatives, zero)
    arr.grow(0);
    arr.grow(-5);
    arr.grow(20); // duplicate
    EXPECT_EQ(arr.length(), 6u);

    EXPECT_EQ(arr.get(3), 0) << "Zero value must be stored correctly";
    EXPECT_EQ(arr.get(4), -5) << "Negative values must be stored correctly";
    EXPECT_EQ(arr.get(5), 20) << "Duplicate values must be stored correctly";

    // Expanded: larger sequence to stress multiple blocks
    for (int i = 0; i < 200; ++i) arr.grow(1000 + i);
    EXPECT_EQ(arr.length(), 206u) << "Large grow sequence must not lose elements";
    EXPECT_EQ(arr.get(205), 1199) << "Last element must match the last appended value";
}

// =======================================================================
//  shrink() — odstráni posledný prvok
// =======================================================================

TEST(PublicMethodsTest, ShrinkRemovesLastElement) {
    TestArray arr;

    arr.grow(5);
    arr.grow(10);

    arr.shrink();

    EXPECT_EQ(arr.length(), 1u) << "Shrink must reduce length by 1";
    EXPECT_EQ(arr.get(0), 5) << "After shrink, remaining first element must be intact";

    // Expanded: shrink repeatedly until empty
    arr.shrink();
    EXPECT_TRUE(arr.empty()) << "After removing all elements, array must be empty";
    EXPECT_EQ(arr.length(), 0u);

    // Reusability after becoming empty via shrinks
    arr.grow(77);
    EXPECT_FALSE(arr.empty());
    EXPECT_EQ(arr.length(), 1u);
    EXPECT_EQ(arr.get(0), 77);
}

TEST(PublicMethodsTest, ShrinkThrowsWhenEmpty) {
    TestArray arr;
    EXPECT_THROW(arr.shrink(), std::out_of_range) << "shrink() on empty must throw";

    // Expanded: after some operations, empty again must still throw
    arr.grow(1);
    arr.shrink();
    EXPECT_TRUE(arr.empty());
    EXPECT_THROW(arr.shrink(), std::out_of_range) << "Repeated shrink() on empty must throw consistently";
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

    // Expanded: boundary reads after larger growth
    for (int i = 0; i < 100; ++i) arr.grow(100 + i);
    EXPECT_EQ(arr.get(0), 7) << "Old prefix must remain intact after further growth";
    EXPECT_EQ(arr.get(arr.length() - 1), 199) << "Last element must be correct";
    EXPECT_EQ(arr.get(2), 100) << "Elements after initial inserts must be correct";
}

TEST(PublicMethodsTest, GetThrowsOnInvalidIndex) {
    TestArray arr;
    arr.grow(5);

    EXPECT_THROW(arr.get(10), std::out_of_range);
    EXPECT_THROW(arr.get(1), std::out_of_range);

    // Expanded: index == length must throw, and huge index must throw
    EXPECT_THROW(arr.get(arr.length()), std::out_of_range) << "get(length) must throw";
    EXPECT_THROW(arr.get(static_cast<size_t>(-1)), std::out_of_range) << "get(very large) must throw";
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

    // Expanded: set on larger structure, check boundaries
    for (int i = 0; i < 100; ++i) arr.grow(i);
    arr.set(0, -1);
    arr.set(arr.length() - 1, 9999);

    EXPECT_EQ(arr.get(0), -1) << "set() must correctly update first element";
    EXPECT_EQ(arr.get(arr.length() - 1), 9999) << "set() must correctly update last element";
}

TEST(PublicMethodsTest, SetThrowsOnInvalidIndex) {
    TestArray arr;
    arr.grow(1);

    EXPECT_THROW(arr.set(5, 111), std::out_of_range);

    // Expanded: set on empty must throw
    TestArray emptyArr;
    EXPECT_THROW(emptyArr.set(0, 123), std::out_of_range) << "set() on empty must throw";

    // Expanded: set(length) must throw
    EXPECT_THROW(arr.set(arr.length(), 222), std::out_of_range) << "set(length) must throw";
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
    EXPECT_EQ(arr.get(1), 99) << "operator[] must return a modifiable reference";

    // Expanded: modify first element via reference
    arr[0] = -10;
    EXPECT_EQ(arr.get(0), -10) << "Reference from operator[] must allow modifying element in-place";

    // Expanded: out-of-range behavior (if implementation checks bounds, it must throw)
    EXPECT_THROW((void)arr[2], std::out_of_range) << "operator[](index==length) must throw if bounds-checked";
    EXPECT_THROW((void)arr[999], std::out_of_range) << "operator[](huge) must throw if bounds-checked";
}


// =======================================================================
//  length() — aktuálny počet prvkov
// =======================================================================

TEST(PublicMethodsTest, LengthReturnsCorrectValue) {
    TestArray arr;

    for (int i = 0; i < 10; i++)
        arr.grow(i);

    EXPECT_EQ(arr.length(), 10u);

    // Expanded: length after shrink operations
    arr.shrink();
    EXPECT_EQ(arr.length(), 9u) << "After shrink, length must decrease by 1";
    arr.shrink();
    EXPECT_EQ(arr.length(), 8u) << "Repeated shrink must decrease length correctly";

    // Expanded: length after additional grows
    arr.grow(100);
    arr.grow(200);
    EXPECT_EQ(arr.length(), 10u) << "After adding 2 elements, length must reflect it correctly";
}

// =======================================================================
//  empty() — či je pole prázdne
// =======================================================================

TEST(PublicMethodsTest, EmptyReturnsCorrectState) {
    TestArray arr;

    EXPECT_TRUE(arr.empty());

    arr.grow(1);
    EXPECT_FALSE(arr.empty());

    // Expanded: after shrink to empty, empty() must return true again
    arr.shrink();
    EXPECT_TRUE(arr.empty()) << "empty() must become true after removing last element";
}


// =======================================================================
//  Copy constructor — hlboká kópia
// =======================================================================

TEST(PublicMethodsTest, CopyConstructorCreatesIndependentCopy) {
    TestArray arr;
    arr.grow(5);
    arr.grow(10);

    // Expanded: make it larger to ensure multiple blocks/counters are copied correctly
    for (int i = 0; i < 200; ++i) arr.grow(1000 + i);

    TestArray copy(arr);

    // hodnoty sa musia zhodovať
    EXPECT_EQ(copy.length(), arr.length()) << "Copy must preserve length";
    EXPECT_EQ(copy.get(0), arr.get(0)) << "Copy must preserve first element";
    EXPECT_EQ(copy.get(1), arr.get(1)) << "Copy must preserve second element";
    EXPECT_EQ(copy.get(copy.length() - 1), arr.get(arr.length() - 1)) << "Copy must preserve last element";

    // Expanded: verify several positions match
    for (size_t idx : {
    size_t{0}, size_t{1}, size_t{2}, size_t{50}, size_t{100}, copy.length() - 1 }) {
        EXPECT_EQ(copy.get(idx), arr.get(idx)) << "Copy must preserve content at various indices";
    }

    // zmena v jednom nesmie ovplyvniť druhý
    copy.set(0, 999);
    EXPECT_NE(copy.get(0), arr.get(0)) << "Modifying copy must not affect original";

    arr.set(1, 888);
    EXPECT_NE(copy.get(1), arr.get(1)) << "Modifying original must not affect copy";
}


// =======================================================================
//  Copy assignment — hlboká kópia
// =======================================================================

TEST(PublicMethodsTest, CopyAssignmentCreatesIndependentCopy) {
    TestArray arr;
    arr.grow(1);
    arr.grow(2);

    // Expanded: bigger source to catch shallow-copy issues
    for (int i = 0; i < 150; ++i) arr.grow(500 + i);

    TestArray copy;
    // Give copy some prior content to ensure assignment replaces previous state
    for (int i = 0; i < 50; ++i) copy.grow(-100 - i);

    copy = arr;

    EXPECT_EQ(copy.length(), arr.length()) << "Assignment must copy full length";
    EXPECT_EQ(copy.get(0), 1);
    EXPECT_EQ(copy.get(1), 2);
    EXPECT_EQ(copy.get(copy.length() - 1), arr.get(arr.length() - 1)) << "Assignment must copy last element";

    // Independence checks
    copy.set(0, 777);
    EXPECT_NE(copy.get(0), arr.get(0)) << "Modifying assigned copy must not affect original";

    arr.set(1, 999);
    EXPECT_NE(copy.get(1), arr.get(1)) << "Modifying original must not affect assigned copy";

    // Expanded: self-assignment safety (should not corrupt data)
    const size_t lenBefore = copy.length();
    int firstBefore = copy.get(0);
    copy = copy;
    EXPECT_EQ(copy.length(), lenBefore) << "Self-assignment must preserve length";
    EXPECT_EQ(copy.get(0), firstBefore) << "Self-assignment must preserve content";
}


// =======================================================================
//  Move constructor — presunie vnútorné dáta
// =======================================================================

TEST(PublicMethodsTest, MoveConstructorTransfersOwnership) {
    TestArray arr;
    for (int i = 0; i < 200; ++i) arr.grow(i);

    TestArray moved(std::move(arr));

    EXPECT_EQ(moved.length(), 200u) << "Moved-to must preserve length";
    EXPECT_EQ(moved.get(0), 0) << "Moved-to must preserve content";
    EXPECT_EQ(moved.get(199), 199) << "Moved-to must preserve last element";

    // Expanded: moved-from should be in a safe, empty state
    EXPECT_TRUE(arr.empty()) << "Moved-from should report empty";
    EXPECT_EQ(arr.length(), 0u) << "Moved-from length should be 0";
    EXPECT_THROW(arr.get(0), std::out_of_range) << "Moved-from must not allow get(0)";
}


// =======================================================================
//  Move assignment — presunie vnútorné dáta
// =======================================================================

TEST(PublicMethodsTest, MoveAssignmentTransfersOwnership) {
    TestArray arr;
    for (int i = 0; i < 120; ++i) arr.grow(1000 + i);

    TestArray moved;
    // Give destination some existing content to ensure it gets replaced safely.
    for (int i = 0; i < 50; ++i) moved.grow(-i);

    moved = std::move(arr);

    EXPECT_EQ(moved.length(), 120u) << "Moved-to must adopt length from source";
    EXPECT_EQ(moved.get(0), 1000) << "Moved-to must adopt content from source";
    EXPECT_EQ(moved.get(119), 1119) << "Moved-to must preserve last element";

    // Expanded: moved-from should be safe/empty
    EXPECT_TRUE(arr.empty()) << "Moved-from must be empty after move assignment";
    EXPECT_EQ(arr.length(), 0u);
    EXPECT_THROW(arr.get(0), std::out_of_range) << "Moved-from must not allow access";

    // Expanded: destination still usable after move assignment
    moved.grow(777);
    EXPECT_EQ(moved.get(moved.length() - 1), 777) << "Moved-to must remain usable after move assignment";
}

