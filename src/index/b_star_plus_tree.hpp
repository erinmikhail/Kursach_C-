#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <associative_container.h>
#include <initializer_list>
#include <not_implemented.h>

#ifndef SYS_PROG_BS_PLUS_TREE_H
#define SYS_PROG_BS_PLUS_TREE_H

template <typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BSP_tree final : private compare
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    // TODO: Another restrictions
    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration

    struct bsptree_node_base
    {
        bool _is_terminated;

        bsptree_node_base() noexcept;
        virtual ~bsptree_node_base() =default;
    };

    struct bsptree_node_term : public bsptree_node_base
    {
        bsptree_node_term* _next;
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _data;
        bsptree_node_term() noexcept;
    };

    struct bsptree_node_middle : public bsptree_node_base
    {
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bsptree_node_base*, maximum_keys_in_node + 2> _pointers;
        bsptree_node_middle() noexcept;
    };

    pp_allocator<value_type> _allocator;
    bsptree_node_base* _root;
    size_t _size;

    pp_allocator<value_type> get_allocator() const noexcept;

public:

    // region constructors declaration

    explicit BSP_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    explicit BSP_tree(pp_allocator<value_type> alloc, const compare& comp = compare());

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BSP_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    BSP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>());

    // endregion constructors declaration

    // region five declaration

    BSP_tree(const BSP_tree& other);

    BSP_tree(BSP_tree&& other) noexcept;

    BSP_tree& operator=(const BSP_tree& other);

    BSP_tree& operator=(BSP_tree&& other) noexcept;

    ~BSP_tree() noexcept;

    // endregion five declaration

    // region iterators declaration

    class bsptree_iterator;
    class bsptree_const_iterator;

    class bsptree_iterator final
    {
        bsptree_node_term* _node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bsptree_iterator;

        friend class BSP_tree;
        friend class bsptree_const_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bsptree_iterator(bsptree_node_term* node = nullptr, size_t index = 0);

    };

    class bsptree_const_iterator final
    {
        const bsptree_node_term* _node;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bsptree_const_iterator;

        friend class BSP_tree;
        friend class bsptree_iterator;

        bsptree_const_iterator(const bsptree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bsptree_const_iterator(const bsptree_node_term* node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    bsptree_iterator begin();
    bsptree_iterator end();

    bsptree_const_iterator begin() const;
    bsptree_const_iterator end() const;

    bsptree_const_iterator cbegin() const;
    bsptree_const_iterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bsptree_iterator find(const tkey& key);
    bsptree_const_iterator find(const tkey& key) const;

    bsptree_iterator lower_bound(const tkey& key);
    bsptree_const_iterator lower_bound(const tkey& key) const;

    bsptree_iterator upper_bound(const tkey& key);
    bsptree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bsptree_iterator, bool> insert(const tree_data_type& data);
    std::pair<bsptree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<bsptree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bsptree_iterator insert_or_assign(const tree_data_type& data);
    bsptree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    bsptree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bsptree_iterator erase(bsptree_iterator pos);
    bsptree_iterator erase(bsptree_const_iterator pos);

    bsptree_iterator erase(bsptree_iterator beg, bsptree_iterator en);
    bsptree_iterator erase(bsptree_const_iterator beg, bsptree_const_iterator en);


    bsptree_iterator erase(const tkey& key);

    // endregion modifiers declaration
};

template<std::input_iterator iterator, comparator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
BSP_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BSP_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BSP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>()) -> BSP_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::compare_pairs(const BSP_tree::tree_data_type &lhs,
                                                      const BSP_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}

// region bsptree_node_base implementation

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_base::bsptree_node_base() noexcept
{
    _is_terminated = false;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_term::bsptree_node_term() noexcept
{
    this->_is_terminated = true;
    this->_next = nullptr;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_node_middle::bsptree_node_middle() noexcept
{
    this->_is_terminated = false;
}

// region BSP_tree constructor implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
pp_allocator<typename BSP_tree<tkey, tvalue, compare, t>::value_type> BSP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept
{
    return _allocator;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::bsptree_const_iterator(const bsptree_node_term *node,
    size_t index) : _node(node), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(const compare& cmp, pp_allocator<value_type> alloc) : compare(cmp), _allocator(alloc), _root(nullptr), _size(0){}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(pp_allocator<value_type> alloc, const compare& cmp) : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)  {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc) : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto it = begin; it != end; ++it) {
        emplace(it->first, it->second);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp, pp_allocator<value_type> alloc) : compare(cmp), _allocator(alloc), _root(nullptr), _size(0)
{
    for (const auto& item : data) {
        emplace(item.first, item.second);
    }
}

// endregion BSP_tree constructor implementations

// region BSP_tree copy and move constructors

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(const BSP_tree& other) : compare(other), _allocator(other._allocator), _root(nullptr), _size(0)
{
    for (auto it = other.cbegin(); it != other.cend(); ++it) {
        emplace(it->first, it->second);
    }
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::BSP_tree(BSP_tree&& other) noexcept : compare(std::move(other)), _allocator(std::move(other._allocator)), _root(other._root), _size(other._size)
{
    other._root = nullptr;
    other._size = 0;
}

// endregion BSP_tree copy and move constructors

// region BSP_tree copy and move assignment operators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>& BSP_tree<tkey, tvalue, compare, t>::operator=(const BSP_tree& other)
{
    if (this != &other) {
        clear(); 
        compare::operator=(other); 
        _allocator = other._allocator; 

        for (auto it = other.cbegin(); it != other.cend(); ++it) { 
            emplace(it->first, it->second); 
        }
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>& BSP_tree<tkey, tvalue, compare, t>::operator=(BSP_tree&& other) noexcept
{
    if (this != &other) {
        clear();
        compare::operator=(std::move(other));
        _allocator = std::move(other._allocator);
        _root = other._root;
        _size = other._size;
        other._root = nullptr;
        other._size = 0;
    }
    return *this;
}

// endregion BSP_tree copy and move assignment operators

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::~BSP_tree() noexcept
{
    clear();
}

// region BSP_tree iterators implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::bsptree_iterator(bsptree_node_term* node, size_t index) : _node(node), _index(index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::reference BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::pointer BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator->() const noexcept
{
    return reinterpret_cast<pointer>(&_node->_data[_index]);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator& BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator++()
{
    if (_node) {
        ++_index;
        while (_node && _index >= _node->_data.size()) {
            _node = _node->_next;
            _index = 0;
        }
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator++(int)
{
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator==(const self& other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::current_node_keys_count() const noexcept
{
    return _node ? _node->_data.size() : 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::bsptree_const_iterator(const bsptree_iterator& it) noexcept : _node(it._node), _index(it._index) {}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::reference BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator*() const noexcept
{
    return *reinterpret_cast<pointer>(const_cast<tree_data_type*>(&_node->_data[_index]));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::pointer BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator->() const noexcept
{
    return reinterpret_cast<pointer>(const_cast<tree_data_type*>(&_node->_data[_index]));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator& BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator++()
{
    if (_node) {
        ++_index;
        while (_node && _index >= _node->_data.size()) {
            _node = _node->_next;
            _index = 0;
        }
    }
    return *this;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator++(int)
{
    self tmp = *this;
    ++(*this);
    return tmp;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator==(const self& other) const noexcept
{
    return _node == other._node && _index == other._index;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::current_node_keys_count() const noexcept
{
    return _node ? _node->_data.size() : 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator::index() const noexcept
{
    return _index;
}

// endregion BSP_tree iterators implementations

// region BSP_tree element access implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BSP_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto it = find(key);

    if (it == end()) throw std::out_of_range("Key not found");
    return it->second; 
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
const tvalue& BSP_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto it = find(key);

    if (it == end()) throw std::out_of_range("Key not found");
    return it->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BSP_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto it = find(key);
    if (it != end()) return it->second;
    return emplace(key, tvalue{}).first->second;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
tvalue& BSP_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto it = find(key);
    if (it != end()) return it->second;
    return emplace(std::move(key), tvalue{}).first->second;
}

// endregion BSP_tree element access implementations

// region BSP_tree iterator begins implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::begin()
{
    if (!_root) return end();
    bsptree_node_base* curr = _root;

    while (!curr->_is_terminated) {
        curr = static_cast<bsptree_node_middle*>(curr)->_pointers.front();
    }
    
    auto* leaf = static_cast<bsptree_node_term*>(curr);
    while (leaf && leaf->_data.empty()) {
        leaf = leaf->_next;
    }

    return bsptree_iterator(leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::end()
{
    return bsptree_iterator(nullptr, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::begin() const
{
    return cbegin();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::end() const
{
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::cbegin() const
{
    if (!_root) return cend();
    bsptree_node_base* curr = _root;
    while (!curr->_is_terminated) {
        curr = static_cast<bsptree_node_middle*>(curr)->_pointers.front();
    }
    
    auto* leaf = static_cast<const bsptree_node_term*>(curr);
    while (leaf && leaf->_data.empty()) {
        leaf = leaf->_next;
    }
    return bsptree_const_iterator(leaf, 0);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::cend() const
{
    return bsptree_const_iterator(nullptr, 0);
}

// endregion BSP_tree iterator begins implementations

// region BSP_tree lookup implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
size_t BSP_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    if (!_root) return end();

    bsptree_node_base* curr = _root;

    while (!curr->_is_terminated){
        auto* internal = static_cast<bsptree_node_middle*>(curr);

        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key, [this](const tkey& k1, const tkey& k2) {return compare_keys(k1, k2); });

        size_t child_idx = std::distance(internal->_keys.begin(), it); 
        curr = internal->_pointers[child_idx];
    }

    auto* leaf = static_cast<bsptree_node_term*>(curr);
    for (size_t i = 0; i < leaf->_data.size(); i++){
        if (!compare_keys(key, leaf->_data[i].first) && !compare_keys(leaf->_data[i].first, key)){
            return bsptree_iterator(leaf, i);
        }
    }

    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    if (!_root) return cend();
    bsptree_node_base* curr = _root;
    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key, [this](const tkey& k1, const tkey& k2) {return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }
    auto* leaf = static_cast<bsptree_node_term*>(curr);
    for (size_t i = 0; i < leaf->_data.size(); i++){
        if (!compare_keys(key, leaf->_data[i].first) && !compare_keys(leaf->_data[i].first, key)){
            return bsptree_const_iterator(leaf, i);
        }
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    if (!_root) return end();
    bsptree_node_base* curr = _root;

    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key, 
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }

    auto* leaf = static_cast<bsptree_node_term*>(curr);
    while (leaf) {

        auto it = std::lower_bound(leaf->_data.begin(), leaf->_data.end(), key, 
            [this](const tree_data_type& d, const tkey& k) { return compare_keys(d.first, k); });

        if (it != leaf->_data.end()) {
            return bsptree_iterator(leaf, std::distance(leaf->_data.begin(), it));
        }
        leaf = leaf->_next;
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    if (!_root) return cend();
    bsptree_node_base* curr = _root;
    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key, 
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }
    auto* leaf = static_cast<bsptree_node_term*>(curr);
    while (leaf) {
        auto it = std::lower_bound(leaf->_data.begin(), leaf->_data.end(), key, 
            [this](const tree_data_type& d, const tkey& k) { return compare_keys(d.first, k); });
        if (it != leaf->_data.end()) return bsptree_const_iterator(leaf, std::distance(leaf->_data.begin(), it));
        leaf = leaf->_next;
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    if (!_root) return end();
    bsptree_node_base* curr = _root;
    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key, 
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }
    auto* leaf = static_cast<bsptree_node_term*>(curr);
    while (leaf) {
        auto it = std::upper_bound(leaf->_data.begin(), leaf->_data.end(), key, 
            [this](const tkey& k, const tree_data_type& d) { return compare_keys(k, d.first); });
        if (it != leaf->_data.end()) return bsptree_iterator(leaf, std::distance(leaf->_data.begin(), it));
        leaf = leaf->_next;
    }
    return end();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_const_iterator BSP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    if (!_root) return cend();
    bsptree_node_base* curr = _root;
    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key, 
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }
    auto* leaf = static_cast<bsptree_node_term*>(curr);
    while (leaf) {
        auto it = std::upper_bound(leaf->_data.begin(), leaf->_data.end(), key, 
            [this](const tkey& k, const tree_data_type& d) { return compare_keys(k, d.first); });
        if (it != leaf->_data.end()) return bsptree_const_iterator(leaf, std::distance(leaf->_data.begin(), it));
        leaf = leaf->_next;
    }
    return cend();
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
bool BSP_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    return find(key) != end();
}

// endregion BSP_tree lookup implementations

// region BSP_tree modifiers implementations

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
void BSP_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    if (!_root) return;

    using leaf_alloc_t = typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<bsptree_node_term>;
    using int_alloc_t =  typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<bsptree_node_middle>;
    leaf_alloc_t l_alloc(_allocator);
    int_alloc_t i_alloc(_allocator);

    std::stack<bsptree_node_base*> st;
    st.push(_root);
    while (!st.empty()) {
        auto* curr = st.top();
        st.pop();              
        
        if (!curr->_is_terminated) {
            auto* internal = static_cast<bsptree_node_middle*>(curr);

            for (auto* child : internal->_pointers) {
                st.push(child);
            }

            std::allocator_traits<int_alloc_t>::destroy(i_alloc, internal);
            std::allocator_traits<int_alloc_t>::deallocate(i_alloc, internal, 1);
        } else {
            auto* leaf = static_cast<bsptree_node_term*>(curr);
            std::allocator_traits<leaf_alloc_t>::destroy(l_alloc, leaf);
            std::allocator_traits<leaf_alloc_t>::deallocate(l_alloc, leaf, 1);
        }
    }
    
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return emplace(data);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return emplace(std::move(data));
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename ...Args>
std::pair<typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator, bool> BSP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type new_data(std::forward<Args>(args)...);

    using leaf_alloc_t = typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<bsptree_node_term>;
    using int_alloc_t = typename std::allocator_traits<pp_allocator<value_type>>::template rebind_alloc<bsptree_node_middle>;
    leaf_alloc_t l_alloc(_allocator);
    int_alloc_t i_alloc(_allocator);

    if (!_root) {

        auto* leaf = std::allocator_traits<leaf_alloc_t>::allocate(l_alloc, 1);

        std::allocator_traits<leaf_alloc_t>::construct(l_alloc, leaf);

        leaf->_data.push_back(std::move(new_data));
        _root = leaf; 
        _size++;

        return {bsptree_iterator(leaf, 0), true};
    }

    std::stack<bsptree_node_middle*> path;
    bsptree_node_base* curr = _root;

    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        path.push(internal); 

        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), new_data.first,
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }

    auto* leaf = static_cast<bsptree_node_term*>(curr);

    auto it = std::lower_bound(leaf->_data.begin(), leaf->_data.end(), new_data,
        [this](const tree_data_type& d1, const tree_data_type& d2) { return compare_keys(d1.first, d2.first); });

    if (it != leaf->_data.end() && !compare_keys(new_data.first, it->first) && !compare_keys(it->first, new_data.first)) {
        return {bsptree_iterator(leaf, std::distance(leaf->_data.begin(), it)), false};
    }

    size_t insert_idx = std::distance(leaf->_data.begin(), it);
    leaf->_data.insert(leaf->_data.begin() + insert_idx, std::move(new_data));
    _size++;

    if (leaf->_data.size() <= maximum_keys_in_node) {
        return {bsptree_iterator(leaf, insert_idx), true};
    }

    auto* new_leaf = std::allocator_traits<leaf_alloc_t>::allocate(l_alloc, 1);
    std::allocator_traits<leaf_alloc_t>::construct(l_alloc, new_leaf);
    
    size_t mid = leaf->_data.size() / 2;

    for (size_t i = mid; i < leaf->_data.size(); ++i) {
        new_leaf->_data.push_back(std::move(leaf->_data[i]));
    }
    leaf->_data.resize(mid);

    new_leaf->_next = leaf->_next;
    leaf->_next = new_leaf;

    tkey split_key = new_leaf->_data.front().first;
    bsptree_node_base* right_child = new_leaf;

    while (!path.empty()) {
        auto* parent = path.top(); 
        path.pop();                

        auto pit = std::upper_bound(parent->_keys.begin(), parent->_keys.end(), split_key,
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        size_t p_idx = std::distance(parent->_keys.begin(), pit);

        parent->_keys.insert(parent->_keys.begin() + p_idx, split_key);
        parent->_pointers.insert(parent->_pointers.begin() + p_idx + 1, right_child);

        if (parent->_keys.size() <= maximum_keys_in_node) {
            return {find(new_data.first), true}; 
        }

        auto* new_internal = std::allocator_traits<int_alloc_t>::allocate(i_alloc, 1);
        std::allocator_traits<int_alloc_t>::construct(i_alloc, new_internal);
        
        size_t pmid = parent->_keys.size() / 2;
        split_key = parent->_keys[pmid]; 

        for (size_t i = pmid + 1; i < parent->_keys.size(); ++i) {
            new_internal->_keys.push_back(std::move(parent->_keys[i]));
        }
        for (size_t i = pmid + 1; i < parent->_pointers.size(); ++i) {
            new_internal->_pointers.push_back(parent->_pointers[i]);
        }

        parent->_keys.resize(pmid);
        parent->_pointers.resize(pmid + 1);

        right_child = new_internal;
    }

    auto* new_root = std::allocator_traits<int_alloc_t>::allocate(i_alloc, 1);
    std::allocator_traits<int_alloc_t>::construct(i_alloc, new_root);
    
    new_root->_keys.push_back(split_key); 
    new_root->_pointers.push_back(_root);   
    new_root->_pointers.push_back(right_child); 
    
    _root = new_root; 

    return {find(new_data.first), true};
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = data.second;
        return it;
    }
    return emplace(data).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    auto it = find(data.first);
    if (it != end()) {
        it->second = std::move(data.second);
        return it;
    }
    return emplace(std::move(data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
template<typename ...Args>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type new_data(std::forward<Args>(args)...);
    
    auto it = find(new_data.first);
    if (it != end()) {
        it->second = std::move(new_data.second);
        return it;
    }
    return emplace(std::move(new_data)).first;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_iterator pos)
{
    if (pos == end()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_const_iterator pos)
{
    if (pos == cend()) return end();
    return erase(pos->first);
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_iterator beg, bsptree_iterator en)
{
    std::vector<tkey> keys_to_erase;
    for (auto it = beg; it != en; ++it) {
        keys_to_erase.push_back(it->first);
    }
    
    bsptree_iterator result = end();
    for (const auto& k : keys_to_erase) {
        result = erase(k);
    }
    return result;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(bsptree_const_iterator beg, bsptree_const_iterator en)
{
    std::vector<tkey> keys_to_erase;
    for (auto it = beg; it != en; ++it) {
        keys_to_erase.push_back(it->first);
    }
    
    bsptree_iterator result = end();
    for (const auto& k : keys_to_erase) {
        result = erase(k);
    }
    return result;
}

template<typename tkey, typename tvalue, comparator<tkey> compare, std::size_t t>
typename BSP_tree<tkey, tvalue, compare, t>::bsptree_iterator BSP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (!_root) return end();
    bsptree_node_base* curr = _root;

    while (!curr->_is_terminated) {
        auto* internal = static_cast<bsptree_node_middle*>(curr);
        auto it = std::upper_bound(internal->_keys.begin(), internal->_keys.end(), key,
            [this](const tkey& k1, const tkey& k2) { return compare_keys(k1, k2); });
        curr = internal->_pointers[std::distance(internal->_keys.begin(), it)];
    }

    auto* leaf = static_cast<bsptree_node_term*>(curr);
    for (size_t i = 0; i < leaf->_data.size(); ++i) {
        if (!compare_keys(key, leaf->_data[i].first) && !compare_keys(leaf->_data[i].first, key)) {

            leaf->_data.erase(leaf->_data.begin() + i);
            _size--;

            bsptree_iterator ret(leaf, i);
            while (ret._node && ret._index >= ret._node->_data.size()) {
                ret._node = ret._node->_next;
                ret._index = 0;
            }
            return ret;
        }
    }
    return end();
}

// endregion BSP_tree modifiers implementations


#endif