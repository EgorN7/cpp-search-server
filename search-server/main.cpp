#include <iostream>
#include <string>

#include "document.h"
#include "read_input_functions.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"



template <typename T_iterator>
std::ostream& operator << (std::ostream& ost, IteratorRange <T_iterator> it_range)
{
    for (auto doc = it_range.begin(); doc != it_range.end(); doc++)
    {
        std::cout << *doc;
    }
    return ost;
}


int main() {
    SearchServer search_server(std::string("and with"));

    AddDocument(search_server, 1, std::string("funny pet and nasty rat"), DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 2, std::string("funny pet with curly hair"), DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, std::string("funny pet with curly hair"), DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, std::string("funny pet and curly hair"), DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, std::string("funny funny pet and nasty nasty rat"), DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, std::string("funny pet and not very nasty rat"), DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, std::string("very nasty rat and not very funny pet"), DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, std::string("pet with rat and rat and rat"), DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, std::string("nasty rat with curly hair"), DocumentStatus::ACTUAL, { 1, 2 });

    std::cout << std::string("Before duplicates removed: ") << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << std::string("After duplicates removed: ") << search_server.GetDocumentCount() << std::endl;
}