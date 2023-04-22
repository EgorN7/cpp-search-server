#pragma once
#include <vector>


// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent();

// ���� ���������, ��� ��������� ������� ��������� �����-����� �� ���������� �������
void TestExcludeMinusWordsFromQuery();

// ���� �� �������� ������������� ����������� ��������� � ���������� �������

void TestMatchDocuments();


// ���� �� ���������� �� ������������� �������� ����������
void TestSortingByRelevance();
// ���� �� ������������ ���������� �������������
void TestCalculatingRelevance();

/// <summary>
/// ���������� �������� �������������� ������ ��������
/// </summary>
/// <param name="rating"> ������ ������</param>
/// <returns>�������� �������������� ������ ��������</returns>
int ArithmeticMeanOfTheRating(std::vector<int> rating);

// ���� �� ������������ ���������� �������� 
void TestCalculatingRating();

// ���� �� ���������� ���������� � �������������� ���������
void TestFilterByPredicate();

// ���� �� ���������� ���������� � �������������� �������
void TestFilterByStatus();

// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer();