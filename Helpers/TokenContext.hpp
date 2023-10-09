#pragma once
#include <memory>

// NOTE: The TokenContextWeak not reside inside the TokenContext class to avoid "non-deduced cotext" in template functions

template <typename T>
struct TokenContextWeak { // use TokenCotext<T>::GetWeak()
	using Data_t = T;

	Data_t* data = nullptr;
	std::weak_ptr<int> token;
};


template <typename T>
class TokenContext { // place as class member
public:
	using Data_t = T;

	TokenContextWeak<T> GetWeak() {
		return { data, token };
	};

private:
	Data_t* data = nullptr;
	std::shared_ptr<int> token = std::make_shared<int>();
};