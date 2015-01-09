#ifndef PRIORITY_QUEUE_HPP_
#define PRIORITY_QUEUE_HPP_

#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <limits>

using namespace std;

//#define DEBUG

// set if the PQ can not go empty, faster
//#define NEVEREMPTY

//id_slot : ID Type for stored elements
//key_slot: type of key_slot used
//meta key slot: information about stored data (e.g. std::numeric_limits<key>)
template< typename GraphType, typename id_slot, typename key_slot, typename meta_key_slot >
class PriorityQueue{
private:
  PriorityQueue( const PriorityQueue & ){}  //do not copy
  void operator=( const PriorityQueue& ){}  //really, do not copy

  struct Element {
    id_slot id;
    id_slot next; // index in n
  };

  const id_slot pSize;
  const key_slot mask;

  id_slot nCounter;
  Element* n; // extra elements that don't fit into buckets
  key_slot curPos;
  Element* b; // buckets
#ifndef NEVEREMPTY
  id_slot _size;
#endif
  key_slot* p; // list of keys for each id
  key_slot curKey;


  unsigned int getMask(const GraphType& graph) const {
    key_slot maxWeight = 0;

    for (unsigned int eid = 0; eid < graph.numberOfEdges(); ++eid) {
      key_slot newWeight = graph.getEdge(eid).getWeight();

      if (newWeight > maxWeight) {
        maxWeight = newWeight;
      }
    }

    maxWeight += 2;

    // to next power of 2 ...
    unsigned int mask = 1;

    while (maxWeight >>= 1) {
      mask++;
    }

    return (1 << mask) - 1; // ... and - 1
  }

  void doPush (const id_slot id, const key_slot key, Element* e = NULL) {
    if (b[key & mask].id == numeric_limits<id_slot>::max()) {
      b[key & mask].id = id;
    } else {
      if (e == NULL) {
        e = &n[nCounter--];
      }
      e->id = id;
      e->next = b[key & mask].next;
      b[key & mask].next = e - n;
    }
  }

  Element* doRemove (const id_slot id, const key_slot key) {
    Element* e = NULL;

    if (b[key & mask].id == id) {
      if (b[key & mask].next == numeric_limits<id_slot>::max()) {
        b[key & mask].id = numeric_limits<id_slot>::max();
      } else {
        e = &n[b[key & mask].next];
        b[key & mask] = *e; // e free now
      }
    } else {
      Element* f = &b[key & mask];

      while (n[f->next].id != id) {
        f = &n[f->next];
      }

      e = &n[f->next];
      f->next = e->next;
    }

    return e;
  }

public:
  PriorityQueue( const GraphType & graph )
  : pSize(graph.numberOfNodes())
  , mask(getMask(graph))
  {
    cout << "go" << endl;

    assert(pSize < numeric_limits<id_slot>::max());

    n = new Element[pSize];
    p = new key_slot[pSize];
    b = new Element[mask + 1];
  }

  size_t size() const{
#ifdef NEVEREMPTY
    throw "Unimplemented";
#else
    return _size;
#endif
  }

  bool empty() const {
#ifdef NEVEREMPTY
    return false;
#else
    return !_size;
#endif
  }

  //push an element onto the heap
  void push( const id_slot & id, const key_slot & key ) {
#ifdef DEBUG/*{{{*/
    cout << "push(" << id << "," << key << ")" << endl;
#endif/*}}}*/
    p[id] = key;
#ifndef NEVEREMPTY
    _size++;
#endif
    doPush(id, key);
  }

  // no values lower than the one returned by getMin() can be inserted any
  // more! no problem for dijkstra though
  inline const id_slot getMin() {
    while (b[curPos].id == numeric_limits<id_slot>::max()) {
      curPos = (curPos + 1) & mask;
      curKey++;
    }
    //if (p[b[curPos].id] != curKey) {
    //  deleteMin();
    //  return getMin();
    //}
    return b[curPos].id;
  }

  inline const key_slot getMinKey() const{
    return curKey;
  }

  inline void deleteMin() {
#ifdef DEBUG/*{{{*/
      cout << "deleteMin()" << endl;
      //cout << "deleteMin() = " << b[curPos].next << ", id: " << b[curPos].next->id << ", key: " << b[curPos].next->key << endl;
#endif/*}}}*/
#ifndef NEVEREMPTY
    _size--;
#endif
    if (b[curPos].next != numeric_limits<id_slot>::max()) {
      b[curPos] = n[b[curPos].next];
    } else {
      b[curPos].id = numeric_limits<id_slot>::max();
    }
  }

  const key_slot & getCurrentKey( register const id_slot& id ) const {
#ifdef DEBUG/*{{{*/
    cout << "getCurrentKey(" << id << ") = " << p[id] << endl;
#endif/*}}}*/
    return p[id];
  }

  bool isReached( const id_slot& id ) const {
#ifdef DEBUG/*{{{*/
    cout << "isReached(" << id << ") = " << ((p[id] != numeric_limits<key_slot>::max()) ? "1" : "0") << endl;
#endif/*}}}*/
    return p[id] != numeric_limits<key_slot>::max();
  }

  void decreaseKey( const id_slot& id, const key_slot& new_key ) {
    //push(id, new_key); // better on desktop
    updateKey(id, new_key); // better on laptop and server
  }

  void increaseKey( const id_slot& id, const key_slot& new_key ) {
    updateKey(id, new_key);
  }

  void updateKey( const id_slot& id, const key_slot& new_key ) {
#ifdef DEBUG/*{{{*/
    cout << "updateKey(" << id << "," << new_key << ")" << endl;
#endif/*}}}*/
    doPush(id, new_key, doRemove(id, p[id]));
    p[id] = new_key;
  }

  void clear() {
    nCounter = pSize - 1;
    curPos = 0;
    curKey = 0;
#ifndef NEVEREMPTY
    _size = 0;
#endif
    memset(p, 0xFF, pSize * sizeof(key_slot));
    memset(b, 0xFF, (mask + 1) * sizeof(Element)); // bad style...
  }

  bool contains( const id_slot& id ) const { // inefficient
    if (!isReached(id)) {
      return false;
    }

    Element* e = b[p[id] & mask];

    while (e != (Element*) (-1)) {
      if (e->id == id) {
        return true;
      }

      e = &n[e->next];
    }

    return false;
  }
};


#endif /* PRIORITY_QUEUE_HPP_ */
