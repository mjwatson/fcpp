#ifndef FCPP_MONADS_H
#define FCPP_MONADS_H

#include <boost/optional.hpp>
#include <boost/function.hpp>

namespace fcpp {
namespace monads {

    template<class T>
    class Some 
    {
      public:
        Some() 
        {
        }

        Some(const T& t) : _value(t) {}

        bool has()
        {
            return _value;
        }

        bool empty()
        {
            return !has();
        }

        const T& get()
        {
            return _value.get();
        }

        const T& get(const T& other)
        {
            return has() ? get() : other;
        }


        template <class F>
        auto bind(F f) -> decltype(f(this->get()))
        {
            typedef decltype(f(get())) RESULT;

            if(has())
            {
                const RESULT& R = f(get());
                return R;
            }
            else 
            {
                return RESULT();
            }
        }

      private:
        boost::optional<T> _value;
    };

    template <class T>
    Some<T> None()
    {
        return Some<T>();
    }
    
}}

#endif
