#ifndef __OVERLOADED_H__
#define __OVERLOADED_H__

// Helper for std::visit
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#endif
