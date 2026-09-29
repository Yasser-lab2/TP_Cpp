#ifndef STACK_VECTOR_HPP
#define STACK_VECTOR_HPP

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace piles {

template <typename T>
class PileVector {
private:
    std::vector<T> elements;

public:
    
    void push(const T& valeur) {
        elements.push_back(valeur);
    }

    
    void pop() {
        elements.pop_back();
    }

    T& top() {
        return elements.back();
    }

    const T& top() const {
        return elements.back();
    }

    bool empty() const {
        return elements.empty();
    }

    std::size_t size() const {
        return elements.size();
    }
};

} 

#endif 
