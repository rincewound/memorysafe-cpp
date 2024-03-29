
#include "pch.h"
#include <optional>
#include <assert.h>
#include <cstdint>
#include <bitset>

template<class T, size_t n>
class MemoryPoolNoAlign
{
public:
	MemoryPoolNoAlign()
	{
		memset(data, 0x00, sizeof(data));
	};

	template<typename... Args>
	std::optional<T*> allocateAndConstruct(Args... args)
	{
		if (auto mem = allocate())
		{
			auto ptr = *mem;
			return new (ptr) T(args...);
		}

		return {};		
	}

	void free(T* block)
	{
		//calculate offset into dataarray, note that this check makes the
		// function nullptr safe as well!
		int32_t blockoffset = reinterpret_cast<uint8_t*>(block) - data;
		if (blockoffset < 0 || blockoffset > sizeof(data))
		{
			return;
		}

		uint32_t elementIndex = blockoffset / sizeof(T);

		// clear markerbyte for this block:
		usedElements[elementIndex] =false;
		// Explicitly call the d'tor to allow freeing
		// of other resources.
		block->~T();
	};

private:

	std::optional<T*> allocate()
	{
		for (size_t i = 0; i < n; i++)
		{
			size_t offset = ElementSize * i;
			if (!usedElements[i])
			{
				usedElements[i] = true;		// Mark block as used
				return reinterpret_cast<T*>(&data[offset]);
			}
		}
		return  {};
	};

	static const size_t ElementSize = sizeof(T);
	uint8_t data[n * ElementSize];
	std::bitset<n> usedElements;
};

class Foo
{
public: 
	Foo(int i, float f): _i(i), _f(f)
	{ }

	~Foo()
	{ }

	int _i;
	float _f;
};

int main()
{
	MemoryPoolNoAlign<Foo, 2> fooPool;
	auto f0 = fooPool.allocateAndConstruct(10, 25.0f);
	auto f1 = fooPool.allocateAndConstruct(1048, 3.141f);

	// Allocation error -> yields nullopt:
	if (auto f3 = fooPool.allocateAndConstruct(1048, 3.141f))
	{
		// should never be reached, since pool is full.
	}
	else
	{
		fooPool.free(*f0);
		assert(f3 = fooPool.allocateAndConstruct(1048, 3.141f));
	}

	fooPool.free(*f0);
	fooPool.free(*f1);
	fooPool.free(nullptr);
	
}