/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2024 contributors

This file is part of the SerenityJediEngine2024 source code.

SerenityJediEngine2024 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD TEMPLATE LIBRARY
//  (c) 2002 Activision
//
//
// Common
// ------
// The raven libraries contain a number of common defines, enums, and typedefs which
// need to be accessed by all templates.  Each of these is included here.
//
// Also included is a safeguarded assert file for all the asserts in RTL.
//
// This file is included in EVERY TEMPLATE, so it should be very light in order to
// reduce compile times.
//
//
// Format
// ------
// In order to simplify code and provide readability, the template library has some
// standard formats.  Any new templates or functions should adhere to these formats:
//
// - All memory is statically allocated, usually by parameter SIZE
// - All classes provide an enum which defines constant variables, including CAPACITY
// - All classes which moniter the number of items allocated provide the following functions:
//     size()   - the number of objects
//     empty()  - does the container have zero objects
//     full()   - does the container have any room left for more objects
//     clear()  - remove all objects
//
//
// - Functions are defined in the following order:
//     Capacity
//     Constructors  (copy, from string, etc...)
//     Range		 (size(), empty(), full(), clear(), etc...)
//     Access        (operator[], front(), back(), etc...)
//     Modification  (add(), remove(), push(), pop(), etc...)
//     Iteration     (begin(), end(), insert(), erase(), find(), etc...)
//
//
// NOTES:
//
//
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// In VC++, Don't Bother With These Warnings
////////////////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) && !defined(__MWERKS__)
#pragma warning ( disable : 4786 )			// Truncated to 255 characters warning
#pragma warning ( disable : 4284 )			// nevamind what this is
#pragma warning ( disable : 4100 )			// unreferenced formal parameter
#pragma warning ( disable : 4512 )			// unable to generate default operator=
#pragma warning ( disable : 4130 )			// logical operation on address of string constant
#pragma warning ( disable : 4127 )			// conditional expression is constant
#pragma warning ( disable : 4996 )			// This function or variable may be unsafe.
#pragma warning ( disable : 5208 )	        // This function or variable may be unsafe.
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(ASSERT_H_INC)
#include <cassert>
#define ASSERT_H_INC
#endif

#if !defined(STRING_H_INC)
#include <cstring>
#define STRING_H_INC
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Forward Dec.
////////////////////////////////////////////////////////////////////////////////////////
class hfile;

// I don't know why this needs to be in the global namespace, but it does
class TRatlNew;

inline void* operator new(size_t, TRatlNew* where)
{
	return where;
}

inline void operator delete(void*, TRatlNew*)
{
}

namespace ratl
{
#ifdef _DEBUG
	extern int HandleSaltValue; //this is used in debug for global uniqueness of handles
	extern int FoolTheOptimizer; //this is used to make sure certain things aren't optimized out
#endif

	////////////////////////////////////////////////////////////////////////////////////////
	// All Raven Template Library Internal Memory Operations
	//
	// This is mostly for future use.  For now, they only provide a simple interface with
	// a couple extra functions (eql and clr).
	////////////////////////////////////////////////////////////////////////////////////////
	namespace mem
	{
		////////////////////////////////////////////////////////////////////////////////////////
		// The Align Struct Is The Root Memory Structure for Inheritance and Object Semantics
		//
		// In most cases, we just want a simple int.  However, sometimes we need to use an
		// unsigned character array
		//
		////////////////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER) && !defined(__MWERKS__)
		struct alignStruct
		{
			int space;
		};
#else
		struct alignStruct
		{
			unsigned char space[16];
		} __attribute__((aligned(16)));
#endif

		inline void* cpy(void* dest, const void* src, const size_t count)
		{
			return memcpy(dest, src, count);
		}

		inline void* set(void* dest, const int c, const size_t count)
		{
			return memset(dest, c, count);
		}

		inline int cmp(const void* buf1, const void* buf2, const size_t count)
		{
			return memcmp(buf1, buf2, count);
		}

		inline bool eql(const void* buf1, const void* buf2, const size_t count)
		{
			return memcmp(buf1, buf2, count) == 0;
		}

		inline void* zero(void* dest, const size_t count)
		{
			return memset(dest, 0, count);
		}

		template <class T>
		void cpy(T* dest, const T* src)
		{
			cpy(dest, src, sizeof(T));
		}

		template <class T>
		void set(T* dest, const int c)
		{
			set(dest, c, sizeof(T));
		}

		template <class T>
		void swap(T* s1, T* s2)
		{
			unsigned char temp[sizeof(T)];
			cpy(static_cast<T*>(temp), s1);
			cpy(s1, s2);
			cpy(s2, static_cast<T*>(temp));
		}

		template <class T>
		int cmp(const T* buf1, const T* buf2)
		{
			return cmp(buf1, buf2, sizeof(T));
		}

		template <class T>
		bool eql(const T* buf1, const T* buf2)
		{
			return cmp(buf1, buf2, sizeof(T)) == 0;
		}

		template <class T>
		void* zero(T* dest)
		{
			return set(dest, 0, sizeof(T));
		}
	}

	namespace str
	{
		inline int len(const char* src)
		{
			return strlen(src);
		}

		inline void cpy(char* dest, const char* src)
		{
			strcpy(dest, src);
		}

		inline void ncpy(char* dest, const char* src, const int destBufferLen)
		{
			strncpy(dest, src, destBufferLen);
		}

		inline void cat(char* dest, const char* src)
		{
			strcat(dest, src);
		}

		inline void ncat(char* dest, const char* src, const int destBufferLen)
		{
			ncpy(dest + len(dest), src, destBufferLen - len(dest));
		}

		inline int cmp(const char* s1, const char* s2)
		{
			return strcmp(s1, s2);
		}

		inline bool eql(const char* s1, const char* s2)
		{
			return strcmp(s1, s2) == 0;
		}

		inline int icmp(const char* s1, const char* s2)
		{
			return Q_stricmp(s1, s2);
		}

		inline int cmpi(const char* s1, const char* s2)
		{
			return Q_stricmp(s1, s2);
		}

		inline bool ieql(const char* s1, const char* s2)
		{
			return Q_stricmp(s1, s2) == 0;
		}

		inline bool eqli(const char* s1, const char* s2)
		{
			return Q_stricmp(s1, s2) == 0;
		}

		inline char* tok(char* s, const char* gap)
		{
			return strtok(s, gap);
		}

		void to_upper(char* dest);
		void to_lower(char* dest);
		void printf(char* dest, const char* formatS, ...);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// The Raven Template Library Compile Assert
	//
	// If, during compile time the stuff under (condition) is zero, this code will not
	// compile.
	////////////////////////////////////////////////////////////////////////////////////////
	template <int condition>
	class compile_assert
	{
#ifdef _DEBUG
		int junk[1 - 2 * !condition]; // Look At Where This Was Being Compiled
	public:
		compile_assert()
		{
			assert(condition);
			junk[0] = FoolTheOptimizer++;
		}

		int operator()()
		{
			assert(condition);
			FoolTheOptimizer++;
			return junk[0];
		}
#else
	public:
		int operator()()
		{
			return 1;
		}
#endif
	};

	////////////////////////////////////////////////////////////////////////////////////////
	// The Raven Template Library Base Class
	//
	// This is the base class for all the Raven Template Library container classes like
	// vector_vs and pool_vs.
	//
	// This class might be a good place to put memory profile code in the future.
	//
	////////////////////////////////////////////////////////////////////////////////////////
	class ratl_base
	{
	public:
		void save(hfile& file);
		void load(hfile& file);

		void ProfilePrint(const char* format, ...);

	public:
		static void* OutputPrint;
	};

	////////////////////////////////////////////////////////////////////////////////////////
	// this is a simplified version of bits_vs
	////////////////////////////////////////////////////////////////////////////////////////
	template <int SZ>
	class bits_base
	{
	protected:
		enum
		{
			BITS_SHIFT = 5,
			// 5.  Such A Nice Number
			BITS_INT_SIZE = 32,
			// Size Of A Single Word
			BITS_AND = BITS_INT_SIZE - 1,
			// Used For And Operation
			ARRAY_SIZE = (SZ + BITS_AND) / BITS_INT_SIZE,
			// Num Words Used
			BYTE_SIZE = ARRAY_SIZE * sizeof(unsigned int),
			// Num Bytes Used
		};

		////////////////////////////////////////////////////////////////////////////////////
		// Data
		////////////////////////////////////////////////////////////////////////////////////
		unsigned int mV[ARRAY_SIZE];

	public:
		enum
		{
			SIZE = SZ,
			CAPACITY = SZ,
		};

		bits_base(const bool init = true, const bool initValue = false)
		{
			if (init)
			{
				if (initValue)
				{
					set();
				}
				else
				{
					clear();
				}
			}
		}

		void clear()
		{
			mem::zero(&mV, BYTE_SIZE);
		}

		void set()
		{
			mem::set(&mV, 0xff, BYTE_SIZE);
		}

		void set_bit(const int i)
		{
			assert(i >= 0 && i < SIZE);
			mV[i >> BITS_SHIFT] |= 1 << (i & BITS_AND);
		}

		void clear_bit(const int i)
		{
			assert(i >= 0 && i < SIZE);
			mV[i >> BITS_SHIFT] &= ~(1 << (i & BITS_AND));
		}

		void mark_bit(const int i, const bool set)
		{
			assert(i >= 0 && i < SIZE);
			if (set)
			{
				mV[i >> BITS_SHIFT] |= 1 << (i & BITS_AND);
			}
			else
			{
				mV[i >> BITS_SHIFT] &= ~(1 << (i & BITS_AND));
			}
		}

		bool operator[](const int i) const
		{
			assert(i >= 0 && i < SIZE);
			return (mV[i >> BITS_SHIFT] & 1 << (i & BITS_AND)) != 0;
		}

		int next_bit(int start = 0, const bool onBit = true) const
		{
			assert(start >= 0 && start <= SIZE); //we have to accept start==size for the way the loops are done
			if (start >= SIZE)
			{
				return SIZE; // Did Not Find
			}
			// Get The Word Which Contains The Start Bit & Mask Out Everything Before The Start Bit
			//--------------------------------------------------------------------------------------
			unsigned int v = mV[start >> BITS_SHIFT];
			if (!onBit)
			{
				v = ~v;
			}
			v >>= start & 31;

			// Search For The First Non Zero Word In The Array
			//-------------------------------------------------
			while (!v)
			{
				start = (start & ~(BITS_INT_SIZE - 1)) + BITS_INT_SIZE;
				if (start >= SIZE)
				{
					return SIZE; // Did Not Find
				}
				v = mV[start >> BITS_SHIFT];
				if (!onBit)
				{
					v = ~v;
				}
			}

			// So, We've Found A Non Zero Word, So Start Masking Against Parts To Skip Over Bits
			//-----------------------------------------------------------------------------------
			if (!(v & 0xffff))
			{
				start += 16;
				v >>= 16;
			}
			if (!(v & 0xff))
			{
				start += 8;
				v >>= 8;
			}
			if (!(v & 0xf))
			{
				start += 4;
				v >>= 4;
			}

			// Time To Search Each Bit
			//-------------------------
			while (!(v & 1))
			{
				start++;
				v >>= 1;
			}
			if (start >= SIZE)
			{
				return SIZE;
			}
			return start;
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////
	// Raven Standard Compare Class
	////////////////////////////////////////////////////////////////////////////////////////
	struct ratl_compare
	{
		float mCost;
		int mHandle;

		bool operator<(const ratl_compare& t) const
		{
			return mCost < t.mCost;
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////
	// this is used to keep track of the constuction state for things that are always constucted
	////////////////////////////////////////////////////////////////////////////////////////
	class bits_true
	{
	public:
		static void clear()
		{
		}

		static void set()
		{
		}

		static void set_bit(const int i)
		{
		}

		static void clear_bit(const int i)
		{
		}

		bool operator[](const int i) const
		{
			return true;
		}

		static int next_bit(const int start = 0, const bool onBit = true)
		{
			assert(onBit); ///I didn't want to add the sz template arg, you could though
			return start;
		}
	};

	namespace storage
	{
		template <class T, int SIZE>
		struct value_semantics
		{
			enum
			{
				CAPACITY = SIZE,
			};

			using TAlign = T; // this is any type that has the right alignment
			using TValue = T; // this is the actual thing the user uses
			using TStorage = T; // this is what we make our array of

			using TConstructed = bits_true;
			using TArray = TStorage[SIZE];

			enum
			{
				NEEDS_CONSTRUCT = 0,
				TOTAL_SIZE = sizeof(TStorage),
				VALUE_SIZE = sizeof(TStorage),
			};

			static void construct(TStorage*)
			{
			}

			static void construct(TStorage* me, const TValue& v)
			{
				*me = v;
			}

			static void destruct(TStorage*)
			{
			}

			static TRatlNew* raw(TStorage* me)
			{
				return static_cast<TRatlNew*>(me);
			}

			static T* ptr(TStorage* me)
			{
				return me;
			}

			static const T* ptr(const TStorage* me)
			{
				return me;
			}

			static T& ref(TStorage* me)
			{
				return *me;
			}

			static const T& ref(const TStorage* me)
			{
				return *me;
			}

			static T* raw_array(TStorage* me)
			{
				return me;
			}

			static const T* raw_array(const TStorage* me)
			{
				return me;
			}

			static void swap(TStorage* s1, TStorage* s2)
			{
				mem::swap(ptr(s1), ptr(s2));
			}

			static int pointer_to_index(const void* s1, const void* s2)
			{
				return static_cast<TStorage*>(s1) - static_cast<TStorage*>(s2);
			}
		};

		template <class T, int SIZE>
		struct object_semantics
		{
			enum
			{
				CAPACITY = SIZE,
			};

			using TAlign = mem::alignStruct; // this is any type that has the right alignment
			using TValue = T; // this is the actual thing the user uses

			using TConstructed = bits_base<SIZE>;

			struct TStorage
			{
				TAlign mMemory[(sizeof(T) + sizeof(TAlign) - 1) / sizeof(TAlign)];
			};

			using TArray = TStorage[SIZE];

			enum
			{
				NEEDS_CONSTRUCT = 1,
				TOTAL_SIZE = sizeof(TStorage),
				VALUE_SIZE = sizeof(TStorage),
			};

			static void construct(TStorage* me)
			{
				new(raw(me)) TValue();
			}

			static void construct(TStorage* me, const TValue& v)
			{
				new(raw(me)) TValue(v);
			}

			static void destruct(TStorage* me)
			{
				ptr(me)->~T();
			}

			static TRatlNew* raw(TStorage* me)
			{
				return static_cast<TRatlNew*>(me);
			}

			static T* ptr(TStorage* me)
			{
				return static_cast<T*>(me);
			}

			static const T* ptr(const TStorage* me)
			{
				return static_cast<const T*>(me);
			}

			static T& ref(TStorage* me)
			{
				return *static_cast<T*>(me);
			}

			static const T& ref(const TStorage* me)
			{
				return *static_cast<const T*>(me);
			}

			static void swap(TStorage* s1, TStorage* s2)
			{
				TValue temp(ref(s1));
				ref(s1) = ref(s2);
				ref(s2) = temp;
			}

			static int pointer_to_index(const void* s1, const void* s2)
			{
				return static_cast<TStorage*>(s1) - static_cast<TStorage*>(s2);
			}
		};

		template <class T, int SIZE, int MAX_CLASS_SIZE>
		struct virtual_semantics
		{
			enum
			{
				CAPACITY = SIZE,
			};

			using TAlign = mem::alignStruct; // this is any type that has the right alignment
			using TValue = T; // this is the actual thing the user uses

			using TConstructed = bits_base<SIZE>;

			struct TStorage
			{
				TAlign mMemory[(MAX_CLASS_SIZE + sizeof(TAlign) - 1) / sizeof(TAlign)];
			};

			using TArray = TStorage[SIZE];

			enum
			{
				NEEDS_CONSTRUCT = 1,
				TOTAL_SIZE = sizeof(TStorage),
				VALUE_SIZE = MAX_CLASS_SIZE,
			};

			static void construct(TStorage* me)
			{
				new(raw(me)) TValue();
			}

			static void destruct(TStorage* me)
			{
				ptr(me)->~T();
			}

			static TRatlNew* raw(TStorage* me)
			{
				return static_cast<TRatlNew*>(me);
			}

			static T* ptr(TStorage* me)
			{
				return static_cast<T*>(me);
			}

			static const T* ptr(const TStorage* me)
			{
				return static_cast<const T*>(me);
			}

			static T& ref(TStorage* me)
			{
				return *static_cast<T*>(me);
			}

			static const T& ref(const TStorage* me)
			{
				return *static_cast<const T*>(me);
			}

			// this is a bit suspicious, we are forced to do a memory swap, and for a class, that, say
			// stores a pointer to itself, it won't work right
			static void swap(TStorage* s1, TStorage* s2)
			{
				mem::swap(s1, s2);
			}

			static int pointer_to_index(const void* s1, const void* s2)
			{
				return static_cast<TStorage*>(s1) - static_cast<TStorage*>(s2);
			}

			template <class CAST_TO>
			static CAST_TO* verify_alloc(CAST_TO* p)
			{
#ifdef _DEBUG
				assert(p);
				assert(dynamic_cast<const T*>(p));
				T* ptr = p; // if this doesn't compile, you are trying to alloc something that is not derived from base
				assert(dynamic_cast<const CAST_TO*>(ptr));
				compile_assert<sizeof(CAST_TO) <= MAX_CLASS_SIZE>();
				assert(sizeof(CAST_TO) <= MAX_CLASS_SIZE);
#endif
				return p;
			}
		};

		// The below versions are for nodes

		template <class T, int SIZE, class NODE>
		struct value_semantics_node
		{
			enum
			{
				CAPACITY = SIZE,
			};

			struct SNode
			{
				NODE nodeData;
				T value;
			};

			using TAlign = SNode; // this is any type that has the right alignment
			using TValue = T; // this is the actual thing the user uses
			using TStorage = SNode; // this is what we make our array of

			using TConstructed = bits_true;
			using TArray = TStorage[SIZE];

			enum
			{
				NEEDS_CONSTRUCT = 0,
				TOTAL_SIZE = sizeof(TStorage),
				VALUE_SIZE = sizeof(TValue),
			};

			static void construct(TStorage*)
			{
			}

			static void construct(TStorage* me, const TValue& v)
			{
				me->value = v;
			}

			static void destruct(TStorage*)
			{
			}

			static TRatlNew* raw(TStorage* me)
			{
				return static_cast<TRatlNew*>(&me->value);
			}

			static T* ptr(TStorage* me)
			{
				return &me->value;
			}

			static const T* ptr(const TStorage* me)
			{
				return &me->value;
			}

			static T& ref(TStorage* me)
			{
				return me->value;
			}

			static const T& ref(const TStorage* me)
			{
				return me->value;
			}

			// this ugly unsafe cast-hack is a backhanded way of getting the node data from the value data
			// this is so node support does not need to be added to the primitive containers
			static NODE& node(TValue& v)
			{
				return *static_cast<NODE*>(static_cast<unsigned char*>(&v) + static_cast<size_t>(&static_cast<TStorage*>
					(nullptr)->nodeData) - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->value));
			}

			static const NODE& node(const TValue& v)
			{
				return *static_cast<const NODE*>(static_cast<unsigned char*>(&v) + static_cast<size_t>(&static_cast<
					TStorage*>(nullptr)->nodeData) - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->value));
			}

			static void swap(TStorage* s1, TStorage* s2)
			{
				mem::swap(&s1->value, &s2->value);
			}

			// this is hideous
			static int pointer_to_index(const void* s1, const void* s2)
			{
				return
					static_cast<TStorage*>((unsigned char*)s1 - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->
						value)) -
					static_cast<TStorage*>((unsigned char*)s2 - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->
						value));
			}
		};

		template <class T, int SIZE, class NODE>
		struct object_semantics_node
		{
			enum
			{
				CAPACITY = SIZE,
			};

			using TAlign = mem::alignStruct; // this is any type that has the right alignment
			using TValue = T; // this is the actual thing the user uses

			using TConstructed = bits_base<SIZE>;

			struct TValueStorage
			{
				TAlign mMemory[(sizeof(T) + sizeof(TAlign) - 1) / sizeof(TAlign)];
			};

			struct SNode
			{
				NODE nodeData;
				TValueStorage value;
			};

			using TStorage = SNode; // this is what we make our array of
			using TArray = TStorage[SIZE];

			enum
			{
				NEEDS_CONSTRUCT = 0,
				TOTAL_SIZE = sizeof(TStorage),
				VALUE_SIZE = sizeof(TValueStorage),
			};

			static void construct(TStorage* me)
			{
				new(raw(me)) TValue();
			}

			static void construct(TStorage* me, const TValue& v)
			{
				new(raw(me)) TValue(v);
			}

			static void destruct(TStorage* me)
			{
				ptr(me)->~T();
			}

			static TRatlNew* raw(TStorage* me)
			{
				return static_cast<TRatlNew*>(&me->value);
			}

			static T* ptr(TStorage* me)
			{
				return static_cast<T*>(&me->value);
			}

			static const T* ptr(const TStorage* me)
			{
				return static_cast<const T*>(&me->value);
			}

			static T& ref(TStorage* me)
			{
				return *static_cast<T*>(&me->value);
			}

			static const T& ref(const TStorage* me)
			{
				return *static_cast<const T*>(&me->value);
			}

			static NODE& node(TStorage* me)
			{
				return me->nodeData;
			}

			static const NODE& node(const TStorage* me)
			{
				return me->nodeData;
			}

			// this ugly unsafe cast-hack is a backhanded way of getting the node data from the value data
			// this is so node support does not need to be added to the primitive containers
			static NODE& node(TValue& v)
			{
				return *static_cast<NODE*>(static_cast<unsigned char*>(&v) + static_cast<size_t>(&static_cast<TStorage*>
					(nullptr)->nodeData) - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->value));
			}

			static const NODE& node(const TValue& v)
			{
				return *static_cast<const NODE*>(static_cast<unsigned char*>(&v) + static_cast<size_t>(&static_cast<
					TStorage*>(nullptr)->nodeData) - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->value));
			}

			static void swap(TStorage* s1, TStorage* s2)
			{
				TValue temp(ref(s1));
				ref(s1) = ref(s2);
				ref(s2) = temp;
			}

			// this is hideous
			static int pointer_to_index(const void* s1, const void* s2)
			{
				return
					static_cast<TStorage*>((unsigned char*)s1 - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->
						value)) -
					static_cast<TStorage*>((unsigned char*)s2 - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->
						value));
			}
		};

		template <class T, int SIZE, int MAX_CLASS_SIZE, class NODE>
		struct virtual_semantics_node
		{
			enum
			{
				CAPACITY = SIZE,
			};

			using TAlign = mem::alignStruct; // this is any type that has the right alignment
			using TValue = T; // this is the actual thing the user uses

			using TConstructed = bits_base<SIZE>;

			struct TValueStorage
			{
				TAlign mMemory[(MAX_CLASS_SIZE + sizeof(TAlign) - 1) / sizeof(TAlign)];
			};

			struct SNode
			{
				NODE nodeData;
				TValueStorage value;
			};

			using TStorage = SNode; // this is what we make our array of
			using TArray = TStorage[SIZE];

			enum
			{
				NEEDS_CONSTRUCT = 1,
				TOTAL_SIZE = sizeof(TStorage),
				VALUE_SIZE = sizeof(TValueStorage),
			};

			static void construct(TStorage* me)
			{
				new(raw(me)) TValue();
			}

			static void destruct(TStorage* me)
			{
				ptr(me)->~T();
			}

			static TRatlNew* raw(TStorage* me)
			{
				return static_cast<TRatlNew*>(&me->value);
			}

			static T* ptr(TStorage* me)
			{
				return static_cast<T*>(&me->value);
			}

			static const T* ptr(const TStorage* me)
			{
				return static_cast<const T*>(&me->value);
			}

			static T& ref(TStorage* me)
			{
				return *static_cast<T*>(&me->value);
			}

			static const T& ref(const TStorage* me)
			{
				return *static_cast<const T*>(&me->value);
			}

			static NODE& node(TStorage* me)
			{
				return me->nodeData;
			}

			static const NODE& node(const TStorage* me)
			{
				return me->nodeData;
			}

			// this ugly unsafe cast-hack is a backhanded way of getting the node data from the value data
			// this is so node support does not need to be added to the primitive containers
			static NODE& node(TValue& v)
			{
				return *static_cast<NODE*>(static_cast<unsigned char*>(&v) + static_cast<size_t>(&static_cast<TStorage*>
					(nullptr)->nodeData) - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->value));
			}

			static const NODE& node(const TValue& v)
			{
				return *static_cast<const NODE*>(static_cast<unsigned char*>(&v) + static_cast<size_t>(&static_cast<
					TStorage*>(nullptr)->nodeData) - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->value));
			}

			// this is a bit suspicious, we are forced to do a memory swap, and for a class, that, say
			// stores a pointer to itself, it won't work right
			static void swap(TStorage* s1, TStorage* s2)
			{
				mem::swap(&s1->value, &s2->value);
			}

			// this is hideous
			static int pointer_to_index(const void* s1, const void* s2)
			{
				return
					static_cast<TStorage*>((unsigned char*)s1 - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->
						value)) -
					static_cast<TStorage*>((unsigned char*)s2 - static_cast<size_t>(&static_cast<TStorage*>(nullptr)->
						value));
			}

			template <class CAST_TO>
			static CAST_TO* verify_alloc(CAST_TO* p)
			{
#ifdef _DEBUG
				assert(p);
				assert(dynamic_cast<const T*>(p));
				T* ptr = p; // if this doesn't compile, you are trying to alloc something that is not derived from base
				assert(dynamic_cast<const CAST_TO*>(ptr));
				compile_assert<sizeof(CAST_TO) <= MAX_CLASS_SIZE>();
				assert(sizeof(CAST_TO) <= MAX_CLASS_SIZE);
#endif
				return p;
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// The Array Base Class, used for most containers
	////////////////////////////////////////////////////////////////////////////////////////
	template <class T>
	class array_base : public ratl_base
	{
	public:
		////////////////////////////////////////////////////////////////////////////////////
		// Capacity Enum
		////////////////////////////////////////////////////////////////////////////////////
		enum
		{
			CAPACITY = T::CAPACITY,
			SIZE = T::CAPACITY,
		};

		////////////////////////////////////////////////////////////////////////////////////
		// Data
		////////////////////////////////////////////////////////////////////////////////////
		using TStorageTraits = T;
		using TTArray = typename T::TArray;
		using TTValue = typename T::TValue;
		using TTConstructed = typename T::TConstructed;

	private:
		TTArray mArray;
		TTConstructed mConstructed;

	public:
		array_base()
		{
		}

		~array_base()
		{
			clear();
		}

		void clear()
		{
			if (T::NEEDS_CONSTRUCT)
			{
				int i = mConstructed.next_bit();
				while (i < SIZE)
				{
					T::destruct(mArray + i);
					i = mConstructed.next_bit(i + 1);
				}
				mConstructed.clear();
			}
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Access Operator
		////////////////////////////////////////////////////////////////////////////////////
		TTValue& operator[](int index)
		{
			assert(index >= 0 && index < SIZE);
			assert(mConstructed[index]);
			return T::ref(mArray + index);
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Const Access Operator
		////////////////////////////////////////////////////////////////////////////////////
		const TTValue& operator[](int index) const
		{
			assert(index >= 0 && index < SIZE);
			assert(mConstructed[index]);
			return T::ref(mArray + index);
		}

		void construct(int i)
		{
			if (T::NEEDS_CONSTRUCT)
			{
				assert(!mConstructed[i]);
				T::construct(mArray + i);
				mConstructed.set_bit(i);
			}
		}

		void construct(int i, const TTValue& v)
		{
			assert(i >= 0 && i < SIZE);
			T::construct(mArray + i, v);
			if (T::NEEDS_CONSTRUCT)
			{
				assert(!mConstructed[i]);
				mConstructed.set_bit(i);
			}
		}

		void fill(const TTValue& v)
		{
			clear();
			for (int i = 0; i < SIZE; i++)
			{
				T::construct(mArray + i, v);
			}
			if (T::NEEDS_CONSTRUCT)
			{
				mConstructed.set();
			}
		}

		void swap(int i, int j)
		{
			assert(i >= 0 && i < SIZE);
			assert(j >= 0 && j < SIZE);
			assert(i != j);
			assert(mConstructed[i]);
			assert(mConstructed[j]);
			T::swap(mArray + i, mArray + j);
		}

		TRatlNew* alloc_raw(int i)
		{
			assert(i >= 0 && i < SIZE);
			if (T::NEEDS_CONSTRUCT)
			{
				assert(!mConstructed[i]);
				mConstructed.set_bit(i);
			}
			return T::raw(mArray + i);
		}

		void destruct(int i)
		{
			assert(i >= 0 && i < SIZE);
			assert(mConstructed[i]);
			if (T::NEEDS_CONSTRUCT)
			{
				T::destruct(mArray + i);
				mConstructed.clear_bit(i);
			}
		}

		int pointer_to_index(const TTValue* me) const
		{
			const int index = T::pointer_to_index(me, mArray);
			assert(index >= 0 && index < SIZE);
			assert(mConstructed[index]);
			return index;
		}

		int pointer_to_index(const TRatlNew* me) const
		{
			const int index = T::pointer_to_index(me, mArray);
			assert(index >= 0 && index < SIZE);
			assert(mConstructed[index]);
			return index;
		}

		typename T::TValue* raw_array()
		{
			return T::raw_array(mArray);
		}

		const typename T::TValue* raw_array() const
		{
			return T::raw_array(mArray);
		}

		template <class CAST_TO>
		CAST_TO* verify_alloc(CAST_TO* p) const
		{
			return T::verify_alloc(p);
		}
	};
}
