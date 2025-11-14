#pragma once
#include <utility>
#include <stdexcept>
#include <iterator>
#include <functional>
#include <type_traits>
template <typename T>
class Vector {
    T* data = nullptr;
    size_t size = 0;
    size_t capacity = 0;
public: 
    using ValueType = T;
    using Pointer = T*;
    using ConstPointer = const T*;
    using Reference = T&;
    using ConstReference = const T&;
    using SizeType = size_t;
    
    template <bool IsConst>
    struct UniversalIterator : public std::iterator<
                        std::random_access_iterator_tag,   
                        T,                     
                        long,                      
                        std::conditional_t<IsConst, const T*, T*>,               
                        std::conditional_t<IsConst, const T&, T&> >
    {
    private:
        std::conditional_t<IsConst, const T*, T*> ptr = nullptr;
    public:
        UniversalIterator(const std::conditional_t<IsConst, const T*, T*>& ptr): ptr(ptr) {}
        UniversalIterator(std::conditional_t<IsConst, const T*, T*>&& ptr): ptr(std::move(ptr)) {}
        UniversalIterator(std::move_iterator<UniversalIterator<IsConst>>&& other): ptr(std::move(other.ptr)) {}

        std::conditional_t<IsConst, const T&, T&> operator*() const { return *ptr; }
        std::conditional_t<IsConst, const T*, T*> operator->() const { return ptr; }
        bool operator!=(const UniversalIterator& other) const { return other.ptr != ptr; }
        bool operator==(const UniversalIterator& other) const { return other.ptr == ptr; }
        UniversalIterator& operator+(long int num) { 
            ptr += num;
            return *this; 
        }
        UniversalIterator& operator-(long int num) { 
            ptr -= num;
            return *this; 
        }
        UniversalIterator operator-(long int num) const { 
            UniversalIterator copy = *this;
            copy.ptr -= num;
            return copy; 
        }
        UniversalIterator& operator--() { 
            --ptr;
            return *this; 
        }
        UniversalIterator& operator++() {
            ++ptr;
            return *this;
        } 
        size_t distance(UniversalIterator& other) { return other.ptr - ptr; }
    };

    using Iterator = UniversalIterator<false>;
    using ConstIterator = UniversalIterator<true>;
    using ReverseIterator = std::reverse_iterator<Vector<T>::Iterator>;
    using ConstReverseIterator = std::reverse_iterator<Vector<T>::ConstIterator>;

    ConstIterator begin() const { return ConstIterator(data); }
    ConstIterator end() const { return ConstIterator(data + size); }
    Iterator begin() { return Iterator(data); }
    Iterator end() { return Iterator(data + size); }
    ConstIterator cbegin() const { return ConstIterator(data); }
    ConstIterator cend() const { return ConstIterator(data + size); }

    ConstReverseIterator rbegin() const { return ConstReverseIterator(data + size); }
    ConstReverseIterator rend() const { return ConstReverseIterator(data); }
    ReverseIterator rbegin() { return ReverseIterator(data + size); }
    ReverseIterator rend() { return ReverseIterator(data); }
    ConstReverseIterator crbegin() const { return ConstReverseIterator(data + size); }
    ConstReverseIterator crend() const { return ConstReverseIterator(data); }
    
    Vector(ConstIterator start, ConstIterator end): size(start.distance(end)), capacity(size) {
        if (start != end) {
            data = static_cast<T*>(operator new(sizeof(T) * capacity));;
            for (size_t index = 0; index < capacity; ++index) {
                new(data + index) T(std::move(*(start)));
                ++start;
            }
        }
    }
    template <typename Iter>
    Vector(Iter start, Iter end): size(std::distance(start, end)), capacity(size) 
    { 
        if (start != end) {
            data = static_cast<T*>(operator new(sizeof(T) * capacity));
            size_t index = 0;
            try {
                for (; index < capacity; ++index) {
                    new(data + index) T(*(start));
                    ++start;
                } 
            } catch (...) {
                for (size_t i = 0; i < index; ++i) data[i].~T();
                operator delete(data);
                size = 0;
                capacity = 0;
                data = nullptr;
                throw;
            }
        }
    }
    template <typename U>
    Vector(std::move_iterator<U>&& start, 
           std::move_iterator<U>&& end): size(std::distance(start, end)), capacity(size) 
    { 
        if (start != end) {
            data = static_cast<T*>(operator new(sizeof(T) * capacity));
            for (size_t index = 0; index < capacity; ++index) {
                new(data + index) T(std::move(*(start)));
                ++start;
            }
        }
    }

    Vector() = default;
    Vector(size_t num): size(num), capacity(num) {
        if (num == 0) return;
        data = static_cast<T*>(operator new(sizeof(T) * num));
        size_t index = 0;
        try {
            for (; index < num; ++index) {
                new(data + index) T();
            }
        } catch (...) {
            for (size_t i = 0; i < index; ++i) data[i].~T();
            operator delete(data);
            size = 0;
            capacity = 0;
            data = nullptr;
            throw;
        }
    }
    Vector(int num, const T& object): size(num), capacity(num) {
        if (num == 0) return;
        data = static_cast<T*>(operator new(sizeof(T) * num));
        int index = 0;
        try {
            for (; index < num; ++index) 
                new(data + index) T(std::move(object));
        } catch (...) {
            for (int i = 0; i < index; ++i) data[i].~T();
            operator delete(data);
            size = 0;
            capacity = 0;
            data = nullptr;
            throw;
        }
    }
    Vector(std::initializer_list<T> init_list): size(init_list.size()), capacity(init_list.size()) {
        if (init_list.size() == 0) return;
        data = static_cast<T*>(operator new(sizeof(T) * init_list.size()));
        size_t index = 0;
        auto it = init_list.begin();
        try {
            for (; it != init_list.end(); ++it, ++index)
                new(data + index) T(*it);
        } catch (...) {
            for (size_t i = 0; i < index; ++i) data[i].~T();
                operator delete(data);
                size = 0;
                capacity = 0;
                data = nullptr;
                throw;
        }

    }
    Vector(const Vector& other): size(other.size), capacity(other.capacity) {
        if (size == 0) return;
        data = static_cast<T*>(operator new(sizeof(T) * size));
        size_t index = 0;
        try {
            for (; index < size; ++index) new(data + index) T(other[index]);
        } catch (...) {
            for (size_t i = 0; i < index; ++i) data[i].~T();
                operator delete(data);
                size = 0;
                capacity = 0;
                data = nullptr;
                throw;
        }
    }
    Vector(Vector&& other): data(other.data), size(other.size), capacity(other.capacity) {
        other.size = 0;
        other.capacity = 0;
        other.data = nullptr;
    }
    Vector& operator=(const Vector& other) {
        Vector copy = other;
        this->Swap(copy);
        return *this;
    }
    Vector& operator=(Vector&& other) {
        Vector copy = std::move(other);
        Swap(copy);
        return *this;
    }

    void Swap(Vector &other) {
        std::swap(other.size, size);
        std::swap(other.capacity, capacity);
        std::swap(other.data, data);
    }
    size_t Size() const { return size; }
    size_t Capacity() const { return capacity; }
    T* Data() { return data; }
    const T* Data() const { return data; }
    bool Empty() const { return size == 0 ? true : false; }
    const T& Back() const { return data[size - 1]; }
    T& Back() { return data[size - 1]; }
    const T& Front() const { return data[0]; }
    T& Front() { return data[0]; }
    const T& At(size_t index) const {
        if (index >= size) 
            throw std::out_of_range("Wrong index");
        return data[index];
    } 
    T& At(size_t index) {
        if (index >= size) 
            throw std::out_of_range("Wrong index");
        return data[index];
    } 
    const T& operator[](size_t index) const { return data[index]; }
    T& operator[](size_t index) { return data[index]; }

    void Clear() {
        for (size_t index = 0; index < size; ++index) 
            data[index].~T();
        size = 0;
    }
    void Resize(size_t count, T value = T()) {
        if (count <= size) {
            for (size_t index = count; index < size; ++index) 
                data[index].~T();
            size = count;
        }
        else if (count <= capacity) {
            for (size_t index = size; index < count; ++index)
                new(data + index) T(std::move(value));
            size = count;
        } else {
            T* new_data = static_cast<T*>(operator new(sizeof(T) * count));
            size_t index = 0;
            try {
                for (; index < size; ++index) new(new_data + index) T(); 
                for (; index < count; ++index) new(new_data + index) T(std::move(value));
            } catch (...) {
                for (size_t i = 0; i < index; ++i) new_data[i].~T();
                operator delete(new_data);
                throw;
            }
            for (size_t index = 0; index < size; ++index) {
                new_data[index] = std::move(data[index]);
                data[index].~T();
            }
            operator delete(data);
            data = new_data;
            size = count;
            capacity = count;
        }
    }
    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity) return;
        T* new_data = static_cast<T*>(operator new(sizeof(T) * new_capacity));
        size_t index = 0;
        try {
            for (; index < size; ++index) {
                new(new_data + index) T();
            }
        } catch (...) {
            for (size_t i = 0; i < index; ++i) new_data[i].~T();
                operator delete(new_data);
            throw;
        }
        for (size_t index = 0; index < size; ++index) {
                new_data[index] = std::move(data[index]);
                data[index].~T();
        }
        operator delete(data);
        data = new_data;
        capacity = new_capacity;
    }
    void ShrinkToFit() {
        if (size == capacity) return;
        if (size == 0) {
            for (size_t index = 0; index < size; ++index) 
            data[index].~T();
            if (capacity != 0) operator delete(data);
            data = nullptr;
            capacity = 0;
            size = 0;
            return;
        }
        T* new_data = static_cast<T*>(operator new(sizeof(T) * size));
        size_t index = 0;
        try {
            for (; index < size; ++index)
                new(new_data + index) T(std::move(data[index]));
        } catch (...) {
            for (size_t i = 0; i < index; ++i) new_data[i].~T();
                operator delete(new_data);
            throw;
        }
        for (size_t index = 0; index < size; ++index) 
                data[index].~T();
        operator delete(data);
        data = new_data;
        capacity = size;
    }

    void PushBack(const T& value) {
        if (size >= capacity) {
            this->Reserve(capacity * 2 + 1);
        }
        try {
            new(data + size) T(value);
            ++size;
        } catch (...) {
            data[size].~T();
            throw;
        }
    }
    void PushBack(T&& value) {
        if (size >= capacity) {
            this->Reserve(capacity * 2 + 1);
        }
        new(data + size) T(std::move(value));
        ++size;
    }
    void PopBack() {
        data[size - 1].~T();
        --size;
    }
    template<typename ...Args>
    void EmplaceBack(Args&& ...args){
        if (size >= capacity) {
            this->Reserve(capacity * 2 + 1);
        }
        new(data + size) T(std::forward<Args>(args)...);
        ++size;
    }

    ~Vector() {
        for (size_t index = 0; index < size; ++index) 
            data[index].~T();
        if (capacity != 0) operator delete(data);
    }
};