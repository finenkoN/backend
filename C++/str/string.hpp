#pragma once
#include <iostream>
#include <cstring>
#include <cstdbool>
#include <string_view>
#include <type_traits>

class String {
private:
		size_t Size_ = 0;
		size_t Capacity_ = 1;
		char * Data_ = nullptr;

public:
	String() {};

	String (char symbol) {
		Data_ = new char[2]();
		Data_[0] = symbol;
		Data_[1] = '\0';

		Size_ = 1;
		Capacity_ = 2;
	};
	
	String (size_t N, char symbol) {
		Data_ = new char[N + 1]();
		for (size_t i = 0; i < N; i++) Data_[i] = symbol;
		Data_[N] = '\0';
		Size_ = N;
		Capacity_ = N + 1;
	};

	String (const char *Copy_Source, size_t N) {
		Data_ = new char[N + 1]();
		memcpy(Data_, Copy_Source, N);
		Data_[N] = '\0';
		Size_ = N;
		Capacity_ = N + 1;
	};

	String (const char *Copy_Source) {
		if (Copy_Source == nullptr) {
			Size_ = 0;
			Capacity_ = 1;
			Data_ = nullptr;
			return;
		}

		int length = strlen(Copy_Source);
		Data_ = new char[length + 1]();
		memcpy(Data_, Copy_Source, length);
		Data_[length] = '\0';
		Size_ = length;
		Capacity_ = length + 1;
	};

	String (const String& CopySource) {	 
		if (CopySource.Data() == nullptr) {
			Size_ = 0;
			Capacity_ = 1;
			Data_ = nullptr;
			return;
		}

		Data_ = new char [CopySource.Size() + 1]();
		memcpy(Data_, CopySource.Data(), CopySource.Size());
		Data_[CopySource.Size()] = '\0';
		Size_ = CopySource.Size();
		Capacity_ = CopySource.Size() + 1;
	};
	
	String& operator= (const String& CopySource) {
		if (CopySource.Data() == Data_)
			return *this;

		if (CopySource.Size_ == 0) {
			Size_ = 0;
			Capacity_ = 1;
			delete[] Data_;
			Data_ = nullptr;
			return *this;
		}
		
		String Copy = *this;
		if (Data_ != nullptr) delete[] Data_;
		Data_ = new char[CopySource.Size_ + 1];
		Capacity_ = CopySource.Size_ + 1;
		Size_ = CopySource.Size_;
		for (size_t i = 0; i < Size_; ++i) 
			Data_[i] = CopySource.Data_[i];	
		Data_[Size_] = '\0';	
		return *this;
	};


	char& operator[](size_t i) { return Data_[i]; };
	const char& operator[] (size_t i) const { return Data_[i]; };

	char& Front() { return Data_[0]; }; 
	const char& Front() const { return Data_[0]; };

	char& Back() { return Data_[Size_ - 1]; };
	const char& Back() const {return Data_[Size_ - 1]; };

	const char *Data() const { return Data_; };
	// char *Data() { return Data_; };
	const char *CStr() const { return Data_; };

	bool Empty() { return Size_ == 0; };

	size_t Size() const { return Size_; };
	size_t Length() const { return Size_; };
	size_t Capacity() const { return Capacity_ - 1; };

	void Clear() { Size_ = 0; };
	
	void Swap(String &other) {
		size_t buffer = Size_;
		Size_ = other.Size_;
		other.Size_ = buffer;

		buffer = Capacity_;
		Capacity_ = other.Capacity_;
		other.Capacity_ = buffer;

		char *st = Data_;
		Data_ = other.Data_;
		other.Data_ = st;
	};

	char PopBack() {
		if (Size_ == 0) return '\0';
		int symbol = Data_[Size_ - 1];
		Data_[Size_ - 1] = '\0';
		Size_--;
		return symbol;
	};

	void PushBack(const char symbol) {
		if (Size_ < Capacity_ - 1) {
			Data_[Size_] = symbol;
			Data_[Size_ + 1] = '\0';
			Size_++;
			return;
		}
	
		if (Data_ == nullptr) {
			Data_ = new char[2]();
			Data_[0] = symbol;
			Data_[1] = '\0';
			Capacity_ = 2;
			Size_ = 1;
			return;
		}

		char *buffer = new char [Capacity_ * 2 - 1]();
		memcpy(buffer, Data_, Size_);
		delete [] Data_;
		Data_ = buffer;
		Data_[Size_] = symbol;
		Data_[Size_ + 1] = '\0';
		Size_++;
		Capacity_ = 2 * Capacity_ - 1;
	};

	String& operator+= (const String& other) {
		if (Capacity_ - 1 < other.Size() + Size_) {
			Capacity_ = Size_ + other.Size() + 1;
			char *buffer = new char [Capacity_]();
	
			if (Data_ != nullptr) {
				memcpy(buffer, Data_, Size_);
				delete[] Data_;
			}
			memcpy(buffer + Size_, other.Data(), other.Size());
			buffer[Capacity_ - 1] = '\0'; 
			Size_ += other.Size();
			Data_ = buffer;
			return *this;
		}

		if (other.Data_ != nullptr)
			memcpy(Data_ + Size_, other.Data_, other.Size());
		Data_[Size_ + other.Size()] = '\0';
		Size_ += other.Size();
		return *this;
	};
	
	void Resize(size_t new_size, char symbol) {
		if (new_size <= Size_) {
			Size_ = new_size;
			Data_[new_size] = '\0';
			return;
		}
		if (new_size <= Capacity_ - 1) {
			for (size_t index = Size_; index < new_size; index++)
			Data_[index] = symbol;
			Size_ = new_size;
			return;
		}
		char *save_buffer = new char[new_size + 1]();
		if (Data_ != nullptr)
		std::memcpy(save_buffer, Data_, Size_);

		delete[] Data_;

		for (size_t index = Size_; index < new_size; index++)
		save_buffer[index] = symbol;

		Data_ = save_buffer;
		Size_ = new_size;
		Capacity_ = new_size + 1;

	};

	void Reserve(size_t new_capacity) {
		if (Capacity_ - 1 < new_capacity) {
			char *buffer = new char [new_capacity + 1];
			if (Data_ != nullptr) { 
				memcpy(buffer, Data_, Size_ + 1);
				delete [] Data_;
			}
			Data_ = buffer;
			Capacity_ = new_capacity + 1;
		}
	};

	void ShrinkToFit() {
		if (Size_ == Capacity_ - 1) return;
		String copy = *this;
		if (Data_ != nullptr) delete[] Data_;
		Data_ = new char[Size_ + 1];
		for (size_t i = 0; i < copy.Size_; ++i) {
			Data_[i] = copy.Data_[i];
		}
		Data_[Size_] = '\0';
		Capacity_ = Size_ + 1;
	};

	friend String operator+ (const String& lhs, const String& rhs);
	friend bool operator== (const String &lhs, const String& rhs);
	friend int operator <=>(const String& lhs, const String& rhs);
	friend std::ostream& operator<< (std::ostream& stream, const String& first);
	friend std::istream& operator>> (std::istream& stream, String& first);
	~String() {
		delete [] Data_;
		Data_ = nullptr;
		Size_ = 0;
		Capacity_ = 1;
	};
};
