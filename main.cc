
#include <concepts>
#include <iostream>
#include <tuple>
#include <memory>

#include <cstddef>

#include <wayland-client.h>

template <class> constexpr std::nullptr_t wl_interface_ptr = nullptr;
#define INTERN_WL_CLIENT(wl_client)                                     \
    template<> constexpr wl_interface const* wl_interface_ptr<wl_client> = &wl_client##_interface;
INTERN_WL_CLIENT(wl_display);
INTERN_WL_CLIENT(wl_registry);     
INTERN_WL_CLIENT(wl_compositor);   
INTERN_WL_CLIENT(wl_shell);        
INTERN_WL_CLIENT(wl_seat);         
INTERN_WL_CLIENT(wl_keyboard);     
INTERN_WL_CLIENT(wl_pointer);      
INTERN_WL_CLIENT(wl_touch);        
INTERN_WL_CLIENT(wl_shm);          
INTERN_WL_CLIENT(wl_surface);      
INTERN_WL_CLIENT(wl_shell_surface);
INTERN_WL_CLIENT(wl_buffer);       
INTERN_WL_CLIENT(wl_shm_pool);     
INTERN_WL_CLIENT(wl_callback);     
INTERN_WL_CLIENT(wl_output);       

template <class T>
concept wl_client_t = std::same_as<decltype (wl_interface_ptr<T>), wl_interface const *const>;

template <wl_client_t T>
[[nodiscard]] auto attach_unique(T* ptr) noexcept {
    static constexpr auto deleter = [](T* ptr) noexcept -> void {
        static constexpr auto interface_addr = wl_interface_ptr<T>;
        if      constexpr (interface_addr == std::addressof(wl_display_interface)) {
            wl_display_disconnect(ptr);
        }
        else if constexpr (interface_addr == std::addressof(wl_keyboard_interface)) {
            wl_keyboard_release(ptr);
        }
        else if constexpr (interface_addr == std::addressof(wl_pointer_interface)) {
            wl_pointer_release(ptr);
        }
        else if constexpr (interface_addr == std::addressof(wl_touch_interface)) {
            wl_touch_release(ptr);
        }
        else {
            wl_proxy_destroy(reinterpret_cast<wl_proxy*>(ptr));
        }
    };
    return std::unique_ptr<T, decltype (deleter)>(ptr, deleter);
}

template <wl_client_t T, class Ch>
decltype (auto) operator << (std::basic_ostream<Ch>& output, T const* ptr) noexcept {
    return output << static_cast<void const*>(ptr)
                  << '['
                  << wl_interface_ptr<T>->name
                  << ']';
}

template <wl_client_t T>
using unique_ptr_t = decltype (attach_unique(std::declval<T*>()));

inline namespace aux
{
template<std::size_t...> struct seq{};
template<std::size_t N, std::size_t... Is> struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<std::size_t... Is> struct gen_seq<0, Is...> : seq<Is...>{};
template<class Ch, class Tuple, std::size_t... Is>
void print(std::basic_ostream<Ch>& output, Tuple const& t, seq<Is...>) noexcept {
    using swallow = int[];
    (void)swallow{0, (void(output << (Is == 0? "" : ", ") << std::get<Is>(t)), 0)...};
}
template<class Ch, class... Args>
decltype (auto) operator<<(std::basic_ostream<Ch>& output, std::tuple<Args...> const& t) noexcept {
    output << '(';
    print(output, t, gen_seq<sizeof...(Args)>());
    output << ')';
    return output;
}
} // ::aux

int main() {
    auto display = attach_unique(wl_display_connect(nullptr));
    auto registry = attach_unique(wl_display_get_registry(display.get()));
    auto reactor = [&](size_t offset, auto... args) noexcept {
        std::cout << std::tuple(offset, args...) << std::endl;
    };
    wl_registry_listener registry_listener {
        .global = [](void* data, auto... args) noexcept {
            (*reinterpret_cast<decltype (reactor)*>(data))(
                offsetof (wl_registry_listener, global),
                args...);
        },
        .global_remove = [](void* data, auto... args) noexcept {
            (*reinterpret_cast<decltype (reactor)*>(data))(
                offsetof (wl_registry_listener, global_remove),
                args...);
        },
    };
    wl_registry_add_listener(registry.get(), &registry_listener, &reactor);
    wl_display_roundtrip(display.get());
    return 0;
}
