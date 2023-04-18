#include "document.h"

Document::Document() = default;
Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator << (std::ostream& ost, const Document& doc)
{
    return ost << std::string("{ document_id = ") << doc.id
        << std::string(", relevance = ") << doc.relevance
        << std::string(", rating = ") << doc.rating << std::string(" }");
}
