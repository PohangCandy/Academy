#pragma once
#pragma once

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName) ProfileName(TagName)
#elseif
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#include<windows.h>

#define PROFILE_NUM 10
#define PROFILE_SAMPLE_Name 64
#define PROFILE_SAMPLE_MIN 2
#define PROFILE_SAMPLE_MAX 2

//--------------------------------------------------------
//클래스로 설계해서 생성자와 소멸자를 이용한 버전
//--------------------------------------------------------
class Profile
{
	const char* _tag;
public:
	Profile(const char* tag)
	{
		//Begin(tag);
		_tag = tag;
	}
	~Profile()
	{
		//End(_tag);
	}
};