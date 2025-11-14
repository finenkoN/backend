#include <type_traits>

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
    struct ControlBlock {
        size_t shared_count;
        size_t weak_count;
        T* object = nullptr;
    };

    ControlBlock* cptr = nullptr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    friend class WeakPtr<T>;

public:
    SharedPtr() = default;
    SharedPtr(T* ptr_) {
        cptr = new ControlBlock;
        cptr->object = ptr_;
        cptr->shared_count = ptr_? 1 : 0;
        cptr->weak_count = 0;
    }
    SharedPtr(const SharedPtr<T>& other) {
        cptr = other.cptr;
        ++other.cptr->shared_count;
    } 

    SharedPtr(const WeakPtr<T>& other) {
        cptr = other.cptr;
        if (other.Expired()) throw std::bad_weak_ptr();
        if (cptr != nullptr) 
            ++cptr->shared_count;
    }   

    SharedPtr& operator=(const SharedPtr<T>& other) noexcept {
        if (other.cptr == cptr) return *this;
        this->~SharedPtr();
        if (other.cptr == nullptr) { 
            cptr = nullptr;
            return *this;
        }
        ++other.cptr->shared_count;
        cptr = other.cptr;
        return *this;
    }

    SharedPtr(SharedPtr<T>&& other) noexcept {
        cptr = other.cptr;
        other.cptr = nullptr;
    } 

    SharedPtr& operator=(SharedPtr<T>&& other) noexcept {
        if (other.cptr == cptr) return *this;
        this->~SharedPtr();
        cptr = std::move(other.cptr);
        other.cptr = nullptr;
        return *this;
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y> &&other) : cptr(other.cptr) {
        if (!std::is_convertible_v<Y, T>) throw std::bad_cast();
    }

    void Swap(SharedPtr<T>& other) { std::swap(other.cptr, cptr); }

    size_t UseCount() const noexcept { return cptr == nullptr ? 0 : cptr->shared_count; }

    T* Get() const noexcept { return cptr == nullptr ? nullptr : cptr->object; }
    
    T& operator*() const { return *cptr->object; }
    T* operator->() const  { return cptr->object; }
    // operator bool() { return (cptr == nullptr ? false : cptr->object != nullptr); }
    operator bool() const { return (cptr == nullptr ? false : cptr->object != nullptr); }

    void Reset(T* ptr_ = nullptr) {
        this->~SharedPtr();
        cptr = new ControlBlock;
        cptr->object = ptr_;
        cptr->shared_count = ptr_ == nullptr ? 0 : 1;
        cptr->weak_count = 0;
    }

    ~SharedPtr() {
        if (cptr == nullptr) return;
        if (cptr->shared_count > 0) {
            --cptr->shared_count;
        }
        if (cptr->shared_count == 0) {
            if (cptr->object != nullptr) {
                delete cptr->object;
                cptr->object = nullptr;
            }
        }

        if (cptr->shared_count == 0 && cptr->weak_count == 0) {
            delete cptr;
            cptr = nullptr;
        }
    }
};

template <typename T, typename...Args>
SharedPtr<T> MakeShared(Args&&... args) {
    T *object = new T(args...);
    return SharedPtr<T>(object);
}

template <typename T>
class WeakPtr {
    friend class SharedPtr<T>;
    struct SharedPtr<T>::ControlBlock *cptr = nullptr;
public:
    WeakPtr() = default;
    
    WeakPtr(const SharedPtr<T>& other) {
        if (other.cptr == nullptr) {
            cptr = nullptr;
            return;
        }
        cptr = other.cptr;
        ++cptr->weak_count;
    }

    WeakPtr(const WeakPtr<T>& other) { 
        if (other.cptr == nullptr) {
            cptr = nullptr;
            return;
        }
        cptr = other.cptr;
        ++cptr->weak_count;
    }
    WeakPtr& operator=(const WeakPtr<T>& other) {
        if (other.cptr == nullptr) {
            cptr = nullptr;
            return *this;
        }
        cptr = other.cptr;
        ++cptr->weak_count;
        return *this;
    }

    WeakPtr(WeakPtr<T>&& other) noexcept { 
        cptr = other.cptr;
        other.cptr = nullptr;
    }
    WeakPtr& operator=(WeakPtr<T>&& other) noexcept {
        this->Reset();
        cptr = other.cptr;
        other.cptr = nullptr;
        return *this;
    }

    bool Expired() const { return cptr == nullptr ? true : cptr->shared_count == 0; }

    SharedPtr<T> Lock() const {
        if (this->Expired()) return nullptr;
        SharedPtr<T> buffer;
        buffer.cptr = cptr;
        ++buffer.cptr->shared_count;
        return buffer;
    }

    void Swap(WeakPtr<T>& other) {
        std::swap(other.cptr, cptr);
    }

    size_t UseCount() const { return cptr == nullptr ? 0 : cptr->shared_count; }

    template <typename Y>
    WeakPtr(const WeakPtr<Y> &&other) {
        if (std::is_convertible_v<Y, T>) *this = (WeakPtr &&) other;
    }

    void Reset() { 
        if (cptr == nullptr) return;
        this->~WeakPtr();
        cptr = nullptr;
    }

    ~WeakPtr() {
        if (cptr == nullptr) return;
        if (cptr->weak_count > 0) {
            --cptr->weak_count;
        }
        // if (cptr->shared_count == 0) {
        //     if (cptr->object != nullptr) {
        //         delete cptr->object;
        //         cptr->object = nullptr;
        //     }

        // }

        if (cptr->shared_count == 0 && cptr->weak_count == 0) {
            delete cptr;
            cptr = nullptr;
        }
    }
};