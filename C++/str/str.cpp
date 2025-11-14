#include "str.hpp"

bool operator== (const String& lhs, const String& rhs)  {
	if ((lhs.Data_ == nullptr) && (rhs.Data_ == nullptr)) return true;
	if ((lhs.Data_ == nullptr && rhs.Data_ != nullptr) ||
			(lhs.Data_ != nullptr && rhs.Data_ == nullptr)) return false;

	if (lhs.Size() == rhs.Size()) {
		return strncmp(lhs.Data_, rhs.Data_, lhs.Size()) == 0;
	}
	else
		return false;
}

String operator+ (const String& lhs, const String& rhs) { 
	String result(lhs);
	result += rhs;
	return result;
}

std::ostream& operator<< (std::ostream& stream, const String& first) {
	for (size_t i = 0; i < first.Size(); i++) { stream << first.Data_[i]; }
	return stream;
}

std::istream& operator>> (std::istream& stream, String& first) {
	for (size_t i = 0; i < first.Size(); i++) { stream >> first.Data_[i]; }
	return stream;
}

int operator<=>(const String& lhs, const String& rhs) {
	size_t nCompared = std::min(rhs.Size(), lhs.Size());
    for (size_t i = 0; i < nCompared; i++) {
        if (lhs[i] < rhs[i])
            return -1;
        else if (lhs[i] > rhs[i])
            return 1;
    }

    if (lhs.Size() < rhs.Size())
        return -1;
    else if (lhs.Size() > rhs.Size())
        return 1;

    return 0;
}
