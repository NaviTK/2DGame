#ifndef _OBJECT_POOL_INCLUDE
#define _OBJECT_POOL_INCLUDE


#include <cstddef>


// Fixed-capacity Object Pool stored contiguously in memory (zero heap allocations
// during gameplay). Pooled types T must implement:
//   bool isActive() const;
//   void deactivate();
template <typename T, std::size_t Capacity>
class ObjectPool
{

public:
	ObjectPool()
	{
		releaseAll();
	}

	// Finds the first inactive slot in the pool and returns a pointer to it,
	// or NULL if all slots are currently active.
	T *acquire()
	{
		for(std::size_t i = 0; i < Capacity; ++i)
		{
			if(!items_[i].isActive())
				return &items_[i];
		}
		return NULL;
	}

	// Marks all objects in the pool as inactive without deallocating memory.
	void releaseAll()
	{
		for(std::size_t i = 0; i < Capacity; ++i)
		{
			items_[i].deactivate();
		}
	}

	// Returns the number of currently active objects in the pool.
	std::size_t activeCount() const
	{
		std::size_t count = 0;
		for(std::size_t i = 0; i < Capacity; ++i)
		{
			if(items_[i].isActive())
				++count;
		}
		return count;
	}

	bool empty() const
	{
		return activeCount() == 0;
	}

	std::size_t capacity() const
	{
		return Capacity;
	}

	T &operator[](std::size_t index) { return items_[index]; }
	const T &operator[](std::size_t index) const { return items_[index]; }

	T *begin() { return &items_[0]; }
	T *end() { return &items_[0] + Capacity; }
	const T *begin() const { return &items_[0]; }
	const T *end() const { return &items_[0] + Capacity; }

private:
	T items_[Capacity];

};


#endif // _OBJECT_POOL_INCLUDE

