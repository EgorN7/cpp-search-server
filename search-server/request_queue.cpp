#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    current_requests_count++;
    if (current_requests_count > min_in_day_) {
        requests_.pop_front();
        current_requests_count--;
    }
    requests_.push_back({ search_server_.FindTopDocuments(raw_query, status) });

    return search_server_.FindTopDocuments(raw_query, status);
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    current_requests_count++;
    if (current_requests_count > min_in_day_) {
        requests_.pop_front();
        current_requests_count--;
    }
    requests_.push_back({ search_server_.FindTopDocuments(raw_query) });

    return search_server_.FindTopDocuments(raw_query);
}
int RequestQueue::GetNoResultRequests() const {
    int count = 0;
    std::deque<QueryResult> dq_copy(requests_);
    while (!dq_copy.empty()) {
        std::vector<Document> s = dq_copy.back().documents;
        if (s.size() == 0) {
            count++;
        }
        dq_copy.pop_back();
    }
    return count;
}