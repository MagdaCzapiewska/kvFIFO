#ifndef __KVFIFO_H__
#define __KVFIFO_H__

#include <utility>
#include <cstdint>

/*

 Wskazówki dla mnie:
 Generalnie chodzi o rzucanie wyjątków i odpowiednie ich przewidywanie,
 o alokowanie pamięci (nie malloc, tylko new), o używanie wskaźników (nie new, tylko make_shared),
 o zwalnianie pamięci i brak wycieków,
 o odpowiednie używanie noexcept i const,
 o zamykanie w namespace'ach (niezaśmiecanie globalnej przestrzeni nazw),
 o kopiowanie przy modyfikowaniu (efektywnie, a nie zbyt często lub niepotrzebnie!).

*/

template <typename K, typename V> class kvfifo {

public:
    kvfifo();
    kvfifo(kvfifo const &);
    kvfifo(kvfifo &&);

    kvfifo& operator=(kvfifo other);

    void push(K const &k, V const &v);

    void pop();

    void pop(K const &);

    void move_to_back(K const &k);

    std::pair<K const &, V &> front();
    std::pair<K const &, V const &> front() const;
    std::pair<K const &, V &> back();
    std::pair<K const &, V const &> back() const;

    std::pair<K const &, V &> first(K const &key);
    std::pair<K const &, V const &> first(K const &key) const;
    std::pair<K const &, V &> last(K const &key);
    std::pair<K const &, V const &> last(K const &key) const;

    bool empty() const;

    std::size_t count(K const &) const;

    void clear();

    // Iterator k_iterator oraz metody k_begin i k_end
    // pozwalające przeglądać zbiór kluczy w rosnącej kolejności wartości
};

#endif // __KVFIFO_H__

