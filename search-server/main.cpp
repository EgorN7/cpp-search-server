#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

//Глобальные переменные программы

/// <summary>
/// Количество выводимых докуметов в результате поиска
/// </summary>
const int MAX_RESULT_DOCUMENT_COUNT = 5;


//глобальные структуры

/// <summary>
/// Структура соотвествия id документа и его реливантности определённому запросу
/// </summary>
struct Document {
    int id;
    double relevance;
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
    void AddDocument(int document_id, const string& document) {
        vector<string> words = SplitIntoWordsNoStop(document);
        double TF = 1.0 / words.size();
        for (const string& word : words)
        {
            word_to_document_freq_[word][document_id] += TF;
        }
        ++document_count_;
    }

    /// <summary>
    /// Метод получения заданного количества наиболее релевантных результатов запроса
    /// </summary>
    /// <param name="raw_query"> - строка запроса</param>
    /// <returns> Возращает номера наиболее подходящих запросу документов и их реаливантность </returns>
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    //приватные атрибуты, структуры и методы класса
private:


    /// <summary>
    /// структра слов поиска и слов-фильтров
    /// </summary>
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    /// <summary>
    /// структра, показывающая принадлежность слова к группам минус-слов и стоп-слов
    /// </summary>
    struct Fin_Word {
        string word;
        bool is_minus_word;
        bool is_stop_word;
    };

    //Приватные атрибуты

    map<string, map<int, double>> word_to_document_freq_; // набор инвентированых индексов документов в классе 
    set<string> stop_words_;                              // набор стоп-слов
    int document_count_ = 0;                         // количество документов

    /// <summary>
    /// Проверка на стоп-слово
    /// </summary>
    /// <param name="word"> - проверяемое слово</param>
    /// <returns> True если слово является стоп-словом </returns>
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    /// <summary>
    /// Метод обработки слова запроса для определение его принадлежности 
    /// к группам минус-слов и стоп-слов
    /// </summary>
    /// <param name="word"> - слово из запроса</param>
    /// <returns>Структура из слова и флагов его принадлежности </returns>
    Fin_Word WorldHandler(const string& word) const {
        
        Fin_Word fin_word;
        if (word[0] == '-') {
            fin_word.is_minus_word = true;
            fin_word.word = word.substr(1);            
        }
        else { 
            fin_word.is_minus_word = false;
            fin_word.word = word;
        }
        fin_word.is_stop_word = IsStopWord(fin_word.word);

        return fin_word;
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
            else { word += c; }
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
    /// Метод получения множеств слов поиска и слов-фильтров из запроса
    /// </summary>
    /// <param name="text"> - строка запроса</param>
    /// <returns></returns>
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            Fin_Word fin_word = WorldHandler(word);
            if (!fin_word.is_stop_word) {
                if (fin_word.is_minus_word) {
                    query.minus_words.insert(fin_word.word);
                }
                else {
                    query.plus_words.insert(fin_word.word);
                }
            }
        }
        return query;
    }

    /// <summary>
    /// Метод поиска всех подходящих докуметов и получения их реливантности запросу
    /// </summary>
    /// <param name="query"> - структура с множествами слов поиска и слов-фильтров</param>
    /// <returns> Возвращает номера подхлдящих запросу документов и их реливантность </returns>
    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> v_matched_documents;
        map<int, double> matched_documents;
        for (const auto& word : query.plus_words)
        {
            if (word_to_document_freq_.count(word) != 0) {
                double idf = log(1.0 * document_count_ / word_to_document_freq_.at(word).size());
                for (const auto& [id, TF] : word_to_document_freq_.at(word))
                {
                    matched_documents[id] += idf * TF;
                }
            }
        }


        set<int> minus_id;
        for (const string& word : query.minus_words)
        {
            if (!word_to_document_freq_.count(word)) { continue; }
            for (const auto& [id, TF] : word_to_document_freq_.at(word))
            {
                minus_id.insert(id);
            }
        }

        for (const auto& [id, relevance] : matched_documents) {

            if (!minus_id.count(id)) {
                v_matched_documents.push_back({ id, relevance });
            }
        }
        return v_matched_documents;
    }
};

/// <summary>
/// Функция заполнения данных класса поисковой програмы
/// </summary>
/// <returns> Возвращает класс с набором методов для поискового запроса </returns>
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

/// <summary>
/// Главная функция поисковой программа
/// </summary>
/// <returns></returns>
int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}