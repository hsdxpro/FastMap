// Important TODOs
// - cache misses cuz unaligned memory read, for example pairs + 3 will point to memory address which will be mapped to end of a cache line, and exceed that line, but can be placed into 1 cache line, rather than 2
// - cache misses when hash collisions occur, try reduce sizeof(Pair)
// - cache misses cuz left directional search in Insert(...)

#pragma once
#include <cassert>

static int FastMapAccessCounter;
static int FastMapSlideCacheMiss;
static int FastMapUnalignedMemAccess;
static int FastMapColliionCacheMiss;

// Helper classes for determining best fit byte size
template<typename Val,  bool is8Bit > class TypeChoose8bit {};
template<typename Val> class TypeChoose8bit< Val, true>  { public: typedef  uint8_t	Type; };
template<typename Val> class TypeChoose8bit< Val, false> { public: typedef  Val		Type; };

template<typename Val,  bool is16Bit > class TypeChoose16bit {};
template<typename Val> class TypeChoose16bit< Val, true>  { public: typedef  uint16_t	Type; };
template<typename Val> class TypeChoose16bit< Val, false> { public: typedef  Val		Type; };

template<typename Val,  bool is32Bit > class TypeChoose32bit {};
template<typename Val> class TypeChoose32bit<Val, true>  { public: typedef  uint32_t	Type; };
template<typename Val> class TypeChoose32bit<Val, false>  { public: typedef  Val		Type; };

template<typename Val> class SmallestContainer { public: typedef typename TypeChoose8bit<typename TypeChoose16bit<typename TypeChoose32bit<Val, sizeof(Val) < 8>::Type, sizeof(Val) < 4>::Type, sizeof(Val) < 2>::Type Type; };


// Our beautiful Map class
// - Important notes
//		- Key size must be <= 64
//		- Value can be any size
//		- MaxCapacity can be any size, default 32 bit

template<typename Key, typename Value, typename MaxSize = uint32_t, typename MaxCapacity = uint32_t>
class FastMap
{
	typedef typename SmallestContainer<Key>::Type Hash;
	typedef typename SmallestContainer<uint8_t[sizeof(MaxSize) / 2]>::Type BucketSize; // This is a half of max size

	// TODO: too large struct -> cache miss
	// TODO: struct reorder with respect to member sizes
	// if(sizeof(Value) > size_t) struct Pair0
	// else struct Pair1

	static const size_t pairAlignment = 64;

	struct Pair
	{
		inline Pair() : size(0), bucketPtr(this){}

		Hash				hash;
		BucketSize			size;
		Value				value;
		Pair*				bucketPtr;
	};

public:
	inline FastMap()
	:size(0), capacityAndMask(0), pairs(nullptr)
	{
		static_assert(sizeof(Key) <= sizeof(uint16_t), "HashMap doesn't support larger keys than 64 bit");
	}

	// - capacity : The intended capacity for container
	// - capacityMultiplier : Increasing this will improve Find(...) performance, but container will use memory
	inline FastMap(MaxSize capacity, MaxSize capacityMultiplier = 2)
	: size(0)
	{
		assert((uint64_t)capacity * capacityMultiplier <= std::numeric_limits<MaxSize>::max());

		// Potentially more memory usage when capacityMultiplier > 1, but less collision -> faster Find(...)
		capacity *= capacityMultiplier;

		capacityAndMask = capacity - 1;

		// capacity must be power of 2 !!
		assert(capacity > 0 && !(capacity & (capacity - 1)));

		// Please use default constructor instead
		assert(capacity > 0);

		// The only buffer our class operates on, buffer consists of many (struct Pair)
		pairs = (Pair*)_aligned_malloc(sizeof(Pair) * capacity, pairAlignment); // Align to worst case cache line size

		// Construct pairs
		for (size_t i = 0; i < capacity; ++i)
			new (&pairs[i]) Pair();
	}

	inline ~FastMap()
	{
		_aligned_free(pairs);
	}

	inline FastMap(const FastMap& other)
	{
		*this = other;
	}

	inline FastMap& operator = (const FastMap& other)
	{
		// Grow buffer size if not enough
		if (GetCapacity() < other.GetCapacity())
		{
			_aligned_free(pairs);
			pairs = (Pair*)_aligned_malloc(sizeof(Pair) * other.GetCapacity(), pairAlignment); // Align to worst case cache line size
		}

		// Copy data from other map
		for (size_t i = 0; i < other.GetCapacity(); ++i)
			pairs[i] = other.pairs[i];

		// Copy variables
		size = other.size;
		capacityAndMask = other.capacityAndMask;

		return *this;
	}

	inline static Hash CalcHash(const Key& key)
	{
		assert(sizeof(Hash) == sizeof(Key));

		return (Hash)key; // key can be float, etc reinterpret it
	}

	inline MaxSize CalcDesiredPos(Hash hash)
	{
		return capacityAndMask & hash;
	}

	inline void Insert(const Key& key, const Value& value, uint32_t nMinReserve)
	{
		FastMapAccessCounter = 0;
		FastMapSlideCacheMiss = 0;
		FastMapUnalignedMemAccess = 0;
		FastMapColliionCacheMiss = 0;

		assert(Find(key) == nullptr);

		// Need buffer grow
		if (size == GetCapacity())
		{
			// Should not run, need more implementation...
			assert(0);

			int32_t oldCapacityAndMask = capacityAndMask;
		
			// New capacity value
			capacityAndMask += nMinReserve;

			// CapacityAndMask is unsigned, but the mask need to be capacity - 1, correct that, unsigned cant hold -1, so default capacity is 0
			if (oldCapacityAndMask == 0)
			{
				--oldCapacityAndMask;
				--capacityAndMask;
			}
				
			// Reallocate bigger memory
			pairs = (Pair*)_aligned_realloc(pairs, sizeof(Pair) * GetCapacity(), pairAlignment);

			// Construct new elems
			//CapacityAndMask + 1 = Capacity
			for (size_t i = oldCapacityAndMask + 1; i < GetCapacity(); ++i)
				new (&pairs[i]) Pair();
		
			// TODO
			// We should recalculate bucket informations cuz capacity_and_mask changed, so desiredPosition can be different for a specific hash
		}

		auto hash = CalcHash(key);
		auto desiredPos = CalcDesiredPos(hash);
		
		Pair* targetBucket = pairs[desiredPos].bucketPtr;
		Pair* targetPlace = targetBucket + targetBucket->size;

		if (targetPlace > pairs + capacityAndMask)
			--targetPlace;

		// Free bucket base
		//if (targetBucket->size == 0)
		//Hash left = ;
		if (targetBucket->size == 0)
		{
			targetBucket->hash = hash;
			targetBucket->value = value;
			targetBucket->bucketPtr = targetBucket;
			targetBucket->size = 1;
		}
		//else if (targetPlace->size == 0) // Free bucket child
		else if (targetPlace->size == 0) // Free bucket child
		{
			targetPlace->hash = hash;
			targetPlace->value = value;
			targetPlace->bucketPtr = targetPlace;
			targetPlace->size = 1;

			Pair* ptr = targetBucket;
			while (ptr != targetPlace)
			{
				++ptr->size;
				++ptr;
			}
		}
		else // Not free slot
		{
			size_t targetPlaceIdx = (targetPlace - pairs);

			// Search for free slot to the right
			size_t freeIdx = targetPlaceIdx + 1;
			while (freeIdx <= capacityAndMask)
			{
				if (pairs[freeIdx].size == 0)
					break;

				++freeIdx;
			}

			// Not found to the right, search to the left
			// TODO: cache miss will occur in Find(...) if future bucketPtr < &pairs[desiredPos] !!, try make more space to the right direction for inserting
			if (freeIdx > capacityAndMask)
			{
				size_t targetBucketIdx = targetBucket - pairs;
				intmax_t freeIdx = targetBucketIdx - 1;
				while (freeIdx >= 0)
				{
					if (pairs[freeIdx].size == 0)
						break;

					--freeIdx;
				}

				assert(freeIdx >= 0);

				// Create space for inserting, memmove right, except bucketPtr struct member
				// And important to update bucketPtr	
				if (targetBucket == targetPlace)
				{
					++targetBucket->size;
				}
				else
				{
					--targetPlace;
					--targetPlaceIdx;

					Pair* ptr = targetBucket;
					while (ptr <= targetPlace)
					{
						++ptr->size;
						++ptr;
					}
				}
				
				intmax_t nElemesToMove = targetPlaceIdx - freeIdx;
				for (intmax_t i = freeIdx; i < freeIdx + nElemesToMove; ++i)
				{
					pairs[i].hash = pairs[i + 1].hash;
					pairs[i].value = pairs[i + 1].value;
					pairs[i].size = pairs[i + 1].size;
				}

				// targetPlace - nak egyel balra kéne mutatnia bizonyos esetekben, gondolkodj ezen
				// Update bucket pointers where necessary
				Pair* freeSlotPtr = pairs + freeIdx;
				for (size_t i = 0; i <= capacityAndMask; ++i)
				{
					if (pairs[i].size != 0 && pairs[i].bucketPtr > freeSlotPtr && pairs[i].bucketPtr <= targetPlace)
						--pairs[i].bucketPtr;
				}
			}
			else // found free slot (right dir)
			{
				Pair* ptr = targetBucket;
				while (ptr != targetPlace)
				{
					++ptr->size;
					++ptr;
				}	
				
				// Create space for inserting, memmove right, except bucketPtr struct member
				// And important to update bucketPtr
				size_t nElemesToMove = freeIdx - targetPlaceIdx;
				Pair savedPair = pairs[targetPlaceIdx];
				for (size_t i = targetPlaceIdx; i < targetPlaceIdx + nElemesToMove; ++i)
				{
					Pair tmp = pairs[i + 1];

					pairs[i + 1].hash	= savedPair.hash;
					pairs[i + 1].value	= savedPair.value;
					pairs[i + 1].size	= savedPair.size;

					savedPair = tmp;
				}

				// Update bucket pointers where necessary
				Pair* freeSlotPtr = pairs + freeIdx;
				for (size_t i = 0; i <= capacityAndMask; ++i)
				{
					if (pairs[i].size != 0 && pairs[i].bucketPtr >= targetPlace && pairs[i].bucketPtr < freeSlotPtr)
						++pairs[i].bucketPtr;
				}
			}

			// "Insert", copy values
			targetPlace->hash = hash;
			targetPlace->value = value;
			targetPlace->size = 1;
		}

		++size;
	}

	inline void Insert(const Key& key, const Value& val)
	{
		Insert(key, val, GetCapacity()); // 200% buffer growing when necessary
	}

	Value* Find(const Key& key)
	{
		auto hash = CalcHash(key);
		auto desiredPos = CalcDesiredPos(hash);

		Pair* ptr = pairs[desiredPos].bucketPtr;

		//if (ptr < &pairs[desiredPos])
		//{
		//	++FastMapSlideCacheMiss;
		//}
		//
		//int bytesToLoad = sizeof(Pair) * ptr->size;
		//const size_t cacheLineSize = 64;
		//bytesToLoad -= cacheLineSize;
		//
		//if (bytesToLoad < 0)
		//	bytesToLoad = 0;
		//
		//FastMapColliionCacheMiss += (float)bytesToLoad / cacheLineSize;
		//
		//Pair* end = ptr + ptr->size;
		//
		//if (ptr->size > 0 && (size_t)end % cacheLineSize <= (size_t)ptr %cacheLineSize && (size_t)end % cacheLineSize != 0)
		//{
		//	++FastMapUnalignedMemAccess;
		//}

		// Additional Cache miss can occur when ptr < &pairs[desiredPos] :( -> bad Insert(...) implementation
		// Additional Cache miss can occur when ptr->size is way too large (hash collision) -> try decrease sizeof(Pair)
		auto nChildsLeft = ptr->size;
		while (nChildsLeft)
		{
			//++FastMapAccessCounter;

			if (ptr->hash == hash) // Unaligned memory access, (struct Pair) sometimes not power of 2, and also [ptr % cacheLineSize, (ptr + ptr->size) % cacheLineSize] intervallum can exceed 1 more cache line than necessary
				return &ptr->value;

			--nChildsLeft;
			++ptr;
		}

		return nullptr;
	}

	// TODO
	inline bool Erase(const Key& key, bool bAllowBufferShrink = false)
	{
		return false;
	}

	inline uint32_t GetCapacity() const { return capacityAndMask + 1; }

protected:
	Pair* pairs;

	MaxSize size;
	MaxCapacity capacityAndMask;
};

// 8 bit max size instance
template<typename Key, typename Value, typename MaxCapacity = uint32_t>
class FastMapMaxSize8Bit : public FastMap < Key, Value, uint8_t, MaxCapacity >
{
public:
	inline FastMapMaxSize8Bit() : FastMap<Key, Value, uint8_t, MaxCapacity>(){}

	// - capacity : The intended capacity for container
	// - capacityMultiplier : Increasing this will improve Find(...) performance, but container will use memory
	inline FastMapMaxSize8Bit(MaxCapacity capacity, MaxCapacity capacityMultiplier = 2) : FastMap<Key, Value, uint8_t, MaxCapacity>(capacity, capacityMultiplier){}
};

// 16 bit max size instance
template<typename Key, typename Value, typename MaxCapacity = uint32_t>
class FastMapMaxSize16Bit : public FastMap < Key, Value, uint16_t, MaxCapacity >
{
public:
	inline FastMapMaxSize16Bit() : FastMap<Key, Value, uint16_t, MaxCapacity>(){}

	// - capacity : The intended capacity for container
	// - capacityMultiplier : Increasing this will improve Find(...) performance, but container will use memory
	inline FastMapMaxSize16Bit(MaxCapacity capacity, MaxCapacity capacityMultiplier = 2) : FastMap<Key, Value, uint16_t, MaxCapacity>(capacity, capacityMultiplier){}
};

// 32 bit max size instance
template<typename Key, typename Value, typename MaxCapacity = uint32_t>
class FastMapMaxSize32Bit : public FastMap < Key, Value, uint32_t, MaxCapacity >
{
public:
	inline FastMapMaxSize32Bit() : FastMap<Key, Value, uint32_t, MaxCapacity>(){}

	// - capacity : The intended capacity for container
	// - capacityMultiplier : Increasing this will improve Find(...) performance, but container will use memory
	inline FastMapMaxSize32Bit(MaxCapacity capacity, MaxCapacity capacityMultiplier = 2) : FastMap<Key, Value, uint32_t, MaxCapacity>(capacity, capacityMultiplier){}
};

// 64 bit max size instance
template<typename Key, typename Value, typename MaxCapacity = uint32_t>
class FastMapMaxSize64Bit : public FastMap < Key, Value, uint64_t, MaxCapacity >
{
public:
	inline FastMapMaxSize64Bit() : FastMap<Key, Value, uint64_t, MaxCapacity>(){}

	// - capacity : The intended capacity for container
	// - capacityMultiplier : Increasing this will improve Find(...) performance, but container will use memory
	inline FastMapMaxSize64Bit(MaxCapacity capacity, MaxCapacity capacityMultiplier = 2) : FastMap<Key, Value, uint64_t, MaxCapacity>(capacity, capacityMultiplier){}
};