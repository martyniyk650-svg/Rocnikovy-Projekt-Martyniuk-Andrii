#include <gtest/gtest.h>

#include "../include/rarray.h"
#include "../include/rarray_impl.tpp"

#include <random>
#include <vector>
#include <limits>

// Jeden test presne podľa náčrtu na papieri:
// 1) 1000-krát: x = náhodné číslo,
//    vector.push_back(x), rarray.grow(x),
//    potom úplné porovnanie obsahu
// 2) 1000-krát: vector.pop_back(), rarray.shrink(),
//    potom opäť úplné porovnanie obsahu

using Arr = ResizableArray<int, 3>;

TEST(VectorComparisonTest, PaperScenario_AllInOne) {
    Arr rarray;
    std::vector<int> vec;

    // Deterministický generátor náhodných čísel (test je reprodukovateľný)
    std::mt19937 rng(123456u);
    std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());

    // -------------------------
    // FÁZA 1: vkladanie prvkov (push/grow) + úplné porovnanie po každom kroku
    // -------------------------
    for (int i = 1; i <= 1000; ++i) {
        const int x = dist(rng);

        vec.push_back(x);
        rarray.grow(x);

        // Dĺžky musia po každom vložení zodpovedať
        ASSERT_EQ(rarray.length(), vec.size())
            << "Nezhoda veľkosti po vložení prvku, i=" << i;

        // Úplné porovnanie obsahu (presne ako na papieri: for j = 0..length-1)
        for (size_t j = 0; j < vec.size(); ++j) {
            SCOPED_TRACE(::testing::Message()
                         << "fáza=vkladanie i=" << i << " j=" << j);

            // Používame get(), aby sme zachytili prípadné výnimky
            int r = 0;
            EXPECT_NO_THROW(r = rarray.get(j))
                << "rarray.get(j) neočakávane vyhodilo výnimku";
            EXPECT_EQ(vec[j], r)
                << "Nezhoda hodnôt po vložení: vec[j] != rarray[j]";
        }
    }

    // -------------------------
    // FÁZA 2: odstraňovanie prvkov (pop_back/shrink) + úplné porovnanie po každom kroku
    // -------------------------
    for (int i = 1; i <= 1000; ++i) {
        ASSERT_FALSE(vec.empty())
            << "std::vector je neočakávane prázdny pred pop_back, i=" << i;
        ASSERT_FALSE(rarray.empty())
            << "rarray je neočakávane prázdny pred shrink, i=" << i;

        vec.pop_back();
        rarray.shrink();

        // Dĺžky musia po každom odstránení zodpovedať
        ASSERT_EQ(rarray.length(), vec.size())
            << "Nezhoda veľkosti po odstránení prvku, i=" << i;

        // Úplné porovnanie zostávajúceho prefixu
        for (size_t j = 0; j < vec.size(); ++j) {
            SCOPED_TRACE(::testing::Message()
                         << "fáza=odstraňovanie i=" << i << " j=" << j);

            int r = 0;
            EXPECT_NO_THROW(r = rarray.get(j))
                << "rarray.get(j) neočakávane vyhodilo výnimku";
            EXPECT_EQ(vec[j], r)
                << "Nezhoda hodnôt po odstránení: vec[j] != rarray[j]";
        }
    }

    // Po 1000 odstráneniach musia byť obe štruktúry prázdne
    EXPECT_TRUE(vec.empty()) << "std::vector musí byť na konci prázdny";
    EXPECT_TRUE(rarray.empty()) << "rarray musí byť na konci prázdny";
    EXPECT_EQ(rarray.length(), 0u) << "rarray.length() musí byť na konci 0";

    // Negatívny test: shrink na prázdnom poli musí vyhodiť výnimku
    EXPECT_THROW(rarray.shrink(), std::out_of_range)
        << "shrink() na prázdnom poli musí vyhodiť výnimku";
}
