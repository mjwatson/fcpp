#ifndef FC_SLIST_HPP
#define FC_SLIST_HPP

#include <stdexcept>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using boost::shared_ptr;

namespace fcpp
{
    class NoSuchEntry : public std::range_error
    {
      public:
        NoSuchEntry() :
            std::range_error("No such entry in persistent data structure.") 
            {}
    };

    template<class T>
    class list
    {
       private:

          struct Node;
          typedef shared_ptr<Node> NodePtr;

          struct Node
          {
              NodePtr tail_;
              T t_;

              Node(NodePtr tail, const T& t) :
                  tail_(tail),
                  t_(t)
                  {}

              ~Node()
              {
                  if(tail_.get() == 0)
                  {
                      return;
                  }

                  NodePtr p = tail_;
                  tail_.reset();
                  while(p.get() && p.use_count() == 1)
                  {
                      NodePtr q = p->tail_;
                      p->tail_.reset();
                      p = q;
                  }
              }
          };

          NodePtr node_;

          bool is_nil()
          {
              node_.get() == 0;
          }

          list() {}

          list(NodePtr node) : node_(node) {}

      public:

          static const list<T> Nil()
          {
              return list<T>();
          };

          bool empty()
          {
              return is_nil();
          }

          size_t size()
          {
              size_t result = 0;
              for(NodePtr n = node_; n != 0; n = n->tail_) {
                  result++;
              }
              return result;
          }

          T& operator[](size_t index)
          {
              NodePtr n = node_; 
              while(n.get() != 0 && 0 < index) {
                  --index;
                  n = n->tail_;
              }

              if(n != 0)
              {
                  return n->t_;
              }
              else
              {
                  throw NoSuchEntry();
              }
          };

          T& head()
          {
              return (*this)[0];
          };

          list<T> tail()
          {
              if(node_ != 0)
              {
                  return list<T>(node_->tail_);
              }
              else
              {
                  // The tail of an empty list is the empty list.
                  return *this;
              }
          };

          list cons(const T& t)
          {
              return list(boost::make_shared<Node>(node_, t));
          };

          struct iterator
          {
              NodePtr node_;

              iterator(NodePtr node) : node_(node) {}

              T& operator*()
              {
                  return node_->t_;
              }

              iterator operator++()
              {
                  node_ = node_->tail_;
                  return *this;
              }

              bool operator==(const iterator& other)
              {
                  return node_ == other.node_;
              }

              bool operator!=(const iterator& other)
              {
                  return ! operator==(other);
              }
          };

          iterator begin()
          {
              return iterator(node_);
          }

          iterator end()
          {
              return iterator(NodePtr());
          };
    };
}

#endif
