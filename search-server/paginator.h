#pragma once
#include <vector>
#include <cmath>


template <typename T_iterator>
class IteratorRange
{
public:
    IteratorRange(T_iterator T_it_begin, T_iterator T_it_end, size_t size_n)
        :it_begin(T_it_begin), it_end(T_it_end), size_it(size_n) {};

    T_iterator begin() { return it_begin; };
    T_iterator end() { return it_end; };
    T_iterator size() { return size_it; };

private:
    T_iterator it_begin;
    T_iterator it_end;
    size_t     size_it;

};

template <typename T>
class Paginator
{
public:
    Paginator(T it_begin, T it_end, size_t docs_on_list)
    {
        total_docs_ = distance(it_begin, it_end);
        total_listings_ = ceil(total_docs_ / static_cast<double> (docs_on_list));
        int now_list = 0;
        while (now_list < total_listings_) {
            T now_it_begin = it_begin + now_list * docs_on_list;
            T now_it_end;
            ++now_list;
            if (now_list == total_listings_) {
                now_it_end = it_end;
            }
            else {
                now_it_end = it_begin + now_list * docs_on_list;
            }

            find_docs_.push_back(IteratorRange(now_it_begin, now_it_end, now_it_end - now_it_begin));
        }
    }

    auto   begin()const { return find_docs_.begin(); };
    auto   end()const { return find_docs_.end(); };
    size_t size()const { return find_docs_.size(); };

private:
    int total_docs_;
    int total_listings_;
    std::vector<IteratorRange<T>> find_docs_;

};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}