#pragma once
#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <execution>


#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);
    explicit SearchServer();
    void AddDocument(int document_id, std::string_view  document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
        DocumentPredicate document_predicate)const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view  raw_query,
        DocumentPredicate document_predicate)const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status)const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status)const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query)const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query)const;

    int GetDocumentCount()const;

    std::set<int>::iterator begin();
    std::set<int>::iterator end();

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
        int document_id)const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
        return MatchDocument(raw_query, document_id);
    }


    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const {
        if (document_ids_.count(document_id) == 0) {
            throw std::out_of_range("Такой id не существует");
        }
        const Query query = ParseQuery(raw_query);

        if (any_of(std::execution::par,
            query.minus_words.begin(), query.minus_words.end(),
            [&](const std::string_view& word) {
                return word_to_document_freqs_.at(std::string(word)).count(document_id);
            })) {
            std::vector<std::string_view> empty;
            return { empty, documents_.at(document_id).status };
        }

        std::vector<std::string_view> matched_words(query.plus_words.size());
        auto words_end = copy_if(std::execution::par,
            query.plus_words.begin(), query.plus_words.end(),
            matched_words.begin(),
            [&](const std::string_view& word) { return word_to_document_freqs_.at(std::string(word)).count(document_id); }
        );
        std::sort(std::execution::par, matched_words.begin(), words_end);
        words_end = std::unique(std::execution::par, matched_words.begin(), words_end);
        matched_words.erase(words_end, matched_words.end());
        return make_tuple(matched_words, documents_.at(document_id).status);
    }

    //////////

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    template<class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id) {
        if (document_ids_.find(document_id) != document_ids_.end()) {
            documents_.erase(document_id);
            document_ids_.erase(document_id);
            auto& items = document_to_word_freqs_.at(document_id);
            std::vector<std::string> words(items.size());
            std::transform(policy, items.begin(), items.end(), words.begin(), [](const auto& item) { return item.first; });
            std::for_each(policy, words.begin(), words.end(),
                [&](auto word) {
                    word_to_document_freqs_.at(word).erase(document_id);
                });
        }
    }



private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;

    bool IsStopWord(std::string_view word)const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text)const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view& text) const;

    Query ParseQuery(const std::string_view& text)const;

    double ComputeWordInverseDocumentFreq(const std::string& word)const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
        DocumentPredicate document_predicate)const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw  std::invalid_argument(std::string("Some of stop words are invalid"));
    }
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
    DocumentPredicate document_predicate)const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate)const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(12);

    std::vector<std::string_view> plus_words = query.plus_words;
    std::sort(policy, plus_words.begin(), plus_words.end());
    auto words_end = std::unique(plus_words.begin(), plus_words.end());
    plus_words.erase(words_end, plus_words.end());

    const auto plus_word_checker =
        [this, &document_predicate, &document_to_relevance](std::string_view word) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    };
    std::for_each(policy, plus_words.begin(), plus_words.end(), plus_word_checker);

    const auto minus_word_checker =
        [this, &document_predicate, &document_to_relevance](std::string_view word) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            return;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
            document_to_relevance.Erase(document_id);
        }
    };
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), minus_word_checker);

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}
std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocuments(const SearchServer& search_server, std::string_view raw_query, int document_id);
std::vector<Document> FindTopDocuments(const SearchServer& search_server, std::string_view raw_query);
void AddDocument(SearchServer& search_server, int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);