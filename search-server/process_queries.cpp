#include <execution>
#include "process_queries.h"


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    
    std::vector<std::vector<Document>> rezult(queries.size());
        transform(std::execution::par, queries.begin(), queries.end(), rezult.begin(),
              [&search_server](std::string str) { return search_server.FindTopDocuments(str); });
    return rezult;    
} 

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<Document> result;

    for (auto& documents : ProcessQueries(search_server, queries)) {
        for (auto& document : documents) {
            result.push_back(std::move(document));
        }
    }
    return result;
}