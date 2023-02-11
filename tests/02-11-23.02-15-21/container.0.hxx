#pragma once

#include "tmp/type_list.hxx"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <mutex>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeinfo>

namespace sdi {
  namespace detail {
    struct abstract_destructor {
      virtual ~abstract_destructor() noexcept = default;

      virtual void destroy() = 0;

    protected:
      std::uint8_t* p;

      explicit abstract_destructor(std::uint8_t* p)
          :p{p}
      {
      }
    };

    template<typename T>
    struct destructor final : abstract_destructor {
      explicit destructor(std::uint8_t* p)
          :abstract_destructor{p}
      {
      }

      void destroy() override
      {
        reinterpret_cast<T*>(p)->~T();
      }
    };

    static_assert(sizeof(destructor<int>) == sizeof(abstract_destructor), "destructors should have same size");

    template<typename... TypeLists>
    struct merge;

    template<typename... TypeLists>
    using merge_t = typename merge<TypeLists...>::type;

    template<>
    struct merge<> final {
      using type = tmp::type_list<>;
    };

    template<typename Head, typename... Tail>
    struct merge<Head, Tail...> final {
      using type = tmp::make_unique_t<decltype(Head{} + merge_t<Tail...>{})>;
    };

    template<typename Type, typename TypeList>
    struct index_of;

    template<typename Type, typename TypeList>
    static std::size_t constexpr index_of_v = index_of<Type, TypeList>::value;

    template<typename Type, typename... TypeList>
    struct index_of<Type, tmp::type_list<TypeList...>> final {
      static std::size_t constexpr value = tmp::generic::find_type_v<Type, TypeList...>;
    };

    template<typename Type, typename TypeList>
    struct index_of_derived;

    template<typename Type, typename TypeList>
    static std::size_t constexpr index_of_derived_v = index_of_derived<Type, TypeList>::value;

    template<typename Type, typename... TypeList>
    struct index_of_derived<Type, tmp::type_list<TypeList...>> final {
      static std::size_t constexpr value = tmp::generic::find_derived_type_v<Type, TypeList...>;
    };

    template<std::size_t Index, typename TypeList>
    struct nth_type;

    template<std::size_t Index, typename TypeList>
    using nth_type_t = typename nth_type<Index, TypeList>::type;

    template<std::size_t Index, typename... Types>
    struct nth_type<Index, tmp::type_list<Types...>> final {
      using type = tmp::nth_type_t<Index, Types...>;
    };

    template<typename TypeList>
    struct byte_size;

    template<typename TypeList>
    static std::size_t constexpr byte_size_v = byte_size<TypeList>::value;

    template<>
    struct byte_size<tmp::type_list<>> final {
      static std::size_t constexpr value = 0U;
    };

    template<typename Head, typename... Tail>
    struct byte_size<tmp::type_list<Head, Tail...>> final {
    private:
      static constexpr std::size_t byte_size_impl() noexcept
      {
        std::size_t sum{0U};
        std::size_t prev{0U};
        for (std::tuple<std::size_t, std::size_t> const info: {std::make_tuple(sizeof(Head), alignof(Head)),
                                                               std::make_tuple(sizeof(Tail), alignof(Tail))...}) {
          auto const size = std::get<0U>(info);
          auto const alignment = std::get<1U>(info);
          if (prev % alignment != 0U) {
            sum += alignment - (prev % alignment);
          }
          sum += size;
          prev = size;
        }
        return sum;
      }

    public:
      static std::size_t constexpr value = tmp::generic::force_constexpr<std::size_t, byte_size_impl()>;
    };

    template<typename Type, typename TypeList>
    struct offset_of;

    template<typename Type, typename TypeList>
    static std::size_t constexpr offset_of_v = offset_of<Type, TypeList>::value;

    template<typename Type, typename... TypeList>
    struct offset_of<Type, tmp::type_list<TypeList...>> final {
      static_assert(tmp::generic::contains_type_v<Type, TypeList...>, "Type must be in the TypeList");

    private:
      static constexpr std::size_t offset_of_impl() noexcept
      {
        std::size_t const idx = index_of_v<Type, tmp::type_list<TypeList...>>;
        std::size_t current_idx{0U};
        std::size_t sum{0U};
        std::size_t prev{0U};
        for (std::tuple<std::size_t, std::size_t> const info: {
            std::make_tuple(sizeof(TypeList), alignof(TypeList))...
        }) {
          auto const size = std::get<0U>(info);
          auto const alignment = std::get<1U>(info);
          if (prev % alignment != 0U) {
            sum += alignment - (prev % alignment);
          }
          if (current_idx == idx) {
            break;
          }
          sum += size;
          prev = size;
          ++current_idx;
        }
        return sum;
      }

    public:
      static std::size_t constexpr value = tmp::generic::force_constexpr<std::size_t, offset_of_impl()>;
    };

    template<typename Type>
    struct dependencies_of final {
    private:
      static tmp::type_list<> check(...) noexcept;

      template<typename T = Type>
      static auto check(int, typename T::dependencies = {}) noexcept -> typename T::dependencies;

    public:
      using type = decltype(check(42));
      static_assert(tmp::is_type_list_v<type>, "Dependencies must be a type_list");
    };

    template<typename Type>
    using dependencies_of_t = typename dependencies_of<Type>::type;
  }

  class resolution_error final : public std::runtime_error {
  public:
    using runtime_error::runtime_error;
  };

  template<typename... Types>
  using known_types = tmp::type_list<Types...>;

  template<typename... Containers>
  using installed_containers = tmp::type_list<Containers...>;

  template<typename KnownTypes, typename InstalledContainers = installed_containers<>>
  class container;

  template<typename KnownTypes, typename... InstalledContainers>
  class container<KnownTypes, installed_containers<InstalledContainers...>> final {
  public:
    using types = detail::merge_t<KnownTypes, typename InstalledContainers::types...>;
    static std::size_t constexpr memory_size = detail::byte_size_v<types>;

  private:
    std::bitset<types::size> initialized;
    std::array<std::uint8_t, memory_size> memory;
    std::array<std::uint8_t, sizeof(detail::abstract_destructor) * types::size> destructors;
    std::size_t last_destructor{0U};

    template<typename Type, typename... Dependencies>
    void construct(tmp::type_list<Dependencies...>)
    {
      emplace<Type>(resolve<Dependencies>()...);
    }

    template<typename Type, std::size_t Index>
    struct resolver final {
      static Type& resolve(container& this_)
      {
        using actual = detail::nth_type_t<Index, types>;
        if (!this_.initialized.test(Index)) {
          this_.construct<actual>(detail::dependencies_of_t<actual>{});
        }
        return *reinterpret_cast<actual*>(this_.memory.data() + detail::offset_of_v<actual, types>);
      }
    };

    template<typename Type>
    struct resolver<Type, sdi::tmp::generic::NOT_FOUND> final {
      static Type& resolve(container&)
      {
        using std::string_literals::operator ""s;
        throw resolution_error(typeid(Type).name() + " could not be resolved."s);
      }
    };

    static std::mutex& mtx() noexcept
    {
      static std::mutex mutex;
      return mutex;
    }

  public:
    constexpr container() noexcept = default;

    container(container const&) = delete;

    container& operator=(container const&) = delete;

    ~container()
    {
      while (last_destructor--) {
        reinterpret_cast<detail::abstract_destructor*>(destructors.data()
            + last_destructor * sizeof(detail::abstract_destructor))->destroy();
      }
    }

    static container& instance() noexcept
    {
      static container c;
      return c;
    }

    template<typename Callable>
    static auto use_threadsafe(Callable&& callable)
    -> decltype(callable(std::declval<container&>()))
    {
      std::lock_guard<std::mutex> lock{mtx()};
      return callable(instance());
    }

    template<typename Type, typename... Args>
    auto emplace(Args&& ... args)
    -> std::enable_if_t<std::is_constructible<Type, Args...>::value, container&>
    {
      static_assert(detail::index_of_v<Type, types> != tmp::generic::NOT_FOUND, "Type must exist in types");
      auto const addr = memory.data() + detail::offset_of_v<Type, types>;
      new(addr) Type{std::forward<Args>(args)...};
      new(destructors.data() + last_destructor++ * sizeof(detail::abstract_destructor)) detail::destructor<Type>{addr};
      initialized.set(detail::index_of_v<Type, types>);
      return *this;
    }

    template<typename Type, typename... Args>
    auto emplace(Args&& ...)
    -> std::enable_if_t<!std::is_constructible<Type, Args...>::value, container&>
    {
      throw resolution_error(typeid(Type).name() + std::string{" could not be automatically constructed."});
    }

    template<typename Type>
    Type& resolve()
    {
      auto constexpr idx = detail::index_of_derived_v<Type, types>;
      return resolver<Type, idx>::resolve(*this);
    }
  };
}
