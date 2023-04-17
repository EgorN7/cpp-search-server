#include <numeric>
#include <utility>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include "test_example_functions.h"
#include "search_server.h"


template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::pair<Key, Value>& container) {
    out << container.first << std::string(": ") << container.second;
    return out;
}


template <typename Container>
void Print(std::ostream& out, const Container& container) {
    bool first_elem = true;
    for (const auto& element : container) {
        if (!first_elem) {
            out << std::string(", ");
        }
        first_elem = false;
        out << element;
    }
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}


template <typename Element>
std::ostream& operator<<(std::ostream& out, const std::set<Element>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template <typename Element>
std::ostream& operator<<(std::ostream& out, const std::vector<Element>& container) {
    out << '[';
    Print(out, container);
    out << ']';
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << '(' << line << std::string(") : ") << func << std::string(" : ");
        std::cerr << std::string("ASSERT_EQUAL(") << t_str << std::string(", ") << u_str << std::string(") failed: ");
        std::cerr << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cerr << std::string(" Hint: ") << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cerr << file << '(' << line << std::string(") : ") << func << std::string(" : ");
        std::cerr << std::string("ASSERT(") << expr_str << std::string(") failed.");
        if (!hint.empty()) {
            std::cerr << std::string(" Hint: ") << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = std::string("cat in the city");
    const std::vector<int> ratings = { 1, 2, 3 };

    // Сначала проверим, что поиск не существующего документа вернет пустоту 
    {
        SearchServer server;
        ASSERT(server.FindTopDocuments(std::string("in")).empty());
    }

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(std::string("in"));
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server(std::string("in the"));
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments(std::string("in")).empty());
    }
}

// Тест проверяет, что поисковая система исключает минус-слова из поискового запроса
void TestExcludeMinusWordsFromQuery() {
    const int doc_id = 42;
    const std::string content = std::string("cat in the city");
    const std::vector<int> ratings = { 1, 2, 3 };
    // Сначала убедимся, что слова не являющиеся минус-словами, находят нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(std::string("in"));
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // Затем убедимся, что поиск по минус-слову возвращает пустоту 
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments(std::string("-in the")).empty());
    }
}

// Тест на проверку сопоставления содержимого документа и поискового запроса

void TestMatchDocument() {

    const int doc_id = 42;
    const std::string content = std::string("cat in the city");
    const std::vector <std::string> content_match_word = { std::string("cat"), std::string("in"), std::string("the") };
    const std::vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    // убедимся,что слова из поискового запроса вернулись
    {
        const auto [matched_words, status] = server.MatchDocument(std::string("in the cat"), doc_id);
        ASSERT_EQUAL(matched_words, content_match_word);
    }

    // убедимся, что присутствие минус-слова возвращает пустой вектор
    {
        const auto [matched_words, status] = server.MatchDocument(std::string("in -the cat"), doc_id);
        ASSERT(matched_words.empty());
    }

}


// Тест на сортировку по релевантности найденых документов
void TestSortingByRelevance() {
    const int doc_id1 = 1; // relev = 0.650672
    const int doc_id2 = 2; // relev = 0.067577
    const int doc_id3 = 3; // relev = 0.135155
    const std::string content_1 = std::string("cat in the city");
    const std::vector<int> ratings_1 = { -1, 2, 2 };
    const std::string content_2 = std::string("black dog was on 3rd avenue");
    const std::vector<int> ratings_2 = {};
    const std::string content_3 = std::string("black cat was in a park");
    const std::vector<int> ratings_3 = { 2, 3, 4 };

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // убедимся,что документы найдены
    const auto found_docs = server.FindTopDocuments(std::string("black cat the city"));
    ASSERT_EQUAL(found_docs.size(), 3);

    const Document& doc1 = found_docs[0];
    const Document& doc2 = found_docs[1];
    const Document& doc3 = found_docs[2];

    // проверим сортировку по релевантности
    ASSERT_EQUAL(doc1.id, doc_id1);
    ASSERT_EQUAL(doc2.id, doc_id3);
    ASSERT_EQUAL(doc3.id, doc_id2);

}
// Тест на правильность вычесления релевантности
void TestCalculatingRelevance() {
    const double epx = 1e-6; //+-0.000001
    const int doc_id1 = 1; // relev = 0.650672
    const int doc_id2 = 2; // relev = 0.135155
    const int doc_id3 = 3; // relev = 0.067577
    const std::string content_1 = std::string("cat in the city");
    const std::vector<int> ratings_1 = { -1, 2, 2 };
    const std::string content_2 = std::string("black dog was on 3rd avenue");
    const std::vector<int> ratings_2 = {};
    const std::string content_3 = std::string("black cat was in a park");
    const std::vector<int> ratings_3 = { 2, 3, 4 };

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // убедимся,что документы найдены
    const auto found_docs = server.FindTopDocuments(std::string("black cat the city"));
    ASSERT_EQUAL(found_docs.size(), 3);

    const Document& doc1 = found_docs[0];
    const Document& doc2 = found_docs[1];
    const Document& doc3 = found_docs[2];



    // проверим расчет релевантности
    double relevance_doc1 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) +
        log(server.GetDocumentCount() * 1.0 / 1) * (1.0 / 4) +
        log(server.GetDocumentCount() * 1.0 / 1) * (1.0 / 4); // слова cat, the и city
    ASSERT(std::abs(doc1.relevance - relevance_doc1) < epx);
    double relevance_doc3 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 6) +
        log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 6); // слова black и cat
    ASSERT(std::abs(doc2.relevance - relevance_doc3) < epx);
    double relevance_doc2 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 6); // слово black
    ASSERT(std::abs(doc3.relevance - relevance_doc2) < epx);
}

/// <summary>
/// вычисление среднего ариметического оценок докумета
/// </summary>
/// <param name="rating"> вектор оценок</param>
/// <returns>среднего ариметического оценок докумета</returns>
int ArithmeticMeanOfTheRating(std::vector<int> rating) {
    if (rating.size() == 0) {
        return 0;
    }
    return std::accumulate(rating.begin(), rating.end(), 0) / static_cast<int>(rating.size());
}

// Тест на правильность вычесления рейтинга 
void TestCalculatingRating() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const std::string content_1 = std::string("cat in the city");
    const std::vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.650672
    const std::string content_2 = std::string("black dog was on 3rd avenue");
    const std::vector<int> ratings_2 = {}; // rating 2 relev = 0.135155
    const std::string content_3 = std::string("black cat was in a park");
    const std::vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.067577

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // проверим рейтинг
    const auto found_docs1 = server.FindTopDocuments(std::string("city"));
    ASSERT_EQUAL(found_docs1.size(), 1);
    const Document& doc1 = found_docs1[0];
    ASSERT_EQUAL(doc1.rating, ArithmeticMeanOfTheRating(ratings_1));

    const auto found_docs2 = server.FindTopDocuments(std::string("dog"));
    ASSERT_EQUAL(found_docs2.size(), 1);
    const Document& doc2 = found_docs2[0];
    ASSERT_EQUAL(doc2.rating, ArithmeticMeanOfTheRating(ratings_2));

    const auto found_docs3 = server.FindTopDocuments(std::string("park"));
    ASSERT_EQUAL(found_docs3.size(), 1);
    const Document& doc3 = found_docs3[0];
    ASSERT_EQUAL(doc3.rating, ArithmeticMeanOfTheRating(ratings_3));
}

// Тест на фильтрацию результата с использованием предиката
void TestFilterByPredicate() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const int doc_id4 = 5;
    const std::string content_1 = std::string("cat in the city");
    const std::vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.650672
    const std::string content_2 = std::string("black dog was on 3rd avenue");
    const std::vector<int> ratings_2 = {}; // rating 2 relev = 0.135155
    const std::string content_3 = std::string("black cat was in a park");
    const std::vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.067577
    const std::string content_4 = std::string("a white cat in a dark alley");
    const std::vector<int> ratings_4 = { 1, 2, 3 }; // rating 3 relev = 0.067577

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
    server.AddDocument(doc_id4, content_4, DocumentStatus::IRRELEVANT, ratings_4);

    // веренем документы с четным ИД
    {
        const auto found_docs = server.FindTopDocuments(std::string("black cat the city"),
            [](int document_id, DocumentStatus status, int rating)
            {
                return document_id % 2 == 0;
            });

        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id2);

    }

    // вернем документы с рейтингом и определённым статусом
    {
        const auto found_docs = server.FindTopDocuments(std::string("black cat the city"),
            [](int document_id, DocumentStatus status, int rating)
            {
                return status == DocumentStatus::ACTUAL and rating > 0;
            });

        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id1);

    }
}

// Тест на фильтрацию результата с использованием статуса
void TestFilterByStatus() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const int doc_id4 = 4;
    const std::string content_1 = std::string("cat in the city");
    const std::vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.650672
    const std::string content_2 = std::string("black dog was on 3rd avenue");
    const std::vector<int> ratings_2 = {}; // rating 2 relev = 0.135155
    const std::string content_3 = std::string("black cat was in a park");
    const std::vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.067577
    const std::string content_4 = std::string("a white cat in a dark alley");
    const std::vector<int> ratings_4 = { 1, 2, 3 }; // rating 3 relev = 0.067577

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
    server.AddDocument(doc_id4, content_4, DocumentStatus::IRRELEVANT, ratings_4);

    // вернем документы с статусом ACTUAL
    {
        const auto found_docs = server.FindTopDocuments(std::string("black cat the city"),
            DocumentStatus::ACTUAL);

        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc1 = found_docs[0];
        const Document& doc2 = found_docs[1];
        ASSERT_EQUAL(doc1.id, doc_id1);
        ASSERT_EQUAL(doc2.id, doc_id2);

    }

    // вернем документы с статусом BANNED
    {
        const auto found_docs = server.FindTopDocuments(std::string("black cat the city"),
            DocumentStatus::BANNED);

        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id3);

    }

    // вернем документы с статусом IRRELEVANT
    {
        const auto found_docs = server.FindTopDocuments(std::string("black cat the city"),
            DocumentStatus::IRRELEVANT);

        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id4);

    }

}



// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeMinusWordsFromQuery();
    TestMatchDocument();
    TestSortingByRelevance();
    TestCalculatingRelevance();
    TestCalculatingRating();
    TestFilterByPredicate();
    TestFilterByStatus();
}