#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <stack>

namespace db_engine {

using RowID = uint32_t;

struct Node {
    bool is_leaf;
    virtual ~Node() = default;
};

// Терминальный узел (Лист) - хранит сами данные
struct LeafNode : public Node {
    std::vector<int> keys;                           // Индексируемые значения (например, возраст)
    std::vector<std::vector<RowID>> values;          // Массивы ID строк (вектор, т.к. возраст может повторяться)
    LeafNode* next = nullptr;                        // Указатель на соседа для быстрого BETWEEN

    LeafNode() { is_leaf = true; }
};

// Внутренний узел - хранит только ключи для маршрутизации
struct InternalNode : public Node {
    std::vector<int> keys;                           // Ключи-разделители
    std::vector<Node*> children;                     // Указатели на дочерние узлы

    InternalNode() { is_leaf = false; }
    
    ~InternalNode() {
        for (auto child : children) {
            delete child; // Каскадное удаление дерева
        }
    }
};

class DbIndex {
private:
    Node* root;
    size_t degree; // Степень дерева (t)
    size_t max_keys;

public:
    // По умолчанию степень = 5 (максимум 9 ключей в узле)
    explicit DbIndex(size_t t = 5) : degree(t), max_keys(2 * t - 1) {
        root = new LeafNode();
    }

    ~DbIndex() {
        delete root;
    }

    void insert(int key, RowID row_id) {
        std::stack<InternalNode*> path;
        Node* curr = root;

        // Спуск до листа с запоминанием пути
        while (!curr->is_leaf) {
            auto internal = static_cast<InternalNode*>(curr);
            path.push(internal);
            auto it = std::upper_bound(internal->keys.begin(), internal->keys.end(), key);
            size_t idx = std::distance(internal->keys.begin(), it);
            curr = internal->children[idx];
        }

        auto leaf = static_cast<LeafNode*>(curr);
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        size_t idx = std::distance(leaf->keys.begin(), it);

        // Если ключ уже есть, просто добавляем RowID к существующему списку
        if (it != leaf->keys.end() && *it == key) {
            leaf->values[idx].push_back(row_id);
            return;
        }

        // Вставляем новый ключ и значение со сдвигом
        leaf->keys.insert(leaf->keys.begin() + idx, key);
        leaf->values.insert(leaf->values.begin() + idx, {row_id});

        // Если нет переполнения - балансировка не нужна
        if (leaf->keys.size() <= max_keys) return;

        // Сплит листа
        auto new_leaf = new LeafNode();
        size_t mid = leaf->keys.size() / 2;

        new_leaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
        new_leaf->values.assign(leaf->values.begin() + mid, leaf->values.end());
        
        leaf->keys.resize(mid);
        leaf->values.resize(mid);

        // Поддерживаем связный список листьев
        new_leaf->next = leaf->next;
        leaf->next = new_leaf;

        int split_key = new_leaf->keys[0];
        Node* right_child = new_leaf;

        while (!path.empty()) {
            auto parent = path.top();
            path.pop();

            auto pit = std::upper_bound(parent->keys.begin(), parent->keys.end(), split_key);
            size_t p_idx = std::distance(parent->keys.begin(), pit);

            parent->keys.insert(parent->keys.begin() + p_idx, split_key);
            parent->children.insert(parent->children.begin() + p_idx + 1, right_child);

            if (parent->keys.size() <= max_keys) return; // Родитель не переполнился

            // Сплит внутреннего узла
            auto new_internal = new InternalNode();
            size_t pmid = parent->keys.size() / 2;
            split_key = parent->keys[pmid]; // Ключ уходит наверх, не оставаясь в узлах

            new_internal->keys.assign(parent->keys.begin() + pmid + 1, parent->keys.end());
            new_internal->children.assign(parent->children.begin() + pmid + 1, parent->children.end());

            parent->keys.resize(pmid);
            parent->children.resize(pmid + 1);

            right_child = new_internal;
        }

        // Если дошли сюда, значит переполнился корень
        auto new_root = new InternalNode();
        new_root->keys.push_back(split_key);
        new_root->children.push_back(root);
        new_root->children.push_back(right_child);
        root = new_root;
    }

    std::vector<RowID> find(int key) const {
        Node* curr = root;
        while (!curr->is_leaf) {
            auto internal = static_cast<InternalNode*>(curr);
            auto it = std::upper_bound(internal->keys.begin(), internal->keys.end(), key);
            curr = internal->children[std::distance(internal->keys.begin(), it)];
        }

        auto leaf = static_cast<LeafNode*>(curr);
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        
        if (it != leaf->keys.end() && *it == key) {
            return leaf->values[std::distance(leaf->keys.begin(), it)];
        }
        return {};
    }

    std::vector<RowID> find_range(int min_key, int max_key) const {
        std::vector<RowID> result;
        Node* curr = root;

        // Быстрый спуск до минимального ключа
        while (!curr->is_leaf) {
            auto internal = static_cast<InternalNode*>(curr);
            auto it = std::upper_bound(internal->keys.begin(), internal->keys.end(), min_key);
            curr = internal->children[std::distance(internal->keys.begin(), it)];
        }

        auto leaf = static_cast<LeafNode*>(curr);
        
        // Линейный проход по связанному списку листьев (фишка B+ дерева)
        while (leaf) {
            for (size_t i = 0; i < leaf->keys.size(); ++i) {
                if (leaf->keys[i] > max_key) return result; // Вышли за верхнюю границу
                if (leaf->keys[i] >= min_key) {
                    // Копируем все найденные RowID в результат
                    result.insert(result.end(), leaf->values[i].begin(), leaf->values[i].end());
                }
            }
            leaf = leaf->next; // Прыгаем в следующий лист
        }
        return result;
    }

    void remove(int key, RowID row_id) {
        Node* curr = root;
        while (!curr->is_leaf) {
            auto internal = static_cast<InternalNode*>(curr);
            auto it = std::upper_bound(internal->keys.begin(), internal->keys.end(), key);
            curr = internal->children[std::distance(internal->keys.begin(), it)];
        }

        auto leaf = static_cast<LeafNode*>(curr);
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        
        if (it != leaf->keys.end() && *it == key) {
            size_t idx = std::distance(leaf->keys.begin(), it);
            auto& rows = leaf->values[idx];
            
            auto pos = std::find(rows.begin(), rows.end(), row_id);
            if (pos != rows.end()) {
                rows.erase(pos);
            }
            
            // Если в этом ключе больше нет строк, удаляем сам ключ
            if (rows.empty()) {
                leaf->keys.erase(leaf->keys.begin() + idx);
                leaf->values.erase(leaf->values.begin() + idx);
            }
        }
    }
};

} 