#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const pair<Key, Value>& container) {
    out << container.first << ": " << container.second;
    return out;
}


template <typename Container>
void Print(ostream& out, const Container& container) {
    bool first_elem = true;
    for (const auto& element : container) {
        if (!first_elem) {
            out << ", "s;
        }
        first_elem = false;
        out << element;
    }
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}


template <typename Element>
ostream& operator<<(ostream& out, const set<Element>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << '[';
    Print(out, container);
    out << ']';
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

//Глобальные переменные программы

/// <summary>
/// Количество выводимых докуметов в результате поиска
/// </summary>
const int MAX_RESULT_DOCUMENT_COUNT = 5;
/// <summary>
/// Структура соотвествия id документа, его реливантности 
/// и рейтингу определённому запросу 
/// </summary>
struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

/// <summary>
/// Перечесление возможных статусов документов
/// </summary>
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

//Функции и классы

/// <summary>
/// Функция чтения всей строки, введенной с консоли
/// </summary>
/// <returns> Возращает переменную типа string с полученными данными</returns>
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

/// <summary>
/// Функция чтения числа, введенного с консоли
/// </summary>
/// <returns> Возращает переменную типа int с полученными данными</returns>
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

/// <summary>
/// Функция проверки на пустую строчку элемента в массиве
/// </summary>
/// <typeparam name="StringContainer"> - шаблон массива</typeparam>
/// <param name="strings"> - массив строк</param>
/// <returns>множество не пустых строк</returns>
template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

/// <summary>
/// Класс посиковового запроса
/// </summary>
class SearchServer {

    //публичные атрибуты, структуры и методы класса
public:

    /// <summary>
    /// Конструктор класса, в качестве параметра подается массив стоп-слов
    /// </summary>
    /// <typeparam name="StringContainer"> - шаблон массива</typeparam>
    /// <param name="stop_words"> массив стоп-слов</param>
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& word : MakeUniqueNonEmptyStrings(stop_words)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Valid Word");
            }
        }
    }

    /// <summary>
    /// Конструктор класса, в качестве параметра подается строка стоп-слов
    /// </summary>
    /// <param name="stop_words_text">- строка стоп-слов</param>
    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    /// <summary>
    /// Метод заполнения множества стоп-слов класса
    /// </summary>
    /// <param name="text"> - строка стоп-слов</param>
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }


    /// <summary>
    /// Метод добаления документов, в которых будет осуществляться поиск, 
    /// подсчет документов в базе и расчет term frequency каждого слова
    /// </summary>
    /// <param name="document_id"> - номер документа</param>
    /// <param name="document"> - содержание документа</param>
    /// <param name="status"> - статус документа</param>
    /// <param name="ratings"> - рейтиги документа</param>
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id)>0) {
            throw invalid_argument("Valid id document");
        }
        if (!IsValidWord(document)) {
            throw invalid_argument("Valid content document");
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        count_documents_.push_back(document_id);
    }

    /// <summary>
    /// Шаблонный метод нахождения нужного количества документов, соответствующих строке запроса
    /// с возможностью передачи критериев для поиска документов
    /// </summary>
    /// <typeparam name="Predicate"> Шаблонный тип для критериев поиска</typeparam>
    /// <param name="raw_query"> - строка запроса</param>
    /// <param name="predicate"> - критерии</param>
    /// <returns>Нужное количество документов, соответствующих строке запроса</returns>
    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query,
        Predicate predicate) const {
        if (raw_query.empty()) throw invalid_argument("Empty query");
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                }
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    /// <summary>
    /// Метод нахождения нужного количества документов, соответствующих строке запроса 
    /// с возможностю явного указания статуса
    /// </summary>
    /// <param name="raw_query"> - строка запроса</param>
    /// <param name="need_status"> - нужный статус документов</param>
    /// <returns>Нужное количество документов, соответствующих строке запроса</returns>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus need_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query,
            [need_status](int document_id, DocumentStatus status, int rating) {
                return status == need_status;
            }
        );
    }

    /// <summary>
    /// Метод получения значения количества докуметов в объекте класса
    /// </summary>
    /// <returns>Количество документов</returns>
    int GetDocumentCount() const {
        return documents_.size();
    }

    /// <summary>
    /// Метод получения слов определённого документа, которые подходят под запрос
    /// </summary>
    /// <param name="raw_query">- строка запроса</param>
    /// <param name="document_id">- нужный документ для проверки</param>
    /// <returns>Вектор слов, соответствущих запросу
    /// и статус документа</returns>
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        if (raw_query.empty()) throw invalid_argument("Empty query");
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

    /// <summary>
    /// Метод получения id документа номеру его порядка, в соотвествии с очередью внесения документов в базу данных
    /// </summary>
    /// <param name="index"> - номер документа по порядку</param>
    /// <returns>Id документа, сооответствущий заданному номеру</returns>
    int GetDocumentId(int index) const {
        if (index < 0 || index >= static_cast<int>(count_documents_.size())) {
            throw out_of_range("Index out_of_range");
        }
        return count_documents_.at(index);
    }

    //приватные атрибуты, структуры и методы класса
private:

    /// <summary>
    /// структра, показывающая принадлежность слова к группам минус-слов и стоп-слов
    /// </summary>
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    /// <summary>
    /// структра слов поиска и слов-фильтров
    /// </summary>
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    /// <summary>
    /// Срутктура данных о документе(рейтинг и статус) 
    /// </summary>
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_; // набор стоп-слов
    map<string, map<int, double>> word_to_document_freqs_; // словарь инвентированых индексов документов в классе 
    map<int, DocumentData> documents_; //словарь добавленных документов, где ключ это id документа, а значение это содержимое документа
    vector<int> count_documents_; //список id, в соотвествии с очередью внесения документов в базу данных

    /// <summary>
    /// Проверка на стоп-слово
    /// </summary>
    /// <param name="word"> - проверяемое слово</param>
    /// <returns> True если слово является стоп-словом </returns>
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    /// <summary>
    /// Проверка строки на наличие спецсимволов
    /// </summary>
    /// <param name="word">- строка для проверки</param>
    /// <returns>Результат проверки в виде bool значения</returns>
    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    /// <summary>
     /// Метод получения вектора слов из строки
     /// </summary>
     /// <param name="text"> - входная строка</param>
     /// <returns>Вектор слов</returns>
    vector<string> SplitIntoWords(const string& text) const {
        vector<string> words;
        string word;
        for (const char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            }
            else {
                word += c;
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }

        return words;
    }

    /// <summary>
    /// Метод разбывки строки на вектор, где отсутствуют стоп-слова
    /// </summary>
    /// <param name="text"> - входная строка</param>
    /// <returns>Вектор слов</returns>
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    /// <summary>
    /// Метод подсчета среднего рейтига документа
    /// </summary>
    /// <param name="ratings"> - вектор оценок</param>
    /// <returns>Средняя оценка</returns>
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }


    /// <summary>
    /// метод проверки слова на его пренадлежность 
    /// к стоп-словам и минус-словам
    /// </summary>
    /// <param name="text">- слово</param>
    /// <returns>Структуру с флагами принадлежност</returns>
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }


    /// <summary>
    /// Метод получения множеств слов поиска и слов-фильтров из запроса
    /// </summary>
    /// <param name="text"> - строка запроса</param>
    /// <returns></returns>
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!IsValidWord(query_word.data)) { throw invalid_argument("Valid query"); }
            if (query_word.data[0] == '-' || query_word.data.empty() 
                || query_word.data[query_word.data.size() - 1] == '-') {
                throw invalid_argument("Valid query"); 
            }
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    /// <summary>
    /// Метод расчета IDF слова
    /// </summary>
    /// <param name="word"> - слово, 
    /// для которого нужно найти IDF</param>
    /// <returns> Значение IDF</returns>
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    /// <summary>
    /// Шаблонный метод поиска документов, соответствующих запаросу
    /// </summary>
    /// <typeparam name="FuncPredict"></typeparam>
    /// <param name="query"> - запрос</param>
    /// <param name="predict"> - функция проверяющая комунеты на необходимые критерии</param>
    /// <returns>Документы соответсвующие запросу</returns>
    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                DocumentData current_document = documents_.at(document_id);
                if (predicate(document_id, current_document.status, current_document.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала проверим, что поиск не существующего документа вернет пустоту 
    {
        SearchServer server;
        ASSERT(server.FindTopDocuments("in"s).empty());
    }

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

// Тест проверяет, что поисковая система исключает минус-слова из поискового запроса
void TestExcludeMinusWordsFromQuery() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убедимся, что слова не являющиеся минус-словами, находят нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // Затем убедимся, что поиск по минус-слову возвращает пустоту 
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("-in the"s).empty());
    }
}

// Тест на проверку сопоставления содержимого документа и поискового запроса

void TestMatchDocument() {

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector <string> content_match_word = { "cat"s, "in"s, "the"s };
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    // убедимся,что слова из поискового запроса вернулись
    {
        const auto [matched_words, status] = server.MatchDocument("in the cat"s, doc_id);
        ASSERT_EQUAL(matched_words, content_match_word);
    }

    // убедимся, что присутствие минус-слова возвращает пустой вектор
    {
        const auto [matched_words, status] = server.MatchDocument("in -the cat"s, doc_id);
        ASSERT(matched_words.empty());
    }

}


// Тест на сортировку по релевантности найденых документов
void TestSortingByRelevance() {
    const int doc_id1 = 1; // relev = 0.650672
    const int doc_id2 = 2; // relev = 0.067577
    const int doc_id3 = 3; // relev = 0.135155
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 };
    const string content_2 = "black dog was on 3rd avenue"s;
    const vector<int> ratings_2 = {};
    const string content_3 = "black cat was in a park"s;
    const vector<int> ratings_3 = { 2, 3, 4 };

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // убедимся,что документы найдены
    const auto found_docs = server.FindTopDocuments("black cat the city"s);
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
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 };
    const string content_2 = "black dog was on 3rd avenue"s;
    const vector<int> ratings_2 = {};
    const string content_3 = "black cat was in a park"s;
    const vector<int> ratings_3 = { 2, 3, 4 };

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // убедимся,что документы найдены
    const auto found_docs = server.FindTopDocuments("black cat the city"s);
    ASSERT_EQUAL(found_docs.size(), 3);

    const Document& doc1 = found_docs[0];
    const Document& doc2 = found_docs[1];
    const Document& doc3 = found_docs[2];



    // проверим расчет релевантности
    double relevance_doc1 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4) +
        log(server.GetDocumentCount() * 1.0 / 1) * (1.0 / 4) +
        log(server.GetDocumentCount() * 1.0 / 1) * (1.0 / 4); // слова cat, the и city
    ASSERT(abs(doc1.relevance - relevance_doc1) < epx);
    double relevance_doc3 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 6) +
        log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 6); // слова black и cat
    ASSERT(abs(doc2.relevance - relevance_doc3) < epx);
    double relevance_doc2 = log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 6); // слово black
    ASSERT(abs(doc3.relevance - relevance_doc2) < epx);
}

/// <summary>
/// вычисление среднего ариметического оценок докумета
/// </summary>
/// <param name="rating"> вектор оценок</param>
/// <returns>среднего ариметического оценок докумета</returns>
int ArithmeticMeanOfTheRating(vector<int> rating) {
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
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.650672
    const string content_2 = "black dog was on 3rd avenue"s;
    const vector<int> ratings_2 = {}; // rating 2 relev = 0.135155
    const string content_3 = "black cat was in a park"s;
    const vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.067577

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::ACTUAL, ratings_3);

    // проверим рейтинг
    const auto found_docs1 = server.FindTopDocuments("city"s);
    ASSERT_EQUAL(found_docs1.size(), 1);
    const Document& doc1 = found_docs1[0];
    ASSERT_EQUAL(doc1.rating, arithmetic_mean_of_the_rating(ratings_1));

    const auto found_docs2 = server.FindTopDocuments("dog"s);
    ASSERT_EQUAL(found_docs2.size(), 1);
    const Document& doc2 = found_docs2[0];
    ASSERT_EQUAL(doc2.rating, arithmetic_mean_of_the_rating(ratings_2));

    const auto found_docs3 = server.FindTopDocuments("park"s);
    ASSERT_EQUAL(found_docs3.size(), 1);
    const Document& doc3 = found_docs3[0];
    ASSERT_EQUAL(doc3.rating, arithmetic_mean_of_the_rating(ratings_3));
}

// Тест на фильтрацию результата с использованием предиката
void TestFilterByPredicate() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const int doc_id3 = 3;
    const int doc_id4 = 5;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.650672
    const string content_2 = "black dog was on 3rd avenue"s;
    const vector<int> ratings_2 = {}; // rating 2 relev = 0.135155
    const string content_3 = "black cat was in a park"s;
    const vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.067577
    const string content_4 = "a white cat in a dark alley"s;
    const vector<int> ratings_4 = { 1, 2, 3 }; // rating 3 relev = 0.067577

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
    server.AddDocument(doc_id4, content_4, DocumentStatus::IRRELEVANT, ratings_4);

    // веренем документы с четным ИД
    {
        const auto found_docs = server.FindTopDocuments("black cat the city"s,
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
        const auto found_docs = server.FindTopDocuments("black cat the city"s,
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
    const string content_1 = "cat in the city"s;
    const vector<int> ratings_1 = { -1, 2, 2 }; // rating 1 relev = 0.650672
    const string content_2 = "black dog was on 3rd avenue"s;
    const vector<int> ratings_2 = {}; // rating 2 relev = 0.135155
    const string content_3 = "black cat was in a park"s;
    const vector<int> ratings_3 = { 2, 3, 4 }; // rating 3 relev = 0.067577
    const string content_4 = "a white cat in a dark alley"s;
    const vector<int> ratings_4 = { 1, 2, 3 }; // rating 3 relev = 0.067577

    SearchServer server;
    server.AddDocument(doc_id1, content_1, DocumentStatus::ACTUAL, ratings_1);
    server.AddDocument(doc_id2, content_2, DocumentStatus::ACTUAL, ratings_2);
    server.AddDocument(doc_id3, content_3, DocumentStatus::BANNED, ratings_3);
    server.AddDocument(doc_id4, content_4, DocumentStatus::IRRELEVANT, ratings_4);

    // вернем документы с статусом ACTUAL
    {
        const auto found_docs = server.FindTopDocuments("black cat the city"s,
            DocumentStatus::ACTUAL);

        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc1 = found_docs[0];
        const Document& doc2 = found_docs[1];
        ASSERT_EQUAL(doc1.id, doc_id1);
        ASSERT_EQUAL(doc2.id, doc_id2);

    }

    // вернем документы с статусом BANNED
    {
        const auto found_docs = server.FindTopDocuments("black cat the city"s,
            DocumentStatus::BANNED);

        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc1 = found_docs[0];
        ASSERT_EQUAL(doc1.id, doc_id3);

    }

    // вернем документы с статусом IRRELEVANT
    {
        const auto found_docs = server.FindTopDocuments("black cat the city"s,
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

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}