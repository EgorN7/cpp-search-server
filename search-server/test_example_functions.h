#pragma once
#include <vector>


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();

// Тест проверяет, что поисковая система исключает минус-слова из поискового запроса
void TestExcludeMinusWordsFromQuery();

// Тест на проверку сопоставления содержимого документа и поискового запроса

void TestMatchDocuments();


// Тест на сортировку по релевантности найденых документов
void TestSortingByRelevance();
// Тест на правильность вычесления релевантности
void TestCalculatingRelevance();

/// <summary>
/// вычисление среднего ариметического оценок докумета
/// </summary>
/// <param name="rating"> вектор оценок</param>
/// <returns>среднего ариметического оценок докумета</returns>
int ArithmeticMeanOfTheRating(std::vector<int> rating);

// Тест на правильность вычесления рейтинга 
void TestCalculatingRating();

// Тест на фильтрацию результата с использованием предиката
void TestFilterByPredicate();

// Тест на фильтрацию результата с использованием статуса
void TestFilterByStatus();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();