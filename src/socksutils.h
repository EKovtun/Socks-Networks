#pragma once
#include "socksproto.h"
#include <QtGlobal>
#include <string>

namespace SOCKS{
	namespace detail{
		class NeedDataException{
			qint64 mSize;
		public:
			NeedDataException(qint64 size = 1): mSize(size){} // По умолчанию требуется один байт
			qint64 size() {return mSize;}
		};

		class BuffReader{
			const char * begin;
			const char * end;
			mutable const char * current;
		public: 
			BuffReader(const char * begin, const char * end): begin(begin), end(end), current(begin) {
				if(begin > end){
					throw NeedDataException();
				}
			}

			template <typename T> 
            T* get () {
				qint64 need = (qint64)sizeof(T) - (end - current);
				if(need > 0){
					throw NeedDataException(need);
				}
				auto ret = (T*)(current);
				current += sizeof(T);
                return ret;
			}

			// То же что и Get только не сдвигает указатель
			template <typename T>
            T* peek(const T* dummy = nullptr) {
				qint64 need = (qint64)sizeof(T) - (end - current);
				if(need > 0){
					throw NeedDataException(need);
				}
				auto ret = (T*)(current);
                return ret;
			}

			inline std::string getString(){
				// Сдивгаемся от current пока не встретим 0 или не упремся в end
				auto p = current;
				for(; *p && (p < end); ++p);
				if(p == end){
					throw NeedDataException(1);
				}
				std::string ret(current);
				current = ++p;
				return ret;
			}


		};

        template <>
        inline SOCKS::DNSName* BuffReader::get<SOCKS::DNSName> () {
			qint64 need = (qint64)1 - (end - current);
            if(need > 0) throw NeedDataException(need);

            auto ret = (SOCKS::DNSName*)(current);
            ++current;

			need += ret->length;
            if(need > 0) throw NeedDataException(need);

            current += ret->length;
            return ret;
        }

        template <>
        inline SOCKS::SOCKS5NegotiationRequest* BuffReader::get<SOCKS5NegotiationRequest> () {
			qint64 need = (qint64)sizeof(SOCKS5NegotiationRequest) - (end - current);
			if(need > 0) throw NeedDataException(need);
			
			auto p = (SOCKS5NegotiationRequest*)current;
			current += sizeof(SOCKS5NegotiationRequest);
			
			need += p->nMethods;

			if(need > 0) throw NeedDataException(need);
			current += p->nMethods;

			return p;
		}


/*
		const unsigned int MAX_SZ = 600;
		class NetBuff{
			typedef unsigned char elem_type;
			elem_type buff[MAX_SZ];
			const elem_type * begin;
			const elem_type * end;
			elem_type * curr;
		public:
			NetBuff();
			char * data();
			qint64 size();
			quint64 freeBytes();
			void seek(qint64);
			void clear();

			// То же что и Get только не сдвигает указатель
			template <typename T>
            T* peek(const T* dummy = nullptr) {
				if(sizeof(T) > end - curr){
					throw NeedDataException();
				}
				//auto ret = (T*)(wPos);
                return (T*)(curr);
			}

			template <typename T> 
            T* get () {
				if(sizeof(T) > end - curr){
					throw NeedDataException();
				}
				auto ret = (T*)(curr);
				curr += sizeof(T);
                return ret;
			}

		};*/
	} //namespace detail
}//namespace SOCKS
