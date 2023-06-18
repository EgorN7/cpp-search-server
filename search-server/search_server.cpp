#include <cmath>
#include <execution>

#include "search_server.h"
#include "log_duration.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text)) 
{
}

SearchServer::SearchServer(std::string_view stop_words_text) : 
    SearchServer(
    SplitIntoWords(stop_words_text)) 
{
 }
SearchServer::SearchServer() = default;

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument(std::string("Invalid document_id"));
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {
        std::string strword(word);
        word_to_document_freqs_[strword][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][strword] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status)const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query)const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount()const {
    return documents_.size();
}

std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}
std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;
    std::vector<std::string_view> plus_words = query.plus_words;
    std::sort(plus_words.begin(), plus_words.end());
    auto words_end = std::unique(plus_words.begin(), plus_words.end());
    plus_words.erase(words_end, plus_words.end());
    
    
    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }
    
    for (std::string_view word : plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(std::string_view word)const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text)const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument(std::string("Word ") + std::string(word) + std::string(" is invalid"));
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int> (ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view& text) const {
    if (text.empty()) {
        throw std::invalid_argument(std::string("Query word is empty"));
    }
    std::string word = std::string(text);
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
        text.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument(std::string("Query word ") + word + std::string(" is invalid"));
    }

    return { text, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text)const {
    Query result;
    for (std::string_view& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    
    return result;
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string, double> empty_map;
    if (document_ids_.find(document_id) == document_ids_.end()) {
        return empty_map;
    } else {
        return document_to_word_freqs_.at(document_id);
    }
   
}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word)const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_ids_.find(document_id) != document_ids_.end()) {
        documents_.erase(document_id);
        document_ids_.erase(document_id);
        for (const auto& [word, _] : document_to_word_freqs_.at(document_id)) {
            word_to_document_freqs_.at(word).erase(document_id);
        }
        document_to_word_freqs_.erase(document_id);
    }

}

std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocuments(const SearchServer& search_server, std::string_view raw_query,int document_id) {
    LOG_DURATION_STREAM("Operation time", std::cout);
    return search_server.MatchDocument(raw_query, document_id);
}

std::vector<Document> FindTopDocuments(const SearchServer& search_server, std::string_view raw_query) {

    LOG_DURATION_STREAM("Operation time", std::cout);
    return search_server.FindTopDocuments(raw_query);
}
void AddDocument(SearchServer& search_server, int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}