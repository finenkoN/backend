
template <typename T>
class UniquePtr {
private:
    T* ptr = nullptr;
public:
    UniquePtr() { };
    UniquePtr(T* CopySource) { 
        ptr = CopySource; 
        CopySource = nullptr;
    };
    UniquePtr(const UniquePtr& CopySource) = delete;
    UniquePtr(UniquePtr&& CopySource) noexcept {
        if (ptr) delete ptr;
        ptr = CopySource.Get();
        if (CopySource.Get() != nullptr) { CopySource.ptr = nullptr; };
    };
    void operator=(const T& CopySource) = delete;
    void operator=(UniquePtr&& CopySource) noexcept {
        if (ptr) delete ptr;
        ptr = CopySource.Get();
        if (CopySource.Get() != nullptr) { CopySource.ptr = nullptr; };
    };
    template <typename V>
    UniquePtr(const UniquePtr<V> &&CopySource) {
        if (!std::is_convertible_v<V, T>) return;
        if (ptr) delete ptr;
        ptr = CopySource.Get();
        if (CopySource.Get() != nullptr) { CopySource.ptr = nullptr; };
    }
    ~UniquePtr() { if (ptr != nullptr) delete ptr; };

    T* Get() const { return ptr; };
    operator bool() const { return (ptr == nullptr ? false : true); };

    T* Release() {
        T* buffer = ptr;
        ptr = nullptr;
        return buffer;
    };

    void Reset(T* ptr_ = nullptr) {
        if (ptr != nullptr) delete ptr;
        ptr = ptr_;
        ptr_ = nullptr;
    };

    void Swap(UniquePtr& ptr_) {
        if (ptr_.Get() == ptr) return;
        T* buffer = ptr_.Get();
        ptr_.ptr = ptr;
        ptr = buffer;
    };

    T& operator*() const { return *ptr; };
    T* operator->() const { return ptr; };
};

template<typename T, typename... U> 
UniquePtr<T> MakeUnique(U... args) { return UniquePtr<T>(new T(args...)); }