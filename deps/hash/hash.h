#pragma once
#include <cstdint>

constexpr uint64_t Prime = 1099511628211u;
constexpr uint64_t Basis = 14695981039346656037u;

template< typename Type >
constexpr uint64_t HashCompute(uint64_t hash, const Type* const data, uint64_t size) {
	const auto element = (uint64_t)(data[0]);
	return (size == 0) ? hash : HashCompute((hash * Prime) ^ element, data + 1, size - 1);
}

template< typename Type >
constexpr uint64_t Hash(const Type* const data, uint64_t size){
	return HashCompute(Basis, data, size);
}

#define HASH( Data ) \
	[ & ]() \
	{ \
		constexpr auto hash = Hash( Data, sizeof(Data) - 1 );	\
		return hash; \
	}()
