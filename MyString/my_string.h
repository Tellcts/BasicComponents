#pragma once
#ifndef _MY_STRING_H_
  #define _MY_STRING_H_

  #include <cassert>
  #include <cstddef>
  #include <cstring>
  #include <iostream>

class MyString {
public:
    MyString();

    MyString(const char* __str);

    ~MyString();

    MyString(const MyString& __other);

    MyString(MyString&& __other) noexcept;

    MyString& operator=(const MyString& __other);

    MyString& operator=(MyString&& __other) noexcept;

    const char* c_str() const;

    size_t size() const;

    const char& operator[](int __index) const;

    char& operator[](int __index);

private:
    void __size_handler_for_copy(const char* __str);
    void __size_handler_for_move(char*& __str);

    static constexpr size_t BUF_SIZE = 20;
    char buffer_[BUF_SIZE];
    size_t size_;
    char* data_;
};

inline void MyString::__size_handler_for_copy(const char* __str) {
    std::cout << "=========拷贝语义被触发=========\n";
    size_t size = strlen(__str);
    size_ = size;
    if (size < BUF_SIZE) {
        data_ = buffer_;
        strcpy(data_, __str);
    } else {
        data_ = new char[size + 1];
        strcpy(data_, __str);
    }
}

inline void MyString::__size_handler_for_move(char*& __str) {
    std::cout << "=========移动语义被触发=========\n";
    size_t size = strlen(__str);
    size_ = size;
    if (size < BUF_SIZE) {
        data_ = buffer_;
        strcpy(data_, __str);
    } else {
        data_ = __str;
        __str = nullptr;
    }
}

inline MyString::MyString() : data_ {buffer_} {
    buffer_[0] = '\0';
    size_ = 0;
}

inline MyString::MyString(const char* __str) {
    if (__str != nullptr) {
        // size_t size = strlen(__str);
        // if (size < BUF_SIZE) {
        //   data_ = buffer_;
        //   strcpy(data_, __str);
        // } else {
        //   data_ = new char[size + 1];
        //   strcpy(data_, __str);
        // }
        __size_handler_for_copy(__str);
    } else {
        data_ = buffer_;
        buffer_[0] = '\0';
        size_ = 0;
    }
}

inline MyString::~MyString() {
    if (data_ && data_ != buffer_) {
        delete[] data_;
        size_ = 0;
    }
}

inline MyString::MyString(const MyString& __other) {
    //   size_t size = strlen(__other.data_);
    //   if (size < BUF_SIZE) {
    //     data_ = buffer_;
    //     strcpy(data_, __other.data_);
    //   } else {
    //     data_ = new char[size + 1];
    //     strcpy(data_, __other.data_);
    //   }
    __size_handler_for_copy(__other.data_);
}

inline MyString::MyString(MyString&& __other) noexcept {
    //   size_t size = strlen(__other.data_);
    //   if (size < BUF_SIZE) {
    //     data_ = buffer_;
    //     strcpy(data_, __other.data_);
    //   } else {
    //     data_ = __other.data_;
    //     __other.data_ = nullptr;
    //   }
    __size_handler_for_move(__other.data_);
    __other.size_ = 0;
}

inline MyString& MyString::operator=(const MyString& __other) {
    if (this == &__other) return *this;
    if (data_ != buffer_) delete[] data_;
    //   size_t size = strlen(__other.data_);
    //   if (size < BUF_SIZE) {
    //     data_ = buffer_;
    //     strcpy(data_, __other.data_);
    //   } else {
    //     data_ = new char[size + 1];
    //     strcpy(data_, __other.data_);
    //   }
    __size_handler_for_copy(__other.data_);
    return *this;
}

inline MyString& MyString::operator=(MyString&& __other) noexcept {
    if (this == &__other) return *this;
    if (data_ != buffer_) delete[] data_;
    //   size_t size = strlen(__other.data_);
    //   if (size < BUF_SIZE) {
    //     data_ = buffer_;
    //     strcpy(data_, __other.data_);
    //   } else {
    //     data_ = __other.data_;
    //     __other.data_ = nullptr;
    //   }
    __size_handler_for_move(__other.data_);
    __other.size_ = 0;
    return *this;
}

inline const char* MyString::c_str() const {
    return data_;
}

inline size_t MyString::size() const {
    return size_;
}

inline const char& MyString::operator[](int __index) const {
    assert(__index >= 0 && static_cast<size_t>(__index) < size_ &&
           "index out of range");
    return data_[__index];
}

inline char& MyString::operator[](int __index) {
    // 复用const版本索引重载
    return const_cast<char&>(static_cast<const MyString&>(*this)[__index]);
    // Or simply return data_[__index];
}

inline std::ostream& operator<<(std::ostream& __os, const MyString& __str) {
    __os << __str.c_str();
    return __os;
}

#endif
