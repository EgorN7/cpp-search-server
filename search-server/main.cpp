#include <iostream>
#include <string>

#include "document.h"
#include "read_input_functions.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "test_example_functions.h"


std::ostream& operator << (std::ostream& ost, const Document& doc)
{
    return ost << std::string("{ document_id = ") << doc.id
        << std::string(", relevance = ") << doc.relevance
        << std::string(", rating = ") << doc.rating << std::string(" }");
}

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
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cout << "Search server testing finished" << std::endl;

    SearchServer search_server(std::string("and in at"));
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, std::string("curly cat curly tail"), DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, std::string("curly dog and fancy collar"), DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, std::string("big cat fancy collar "), DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, std::string("big dog sparrow Eugene"), DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, std::string("big dog sparrow Vasiliy"), DocumentStatus::ACTUAL, { 1, 1, 1 });
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest(std::string("empty request"));
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest(std::string("curly dog"));
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest(std::string("big collar"));
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest(std::string("sparrow"));
    std::cout << std::string("Total empty requests: ") << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}