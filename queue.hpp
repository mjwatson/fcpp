#ifndef FC_BANKERS_QUEUE_HPP
#define FC_BANKERS_QUEUE_HPP

#include "list.hpp"
#include <assert.h>

namespace fcpp
{
    template<class T>
    class queue
    {
        public:

            queue()
            {
                assert(bankers_queue_invariant());
            }

            bool operator==(const queue& other)
            {
                if(empty() && other.empty())
                {
                    return true;
                }

                if(empty() || other.empty())
                {
                    return false;
                }

                if(head() != other.head())
                {
                    return false;
                }

                
                return tail() == other.tail();
            }

            size_t size() const
            {
                return active_list_.size() + passive_list_.size();
            }

            bool empty() const
            {
                return active_list_.empty() && passive_list_.empty();
            }

            T head() const
            {
               return active_list_.head();
            }

            bool head(T& t) const
            {
                if(empty())
                {
                    return false;
                }
                else
                {
                    return head();
                }

            }

            queue tail() const
            {
                list<T> l = active_list_.tail();

                if(!l.empty())
                {
                    return queue( l, passive_list_ );
                }
                else
                {
                    queue q( passive_list_.reverse(), l );
                    return q;
                }
            }

            queue push(T& t) const
            {
                if(empty())
                {
                    return queue( list<T>(t), list<T>::Nil() );
                }
                else
                {
                    return queue( active_list_, passive_list_.cons(t) );
                }
            }

            class iterator
            {
              public:
                iterator(const queue& q) : q_(q) {}

                iterator& operator++()
                {
                    q_ = q_.tail();

                    return *this;
                }

                T operator*()
                {
                    return q_.head();
                }

                bool operator==(const iterator& other)
                {
                    return q_ == other.q_;
                }

                bool operator!=(const iterator& other)
                {
                    return !operator==(other);
                }

              private:
                queue q_;
            };

            iterator begin()
            {
                return iterator(*this);
            }

            iterator end()
            {
                return iterator(queue());
            }

        private:

            queue( list<T> a, list<T> p) : active_list_(a), passive_list_(p)
            {
                assert(bankers_queue_invariant());
            }

            // The bankers queue invariant holds if the active_list_ being empty implies 
            // that the passive_list_ is also empty.
            bool bankers_queue_invariant() const
            {
                return !(active_list_.empty() && !passive_list_.empty()); 
            }

            list<T> active_list_;
            list<T> passive_list_;
    };

}

#endif
