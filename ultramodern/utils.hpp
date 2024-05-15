#include <variant>

template<class... Ts> 
struct overloaded : Ts... 
{ 
    using Ts::operator()...; 
};

template<class T, class... Ts>
auto match(const T& event, Ts&&... args){
    return std::visit(overloaded{std::forward<Ts>(args)...}, event);
}