#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> ids_to_remove;
    std::set<std::set<std::string>> document_words;

    for (const int id : search_server) {
        std::set<std::string> words;
        for (const auto& [word, _] : search_server.GetWordFrequencies(id)) { 
            words.insert(word);
        }
        if (document_words.count(words)) {
            ids_to_remove.insert(id);
            continue;
        }
        document_words.insert(words);
    }

    for (const int id : ids_to_remove) {
        std::cout << std::string("Found duplicate document id ") << id << std::endl;
        search_server.RemoveDocument(id);
    }
}