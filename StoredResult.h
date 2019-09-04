#ifndef CONNECT4_STOREDRESULT_H
#define CONNECT4_STOREDRESULT_H

class StoredResult {
public:
    mutable int8_t value;

    inline StoredResult(const StoredResult& other) = default;

    inline explicit StoredResult(const int8_t v) : value(v) {}

//    inline explicit StoredResult() : value(0) {}

    inline const StoredResult& operator=(const StoredResult other) noexcept {
        value = other.value;
        return *this;
    }

    inline bool is_null() const {
        return value == 100;
    }
};

static const StoredResult NULL_SEARCH_RESULT(100);

#endif //CONNECT4_STOREDRESULT_H
