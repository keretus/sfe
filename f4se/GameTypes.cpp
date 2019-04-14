#include "f4se/GameTypes.h"

const char * BSString::Get(void)
{
	return m_data ? m_data : "";
}

StringCache::Ref::Ref()
{
	CALL_MEMBER_FN(this, ctor)("");
}

StringCache::Ref::Ref(const char * buf)
{
	CALL_MEMBER_FN(this, ctor)(buf);
}

StringCache::Ref::Ref(const wchar_t * buf)
{
	CALL_MEMBER_FN(this, ctor_w)(buf);
}

void StringCache::Ref::Release()
{
	CALL_MEMBER_FN(this, Release)();
}

bool StringCache::Ref::operator==(const char * lhs) const
{
	Ref tmp(lhs);
	bool res = data == tmp.data;
	CALL_MEMBER_FN(&tmp, Release)();
	return res;
}

void SimpleLock::Lock(UInt32 pauseAttempts)
{
	SInt32 myThreadID = GetCurrentThreadId();

	_mm_lfence();
	if (threadID == myThreadID)
	{
		InterlockedIncrement(&lockCount);
	}
	else
	{
		UInt32 attempts = 0;
		if (InterlockedCompareExchange(&lockCount, 1, 0))
		{
			do
			{
				++attempts;
				_mm_pause();
				if (attempts >= pauseAttempts) {
					UInt32 spinCount = 0;
					while (InterlockedCompareExchange(&lockCount, 1, 0))
						Sleep(++spinCount < kFastSpinThreshold ? 1 : 0);
					break;
				}
			} while (InterlockedCompareExchange(&lockCount, 1, 0));
			_mm_lfence();
		}

		threadID = myThreadID;
		_mm_sfence();
	}
}

void SimpleLock::Release(void)
{
	SInt32 myThreadID = GetCurrentThreadId();

	_mm_lfence();
	if (threadID == myThreadID)
	{
		if (lockCount == 1)
		{
			threadID = 0;
			_mm_mfence();
			InterlockedCompareExchange(&lockCount, 0, 1);
		}
		else
		{
			InterlockedDecrement(&lockCount);
		}
	}
}

/*
// This implementation isn't quite right when compared to the came code, something is off
void BSReadWriteLock::LockForRead()
{
	SInt32 myThreadID = GetCurrentThreadId();

	if (threadID == myThreadID)
	{
		InterlockedIncrement(&lockValue);
	}
	else
	{
		UInt32 lockCount = lockValue & kLockCountMask;
		UInt32 spinCount = 0;
		UInt32 lockResult = InterlockedCompareExchange(&lockValue, lockCount + 1, lockCount);
		while (lockResult != lockCount + 1)
		{
			if ((lockResult & kLockWrite) != 0)
			{
				Sleep(++spinCount < kFastSpinThreshold ? 0 : 1);
				lockResult = lockValue;
			}

			lockCount = lockValue & kLockCountMask;
			lockResult = InterlockedCompareExchange(&lockValue, lockCount + 1, lockCount);
		}

		threadID = myThreadID;
	}
}
*/

/*
void BSReadWriteLock::LockForWrite()
{
	SInt32 myThreadID = GetCurrentThreadId();

	if (threadID == myThreadID)
	{
		InterlockedIncrement(&lockValue);
	}
	else
	{
		UInt32 spinCount = 0;
		while (InterlockedCompareExchange(&lockValue, UInt32(1 | kLockWrite), 0) != UInt32(1 | kLockWrite))
			Sleep(++spinCount < kFastSpinThreshold ? 0 : 1);

		threadID = myThreadID;
		_mm_mfence();
	}
}
*/

void BSReadWriteLock::LockForReadAndWrite()
{
	SInt32 myThreadID = GetCurrentThreadId();

	if (threadID == myThreadID)
	{
		InterlockedIncrement(&lockValue);
	}
	else
	{
		UInt32 spinCount = 0;
		while (InterlockedCompareExchange(&lockValue, 1, 0) != 1)
			Sleep(++spinCount >= kFastSpinThreshold ? 1 : 0);

		threadID = myThreadID;
		_mm_mfence();
	}
}

bool BSReadWriteLock::TryLockForWrite()
{
	SInt32 myThreadID = GetCurrentThreadId();

	bool result = false;
	if (threadID == myThreadID)
	{
		InterlockedIncrement(&lockValue);
		result = true;
	}
	else
	{
		result = InterlockedCompareExchange(&lockValue, UInt32(1 | kLockWrite), 0) == UInt32(1 | kLockWrite);
		if (result)
		{
			threadID = myThreadID;
			_mm_mfence();
		}
	}
	return result;
}
bool BSReadWriteLock::TryLockForRead()
{
	SInt32 myThreadID = GetCurrentThreadId();

	bool result = false;
	if (threadID == myThreadID)
	{
		InterlockedIncrement(&lockValue);
		result = true;
	}
	else
	{
		UInt32 lockCount = lockValue & kLockCountMask;
		UInt32 lockResult = InterlockedCompareExchange(&lockValue, lockCount + 1, lockCount);
		while ((lockResult & kLockWrite) == 0)
		{
			if (lockResult == lockCount)
				break;

			lockCount = lockResult & kLockCountMask;
			lockResult = InterlockedCompareExchange(&lockValue, lockCount + 1, lockCount);
		}

		result = ~(lockResult >> 31) & 1;
	}

	return result;
}

void BSReadWriteLock::Unlock()
{
	SInt32 myThreadID = GetCurrentThreadId();
	if (threadID == myThreadID)
	{
		UInt32 lockCount = lockValue - 1;
		if (lockValue == kLockWrite)
		{
			threadID = 0;
			_mm_mfence();
			InterlockedExchange(&lockValue, 0);
		}
		else
		{
			InterlockedDecrement(&lockValue);
		}
	}
	else
	{
		InterlockedDecrement(&lockValue);
	}
}