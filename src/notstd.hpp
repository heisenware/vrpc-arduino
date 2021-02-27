#ifndef NOTSTD_HPP
#define NOTSTD_HPP

/** Much of the code below is taken from or inspired by:
 * https://stackoverflow.com/questions/32074410/stdfunction-bind-like-type-erasure-without-standard-c-library
 *
**/

  template<class T>struct tag{using type=T;};
  template<class Tag>using type_t=typename Tag::type;
  using size_t=decltype(sizeof(int));

namespace notstd {

  // move

  template<class T>
  T&& move(T&t){return static_cast<T&&>(t);}

  // forward

  template<class T>
  struct remove_reference:tag<T>{};
  template<class T>
  struct remove_reference<T&>:tag<T>{};
  template<class T>using remove_reference_t=type_t<remove_reference<T>>;

  template<class T>
  T&& forward( remove_reference_t<T>& t ) {
    return static_cast<T&&>(t);
  }
  template<class T>
  T&& forward( remove_reference_t<T>&& t ) {
    return static_cast<T&&>(t);
  }

  // decay

  template<class T>
  struct remove_const:tag<T>{};
  template<class T>
  struct remove_const<T const>:tag<T>{};

  template<class T>
  struct remove_volatile:tag<T>{};
  template<class T>
  struct remove_volatile<T volatile>:tag<T>{};

  template<class T>
  struct remove_cv:remove_const<type_t<remove_volatile<T>>>{};

  template<class T>
  struct decay3:remove_cv<T>{};
  template<class R, class...Args>
  struct decay3<R(Args...)>:tag<R(*)(Args...)>{};
  template<class T>
  struct decay2:decay3<T>{};
  template<class T, size_t N>
  struct decay2<T[N]>:tag<T*>{};

  template<class T>
  struct decay:decay2<remove_reference_t<T>>{};

  template<class T>
  using decay_t=type_t<decay<T>>;


  // is_convertible

  template<class T>
  T declval(); // no implementation

  template<class T, T t>
  struct integral_constant{
    static constexpr T value=t;
    constexpr integral_constant() {};
    constexpr operator T()const{ return value; }
    constexpr T operator()()const{ return value; }
  };
  template<bool b>
  using bool_t=integral_constant<bool, b>;
  using true_type=bool_t<true>;
  using false_type=bool_t<false>;

  template<class...>struct voider:tag<void>{};
  template<class...Ts>using void_t=type_t<voider<Ts...>>;

  namespace details {
    template<template<class...>class Z, class, class...Ts>
    struct can_apply:false_type{};
    template<template<class...>class Z, class...Ts>
    struct can_apply<Z, void_t<Z<Ts...>>, Ts...>:true_type{};
  }
  template<template<class...>class Z, class...Ts>
  using can_apply = details::can_apply<Z, void, Ts...>;

  namespace details {
    template<class From, class To>
    using try_convert = decltype( To{declval<From>()} );
  }
  template<class From, class To>
  struct is_convertible : can_apply< details::try_convert, From, To > {};
  template<>
  struct is_convertible<void,void>:true_type{};

  // enable_if

  template<bool, class=void>
  struct enable_if {};
  template<class T>
  struct enable_if<true, T>:tag<T>{};
  template<bool b, class T=void>
  using enable_if_t=type_t<enable_if<b,T>>;



  // result_of

  // namespace details {
  //   template<class F, class...Args>
  //   using invoke_t = decltype( declval<F>()(declval<Args>()...) );

  //   template<class Sig,class=void>
  //   struct result_of {};
  //   template<class F, class...Args>
  //   struct result_of<F(Args...), void_t< invoke_t<F, Args...> > >:
  //     tag< invoke_t<F, Args...> >
  //   {};
  // }
  // template<class Sig>
  // using result_of = details::result_of<Sig>;
  // template<class Sig>
  // using result_of_t=type_t<result_of<Sig>>;

  // aligned_storage

  template<size_t size, size_t align>
  struct alignas(align) aligned_storage_t {
    char buff[size];
  };

  // is_same

  template<class A, class B>
  struct is_same:false_type{};
  template<class A>
  struct is_same<A,A>:true_type{};

  // tuple

  template<size_t i, typename Item>
  struct tuple_leaf {
      Item value;
  };

  template<size_t i, typename... Items>
  struct tuple_impl;

  template<size_t i>
  struct tuple_impl<i>{};

  template<size_t i, typename HeadItem, typename... TailItems>
  struct tuple_impl<i, HeadItem, TailItems...> :
      public tuple_leaf<i, HeadItem>,
      public tuple_impl<i + 1, TailItems...>
      {};

  template<size_t i, typename HeadItem, typename... TailItems>
  HeadItem& get(tuple_impl<i, HeadItem, TailItems...>& tuple) {
      // Fully qualified name for the member, to find the right one
      // (they are all called `value`).
      return tuple.tuple_leaf<i, HeadItem>::value;
  }

  template<typename... Items>
  using tuple = tuple_impl<0, Items...>;

   /// Finds the size of a given tuple type.
  template<typename _Tp>
    struct tuple_size;

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 2313. tuple_size should always derive from integral_constant<size_t, N>
  template<typename _Tp>
    struct tuple_size<const _Tp>
    : integral_constant<size_t, tuple_size<_Tp>::value> { };

  template<typename _Tp>
    struct tuple_size<volatile _Tp>
    : integral_constant<size_t, tuple_size<_Tp>::value> { };

  template<typename _Tp>
    struct tuple_size<const volatile _Tp>
    : integral_constant<size_t, tuple_size<_Tp>::value> { };

  /// class tuple_size
  template<typename... _Elements>
    struct tuple_size<tuple<_Elements...>>
    : public integral_constant<size_t, sizeof...(_Elements)> { };


  // function

  namespace details {
    template<typename Sig, size_t sz, size_t algn>
    struct Function;

    template<typename R, class...Args, size_t sz, size_t algn>
    struct Function<R(Args...), sz, algn>{
      struct vtable_t {
        void(*mover)(void* src, void* dest);
        void(*destroyer)(void*);
        R(*invoke)(void const* t, Args&&...args);
        template<typename T>
        static vtable_t const* get() {
          static const vtable_t table = {
            [](void* src, void*dest) {
              new(dest) T(notstd::move(*static_cast<T*>(src)));
            },
            [](void* t){ static_cast<T*>(t)->~T(); },
            [](void const* t, Args&&...args)->R {
              return (*static_cast<T const*>(t))(notstd::forward<Args>(args)...);
            }
          };
          return &table;
        }
      };
      vtable_t const* table = nullptr;
      notstd::aligned_storage_t<sz, algn> data;
      template<typename F,
        class dF=notstd::decay_t<F>,
        // don't use this ctor on own type:
        notstd::enable_if_t<!notstd::is_same<dF, Function>{}>* = nullptr
      >
      Function( F&& f ):
        table( vtable_t::template get<dF>() )
      {
        // a higher quality Function would handle null function pointers
        // and other "nullable" callables, and construct as a null Function

        static_assert( sizeof(dF) <= sz, "object too large" );
        static_assert( alignof(dF) <= algn, "object too aligned" );
        new(&data) dF(notstd::forward<F>(f));
      }
      // I find this overload to be useful, as it forces some
      // functions to resolve their overloads nicely:
      // Function( R(*)(Args...) )
      ~Function() {
        if (table)
          table->destroyer(&data);
      }
      Function(Function&& o):
        table(o.table)
      {
        if (table)
          table->mover(&o.data, &data);
      }
      Function() = default;
      Function(const Function&) = default;
      Function& operator=(Function&& o){
        // this is a bit rude and not very exception safe
        // you can do better:
        this->~Function();
        new(this) Function( notstd::move(o) );
        return *this;
      }
      explicit operator bool()const{return table;}
      R operator()(Args...args)const{
        return table->invoke(&data, notstd::forward<Args>(args)...);
      }
    };
  }

  template<typename Sig>
  using function = details::Function<Sig, sizeof(void*)*4, alignof(void*) >;

}

#endif
