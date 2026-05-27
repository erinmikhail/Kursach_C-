#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include "index/b_star_plus_tree.hpp" 

using namespace db_engine;

// 1. Базовый тест: Вставка и точный поиск (Point Query)
TEST(DbIndexTests, BasicInsertAndFind) {
    DbIndex index(5); // Степень 5
    
    index.insert(42, 100);
    index.insert(15, 200);
    index.insert(100, 300);

    auto res1 = index.find(42);
    ASSERT_EQ(res1.size(), 1);
    EXPECT_EQ(res1[0], 100);

    auto res2 = index.find(999); // Несуществующий ключ
    EXPECT_TRUE(res2.empty());
}

// 2. Тест на неуникальные ключи (Несколько строк с одинаковым значением)
TEST(DbIndexTests, DuplicateKeysHandling) {
    DbIndex index(5);
    
    // Представим, что у нас три Ивана с возрастом 25 (RowID: 1, 2, 3)
    index.insert(25, 1);
    index.insert(25, 2);
    index.insert(25, 3);

    auto res = index.find(25);
    ASSERT_EQ(res.size(), 3);
    
    // Проверяем, что все RowID на месте
    EXPECT_NE(std::find(res.begin(), res.end(), 1), res.end());
    EXPECT_NE(std::find(res.begin(), res.end(), 2), res.end());
    EXPECT_NE(std::find(res.begin(), res.end(), 3), res.end());
}

// 3. Тест на сплит (Разделение узлов при переполнении)
TEST(DbIndexTests, NodeSplitTrigger) {
    DbIndex index(2); // Степень 2: максимум 3 ключа в узле. Будет много сплитов!
    
    for (int i = 1; i <= 20; ++i) {
        index.insert(i * 10, i); // Ключи: 10, 20, 30... 200
    }

    // Проверяем, что ничего не потерялось после сплитов и роста дерева в высоту
    for (int i = 1; i <= 20; ++i) {
        auto res = index.find(i * 10);
        ASSERT_EQ(res.size(), 1);
        EXPECT_EQ(res[0], static_cast<RowID>(i));
    }
}

// 4. Тест на диапазонный поиск (Та самая фишка B*+ дерева для BETWEEN)
TEST(DbIndexTests, RangeSearchBetween) {
    DbIndex index(3); // Степень 3
    
    // Вставляем ключи вразброс, чтобы дерево их отсортировало
    std::vector<int> keys = {50, 10, 90, 20, 80, 30, 70, 40, 60, 100};
    for (size_t i = 0; i < keys.size(); ++i) {
        index.insert(keys[i], static_cast<RowID>(i + 1));
    }

    // Ищем значения BETWEEN 30 AND 70
    auto res = index.find_range(30, 70);
    
    // Должны найтись: 30, 40, 50, 60, 70 (всего 5 штук)
    ASSERT_EQ(res.size(), 5);
}

// 5. Тест на удаление (DELETE)
TEST(DbIndexTests, RemoveRowID) {
    DbIndex index(5);
    
    index.insert(25, 10);
    index.insert(25, 20); // Два человека с возрастом 25
    
    // Удаляем только одного
    index.remove(25, 10);
    
    auto res1 = index.find(25);
    ASSERT_EQ(res1.size(), 1);
    EXPECT_EQ(res1[0], 20); // Остался только второй
    
    // Удаляем последнего
    index.remove(25, 20);
    
    auto res2 = index.find(25);
    EXPECT_TRUE(res2.empty()); // Ключ 25 должен полностью исчезнуть
}