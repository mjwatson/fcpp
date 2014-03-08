#ifndef FCPP_HAMT
#define FCPP_HAMT

#include <stdint.h>
#include <vector>
#include <functional>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <assert.h>

namespace fcpp {

// -------------------------------------------------------------------------
// ARRAY MAPPED TRIE
// -------------------------------------------------------------------------

// Each node in the tree uses a bitmask index to indicate which of the
// child nodes are present. Larger indexs have wider branching factor (and
// so less depth in the tree), but tradeoff against making each node larger.

namespace Index
{
    typedef uint32_t Key;
    typedef uint32_t BitMask;
    static const size_t INDEX_SIZE = 32;
    static const size_t KEY_SIZE   = 5; 
    static const Key    KEY_MASK   = 0x1f; 
}

template<class T>
struct AMT
{
    Index::BitMask index_;
    std::vector<T> entries_;

    AMT() : index_(0) {}

    AMT(const AMT& other) : index_(other.index_), entries_(other.entries_) {}

    Index::Key bit(Index::Key k)
    {
        return 0x1 << (k & Index::KEY_MASK);
    }

    bool empty()
    {
        return index_ == 0;
    }

    bool has_index(Index::Key k)
    {
        return index_ & bit(k);
    }

    size_t get_offset(Index::Key k)
    {
        // Returns the offset in the entries of this value
        // Given entries is 0-indexed, this is the number of bits below bit() that are set.

        uint32_t lower_bits     = bit(k) - 1;
        uint32_t lower_bits_set = index_ & lower_bits;  
        size_t   count          = __builtin_popcount(lower_bits_set);

        return count;
    }

    void set_flag(Index::Key k)
    {
        index_ |= bit(k);
    }

    void clear_flag(Index::Key k)
    {
        index_ &= ~bit(k);
    }

    const T get_index(Index::Key k)
    {
        if(!has_index(k))
        {
            return T();
        }

        return entries_[get_offset(k)];
    }

    void set_index(Index::Key k, const T& t)
    {
        size_t offset = get_offset(k);
        if(has_index(k))
        {
            entries_[get_offset(k)] = t;
        }
        else
        {
            set_flag(k);
            entries_.push_back(t);
            std::copy(entries_.begin() + offset, entries_.end()-1, entries_.begin() + offset + 1);
            entries_[offset] = t;
        }
    }

    void rm_index(Index::Key k)
    {
        if(!has_index(k))
        {
            return;
        }
            
        clear_flag(k);
        entries_.erase(entries_.begin() + get_offset(k));
    }
};

// -------------------------------------------------------------------------
// HASH ARRAY MAPPED TRIE
// -------------------------------------------------------------------------

struct Hash
{
    uint32_t original_hash_;
    uint32_t hash_;

    Hash(uint32_t hash) : original_hash_(hash), hash_(hash) {};

    Index::Key getNextIndex()
    {
        Index::Key key = getNextIndexWithoutUpdate();
        hash_ = hash_ >> Index::KEY_SIZE;
        return key;
    }

    Index::Key getNextIndexWithoutUpdate()
    {
        return hash_ & Index::KEY_MASK;
    }
};

template <class KEY, class VALUE, class HASH = std::hash<KEY> >
class hamt
{
  private:

      struct Node; 
      struct KeyNode; 
      typedef boost::shared_ptr<Node> NodePtr;
      typedef boost::shared_ptr<KeyNode> KeyNodePtr;

      struct Node
      {
          virtual ~Node() {}
          virtual NodePtr assoc(const KEY& k, const VALUE& v, Hash h)  = 0;
          virtual NodePtr dissoc(const KEY& k, Hash h) = 0;
          virtual const VALUE* get(const KEY& k, Hash h)    = 0;
      };

      struct KeyNode : public Node
      {
          AMT<NodePtr> index_;

          KeyNode() {}

          KeyNode(const KeyNode& other) : index_(other.index_) {}

          KeyNodePtr clone()
          {
              return KeyNodePtr(new KeyNode(*this));
          }

          NodePtr assoc(const KEY& k, const VALUE& v, Hash h)
          {
              KeyNodePtr node = clone();

              Index::Key key = h.getNextIndex();
              
              NodePtr child = node->index_.get_index(key);

              node->index_.set_index(key, child ? child->assoc(k,v,h) : NodePtr(new ValueNode(k,v,h)));

              return node;
          }

          NodePtr dissoc(const KEY& k, Hash h)
          {
              KeyNodePtr node = clone();

              Index::Key key = h.getNextIndex();

              NodePtr child = node->index_.get_index(key);

              bool index_remains = child && child->dissoc(k,h);

              if(!index_remains)
              {
                 node->index_.rm_index(key);
              }

              return (node->index_.empty()) ? NodePtr() : node;
          }

          const VALUE* get(const KEY& k, Hash h)
          {
              Index::Key key = h.getNextIndex();

              NodePtr child = index_.get_index(key);

              if(child)
              {
                  return child->get(k,h);
              }
              else
              {
                  return 0;
              }
          }
      };

      struct ValueNode : public Node
      {
          KEY k_;
          VALUE v_;
          Hash h_;

          ValueNode(KEY k, VALUE v, Hash h) : k_(k), v_(v), h_(h) {}

          bool match(const KEY& k, Hash h)
          {
              return h.original_hash_ == h_.original_hash_ && k == k_;
          }

          NodePtr assoc(const KEY& k, const VALUE& v, Hash h)
          {
              if(match(k,h))
              {
                  return NodePtr(new ValueNode(k,v,h));
              }
              else
              {
                  KeyNode* kn = new KeyNode();
                  kn->assoc(k_, v_, h_);
                  kn->assoc(k,v,h);

                  return NodePtr(kn);
              }
          }

          NodePtr dissoc(const KEY& k, Hash h)
          {
              // We check at the top-level there is something to remove before delegating to dissoc
              // Therefore this should always be an exact match
              assert(match(k,h));
              return NodePtr();
          }

          const VALUE* get(const KEY& k, Hash h)
          {
              return match(k,h) ? &v_ : 0; 
          }
      };

      NodePtr root_;

      Hash hash(const KEY& k)
      {
          HASH hasher;
          return Hash(hasher(k));
      }

      NodePtr root()
      {
          return root_;
      }

      hamt(NodePtr root) : root_(root ? root : NodePtr(new KeyNode)) {}

  public:

      hamt() : root_(new KeyNode)
      {
      }

      hamt(const hamt& other) : root_(other.root_) {}

      hamt assoc(KEY k, VALUE v)
      {
          return hamt(root_->assoc(k,v, hash(k)));
      }

      bool contains(KEY k)
      {
          return 0 != root_->get(k, hash(k));
      }

      hamt dissoc(KEY k)
      {
          Hash h = hash(k);
          if(contains(k))
          {
              return hamt(root_->dissoc(k, hash(k)));
          }
          else
          {
              return *this;
          }
      }

      const VALUE get(KEY k)
      {
          const VALUE* v = root_->get(k, hash(k));
          if(v) 
          {
              return v;
          }
          else
          {
              throw std::range_error("HAMT: no such entry.");
          }
      }
      
      const VALUE get(KEY k, VALUE default_value)
      {
          const VALUE* v = root_->get(k, hash(k));
          return v ? *v : default_value;
      };

      bool empty()
      {
          return root_->empty();
      }
};

}

#endif
