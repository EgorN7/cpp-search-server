#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

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
    int id;
    double relevance;
    int rating;
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
    int result;
    cin >> result;
    ReadLine();
    return result;
}

/// <summary>
/// Класс посиковового запроса
/// </summary>
class SearchServer {

    //публичные атрибуты, структуры и методы класса
public:

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
    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    /// <summary>
    /// Шаблонный метод нахождения нужного количества документов, соответствующих строке запроса
    /// с возможностью передачи критериев для поиска документов
    /// </summary>
    /// <typeparam name="FuncPredict"> Шаблонный тип для критериев поиска</typeparam>
    /// <param name="raw_query"> - строка запроса</param>
    /// <param name="predict"> - критерии</param>
    /// <returns>Нужное количество документов, соответствующих строке запроса</returns>
    template<typename FuncPredict>
    vector<Document> FindTopDocuments(const string& raw_query,
        FuncPredict predict) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predict);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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

    /// <summary>
    /// Проверка на стоп-слово
    /// </summary>
    /// <param name="word"> - проверяемое слово</param>
    /// <returns> True если слово является стоп-словом </returns>
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
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
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
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
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
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
    template<typename FuncPredict>
    vector<Document> FindAllDocuments(const Query& query, FuncPredict predict) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (predict(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
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

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}
int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}