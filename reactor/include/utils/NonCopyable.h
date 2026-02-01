#pragma once
class NonCopyable
{
public:
    NonCopyable() = default;
protected:
    NonCopyable(NonCopyable const&) = delete;
    NonCopyable& operator=(NonCopyable const&) = delete;
    NonCopyable(NonCopyable const&&) = delete;
    NonCopyable& operator=(NonCopyable const&&) = delete;
};