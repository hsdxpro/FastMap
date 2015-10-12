#include <iostream>
#include <random>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <map>
#include <unordered_map>
#include "FastMap.h"
#include <cassert>
#include <windows.h>
#include "rh_hash_table.hpp"

#undef max
typedef uint64_t numType;

int main()
{
	//__declspec(align(64)) int16_t hashes[64] = {	1, 2, 3, 4, 5, 6, 0, 8, 9, 1 , 11, 12, 13, 14, 15, 16,
	//												1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 11, 12, 13, 14, 15, 16 ,
	//												1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 11, 12, 13, 14, 15, 16 ,
	//												1, 2, 3, 4, 5, 6, 40, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	//
	////__declspec(align(64)) int16_t hash[16] = { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
	//
	//int16_t hash = 40;
	//__m256i _hash = _mm256_set1_epi16(hash);
	//
	//__declspec(align(64)) uint16_t resultShit[16];
	//
	//for (size_t i = 0; i < sizeof(hashes) / sizeof(hash); ++i)
	//{
	//	__m256i _hashes = _mm256_load_si256((__m256i*)hashes + i);
	//	__m256i _cmpmask = _mm256_cmpeq_epi16(_hash, _hashes);
	//	
	//	_cmpmask = _mm256_and_si256(_hashes, _cmpmask);
	//	//_mm256_mask_abs_epi16
	//	//int tmp = _mm256_movemask_epi8(cmpResult);
	//	//__m256i a = _mm256_mullo_epi16(cmpResult, _hashes);
	//	_
	//	_cmpmask = _mm256_hadd_epi16(_cmpmask, _cmpmask);// 8 elem
	//	_cmpmask = _mm256_hadd_epi16(_cmpmask, _cmpmask);// 2 elem
	//	_cmpmask = _mm256_hadd_epi16(_cmpmask, _cmpmask);// 1 elem
	//	//_cmpmask = _mm256_hadd_epi16(_cmpmask, _cmpmask);// 1 elem
	//	
	//	//_mm256_store_si256
	//	_mm256_store_si256((__m256i*)resultShit, _cmpmask);
	//
	//	if (resultShit[0])
	//	{
	//		auto value = ((__m256i*)hashes + i)[resultShit[0]];
	//	}
	//
	//	//__m128i res = _mm256_extracti128_si256(cmpResult, 0);
	//	//_mm_extract_epi16()
	//}

	//SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer;
	//DWORD buffer_size = 0;
	//GetLogicalProcessorInformation(0, &buffer_size);
	//
	//buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
	//
	//DWORD line_size = 0;
	//
	//if (GetLogicalProcessorInformation(buffer, &buffer_size))
	//{
	//	for (int i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
	//		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
	//			line_size = buffer[i].Cache.LineSize;
	//			break;
	//		}
	//	}
	//}
	//
	//free(buffer);

	HANDLE hProc = GetCurrentProcess();
	DWORD_PTR procMask;
	DWORD_PTR sysMask;
	HANDLE hDup;
	DuplicateHandle(hProc,
		hProc,
		hProc,
		&hDup,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);//Gets the current process affinity mask
	GetProcessAffinityMask(hDup, &procMask, &sysMask);//new Mask, uses only the first CPU
	DWORD newMask = 2;
	//Set the affinity mask for the process
	BOOL res = SetProcessAffinityMask(hDup, (DWORD_PTR)newMask);

	if (res == 0)
	{
		assert(0);
		//Error setting affinity mask!!
	}

	// ~ 3 * 64 more cache line needed, so 4 * 64 byte / sizeof(Pair) = bucketSize
	float bestCyclePerElem = FLT_MAX;
	numType dummyVal = 0;
	for (size_t z = 0; z < 4; ++z)
	{
		const uint64_t nPairs = 4096;
		const uint64_t minKeyVal = 0;
		const uint64_t maxKeyVal = std::numeric_limits<numType>::max();
		const float normedPercentToFound = 1.0f;

		// Set up random generation
		std::uniform_int_distribution<numType> randGen(minKeyVal, maxKeyVal);
		std::mt19937_64 randAlgo;
		randAlgo.seed(5000);

		const numType valueUsedForMissedLookUp = 1;

		// Generate and save numbers between (minKeyVal, minKeyVal), except value used for missed lookups
		std::vector<numType> nonRepetativeRandValue(nPairs);
		std::vector<numType> lookupValues(nPairs);

		auto nIter = nonRepetativeRandValue.size() + valueUsedForMissedLookUp + 1;
		int C = 0;
		for (size_t i = valueUsedForMissedLookUp + 1; i < nIter; ++i)
		{
			auto idx = i - valueUsedForMissedLookUp - 1;

			numType val = rand() % maxKeyVal;

			auto it = std::find(nonRepetativeRandValue.begin(), nonRepetativeRandValue.end(), val);
			while(it != nonRepetativeRandValue.end())
			{
				val = rand() % maxKeyVal;
				it = std::find(nonRepetativeRandValue.begin(), nonRepetativeRandValue.end(), val);
			}

			nonRepetativeRandValue[idx] = val;

			if ((float)i / nIter >= normedPercentToFound)
			{
				lookupValues[idx] = valueUsedForMissedLookUp;
			}
			else
			{
				lookupValues[idx] = val;
			}

			//numType val;
			//do
			//{
			//	val = i;// randGen(randAlgo);
			//} while (std::find(nonRepetativeRandValue.begin(), nonRepetativeRandValue.end(), val) != nonRepetativeRandValue.end());


			//	nonRepetativeRandValue[i] = val;

		}

		//for (size_t i = 0; i < lookupValues.size(); ++i)
		//{
		//	if ((float)i / lookupValues.size() >= normedPercentToFound)
		//	{
		//		uint16_t val;
		//		do
		//		{
		//			val = i;// randGen(randAlgo);
		//		} while (std::find(nonRepetativeRandValue.begin(), nonRepetativeRandValue.end(), val) != nonRepetativeRandValue.end() && 
		//			std::find(lookupValues.begin(), lookupValues.end(), val) != lookupValues.end());
		//
		//		lookupValues[i] = val;
		//	}
		//}
		uint64_t insertTime0 = 0;
		uint64_t insertTime1 = 0;
		uint64_t insertTime2 = 0;
		uint64_t insertTime3 = 0;
		uint64_t lookupTime0 = 0;
		uint64_t lookupTime1 = 0;
		uint64_t lookupTime2 = 0;
		uint64_t lookupTime3 = 0;
		uint64_t removeTime0 = 0;
		uint64_t removeTime1 = 0;
		uint64_t removeTime2 = 0;
		uint64_t removeTime3 = 0;
		uint64_t destructTime0 = 0;
		uint64_t destructTime1 = 0;
		uint64_t destructTime2 = 0;

		std::map<numType, numType> stdmap;
		std::unordered_map<numType, numType> stdumap;

		FastMapMaxSize32Bit<numType, numType> fastMap((uint32_t)nonRepetativeRandValue.size(), 2); // * 2 cuz RobinMap uses twice more memory, so less collision will occur (cheater:P)
		hash_table<numType, numType> robinMap;

		// std::map insert
		{
			auto beg = __rdtsc();
			for (auto& val : nonRepetativeRandValue)
			{
				stdmap.insert(std::make_pair(val, val));
			}
			auto end = __rdtsc();
			insertTime0 = end - beg;
		}

		// std::unordered_map insert
		{
			auto beg = __rdtsc();
			for (auto& val : nonRepetativeRandValue)
			{
				stdumap.insert(std::make_pair(val, val));
			}
			auto end = __rdtsc();
			insertTime1 = end - beg;
		}

		// fastMap insert
		{
			auto beg = __rdtsc();
			for (auto& val : nonRepetativeRandValue)
			{
				fastMap.Insert(val, val);
			}
			auto end = __rdtsc();
			insertTime2 = end - beg;
		}

		// RobinMap insert
		{
			auto beg = __rdtsc();
			for (auto& val : nonRepetativeRandValue)
			{
				robinMap.insert(val, val);
			}
			auto end = __rdtsc();
			insertTime3 = end - beg;
		}

		// std::map lookup
		{
			auto beg = __rdtsc();
			for (int i = (int)lookupValues.size() - 1; i >= 0; --i)
			{
				auto it = stdmap.find(lookupValues[i]);
				if (it != stdmap.end())
					dummyVal = it->second;
			}
			auto end = __rdtsc();
			lookupTime0 = end - beg;
		}

		uint64_t nFound0 = 0;
		// std::unordered_map lookup
		{
			auto beg = __rdtsc();
			for (int i = (int)lookupValues.size() - 1; i >= 0; --i)
			{
				auto it = stdumap.find(lookupValues[i]);
				if (it != stdumap.end())
				{
					++nFound0;
					dummyVal = it->second;
				}
					
			}
			auto end = __rdtsc();
			lookupTime1 = end - beg;
		}

		uint64_t nFound1 = 0;
		// fatMap lookup
		{
			auto beg = __rdtsc();
			for (int i = (int)lookupValues.size() - 1; i >= 0; --i)
			{
				auto valPtr = fastMap.Find(lookupValues[i]);
				if (valPtr)
				{
					++nFound1;
					dummyVal = *valPtr;
				}
				else
				{
					int asd = 5;
					asd++;
				}
			}
			auto end = __rdtsc();
			lookupTime2 = end - beg;
		}
		assert(nFound0 == nFound1);

		uint64_t nFound2 = 0;
		// robinMap lookup
		{
			auto beg = __rdtsc();
			for (int i = (int)lookupValues.size() - 1; i >= 0; --i)
			{
				auto valPtr = robinMap.find(lookupValues[i]);
				if (valPtr)
				{
					++nFound2;
					dummyVal = *valPtr;
				}

			}
			auto end = __rdtsc();
			lookupTime3 = end - beg;
		}
		assert(nFound0 == nFound2);

		// std::map remove
		{
			auto beg = __rdtsc();
			for (size_t i = 0; i < lookupValues.size(); ++i)
			{
				stdmap.erase(lookupValues[i]);
			}
			auto end = __rdtsc();
			removeTime0 = end - beg;
		}
		
		// std::unordered_map remove
		{
			auto beg = __rdtsc();
			for (size_t i = 0; i < lookupValues.size(); ++i)
			{
				stdumap.erase(lookupValues[i]);
			}
			auto end = __rdtsc();
			removeTime1 = end - beg;
		}
		
		// fastMap remove
		{
			auto beg = __rdtsc();
			for (size_t i = 0; i < lookupValues.size(); ++i)
			{
				fastMap.Erase(lookupValues[i]);
			}
			auto end = __rdtsc();
			removeTime2 = end - beg;
		}

		// robinmap remove
		{
			auto beg = __rdtsc();
			for (size_t i = 0; i < lookupValues.size(); ++i)
			{
				robinMap.erase(lookupValues[i]);
			}
			auto end = __rdtsc();
			removeTime3 = end - beg;
		}

		//nFound2 = 0;
		//// fastMap lookup
		//{
		//	//auto beg = __rdtsc();
		//	for (int i = lookupValues.size() - 1; i >= 0; --i)
		//	{
		//		auto valPtr = fastMap.Find(lookupValues[i]);
		//		if (valPtr)
		//		{
		//			++nFound2;
		//			dummyVal = *valPtr;
		//		}
		//	}
		//	//auto end = __rdtsc();
		//	//lookupTime3 = end - beg;
		//}
		//assert(nFound2 == 0 );
		//
		//
		//// fastMap insert
		//{
		//	auto beg = __rdtsc();
		//	for (auto& val : nonRepetativeRandValue)
		//	{
		//		fastMap.Insert(val, val);
		//		robinMap.insert(val, val);
		//	}
		//	auto end = __rdtsc();
		//	insertTime2 = end - beg;
		//}
		//
		//nFound2 = 0;
		//// fastMap lookup
		//{
		//	//auto beg = __rdtsc();
		//	for (int i = lookupValues.size() - 1; i >= 0; --i)
		//	{
		//		auto valPtr = fastMap.Find(lookupValues[i]);
		//		if (valPtr)
		//		{
		//			++nFound2;
		//			dummyVal = *valPtr;
		//		}
		//		else
		//		{
		//			int asd = 5;
		//			asd++;
		//		}
		//	}
		//	//auto end = __rdtsc();
		//	//lookupTime3 = end - beg;
		//}
		//assert(nFound2 == nPairs);

		// std::map destruct
		//uint64_t destructBeg0;
		//{
		//	std::map<uint16_t, uint16_t> stdmapCopy = stdmap;
		//	destructBeg0 = __rdtsc();
		//}
		//auto destructEnd0 = __rdtsc();
		//destructTime0 = destructEnd0 - destructBeg0;
		//
		//// std::unordered_map destruct
		//uint64_t destructBeg1;
		//{
		//	std::unordered_map<uint16_t, uint16_t> stdmapCopy = stdumap;
		//	destructBeg1 = __rdtsc();
		//}
		//auto destructEnd1 = __rdtsc();
		//destructTime1 = destructEnd1 - destructBeg1;
		//
		//// std::unordered_map destruct
		//uint64_t destructBeg2 = 0;
		//{
		//	FastMap16bit<uint16_t, uint16_t> tmp;
		//	tmp = fastMap;
		//	destructBeg2 = __rdtsc();
		//}
		//auto destructEnd2 = __rdtsc();
		//destructTime2 = destructEnd2 - destructBeg2;

		//std::cout << "std::map           Insert:   " << insertTime0 << std::endl;
		//std::cout << "std::unordered_map Insert:   " << insertTime1 << std::endl;
		//std::cout << "FastMap            Insert:   " << insertTime2 << std::endl;
		//std::cout << "RobinMap           Insert:   " << insertTime3 << std::endl << std::endl;

		std::cout << "std::map           Lookup cycles:   " << lookupTime0 << std::endl;
		std::cout << "std::unordered_map Lookup cycles:   " << lookupTime1 << std::endl;
		std::cout << "FastMap            Lookup cycles:   " << lookupTime2 << std::endl;
		std::cout << "RobinMap           Lookup cycles:   " << lookupTime3 << std::endl << std::endl;
		//std::cout << "FastMapAccessCounter         " << FastMapAccessCounter << std::endl;
		//std::cout << "RobinMapAccessCounter        " << RobinMapAccessCounter << std::endl;
		//std::cout << "FastMapColliionCacheMiss     " << FastMapColliionCacheMiss << std::endl;
		//std::cout << "FastMapSlideCacheMiss        " << FastMapSlideCacheMiss << std::endl;
		//std::cout << "FastMapUnalignedMemAccess    " << FastMapUnalignedMemAccess << std::endl << std::endl;

		//std::cout << "std::map             cycle / element: " << (double)lookupTime0 / (double)nPairs << std::endl;
		//std::cout << "std::unordered_map   cycle / element: " << (double)lookupTime1 / (double)nPairs << std::endl;
		//std::cout << "FastMap              cycle / element: " << (double)lookupTime2 / (double)nPairs << std::endl;
		//std::cout << "RobinMap             cycle / element: " << (double)lookupTime3 / (double)nPairs << std::endl << std::endl;
		//std::cout << "FastMap               number of misaligned cache reads: " << FastMapAccessCounter << std::endl;
		if ((float)lookupTime2 / (float)nPairs < bestCyclePerElem)
		{
			bestCyclePerElem = (float)lookupTime2 / (float)nPairs;
		}


		//std::cout << "RobinMap               number of buffer access: " << RobinMapAccessCounter << std::endl << std::endl << std::endl;
		//std::cout << "std::map            Remove:   " << removeTime0 << std::endl;
		//std::cout << "std::unordered_map  Remove:   " << removeTime1 << std::endl;
		//std::cout << "FastMap             Remove:   " << removeTime2 << std::endl;
		//std::cout << "RobinMap            Remove:   " << removeTime3 << std::endl << std::endl;
		//
		////std::cout << "std::map           Destruct: " << destructTime0 << std::endl;
		//std::cout << "std::unordered_map Destruct: " << destructTime1 << std::endl;
		//std::cout << "FastMap             Destruct: " << destructTime2 << std::endl << std::endl;
	}
	std::cout << "best cycle per elem: " << bestCyclePerElem << std::endl;
	getchar();

	return (int)dummyVal;
}